#include "telegram_qr_login_dialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QTimer>

#include "qr_login_dialog.h"

namespace fasthub::ui {

TelegramQrLoginDialog::TelegramQrLoginDialog(std::shared_ptr<providers::telegram::TelegramProvider> provider, QWidget* parent)
    : QDialog(parent), m_provider(provider) {
    setupUi();

    connect(m_provider.get(), &providers::telegram::TelegramProvider::qrLoginStateChanged,
            this, &TelegramQrLoginDialog::onStateChanged);

    restartFlow();
}

void TelegramQrLoginDialog::setupUi() {
    setWindowTitle("Telegram - Login via QR");
    resize(380, 480);

    QVBoxLayout* layout = new QVBoxLayout(this);

    m_qrLabel = new QLabel(this);
    m_qrLabel->setAlignment(Qt::AlignCenter);
    m_qrLabel->setMinimumSize(260, 260);
    layout->addWidget(m_qrLabel);

    m_statusLabel = new QLabel("Starting...", this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_statusLabel);

    // API credentials form (keys from my.telegram.org, stored DPAPI-protected).
    layout->addWidget(new QLabel("API credentials (my.telegram.org):", this));
    m_apiIdEdit = new QLineEdit(this);
    m_apiIdEdit->setPlaceholderText("api_id");
    layout->addWidget(m_apiIdEdit);
    m_apiHashEdit = new QLineEdit(this);
    m_apiHashEdit->setPlaceholderText("api_hash");
    m_apiHashEdit->setEchoMode(QLineEdit::Password);
    layout->addWidget(m_apiHashEdit);

    m_saveCredentialsButton = new QPushButton("Save credentials and continue", this);
    connect(m_saveCredentialsButton, &QPushButton::clicked, this, &TelegramQrLoginDialog::onSaveCredentials);
    layout->addWidget(m_saveCredentialsButton);

    m_retryButton = new QPushButton("Retry QR login", this);
    connect(m_retryButton, &QPushButton::clicked, this, &TelegramQrLoginDialog::restartFlow);
    layout->addWidget(m_retryButton);

    QHBoxLayout* bottom = new QHBoxLayout();
    bottom->addStretch();
    QPushButton* close = new QPushButton("Close", this);
    connect(close, &QPushButton::clicked, this, &QDialog::accept);
    bottom->addWidget(close);
    layout->addLayout(bottom);
}

void TelegramQrLoginDialog::onStateChanged(const QString& state, const QString& qrLink, const QString& details) {
    if (state == QStringLiteral("wait_qr") && !qrLink.isEmpty()) {
        // Real Telegram login link: scanning it in the official app authorizes
        // this exact TDLib session.
        const QPixmap qr = QrLoginDialog::makeQrPixmap(qrLink, 240);
        if (!qr.isNull()) {
            m_qrLabel->setPixmap(qr);
        }
        m_statusLabel->setText("Open Telegram on your phone:\nSettings -> Devices -> Link Desktop Device\nand scan this code.");
    } else if (state == QStringLiteral("ready")) {
        m_qrLabel->clear();
        m_qrLabel->setText(u8"\u2714");
        m_statusLabel->setText("Authorized! Loading your chats...");
        QTimer::singleShot(1200, this, &QDialog::accept);
    } else if (state == QStringLiteral("error")) {
        m_statusLabel->setText(QString("Error: %1").arg(details));
    } else if (state == QStringLiteral("unavailable")) {
        m_statusLabel->setText(details);
    } else if (state == QStringLiteral("sending")) {
        m_statusLabel->setText("Contacting Telegram...");
    }
}

void TelegramQrLoginDialog::onSaveCredentials() {
    const QString apiId = m_apiIdEdit->text().trimmed();
    const QString apiHash = m_apiHashEdit->text().trimmed();
    if (apiId.isEmpty() || apiHash.isEmpty()) {
        QMessageBox::warning(this, "Telegram", "Both api_id and api_hash are required.");
        return;
    }
    m_provider->saveApiCredentials(apiId, apiHash);
    restartFlow();
}

void TelegramQrLoginDialog::restartFlow() {
    m_qrLabel->clear();
    m_statusLabel->setText("Starting...");
    if (m_provider) {
        m_provider->startQrLogin();
    }
}

} // namespace fasthub::ui

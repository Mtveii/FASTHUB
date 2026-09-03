#include "discord_login_dialog.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>

namespace fasthub::ui {

DiscordLoginDialog::DiscordLoginDialog(std::shared_ptr<providers::discord::DiscordOauthService> service, QWidget* parent)
    : QDialog(parent), m_service(service) {
    setupUi();

    if (m_service) {
        connect(m_service.get(), &providers::discord::DiscordOauthService::loginStateChanged,
                this, &DiscordLoginDialog::onStateChanged);
    }
}

void DiscordLoginDialog::setupUi() {
    setWindowTitle("Discord - Login (Official OAuth2)");
    resize(420, 320);

    QVBoxLayout* layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel(
        "Login uses the official Discord OAuth2 flow in your browser.\n"
        "Create an application at discord.com/developers/applications\n"
        "and add redirect URI:  http://127.0.0.1:8976/callback", this));

    m_clientIdEdit = new QLineEdit(this);
    m_clientIdEdit->setPlaceholderText("Application ID (client_id)");
    layout->addWidget(m_clientIdEdit);

    m_clientSecretEdit = new QLineEdit(this);
    m_clientSecretEdit->setPlaceholderText("Client secret");
    m_clientSecretEdit->setEchoMode(QLineEdit::Password);
    layout->addWidget(m_clientSecretEdit);

    QPushButton* saveButton = new QPushButton("Save credentials", this);
    connect(saveButton, &QPushButton::clicked, this, &DiscordLoginDialog::onSaveCredentials);
    layout->addWidget(saveButton);

    QPushButton* loginButton = new QPushButton("Open browser and log in", this);
    connect(loginButton, &QPushButton::clicked, this, &DiscordLoginDialog::onStartLogin);
    layout->addWidget(loginButton);

    m_statusLabel = new QLabel("Idle.", this);
    m_statusLabel->setWordWrap(true);
    layout->addWidget(m_statusLabel);

    QHBoxLayout* bottom = new QHBoxLayout();
    bottom->addStretch();
    QPushButton* close = new QPushButton("Close", this);
    connect(close, &QPushButton::clicked, this, &QDialog::accept);
    bottom->addWidget(close);
    layout->addLayout(bottom);
}

void DiscordLoginDialog::onSaveCredentials() {
    const QString id = m_clientIdEdit->text().trimmed();
    const QString secretValue = m_clientSecretEdit->text().trimmed();
    if (!m_service) return;
    if (id.isEmpty() || secretValue.isEmpty()) {
        QMessageBox::warning(this, "Discord", "Both Application ID and Client Secret are required.");
        return;
    }
    m_service->saveAppCredentials(id, secretValue);
    m_statusLabel->setText("Credentials saved securely.");
}

void DiscordLoginDialog::onStartLogin() {
    if (!m_service) return;
    if (!m_service->hasCredentials()) {
        QMessageBox::warning(this, "Discord", "Save credentials first.");
        return;
    }
    m_service->beginLogin();
}

void DiscordLoginDialog::onStateChanged(const QString& step, const QString& details) {
    m_statusLabel->setText(QString("[%1] %2").arg(step, details));
}

} // namespace fasthub::ui

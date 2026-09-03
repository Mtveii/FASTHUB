#include "telegram_widget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>

#include "qr_login_dialog.h"
#include "telegram_qr_login_dialog.h"

namespace fasthub::ui {

TelegramWidget::TelegramWidget(std::shared_ptr<providers::telegram::TelegramProvider> provider, QWidget* parent)
    : QWidget(parent), m_provider(provider) {
    setupUi();
    loadConversations();
}

void TelegramWidget::setupUi() {
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);

    // Left: Chat List
    m_chatListWidget = new QListWidget(this);
    m_chatListWidget->setMaximumWidth(280);
    connect(m_chatListWidget, &QListWidget::currentRowChanged, this, &TelegramWidget::onConversationSelected);

    // Right: Message thread & input
    QWidget* rightPanel = new QWidget(this);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(4, 4, 4, 4);

    m_messageDisplay = new QTextEdit(this);
    m_messageDisplay->setReadOnly(true);

    QHBoxLayout* inputLayout = new QHBoxLayout();
    m_messageInput = new QLineEdit(this);
    m_messageInput->setPlaceholderText("Write a message...");
    connect(m_messageInput, &QLineEdit::returnPressed, this, &TelegramWidget::onSendMessageClicked);

    inputLayout->addWidget(m_messageInput);
    m_sendButton = new QPushButton("Send", this);
    connect(m_sendButton, &QPushButton::clicked, this, &TelegramWidget::onSendMessageClicked);

    QPushButton* qrButton = new QPushButton("Login via QR", this);
    qrButton->setToolTip("Show QR code for Telegram login");
    connect(qrButton, &QPushButton::clicked, this, &TelegramWidget::onQrLoginClicked);

    inputLayout->addWidget(m_sendButton);
    inputLayout->addWidget(qrButton);

    rightLayout->addWidget(m_messageDisplay);
    rightLayout->addLayout(inputLayout);

    splitter->addWidget(m_chatListWidget);
    splitter->addWidget(rightPanel);
    splitter->setSizes({280, 920});

    mainLayout->addWidget(splitter);
}

void TelegramWidget::loadConversations() {
    if (!m_provider) return;

    m_conversations = m_provider->getConversations();
    m_chatListWidget->clear();
    for (const auto& conv : m_conversations) {
        m_chatListWidget->addItem(QString::fromStdString(conv.title));
    }

    if (!m_conversations.empty()) {
        m_chatListWidget->setCurrentRow(0);
    }
}

void TelegramWidget::onConversationSelected(int row) {
    if (row < 0 || row >= static_cast<int>(m_conversations.size())) {
        return;
    }

    m_currentConversationId = m_conversations[row].internalId;
    loadMessages(m_currentConversationId);
}

void TelegramWidget::loadMessages(const std::string& conversationId) {
    if (!m_provider) return;

    auto messages = m_provider->getMessages(conversationId);
    QString html;
    for (const auto& msg : messages) {
        // Escape untrusted provider text: messages must never be able to
        // inject HTML into the rich-text display.
        QString sender = QString::fromStdString(msg.senderName).toHtmlEscaped();
        QString text = QString::fromStdString(msg.text).toHtmlEscaped();
        html += QString("<b>%1:</b> %2<br>").arg(sender, text);
    }

    m_messageDisplay->setHtml(html);
}

void TelegramWidget::onSendMessageClicked() {
    QString text = m_messageInput->text().trimmed();
    if (text.isEmpty() || m_currentConversationId.empty() || !m_provider) {
        return;
    }

    if (m_provider->sendMessage(m_currentConversationId, text.toStdString())) {
        m_messageInput->clear();
        loadMessages(m_currentConversationId);
    }
}

void TelegramWidget::onQrLoginClicked() {
    // Real login: TDLib requestQrCodeAuthentication flow. The QR carries a
    // session token issued by Telegram; scanning it in the official app
    // authorizes FastHub (same mechanism as web.telegram.org).
    if (m_provider && m_provider->isTdlibAvailable()) {
        TelegramQrLoginDialog dialog(m_provider, this);
        dialog.exec();
        return;
    }

    // TDLib missing: explain instead of pretending.
    QrLoginDialog dialog(
        "Telegram - TDLib not found",
        "https://github.com/tdlib/td",
        "tdjson.dll was not found next to FastHub.exe, so real device-QR login is disabled.\n"
        "Place tdjson.dll (TDLib build) beside the executable and restart.",
        this);
    dialog.exec();
}

} // namespace fasthub::ui

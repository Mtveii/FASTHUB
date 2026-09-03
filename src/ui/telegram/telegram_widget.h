#pragma once

#include <QWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <memory>
#include "providers/telegram/telegram_provider.h"

namespace fasthub::ui {

class TelegramWidget : public QWidget {
    Q_OBJECT
public:
    explicit TelegramWidget(std::shared_ptr<providers::telegram::TelegramProvider> provider, QWidget* parent = nullptr);
    ~TelegramWidget() override = default;

private slots:
    void onConversationSelected(int row);
    void onSendMessageClicked();
    void onQrLoginClicked();

private:
    void setupUi();
    void loadConversations();
    void loadMessages(const std::string& conversationId);

    std::shared_ptr<providers::telegram::TelegramProvider> m_provider;
    QListWidget* m_chatListWidget;
    QTextEdit* m_messageDisplay;
    QLineEdit* m_messageInput;
    QPushButton* m_sendButton;

    std::vector<core::Conversation> m_conversations;
    std::string m_currentConversationId;
};

} // namespace fasthub::ui

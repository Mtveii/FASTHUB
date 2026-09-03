#pragma once

#include <QObject>
#include <memory>

#include "core/i_chat_provider.h"
#include "td_client.h"

namespace fasthub::providers::telegram {

/// @brief Telegram provider with TDLib-backed QR login.
///
/// If tdjson.dll is present, real authentication is available:
/// setQrLogin() drives the official "login from another device" flow
/// (requestQrCodeAuthentication) and reports state transitions through
/// qrLoginStateChanged(). Without the DLL the provider degrades to the
/// local-cache mock mode.
class TelegramProvider : public QObject, public core::IChatProvider {
    Q_OBJECT
public:
    explicit TelegramProvider();
    ~TelegramProvider() override;

    core::ProviderStatus getStatus() const override;
    std::vector<core::Conversation> getConversations() override;
    std::vector<core::Message> getMessages(const std::string& conversationId) override;
    bool sendMessage(const std::string& conversationId, const std::string& text) override;

    /// @brief Legacy mock-mode entry points (kept for offline demo mode).
    bool login(const std::string& phoneNumber);
    bool verifyCode(const std::string& code);

    /// @brief true if TDLib integration is usable on this machine.
    bool isTdlibAvailable() const { return m_tdClient && m_tdClient->isAvailable(); }

public slots:
    /// @brief Starts (or restarts) the official QR login flow.
    /// Requires api_id/api_hash saved via saveApiCredentials().
    void startQrLogin();

    /// @brief Persists Telegram API credentials via ISecureStorage (DPAPI).
    void saveApiCredentials(const QString& apiId, const QString& apiHash);

signals:
    /// @brief Emitted on every authorization step.
    /// @param state One of: unavailable|sending|wait_qr|ready|error
    /// @param qrLink For wait_qr: the tg://login?token=... link to encode.
    /// @param details Human-readable hint (error text etc).
    void qrLoginStateChanged(QString state, QString qrLink, QString details);

private slots:
    void onTdUpdate(const QJsonObject& update);

private:
    void handleAuthState(const QString& authState, const QJsonObject& update);

    core::ProviderStatus m_status = core::ProviderStatus::Available;
    std::unique_ptr<TdClient> m_tdClient;
    bool m_parametersSet = false;
};

} // namespace fasthub::providers::telegram

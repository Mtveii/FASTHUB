#pragma once

#include <QDialog>
#include <memory>

#include "providers/telegram/telegram_provider.h"

class QLabel;
class QLineEdit;
class QPushButton;

namespace fasthub::ui {

/// @brief Real Telegram login dialog driven by TDLib authorization states.
///
/// Shows a live QR (tg://login?token=...) produced by requestQrCodeAuthentication,
/// an API-credentials form when keys are missing, and terminal states
/// (authorized / error). Replaces the old website-link QR stub.
class TelegramQrLoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit TelegramQrLoginDialog(std::shared_ptr<providers::telegram::TelegramProvider> provider, QWidget* parent = nullptr);

private slots:
    void onStateChanged(const QString& state, const QString& qrLink, const QString& details);
    void onSaveCredentials();
    void restartFlow();

private:
    void setupUi();

    std::shared_ptr<providers::telegram::TelegramProvider> m_provider;
    QLabel* m_statusLabel;
    QLabel* m_qrLabel;
    QLineEdit* m_apiIdEdit;
    QLineEdit* m_apiHashEdit;
    QPushButton* m_saveCredentialsButton;
    QPushButton* m_retryButton;
};

} // namespace fasthub::ui

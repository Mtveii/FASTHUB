#pragma once

#include <QDialog>
#include <QPixmap>

namespace fasthub::ui {

/// @brief Renders a QR code for provider login.
///
/// Current scope (honest): the QR payload is the official web login URL of
/// the corresponding service, so scanning it with a phone opens the real
/// login page. Full cross-device QR *handshake* (Telegram MTProto, Discord
/// remote-auth websocket) requires the official SDKs and lands together with
/// those integrations; the dialog states this explicitly to the user.
class QrLoginDialog : public QDialog {
    Q_OBJECT
public:
    /// @param title Window title (e.g. "Telegram - QR Login").
    /// @param loginUrl Payload encoded into the QR image.
    /// @param note Human-readable explanation shown under the code.
    QrLoginDialog(const QString &title, const QString &loginUrl, const QString &note,
                  QWidget *parent = nullptr);

    /// @brief Renders @p text as a QR pixmap (black on transparent), scaled
    ///        to @p sizePx. Returns a null pixmap on encoding failure.
    static QPixmap makeQrPixmap(const QString &text, int sizePx);

private:
    void setupUi(const QString &note, const QPixmap &qr);
};

} // namespace fasthub::ui

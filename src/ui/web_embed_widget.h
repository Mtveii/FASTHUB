#pragma once

#include <QWidget>
#include <QWebEngineView>
#include <QString>

namespace fasthub::ui {

/// @brief Embedded provider web client (owner-approved exception to the
///        "no browser engine" rule - see AGENTS.md §11 amendment).
///
/// Each instance uses a PERSISTENT Chromium profile stored under
/// AppData/FastHubTeam/FastHub/webprofiles/<name>: the user logs into the
/// provider's real web app once and the session survives restarts.
/// Chrome-less by design: no navigation buttons - the provider's own UI
/// is the interface.
class WebEmbedWidget : public QWidget {
    Q_OBJECT
public:
    /// @param name Unique profile id ("discord", "steam", "telegram-web").
    /// @param homeUrl First page loaded (login or app start page).
    WebEmbedWidget(const QString &name, const QString &homeUrl, QWidget *parent = nullptr);

    /// @brief Navigates back to the home URL.
    void goHome();

private slots:
    void onUrlChanged(const QUrl &url);

private:
    QString m_name;
    QString m_homeUrl;
    QWebEngineView *m_view = nullptr;
};

} // namespace fasthub::ui

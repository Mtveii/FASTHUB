#include "web_embed_widget.h"

#include <QVBoxLayout>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEnginePage>
#include <QWebEnginePermission>
#include <QStandardPaths>
#include <QDir>

#include "context_broker.h"
#include "interface_settings.h"
#include "infrastructure/logger.h"

namespace fasthub::ui {

WebEmbedWidget::WebEmbedWidget(const QString &name, const QString &homeUrl, QWidget *parent)
    : QWidget(parent), m_name(name), m_homeUrl(homeUrl) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // Persistent profile: cookies/localStorage survive app restarts, so the
    // provider session (login) is remembered - the whole point of this mode.
    const QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
                             QStringLiteral("/webprofiles/") + m_name;
    QDir().mkpath(dataPath);

    auto *profile = new QWebEngineProfile(m_name, this);
    profile->setPersistentStoragePath(dataPath);
    profile->setHttpCacheType(QWebEngineProfile::DiskHttpCache);
    profile->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);
    profile->setPersistentPermissionsPolicy(
        QWebEngineProfile::PersistentPermissionsPolicy::AskEveryTime);

    // Pre-grant any previously stored media permissions so Discord/Steam
    // do not start with a stale "denied" state from an earlier run.
    for (const auto &perm : profile->listAllPermissions()) {
        using PT = QWebEnginePermission::PermissionType;
        switch (perm.permissionType()) {
        case PT::MediaAudioCapture:
        case PT::MediaVideoCapture:
        case PT::MediaAudioVideoCapture:
        case PT::DesktopVideoCapture:
        case PT::DesktopAudioVideoCapture:
            perm.grant();
            break;
        default:
            break;
        }
    }

    m_view = new QWebEngineView(profile, this);
    m_view->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    m_view->settings()->setAttribute(QWebEngineSettings::ScreenCaptureEnabled, true);

    // Grant camera/microphone/screen-capture permissions on request so that
    // providers embedded via WebEngine (Discord/Steam web clients) can use
    // the PC's hardware (mic, camera, screen share) instead of being silently
    // blocked by Chromium's default permission policy.
    connect(m_view->page(), &QWebEnginePage::permissionRequested, this,
            [](QWebEnginePermission permission) {
                using PT = QWebEnginePermission::PermissionType;
                switch (permission.permissionType()) {
                case PT::MediaAudioCapture:
                case PT::MediaVideoCapture:
                case PT::MediaAudioVideoCapture:
                case PT::DesktopVideoCapture:
                case PT::DesktopAudioVideoCapture:
                case PT::Notifications:
                    permission.grant();
                    break;
                default:
                    break;
                }
            });
    m_view->setZoomFactor(InterfaceSettings::instance().webZoomPercent() / 100.0);
    connect(
        &InterfaceSettings::instance(), &InterfaceSettings::webZoomChanged, this,
        [this](int pct) { m_view->setZoomFactor(pct / 100.0); }, Qt::UniqueConnection);
    layout->addWidget(m_view);

    connect(m_view, &QWebEngineView::urlChanged, this, &WebEmbedWidget::onUrlChanged);

    // Publish page selection to the AI context broker (async JS round-trip).
    connect(m_view->page(), &QWebEnginePage::selectionChanged, this, [this]() {
        m_view->page()->runJavaScript(
            QStringLiteral("window.getSelection().toString()"), 0, [this](const QVariant &result) {
                const QString sel = result.toString();
                if (!sel.isEmpty()) {
                    ContextBroker::instance().setSelection(m_name, sel.left(8000));
                }
            });
    });
    goHome();
}

void WebEmbedWidget::goHome() { m_view->load(QUrl(m_homeUrl)); }

void WebEmbedWidget::onUrlChanged(const QUrl &url) {
    // Light-touch telemetry for the developer console; no content is logged.
    infrastructure::Logger::instance().info("WebEmbed",
                                            QString("[%1] -> %2").arg(m_name, url.host()));
}

} // namespace fasthub::ui

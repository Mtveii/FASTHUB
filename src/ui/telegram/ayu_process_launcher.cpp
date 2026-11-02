#include "ayu_process_launcher.h"
#include "ayu_window_tracker.h"
#include "embed_types.h"

#include "infrastructure/logger.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>

namespace fasthub::ui::ayu {

ProcessLauncher::ProcessLauncher(QString imageName) : m_imageName(std::move(imageName)) {}

QString ProcessLauncher::locateExecutable() const {
    const QString appDir = QCoreApplication::applicationDirPath();
    // Layout: <root>/third_party/ayugram/FastHub_Telegram.exe. Keep legacy
    // AyuGram.exe fallback during transition.
    QStringList candidates = {
        QDir::cleanPath(appDir + QStringLiteral("/../../../third_party/ayugram/") + m_imageName),
        QDir::cleanPath(appDir + QStringLiteral("/../../../../third_party/ayugram/") + m_imageName),
        QDir::cleanPath(appDir + QStringLiteral("/") + m_imageName),
    };
    // Fallback to old name if new binary not yet built.
    if (m_imageName.compare(QStringLiteral("FastHub_Telegram.exe"), Qt::CaseInsensitive) == 0) {
        candidates +=
            QDir::cleanPath(appDir + QStringLiteral("/../../../third_party/ayugram/AyuGram.exe"));
        candidates += QDir::cleanPath(
            appDir + QStringLiteral("/../../../../third_party/ayugram/AyuGram.exe"));
    }
    for (const QString &path : candidates) {
        if (QFileInfo::exists(path)) {
            return path;
        }
    }
    infrastructure::Logger::instance().warn(
        "Telegram",
        QStringLiteral("locateExecutable miss: %1").arg(candidates.join(QLatin1String(" | "))));
    return {};
}

bool ProcessLauncher::isProcessRunning() const {
    return WindowTracker::isProcessRunning(m_imageName);
}

bool ProcessLauncher::hasWindowlessOrphan() const {
    // allowHidden=true: a healthy instance hidden by our overlay owns a
    // window and must NOT be classified as an orphan.
    WindowTracker::Query query;
    query.imageName = m_imageName;
    query.allowHidden = true;
    return !WindowTracker::findWindow(query) && isProcessRunning();
}

void ProcessLauncher::terminateAllInstances() {
    QProcess::startDetached(QStringLiteral("taskkill"),
                            {QStringLiteral("/IM"), m_imageName, QStringLiteral("/F")});
}

} // namespace fasthub::ui::ayu

#pragma once

#include <QWidget>
#include <QProcess>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QResizeEvent>
#include <QShowEvent>
#include <QHideEvent>

#include "../telegram/embed_types.h"
#include "../telegram/ayu_geometry_sync.h"
#include "../telegram/ayu_process_launcher.h"

#include <windows.h>

namespace fasthub::ui {

/// @brief Telegram tab = the real FastHub_Telegram desktop client, embedded.
///
/// Integration model (owner-approved 2025-08-25):
/// - FastHub_Telegram runs as a SEPARATE process; its top-level window is captured
///   (HWND) and kept positioned precisely over this tab (overlay scheme).
///   HWND reparenting (SetParent/WS_CHILD) was tried and rejected: FastHub_Telegram's
///   Qt keeps resetting its window style, the window drops out of
///   hit-testing and never receives mouse input.
/// - This class is ONLY a coordinator (state machine + wiring): process
///   lifecycle lives in ayu::ProcessLauncher, window discovery in
///   ayu::WindowTracker, geometry in ayu::GeometrySync. The visibility
///   decision itself is the pure function ayu::shouldShowEmbed().
/// - The window is grabbed only AFTER FastHub_Telegram finished initializing (real
///   window title); grabbing during startup corrupts its UI and kills all
///   input.
/// - FastHub_Telegram's own drawn title strip is cut off with SetWindowRgn so the
///   client area fits the tab edge-to-edge.
/// - Separate-process embedding is "mere aggregation": GPL-3.0 covers
///   FastHub_Telegram itself; FastHub's licensing is unaffected.
/// - If FastHub_Telegram closes or crashes, an overlay offers relaunch; the official
///   web client exists only as a MANUAL fallback button (Telegram must not
///   silently degrade to web mode).
///
/// @note History: this used to be fasthub_telegram_embed_widget.h; the old file was
/// lost to an external exclusive file lock and could not be edited/deleted.
class ObsidianEmbedWidget : public QWidget {
    Q_OBJECT
public:
    explicit ObsidianEmbedWidget(QWidget *parent = nullptr);
    ~ObsidianEmbedWidget() override;

    /// @brief Accessors for the global WinEvent callback (see .cpp).
    static ObsidianEmbedWidget *s_activeInstanceForCallback() { return s_activeInstance; }
    quint64 embeddedHwnd() const { return m_ayuHwnd; }
    void handleStyleChange();
    void refreshOverlayPublic() { refreshOverlay(); }

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private slots:
    void launchOrAttach();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void pollForWindow();
    void showWebFallback();

private:
    /// @brief Single entry point for all sync paths (timers, events, tab
    /// switches). Evaluates visibility first and exits before any rect
    /// math when the overlay must stay hidden.
    void refreshOverlay();

    void setupUi();
    void setState(ayu::EmbedState next, const char *reason);
    void setOverlay(const QString &text, bool showRelaunch);
    bool attachNativeWindow(quint64 hwnd);
    void detachFromNative(bool restoreRegion);
    void shutdownEmbeddedProcess();
    void applyFramelessStyle(quint64 hwnd);

    void installWinEventHook();
    void removeWinEventHook();
    void ensureJobObject();
    void assignProcessToJob(qint64 pid);
    void closeJobObject();

    ayu::ProcessLauncher m_launcher{QStringLiteral("Obsidian.exe")};
    ayu::GeometrySync m_geometry;

    QProcess *m_process = nullptr;
    QStackedWidget *m_stack = nullptr;
    QWidget *m_overlay = nullptr;
    QLabel *m_overlayText = nullptr;
    QWidget *m_container = nullptr; ///< Placeholder page under the Ayu window
    QTimer m_pollTimer;             ///< Startup window poll (300 ms)
    QTimer m_fallbackTimer;         ///< Slow safety net (10 s)
    quint64 m_ayuHwnd = 0;
    DWORD m_ayuPid = 0;        ///< FastHub_Telegram process id (focus checks)
    void *m_winHook = nullptr; ///< WinEvent hook (HWINEVENTHOOK)
    HANDLE m_job =
        nullptr; ///< Job object: makes child appear under FastHub in Task Manager + kill-on-close
    int m_waitTicks = 0;
    bool m_attached = false;
    bool m_lastShown = false; ///< Last applied visibility (early exit)
    ayu::EmbedState m_state = ayu::EmbedState::Idle;

    /// Single-instance routing for the global WinEvent callback.
    static ObsidianEmbedWidget *s_activeInstance;
};

} // namespace fasthub::ui

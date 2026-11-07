#include "obsidian_embed_widget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCoreApplication>
#include <QApplication>
#include <QDir>
#include <QWebEngineView>
#include <QWebEngineProfile>
#include <QStandardPaths>

#include <windows.h>
#include <dwmapi.h>

#include "../telegram/ayu_window_tracker.h"
#include "infrastructure/logger.h"
#include "interface_settings.h"

namespace fasthub::ui {

using ayu::EmbedState;
using ayu::stateName;

namespace {

// Win32 constants kept numeric on purpose: some are guarded by
// WINVER >= 0x0500 in older SDK headers and we do not want the embed
// module to depend on project-wide WINVER settings.
constexpr DWORD kEvtLocationChange = 0x800B; // EVENT_OBJECT_LOCATIONCHANGE
constexpr DWORD kEvtStyleChange = 0x8011;    // EVENT_OBJECT_STYLECHANGE
constexpr LONG kObjIdWindow = 0xFFFF;        // OBJID_WINDOW
constexpr LONG kIndexContainer = 0;          // INDEXID_CONTAINER
constexpr int kGwlHwndParent = -8;           // GWL_HWNDPARENT (sets OWNER)

/// @brief Global WinEvent callback: routes to the active widget instance.
/// The Telegram tab is single-instance, so static routing is safe.
void CALLBACK winEventThunk(HWINEVENTHOOK, DWORD event, HWND hwnd, LONG idObject, LONG idChild,
                            DWORD, DWORD) {
    auto *self = ObsidianEmbedWidget::s_activeInstanceForCallback();
    if (!self || idObject != kObjIdWindow || idChild != kIndexContainer) {
        return;
    }
    if (hwnd != reinterpret_cast<HWND>(self->embeddedHwnd())) {
        return;
    }
    if (event == kEvtStyleChange) {
        // FastHub_Telegram (or Qt) re-applied its own style - strip chrome again.
        self->handleStyleChange();
    }
    self->refreshOverlayPublic();
}

} // namespace

ObsidianEmbedWidget *ObsidianEmbedWidget::s_activeInstance = nullptr;

ObsidianEmbedWidget::ObsidianEmbedWidget(QWidget *parent) : QWidget(parent) {
    setupUi();
    ensureJobObject();
    s_activeInstance = this;
    if (InterfaceSettings::instance().ayuAutoLaunch()) {
        QTimer::singleShot(150, this, &ObsidianEmbedWidget::launchOrAttach);
    } else {
        setState(EmbedState::Idle, "auto-launch disabled");
        setOverlay(QStringLiteral("FastHub_Telegram launch is disabled in settings."), true);
    }
}

ObsidianEmbedWidget::~ObsidianEmbedWidget() {
    if (s_activeInstance == this) {
        s_activeInstance = nullptr;
    }
    shutdownEmbeddedProcess();
    closeJobObject();
}

void ObsidianEmbedWidget::setupUi() {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_stack = new QStackedWidget(this);
    layout->addWidget(m_stack);

    // Page 0: status overlay (launching / not running / relaunch).
    m_overlay = new QWidget(this);
    auto *overlayLayout = new QVBoxLayout(m_overlay);
    overlayLayout->addStretch();
    m_overlayText = new QLabel(QStringLiteral("Launching FastHub_Telegram..."), m_overlay);
    m_overlayText->setAlignment(Qt::AlignCenter);
    overlayLayout->addWidget(m_overlayText);

    auto *buttons = new QHBoxLayout();
    buttons->addStretch();
    auto *relaunch =
        new QPushButton(QStringLiteral("Launch / Reattach FastHub_Telegram"), m_overlay);
    relaunch->setProperty("primary", true);
    connect(relaunch, &QPushButton::clicked, this, &ObsidianEmbedWidget::launchOrAttach);
    buttons->addWidget(relaunch);

    // Manual fallback only - Telegram never silently degrades to web mode.
    auto *webBtn = new QPushButton(QStringLiteral("Use Web Version"), m_overlay);
    connect(webBtn, &QPushButton::clicked, this, &ObsidianEmbedWidget::showWebFallback);
    buttons->addWidget(webBtn);
    buttons->addStretch();
    overlayLayout->addLayout(buttons);
    overlayLayout->addStretch();
    m_stack->addWidget(m_overlay);

    // Page 1: empty placeholder - the FastHub_Telegram top-level window floats above.
    m_container = new QWidget(this);
    m_stack->addWidget(m_container);

    // Startup poll: wait for the ready top-level window to appear.
    connect(&m_pollTimer, &QTimer::timeout, this, &ObsidianEmbedWidget::pollForWindow);
    m_pollTimer.setInterval(300);

    // Slow safety net for everything the WinEvent hook might miss. The old
    // per-1.5-s chrome polling is gone: style/location changes now arrive
    // event-driven via SetWinEventHook.
    connect(&m_fallbackTimer, &QTimer::timeout, this, [this]() {
        if (m_state == EmbedState::Attached && InterfaceSettings::instance().ayuStripChrome()) {
            applyFramelessStyle(m_ayuHwnd);
        }
        refreshOverlay();
    });
    m_fallbackTimer.setInterval(10000);

    // Instant sync on tab switches (the eventFilter below only sees
    // top-level move/resize/activation).
    connect(m_stack, &QStackedWidget::currentChanged, this, [this](int) { refreshOverlay(); });

    qApp->installEventFilter(this);
}

bool ObsidianEmbedWidget::eventFilter(QObject *obj, QEvent *event) {
    if (m_attached && obj == window()) {
        switch (event->type()) {
        case QEvent::Move:
        case QEvent::Resize:
        case QEvent::WindowStateChange:
        case QEvent::WindowActivate:
        case QEvent::WindowDeactivate:
        case QEvent::Show:
        case QEvent::Hide:
            refreshOverlay();
            break;
        default:
            break;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void ObsidianEmbedWidget::setState(EmbedState next, const char *reason) {
    if (m_state == next) {
        return;
    }
    infrastructure::Logger::instance().debug(
        "Telegram",
        QStringLiteral("state: %1 -> %2 (%3)").arg(stateName(m_state), stateName(next), reason));
    m_state = next;
}

void ObsidianEmbedWidget::launchOrAttach() {
    infrastructure::Logger::instance().info(
        "Telegram", QStringLiteral("launchOrAttach requested (state=%1).").arg(stateName(m_state)));
    if (m_state == EmbedState::ShuttingDown || m_state == EmbedState::ZombieCleanup) {
        return; // Guard: no re-entry during teardown/cleanup
    }

    // Already attached and alive? Nothing to do.
    if (m_state == EmbedState::Attached && m_ayuHwnd &&
        IsWindow(reinterpret_cast<HWND>(m_ayuHwnd))) {
        m_stack->setCurrentWidget(m_container);
        refreshOverlay();
        return;
    }

    // Attach to an already-running instance first (single-instance apps).
    // allowHidden=true: our overlay may have the window hidden right now -
    // that is still a perfectly healthy instance to attach to.
    ayu::WindowTracker::Query query;
    query.imageName = QStringLiteral("Obsidian.exe");
    query.allowHidden = true;
    const HWND existing = ayu::WindowTracker::findWindow(query);
    if (existing) {
        if (attachNativeWindow(reinterpret_cast<quint64>(existing))) {
            return;
        }
    }

    const QString exe = m_launcher.locateExecutable();
    if (exe.isEmpty()) {
        infrastructure::Logger::instance().warn(
            "Telegram", QStringLiteral("FastHub_Telegram.exe not found (app dir: %1)")
                            .arg(QCoreApplication::applicationDirPath()));
        setState(EmbedState::Idle, "exe not found");
        setOverlay(QStringLiteral("FastHub_Telegram.exe not found in third_party/ayugram."), true);
        return;
    }

    // Zombie guard: an orphaned FastHub_Telegram process from a previous FastHub
    // session (force-killed) owns NO windows and blocks single-instance
    // relaunch. The allowHidden lookup above already ruled out merely
    // hidden healthy instances. ASYNC: taskkill runs detached, we retry
    // after a delay instead of blocking the UI thread with msleep.
    if (m_launcher.hasWindowlessOrphan()) {
        if (m_launcher.zombieKillAttempts() >= ayu::ProcessLauncher::kMaxZombieKills) {
            setState(EmbedState::Idle, "zombie kill limit reached");
            setOverlay(
                QStringLiteral(
                    "Stale FastHub_Telegram.exe could not be terminated. Close it manually."),
                true);
            return;
        }
        m_launcher.noteZombieKill();
        setState(EmbedState::ZombieCleanup, "windowless orphan found");
        infrastructure::Logger::instance().warn(
            "Telegram",
            "Found windowless orphan FastHub_Telegram.exe - terminating it before relaunch.");
        m_launcher.terminateAllInstances();
        setOverlay(QStringLiteral("Terminating stale FastHub_Telegram.exe..."), false);
        QTimer::singleShot(1200, this, &ObsidianEmbedWidget::launchOrAttach);
        return;
    }
    m_launcher.resetZombieKills();

    if (!m_process) {
        m_process = new QProcess(this);
        connect(m_process, &QProcess::finished, this, &ObsidianEmbedWidget::onProcessFinished);
        connect(m_process, &QProcess::started, this, [this]() {
            // Assign to Job Object so Task Manager shows it under FastHub
            // and it dies together with FastHub (kill-on-close).
            assignProcessToJob(m_process->processId());
        });
        // Fail fast without blocking the UI thread: start() is async, a
        // FailedToStart error surfaces here instead of waiting for the
        // window-poll timeout.
        connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
            if (error == QProcess::FailedToStart && m_state != EmbedState::ShuttingDown) {
                infrastructure::Logger::instance().warn(
                    "Telegram", QStringLiteral("FastHub_Telegram.exe failed to start (err %1).")
                                    .arg(m_process ? m_process->errorString() : QString()));
                setState(EmbedState::Idle, "process failed to start");
                m_pollTimer.stop();
                setOverlay(QStringLiteral("Failed to start FastHub_Telegram.exe."), true);
            }
        });
    }
    if (m_process->state() != QProcess::Running && m_process->state() != QProcess::Starting) {
        m_process->setWorkingDirectory(QFileInfo(exe).absolutePath());
        setState(EmbedState::Launching, "spawn");
        m_process->start(exe, QStringList());
        // QProcess::started is async; also try immediate assign in case pid is already known
        QTimer::singleShot(300, this, [this]() {
            if (m_process && m_process->processId() > 0) {
                assignProcessToJob(m_process->processId());
            }
        });
    } else {
        setState(EmbedState::PollingWindow, "reuse running process");
        // Already running (attached to existing instance) - try to adopt into job if we spawned it
        if (m_process && m_process->processId() > 0) {
            assignProcessToJob(m_process->processId());
        }
    }

    m_waitTicks = 0;
    setOverlay(QStringLiteral("Launching FastHub_Telegram..."), false);
    m_pollTimer.start();
    setState(EmbedState::PollingWindow, "poll started");
}

void ObsidianEmbedWidget::pollForWindow() {
    if (m_state != EmbedState::PollingWindow) {
        m_pollTimer.stop(); // Guard: stale timer after state change
        return;
    }

    const DWORD pid = m_process ? m_process->processId() : 0;
    ayu::WindowTracker::Query query;
    query.targetPid = pid; // Spawned process: exact pid, cheap path
    const HWND hwnd = ayu::WindowTracker::findWindow(query);
    if (!hwnd) {
        if (++m_waitTicks > 60) { // ~18s
            m_pollTimer.stop();
            setState(EmbedState::Idle, "no window appeared");
            setOverlay(QStringLiteral(
                           "FastHub_Telegram window not detected. Is it showing a login screen?"),
                       true);
        }
        return;
    }
    // Obsidian (Electron) has no placeholder title state; any top-level window
    // owned by the process is considered ready.
    m_pollTimer.stop();
    attachNativeWindow(reinterpret_cast<quint64>(hwnd));
}

bool ObsidianEmbedWidget::attachNativeWindow(quint64 hwndRaw) {
    const HWND hwnd = reinterpret_cast<HWND>(hwndRaw);
    if (!hwnd || m_attached) {
        return m_attached;
    }

    m_ayuHwnd = hwndRaw;
    m_ayuPid = 0;
    GetWindowThreadProcessId(hwnd, &m_ayuPid);
    applyFramelessStyle(hwndRaw);
    installWinEventHook();

    m_stack->setCurrentWidget(m_container);
    m_attached = true;
    m_geometry.resetCache();
    m_lastShown = false;

    setState(EmbedState::Attached, "hwnd captured");
    refreshOverlay();
    m_fallbackTimer.start();

    infrastructure::Logger::instance().info("Telegram",
                                            "FastHub_Telegram window embedded into FastHub.");
    return true;
}

void ObsidianEmbedWidget::applyFramelessStyle(quint64 hwndRaw) {
    const HWND hwnd = reinterpret_cast<HWND>(hwndRaw);
    if (!hwnd || !IsWindow(hwnd)) {
        return;
    }
    // Strip OS chrome (caption/frame/sysmenu) and any WS_CHILD leftovers;
    // keep WS_POPUP so the window remains a normal hit-testable top-level.
    // NOTE: visibility (WS_VISIBLE) is deliberately preserved as-is - it is
    // owned exclusively by refreshOverlay(); touching it here caused a
    // hide/show fight between this guard and the visibility watchdog.
    LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    const LONG_PTR chrome = WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX |
                            WS_SYSMENU | WS_CHILD | WS_BORDER | WS_DLGFRAME;
    const LONG_PTR desired = (style & ~chrome) | WS_POPUP | (style & WS_VISIBLE);
    if (style != desired) {
        SetWindowLongPtrW(hwnd, GWL_STYLE, desired);
        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                     SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
    // Hide from the taskbar / Alt-Tab: FastHub is the only entry.
    // Strip any lingering client/window edges that draw 1px borders.
    const LONG_PTR ex = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    const LONG_PTR desiredEx = ((ex | WS_EX_TOOLWINDOW) & ~WS_EX_APPWINDOW & ~WS_EX_CLIENTEDGE &
                                ~WS_EX_WINDOWEDGE & ~WS_EX_DLGMODALFRAME & ~WS_EX_STATICEDGE);
    if (ex != desiredEx) {
        SetWindowLongPtrW(hwnd, GWL_EXSTYLE, desiredEx);
        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                     SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // Ownership (GWL_HWNDPARENT despite the name sets the OWNER, not the
    // parent): Windows itself keeps an owned window ABOVE its owner in
    // z-order, hides it with the owner and disables it while a modal child
    // is up. This is the OS-level guarantee against FastHub_Telegram sinking UNDER
    // FastHub - independent of any raise-timing races. Re-applied here
    // because FastHub_Telegram's Qt may reset its window state on theme/DPI events.
    const HWND mainHwnd = window() ? reinterpret_cast<HWND>(window()->winId()) : nullptr;
    if (mainHwnd && GetWindow(hwnd, GW_OWNER) != mainHwnd) {
        SetWindowLongPtrW(hwnd, kGwlHwndParent, reinterpret_cast<LONG_PTR>(mainHwnd));
    }

// DWM enhancements for FastHub_Telegram: Windows 11 rounded corners and frame extension
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWCP_ROUND
#define DWMWCP_ROUND 2
#endif
    DWORD cornerPref = DWMWCP_ROUND;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPref, sizeof(cornerPref));

    MARGINS margins = {1, 1, 1, 1};
    DwmExtendFrameIntoClientArea(hwnd, &margins);
}

void ObsidianEmbedWidget::refreshOverlay() {
    if (m_state != EmbedState::Attached || !m_ayuHwnd) {
        return;
    }
    const HWND ayu = reinterpret_cast<HWND>(m_ayuHwnd);

    if (!IsWindow(ayu)) {
        // Distinguish "user closed the window" from "process died": the
        // messages and log levels differ, recovery hints differ too.
        const bool processAlive =
            (m_process && m_process->state() == QProcess::Running) || m_launcher.isProcessRunning();
        detachFromNative(false);
        if (processAlive) {
            infrastructure::Logger::instance().warn(
                "Telegram", "FastHub_Telegram window handle vanished but the process still runs.");
            setOverlay(QStringLiteral("The FastHub_Telegram window was closed, but its process "
                                      "is still running. Reattach or terminate it?"),
                       true);
        } else {
            infrastructure::Logger::instance().info(
                "Telegram", "FastHub_Telegram exited (window gone, process gone).");
            setOverlay(QStringLiteral("FastHub_Telegram was closed."), true);
        }
        return;
    }

    // --- Visibility decision (pure policy, see embed_types.h). ----------
    ayu::VisibilityInput in;
    in.tabVisible = isVisible() && m_stack->currentWidget() == m_container;
    in.iconic = IsIconic(reinterpret_cast<HWND>(window()->winId())) != FALSE;
    in.appActive = window()->isActiveWindow();
    // "User interacts with FastHub_Telegram" = foreground window belongs to the
    // FastHub_Telegram process. PID comparison instead of GA_ROOTOWNER: since we
    // own the overlay window (z-order guarantee), the root owner of any
    // FastHub_Telegram dialog resolves to the FastHub main window, not to ayu.
    const HWND fg = GetForegroundWindow();
    if (fg) {
        DWORD fgPid = 0;
        GetWindowThreadProcessId(fg, &fgPid);
        in.ayuActive = m_ayuPid != 0 && fgPid == m_ayuPid;
    }

    const bool show = ayu::shouldShowEmbed(in);
    const bool currentlyVisible = IsWindowVisible(ayu) != FALSE;

    if (!show) {
        // Early exit BEFORE any rect math: hidden overlay costs nothing.
        if (currentlyVisible) {
            ShowWindow(ayu, SW_HIDE);
        }
        m_lastShown = false;
        return;
    }

    m_geometry.apply(ayu, window(), m_container);

    if (!currentlyVisible) {
        ShowWindow(ayu, SW_SHOWNA); // Never steal focus
    }
    m_lastShown = true;

    // Keep FastHub_Telegram DIRECTLY above the FastHub main window. Unconditional:
    // the previous "raise only if someone got above" check raced with
    // activation - Windows finished raising FastHub AFTER our z-order
    // check, leaving the overlay underneath (user saw an empty rectangle).
    // Reaching here means FastHub is focused (see shouldShowEmbed), so this
    // cannot steal the z-order from unrelated applications.
    SetWindowPos(ayu, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void ObsidianEmbedWidget::handleStyleChange() {
    if (InterfaceSettings::instance().ayuStripChrome()) {
        applyFramelessStyle(m_ayuHwnd);
    }
}

void ObsidianEmbedWidget::installWinEventHook() {
    removeWinEventHook();
    if (!m_winHook) {
        // Event-driven replacement for the old 1.5 s style/move polling:
        // FastHub_Telegram re-applying its chrome or moving triggers these events
        // and we react instantly instead of guessing on a timer.
        m_winHook =
            reinterpret_cast<void *>(SetWinEventHook(kEvtLocationChange, kEvtStyleChange, nullptr,
                                                     &winEventThunk, 0, 0, WINEVENT_OUTOFCONTEXT));
    }
}

void ObsidianEmbedWidget::removeWinEventHook() {
    if (m_winHook) {
        UnhookWinEvent(static_cast<HWINEVENTHOOK>(m_winHook));
        m_winHook = nullptr;
    }
}

void ObsidianEmbedWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    refreshOverlay();
}

void ObsidianEmbedWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    refreshOverlay();
}

void ObsidianEmbedWidget::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);
    refreshOverlay();
}

void ObsidianEmbedWidget::onProcessFinished(int exitCode, QProcess::ExitStatus status) {
    Q_UNUSED(exitCode);
    if (m_state == EmbedState::ShuttingDown) {
        return; // Deliberate shutdown - no user-facing message
    }

    const bool wasAttached = m_state == EmbedState::Attached;
    setState(EmbedState::Idle, "process finished");
    m_pollTimer.stop();
    m_fallbackTimer.stop();
    m_attached = false;
    m_geometry.resetCache();

    if (wasAttached && m_stack->currentWidget() != m_overlay) {
        const bool crashed = status == QProcess::CrashExit;
        infrastructure::Logger::instance().warn(
            "Telegram",
            QStringLiteral("FastHub_Telegram process finished (status %1, exit code %2).")
                .arg(int(status))
                .arg(exitCode));
        setOverlay(crashed ? QStringLiteral("FastHub_Telegram crashed unexpectedly.")
                           : QStringLiteral("FastHub_Telegram was closed."),
                   true);
    } else if (m_stack->currentWidget() != m_overlay) {
        setOverlay(QStringLiteral("FastHub_Telegram was closed."), true);
    }
}

void ObsidianEmbedWidget::detachFromNative(bool restoreRegion) {
    m_pollTimer.stop();
    m_fallbackTimer.stop();
    removeWinEventHook();
    if (m_ayuHwnd) {
        const HWND ayu = reinterpret_cast<HWND>(m_ayuHwnd);
        if (IsWindow(ayu)) {
            ShowWindow(ayu, SW_HIDE);
            // Undo the title-bar cut so a surviving FastHub_Telegram regains chrome.
            if (restoreRegion) {
                SetWindowRgn(ayu, nullptr, TRUE);
            }
        }
        m_ayuHwnd = 0;
        m_ayuPid = 0;
    }
    m_attached = false;
    m_lastShown = false;
    m_geometry.resetCache();
    if (m_state == EmbedState::Attached) {
        setState(EmbedState::Idle, "detached");
    }
}

void ObsidianEmbedWidget::showWebFallback() {
    // Lazy-create the web client page once. Manual action ONLY.
    if (m_stack->count() < 3) {
        const QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
                                 QStringLiteral("/webprofiles/telegram-web");
        QDir().mkpath(dataPath);
        auto *profile = new QWebEngineProfile(QStringLiteral("telegram-web-fallback"), this);
        profile->setPersistentStoragePath(dataPath);
        profile->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);
        auto *view = new QWebEngineView(profile, this);
        view->load(QUrl(QStringLiteral("https://web.telegram.org/a/")));
        m_stack->addWidget(view);
    }
    shutdownEmbeddedProcess();
    m_stack->setCurrentIndex(2);
}

void ObsidianEmbedWidget::setOverlay(const QString &text, bool showRelaunch) {
    m_overlayText->setText(text);
    m_stack->setCurrentWidget(m_overlay);
    refreshOverlay();
    // Relaunch button is the first QPushButton on the overlay row.
    const auto buttons = m_overlay->findChildren<QPushButton *>();
    if (buttons.size() >= 2) {
        buttons[0]->setVisible(showRelaunch);
    }
}

void ObsidianEmbedWidget::shutdownEmbeddedProcess() {
    setState(EmbedState::ShuttingDown, "shutdown");
    detachFromNative(true);
    // Embedded or not, FastHub_Telegram stays the user's app: we never kill it on tab
    // teardown unless WE spawned it in this session and it is still running.
    if (m_process && m_process->state() == QProcess::Running) {
        m_process->disconnect(); // No finished() handling during shutdown
        // Graceful close via window message; hard kill as last resort only.
        // allowHidden=true: the overlay may have the window hidden now.
        ayu::WindowTracker::Query query;
        query.targetPid = m_process->processId();
        query.allowHidden = true;
        const HWND hwnd = ayu::WindowTracker::findWindow(query);
        if (hwnd) {
            PostMessageW(hwnd, WM_CLOSE, 0, 0);
            if (!m_process->waitForFinished(3000)) {
                m_process->kill();
            }
        }
    }
    m_attached = false;
    m_ayuHwnd = 0;
    m_ayuPid = 0;
    setState(EmbedState::Idle, "shutdown complete");
}

void ObsidianEmbedWidget::ensureJobObject() {
    if (m_job) {
        return;
    }
    m_job = CreateJobObjectW(nullptr, nullptr);
    if (!m_job) {
        infrastructure::Logger::instance().warn("Telegram", "CreateJobObject failed");
        return;
    }
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION info{};
    info.BasicLimitInformation.LimitFlags =
        JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE | JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
    if (!SetInformationJobObject(m_job, JobObjectExtendedLimitInformation, &info, sizeof(info))) {
        infrastructure::Logger::instance().warn("Telegram", "SetInformationJobObject failed");
    }
}

void ObsidianEmbedWidget::assignProcessToJob(qint64 pid) {
    if (pid <= 0) {
        return;
    }
    ensureJobObject();
    if (!m_job) {
        return;
    }
    HANDLE hProc =
        OpenProcess(PROCESS_SET_QUOTA | PROCESS_TERMINATE | PROCESS_SET_LIMITED_INFORMATION, FALSE,
                    static_cast<DWORD>(pid));
    if (!hProc) {
        return;
    }
    if (!AssignProcessToJobObject(m_job, hProc)) {
        const DWORD err = GetLastError();
        // ERROR_ACCESS_DENIED (5) or ERROR_ALREADY_ASSIGNED (5? 0x5) means process already in
        // another job (e.g., OS job). Silent breakaway allows nesting on Win8+ if parent was
        // created with JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK.
        if (err != ERROR_ACCESS_DENIED && err != ERROR_ALREADY_ASSIGNED) {
            infrastructure::Logger::instance().warn(
                "Telegram",
                QStringLiteral("AssignProcessToJobObject failed pid=%1 err=%2").arg(pid).arg(err));
        }
    } else {
        infrastructure::Logger::instance().info(
            "Telegram",
            QStringLiteral("FastHub_Telegram pid=%1 assigned to Job (Task Manager tree)").arg(pid));
    }
    CloseHandle(hProc);
}

void ObsidianEmbedWidget::closeJobObject() {
    if (m_job) {
        CloseHandle(m_job);
        m_job = nullptr;
    }
}

} // namespace fasthub::ui

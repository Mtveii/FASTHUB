#pragma once

#include <QString>

namespace fasthub::ui::ayu {

/// @brief Lifecycle of the embedded AyuGram client.
///
/// One explicit state machine instead of scattered booleans: every callback
/// (timers, process signals, event filter) checks the state before touching
/// shared members, which closes the races between zombie-cleanup, polling
/// and attach paths.
enum class EmbedState {
    Idle,           // Nothing running, overlay offers launch
    Launching,      // QProcess::start() issued, waiting for the process
    PollingWindow,  // Process alive, waiting for its ready top-level window
    Attached,       // HWND captured, geometry/visibility sync active
    ZombieCleanup,  // Killing a windowless orphan before relaunch
    ShuttingDown,   // Teardown in progress; ignore all events
};

/// @brief Human-readable state name (state-transition logging).
inline const char* stateName(EmbedState s) {
    switch (s) {
        case EmbedState::Idle:          return "Idle";
        case EmbedState::Launching:     return "Launching";
        case EmbedState::PollingWindow: return "PollingWindow";
        case EmbedState::Attached:      return "Attached";
        case EmbedState::ZombieCleanup: return "ZombieCleanup";
        case EmbedState::ShuttingDown:  return "ShuttingDown";
    }
    return "?";
}

/// @brief Height of AyuGram's self-drawn title bar (logical px).
///
/// Set to 0 so no top part of AyuGram is clipped.
constexpr int kAyuTitleBarHeight = 0;

/// @brief Image name used for process/window matching.
constexpr wchar_t kAyuImageName[] = L"FastHub_Telegram.exe";

/// @brief Inputs of the overlay visibility decision (unit-testable).
struct VisibilityInput {
    bool tabVisible = false; ///< Telegram tab is current and widget shown
    bool iconic = false;     ///< FastHub main window minimized
    bool appActive = false;  ///< FastHub is the foreground application
    bool ayuActive = false;  ///< User interacts with AyuGram itself
};

/// @brief Overlay visibility policy.
///
/// Show only when the tab is on screen, FastHub is not minimized and either
/// FastHub or AyuGram itself has user focus. Pure function - covered by
/// unit tests in telegram_tests.
inline bool shouldShowEmbed(const VisibilityInput& in) {
    return in.tabVisible && !in.iconic && (in.appActive || in.ayuActive);
}

} // namespace fasthub::ui::ayu

#pragma once

#include <QString>

#include <windows.h>

namespace fasthub::ui::ayu {

/// @brief Finds and readiness-checks AyuGram's top-level window.
///
/// Extracted from the widget so the next embedded-chat module (Discord /
/// Steam native clients) can reuse the same machinery with different
/// image names.
class WindowTracker {
public:
    /// @brief Matching policy for findWindow().
    struct Query {
        DWORD targetPid = 0;      ///< 0 = match by image name only
        QString imageName;        ///< e.g. "ayugram.exe" (case-insensitive)
        bool allowHidden = false; ///< count windows hidden by our overlay
        HWND found = nullptr;     ///< result of findWindow()
    };

    /// @brief Enumerates top-level windows, returns the first match.
    ///
    /// Priority: exact pid match (no image-name probing needed) -> image
    /// name. Image-name lookups are cached per pid for the duration of one
    /// call so a full-system scan does not run OpenProcess +
    /// QueryFullProcessImageNameW per window on every poll tick.
    static HWND findWindow(const Query& query);

    /// @brief True when the window finished initializing.
    ///
    /// AyuGram rebuilds its window tree during startup; grabbing too early
    /// corrupts its UI (the finished UI never receives input). The
    /// initializing window still carries the placeholder title
    /// 'AyuGramDesktop'; the ready one is 'AyuGram' / 'AyuGram (n)'.
    static bool isWindowTitleReady(HWND hwnd);

    /// @brief True when any process with the given image name is running.
    static bool isProcessRunning(const QString& imageName);

private:
    static bool titleMatchesReadyState(const wchar_t* title, size_t len);
};

} // namespace fasthub::ui::ayu

#pragma once

#include <QRect>
#include <QString>

#include <windows.h>

class QWidget;

namespace fasthub::ui::ayu {

/// @brief Pure target-rect math for the overlay window (unit-testable).
///
/// The overlay must cover the tab container exactly, minus AyuGram's
/// self-drawn title strip: the window is made TALLER by the strip and
/// shifted UP, so after SetWindowRgn cuts the strip the visible content
/// fits edge-to-edge.
///
/// All inputs are mixed logical/native on purpose - this mirrors exactly
/// what the Win32 side measures:
///   - @p clientOrigin    native screen coords of the main window client area
///   - @p clientSize      native size of the main window client area
///   - @p windowLogical   logical size of the main window
///   - @p containerRel    container offset inside the main window, logical
///   - @p containerSize   container size, logical
///   - @p titleBarHeight  strip height, logical px
struct OverlayRect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    int titlePx = 0; ///< Strip height in native px (for SetWindowRgn)
};

/// Computes the overlay rect. Safe against zero/negative sizes (returns a
/// degenerate rect instead of dividing by zero).
OverlayRect computeOverlayRect(const POINT& clientOrigin,
                               const SIZE& clientSize,
                               const QSize& windowLogical,
                               const QPoint& containerRel,
                               const QSize& containerSize,
                               int titleBarHeight);

/// @brief Owns geometry application for one embedded HWND.
///
/// Responsibilities:
///   - measure the FastHub main window via Win32 and compute the target rect
///   - call MoveWindow only when the rect actually changed
///   - call SetWindowRgn only when the REGION shape changed (moving without
///     resizing used to re-create regions needlessly; failed SetWindowRgn
///     leaked the HRGN)
class GeometrySync {
public:
    /// @brief Result of an apply attempt.
    enum class ApplyResult { Skipped, Moved, Failed };

    void resetCache();

    /// @brief Measures the main window and moves/regions @p ayu if needed.
    /// @param mainWindow top-level FastHub window (owner of the tab).
    /// @param container  widget whose bounds define the overlay area.
    ApplyResult apply(HWND ayuHwnd, QWidget* mainWindow, QWidget* container);

private:
    bool m_lastValid = false;
    int m_lastX = 0;
    int m_lastY = 0;
    int m_lastW = 0;
    int m_lastH = 0;
    // Region shape cache: MoveWindow with only x/y changed must NOT
    // recreate the region.
    bool m_lastRgnValid = false;
    int m_lastRgnW = 0;
    int m_lastRgnH = 0;
    int m_lastRgnTitlePx = 0;
};

} // namespace fasthub::ui::ayu

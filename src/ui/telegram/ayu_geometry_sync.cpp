#include "ayu_geometry_sync.h"
#include "embed_types.h"

#include <QWidget>

#include "infrastructure/logger.h"

namespace fasthub::ui::ayu {

OverlayRect computeOverlayRect(const POINT& clientOrigin,
                               const SIZE& clientSize,
                               const QSize& windowLogical,
                               const QPoint& containerRel,
                               const QSize& containerSize,
                               int titleBarHeight) {
    if (clientSize.cx <= 0 || clientSize.cy <= 0 ||
        windowLogical.width() <= 0 || windowLogical.height() <= 0) {
        return {};
    }

    // Inside ONE window the logical->native scale is uniform, so measuring
    // the container relative to the main window and scaling by
    // nativeClient/logicalClient is exact on any mix of monitor DPIs.
    const double sx = static_cast<double>(clientSize.cx) / windowLogical.width();
    const double sy = static_cast<double>(clientSize.cy) / windowLogical.height();

    OverlayRect r;
    r.titlePx = qRound(titleBarHeight * sy);
    r.x = clientOrigin.x + qRound(containerRel.x() * sx);
    // Shift up by the strip: the region cut then reveals exactly the
    // client content inside the tab bounds.
    r.y = clientOrigin.y + qRound(containerRel.y() * sy) - r.titlePx;
    r.w = qRound(containerSize.width() * sx);
    r.h = qRound(containerSize.height() * sy) + r.titlePx;
    return r;
}

void GeometrySync::resetCache() {
    m_lastValid = false;
    m_lastRgnValid = false;
}

GeometrySync::ApplyResult GeometrySync::apply(HWND ayuHwnd,
                                              QWidget* mainWindow,
                                              QWidget* container) {
    if (!ayuHwnd || !mainWindow || !container || !container->isVisible()) {
        return ApplyResult::Skipped;
    }
    const HWND mainHwnd = reinterpret_cast<HWND>(mainWindow->winId());
    if (!mainHwnd) {
        return ApplyResult::Skipped;
    }

    RECT clientRect{};
    if (!GetClientRect(mainHwnd, &clientRect)) {
        return ApplyResult::Failed;
    }
    const SIZE clientSize{clientRect.right - clientRect.left,
                          clientRect.bottom - clientRect.top};
    if (clientSize.cx <= 0 || clientSize.cy <= 0) {
        return ApplyResult::Skipped; // Minimized / still laying out
    }
    POINT clientOrigin{0, 0};
    if (!ClientToScreen(mainHwnd, &clientOrigin)) {
        return ApplyResult::Failed;
    }

    const OverlayRect r = computeOverlayRect(
        clientOrigin, clientSize, mainWindow->size(),
        container->mapTo(mainWindow, QPoint(0, 0)), container->size(),
        kAyuTitleBarHeight);

    ApplyResult result = ApplyResult::Skipped;
    if (!m_lastValid || r.x != m_lastX || r.y != m_lastY ||
        r.w != m_lastW || r.h != m_lastH) {
        if (!MoveWindow(ayuHwnd, r.x, r.y, r.w, r.h, TRUE)) {
            infrastructure::Logger::instance().warn(
                "Telegram", QStringLiteral("MoveWindow failed (err %1).")
                                .arg(GetLastError()));
            return ApplyResult::Failed;
        }
        m_lastX = r.x; m_lastY = r.y; m_lastW = r.w; m_lastH = r.h;
        m_lastValid = true;
        result = ApplyResult::Moved;

        infrastructure::Logger::instance().debug("Telegram",
            QStringLiteral("geo: origin=%1,%2 rel=%3,%4 scale=%5 -> %6,%7 %8x%9")
                .arg(clientOrigin.x).arg(clientOrigin.y)
                .arg(container->mapTo(mainWindow, QPoint(0, 0)).x())
                .arg(container->mapTo(mainWindow, QPoint(0, 0)).y())
                .arg(double(clientSize.cx) / mainWindow->width(), 0, 'f', 2)
                .arg(r.x).arg(r.y).arg(r.w).arg(r.h));
    }

    // Region only when its SHAPE changed - not on every move.
    if (!m_lastRgnValid || r.w != m_lastRgnW || r.h != m_lastRgnH ||
        r.titlePx != m_lastRgnTitlePx) {
        if (r.titlePx <= 0) {
            SetWindowRgn(ayuHwnd, nullptr, TRUE);
        } else {
            HRGN region = CreateRectRgn(0, r.titlePx, r.w, r.h);
            if (!region) {
                infrastructure::Logger::instance().error(
                    "Telegram", "CreateRectRgn failed.");
                return ApplyResult::Failed;
            }
            // SetWindowRgn takes ownership of the region ONLY on success;
            // a failed call would leak the GDI object.
            if (SetWindowRgn(ayuHwnd, region, FALSE) == 0) {
                DeleteObject(region);
                infrastructure::Logger::instance().warn(
                    "Telegram", QStringLiteral("SetWindowRgn failed (err %1).")
                                    .arg(GetLastError()));
                return ApplyResult::Failed;
            }
        }
        m_lastRgnW = r.w;
        m_lastRgnH = r.h;
        m_lastRgnTitlePx = r.titlePx;
        m_lastRgnValid = true;
    }

    return result;
}

} // namespace fasthub::ui::ayu

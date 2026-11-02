#include "canvas_view.h"

#include <QWheelEvent>
#include <QKeyEvent>
#include <QScrollBar>
#include <QApplication>

namespace fasthub::ui {

CanvasView::CanvasView(QWidget *parent) : QGraphicsView(parent) {
    setRenderHint(QPainter::Antialiasing);
    setBackgroundBrush(QColor("#121418"));
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setCacheMode(QGraphicsView::CacheBackground);
    setDragMode(QGraphicsView::NoDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
}

void CanvasView::updateDragMode() {
    if (m_spaceHeld) {
        setDragMode(QGraphicsView::ScrollHandDrag);
        viewport()->setCursor(Qt::OpenHandCursor);
    } else {
        setDragMode(QGraphicsView::NoDrag);
        viewport()->unsetCursor();
    }
}

void CanvasView::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spaceHeld = true;
        updateDragMode();
    }
    QGraphicsView::keyPressEvent(event);
}

void CanvasView::keyReleaseEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spaceHeld = false;
        updateDragMode();
    }
    QGraphicsView::keyReleaseEvent(event);
}

void CanvasView::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::MiddleButton) {
        setDragMode(QGraphicsView::ScrollHandDrag);
        // synthesize left-button press for hand drag
        QMouseEvent fake(QEvent::MouseButtonPress, event->pos(), Qt::LeftButton, Qt::LeftButton,
                         event->modifiers());
        QGraphicsView::mousePressEvent(&fake);
        return;
    }
    QGraphicsView::mousePressEvent(event);
}

void CanvasView::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::MiddleButton) {
        QMouseEvent fake(QEvent::MouseButtonRelease, event->pos(), Qt::LeftButton, Qt::LeftButton,
                         event->modifiers());
        QGraphicsView::mouseReleaseEvent(&fake);
        if (!m_spaceHeld)
            setDragMode(QGraphicsView::NoDrag);
        return;
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void CanvasView::wheelEvent(QWheelEvent *event) {
    // Ctrl+wheel → zoom, plain wheel → scroll (default)
    if (event->modifiers() & Qt::ControlModifier) {
        constexpr double kZoomStep = 1.15;
        double factor = event->angleDelta().y() > 0 ? kZoomStep : 1.0 / kZoomStep;
        // Clamp zoom range
        double cur = transform().m11();
        double next = cur * factor;
        if (next < 0.15 || next > 4.0) {
            event->accept();
            return;
        }
        scale(factor, factor);
        event->accept();
        return;
    }
    QGraphicsView::wheelEvent(event);
}

void CanvasView::resizeEvent(QResizeEvent *event) {
    QGraphicsView::resizeEvent(event);
    // Keep scene rect ample so panning doesn't hit edge immediately
    if (scene()) {
        scene()->setSceneRect(scene()->itemsBoundingRect().adjusted(-400, -400, 400, 400));
    }
}

} // namespace fasthub::ui

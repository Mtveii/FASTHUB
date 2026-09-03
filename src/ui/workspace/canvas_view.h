#pragma once

#include <QGraphicsView>

namespace fasthub::ui {

/// @brief QGraphicsView for canvas — pan (Space+drag / middle-drag) and Ctrl+wheel zoom.
/// Reuses GraphView zoom/pan pattern, no duplication of physics.
class CanvasView : public QGraphicsView {
    Q_OBJECT
public:
    explicit CanvasView(QWidget* parent = nullptr);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    bool m_spaceHeld = false;
    void updateDragMode();
};

} // namespace fasthub::ui

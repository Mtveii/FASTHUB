#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>

#include "core/canvas/canvas_document.h"

namespace fasthub::ui {

class CanvasScene;
class CanvasView;

class CanvasWidget : public QWidget {
    Q_OBJECT
public:
    explicit CanvasWidget(QWidget* parent = nullptr);
    explicit CanvasWidget(const QString& filePath, QWidget* parent = nullptr);
    ~CanvasWidget() override = default;

    /// @brief Load canvas file and populate scene. Returns false on parse error (shows error, not crash).
    bool loadFromFile(const QString& filePath);

    /// @brief Persist current document to m_filePath atomically (QSaveFile). Returns true on success.
    bool saveToFile() const;

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void setupUi();
    void updateWidgetPositions();
    void createCardWidget(const QString& text, int x, int y, int w, int h, const QString& colorId);

    QString m_filePath;
    core::canvas::CanvasDocument m_document;

    CanvasScene* m_scene = nullptr;
    CanvasView* m_view = nullptr;

    QWidget* m_topRightToolbar = nullptr;
    QWidget* m_bottomCenterToolbar = nullptr;
    QLabel* m_hintLabel = nullptr;
    QLabel* m_backlinksLabel = nullptr;
};

} // namespace fasthub::ui

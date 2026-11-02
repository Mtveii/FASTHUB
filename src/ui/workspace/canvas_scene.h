#pragma once

#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsPathItem>
#include <QGraphicsProxyWidget>
#include <QHash>

#include "core/canvas/canvas_document.h"

namespace fasthub::ui {

/// @brief Graphics item for a single canvas node.
/// Handles text/file/link/group types.
class CanvasNodeItem : public QGraphicsRectItem {
public:
    explicit CanvasNodeItem(const core::canvas::CanvasNode &node, const QString &canvasFilePath,
                            QGraphicsItem *parent = nullptr);

    /// @brief Bounding rect in scene coords.
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QString nodeId() const { return m_id; }

private:
    QString m_id;
    core::canvas::CanvasNodeType m_type;
    QString m_text;
    QString m_filePath;
    QString m_url;
    QString m_label;
    QColor m_color;
    QGraphicsProxyWidget *m_proxy = nullptr;
};

/// @brief Edge path between two nodes with side-aware cubic curves.
class CanvasEdgeItem : public QGraphicsPathItem {
public:
    CanvasEdgeItem(const core::canvas::CanvasEdge &edge, QGraphicsItem *from, QGraphicsItem *to);

    void updatePath();

private:
    core::canvas::CanvasSide m_fromSide;
    core::canvas::CanvasSide m_toSide;
    QGraphicsItem *m_fromItem = nullptr;
    QGraphicsItem *m_toItem = nullptr;
    QString m_label;
    QColor m_color;

    static QPointF sidePoint(const QRectF &rect, core::canvas::CanvasSide side);
};

/// @brief QGraphicsScene rendering a CanvasDocument.
/// Order: groups (background) → edges → text/file/link nodes on top.
class CanvasScene : public QGraphicsScene {
    Q_OBJECT
public:
    explicit CanvasScene(QObject *parent = nullptr);

    /// @brief Replace scene contents from document. Resolves file paths against canvasFilePath.
    void setDocument(const core::canvas::CanvasDocument &doc, const QString &canvasFilePath);

    /// @brief Clear all items.
    void clearCanvas();

private:
    QHash<QString, CanvasNodeItem *> m_nodeItems;
    QVector<CanvasEdgeItem *> m_edgeItems;
};

} // namespace fasthub::ui

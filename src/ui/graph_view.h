#pragma once

#include <QGraphicsView>
#include <QGraphicsObject>
#include <QVector>
#include <QString>
#include <QTimer>

class QGraphicsLineItem;

namespace fasthub::ui {

/// @brief One note node on the graph canvas.
///
/// Emits clicked(index) on a genuine click (press + release without drag).
/// Physics data lives in GraphView's model; items only draw themselves.
class NoteNodeItem final : public QGraphicsObject {
    Q_OBJECT
public:
    NoteNodeItem(QString name, int index, float radius);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    int index() const { return m_index; }
    float radius() const { return m_radius; }
    void addIncidentEdge(QGraphicsLineItem *line, bool isStart);

signals:
    /// @brief Emitted on click (not drag); @p index is the model node index.
    void clicked(int index);
    void positionDropped();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    QString m_name;
    int m_index;
    float m_radius;
    QPointF m_pressPos;
    QVector<QPair<QGraphicsLineItem *, bool>> m_incidentEdges;
};

/// @brief Obsidian-style link graph for the workspace notes.
///
/// Scans all Markdown files under the workspace root, extracts [[wiki link]]
/// connections and lays the result out with a simple force-directed
/// simulation (repulsion + edge springs + centering). Nodes are draggable;
/// clicking a node emits noteActivated() with the file path.
///
/// Bounded work: scanning caps at kMaxFiles files / kMaxNodes nodes so a
/// huge folder cannot freeze the UI thread (the scan is synchronous but
/// strictly bounded).
class GraphView : public QGraphicsView {
    Q_OBJECT
public:
    explicit GraphView(QWidget *parent = nullptr);
    ~GraphView() override;

    /// @brief Rebuilds the graph for the given workspace root (empty = clear).
    void setWorkspaceRoot(const QString &root);
    void saveGraphCache();

signals:
    /// @brief Emitted when the user clicks a node; @p path is absolute.
    void noteActivated(const QString &path);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private slots:
    void simulationTick();

private:
    struct Node {
        QString name;
        QString path;
        QPointF pos;
        QPointF vel;
        float radius = 10.0f;
        int degree = 0;
        bool pinned = false;
        NoteNodeItem *item = nullptr;
    };
    struct Edge {
        int from = -1;
        int to = -1;
        QGraphicsLineItem *line = nullptr;
    };

    void clearGraph();
    void scanNotes();
    void buildScene();
    void startSimulation();
    void applyPositions();

    QString m_root;
    QVector<Node> m_nodes;
    QVector<Edge> m_edges;
    QTimer m_simTimer;
    qreal m_alpha = 1.0;
};

} // namespace fasthub::ui

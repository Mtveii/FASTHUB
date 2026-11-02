#include "graph_view.h"

#include <QApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QRegularExpression>
#include <QWheelEvent>
#include <QPainter>
#include <QtConcurrent>
#include <QJsonDocument>
#include <QJsonObject>

#include <cmath>

#include "infrastructure/logger.h"

namespace fasthub::ui {

namespace {

// Hard bounds so a giant folder cannot hang the scan or the simulation.
constexpr int kMaxFilesScanned = 20000;
constexpr int kMaxNodes = 500;
constexpr int kSimTickMs = 16;
constexpr qreal kMinAlpha = 0.005;

// Physics tuning (px units).
constexpr qreal kRepulsion = 90000.0;  // Coulomb-like constant
constexpr qreal kSpringLength = 150.0; // Rest length of a link
constexpr qreal kSpringStiffness = 0.02;
constexpr qreal kGravity = 0.015;
constexpr qreal kMaxStep = 25.0; // Per-tick position clamp

// Palette (matches the FastHub dark workspace theme).
const QColor kBackgroundColor(0x1B, 0x1E, 0x27);
const QColor kNodeFill(0x6E, 0x7B, 0xFF);
const QColor kNodeBorder(0xC9, 0xA8, 0x57);
const QColor kEdgeColor(0x3A, 0x40, 0x55);
const QColor kTextColor(0xE8, 0xEA, 0xF2);

} // namespace

// ---------------------------------------------------------------- node ---

NoteNodeItem::NoteNodeItem(QString name, int index, float radius)
    : m_name(std::move(name)), m_index(index), m_radius(radius) {
    setFlag(QGraphicsItem::ItemIsMovable); // scene handles dragging
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setZValue(1.0);
    setToolTip(m_name);
}

QRectF NoteNodeItem::boundingRect() const {
    QFont f = scene() ? scene()->font() : QApplication::font();
    f.setPointSizeF(f.pointSizeF() * 0.85);
    const QFontMetrics fm(f);
    const int textW = fm.horizontalAdvance(m_name);
    const qreal w = qMax<qreal>(m_radius * 2.0, textW + 12.0);
    const qreal labelH = fm.height() + 4.0;
    // Circle + label underneath.
    return QRectF(-w / 2.0, -m_radius, w, m_radius * 2.0 + labelH);
}

void NoteNodeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                         QWidget *widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing);

    painter->setBrush(isSelected() ? kNodeBorder : kNodeFill);
    painter->setPen(QPen(kNodeBorder, 1.5));
    painter->drawEllipse(QPointF(0, 0), m_radius, m_radius);

    painter->setPen(isSelected() ? kNodeFill : kTextColor);
    QFont f = painter->font();
    f.setPointSizeF(f.pointSizeF() * 0.85);
    painter->setFont(f);
    const QFontMetrics fm(f);
    const int labelH = fm.height();
    const QRectF bounds = boundingRect();
    painter->drawText(QRectF(bounds.left(), bounds.bottom() - labelH, bounds.width(), labelH),
                      Qt::AlignHCenter | Qt::AlignVCenter, m_name);
}

void NoteNodeItem::addIncidentEdge(QGraphicsLineItem *line, bool isStart) {
    m_incidentEdges.append({line, isStart});
}

void NoteNodeItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    m_pressPos = event->scenePos();
    setZValue(1000.0);
    setCursor(Qt::ClosedHandCursor);
    QGraphicsObject::mousePressEvent(event);
}

void NoteNodeItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    setZValue(1.0);
    unsetCursor();
    QGraphicsObject::mouseReleaseEvent(event);
    if ((event->scenePos() - m_pressPos).manhattanLength() < QApplication::startDragDistance()) {
        emit clicked(m_index);
    } else {
        emit positionDropped();
    }
}

QVariant NoteNodeItem::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == QGraphicsItem::ItemPositionHasChanged) {
        const QPointF newPos = value.toPointF();
        for (const auto &p : m_incidentEdges) {
            QGraphicsLineItem *line = p.first;
            QLineF l = line->line();
            if (p.second) {
                l.setP1(newPos);
            } else {
                l.setP2(newPos);
            }
            line->setLine(l);
        }
    } else if (change == QGraphicsItem::ItemSelectedHasChanged) {
        update();
    }
    return QGraphicsObject::itemChange(change, value);
}

// ---------------------------------------------------------------- view ---

GraphView::GraphView(QWidget *parent) : QGraphicsView(parent) {
    auto *sceneObj = new QGraphicsScene(this);
    setScene(sceneObj);
    setRenderHint(QPainter::Antialiasing);
    setBackgroundBrush(kBackgroundColor);
    sceneObj->setBackgroundBrush(kBackgroundColor);

    m_simTimer.setInterval(kSimTickMs);
    connect(&m_simTimer, &QTimer::timeout, this, &GraphView::simulationTick);
}

GraphView::~GraphView() { saveGraphCache(); }

void GraphView::saveGraphCache() {
    if (m_root.isEmpty() || m_nodes.isEmpty())
        return;
    QDir().mkpath(m_root + "/.fasthub");
    const QString cachePath = m_root + "/.fasthub/graph_cache.json";
    QJsonObject obj;
    for (const auto &node : m_nodes) {
        QJsonObject val;
        val["x"] = node.pos.x();
        val["y"] = node.pos.y();
        val["mtime"] = QFileInfo(node.path).lastModified().toSecsSinceEpoch();
        val["pinned"] = node.pinned;
        obj.insert(node.path, val);
    }
    QFile file(cachePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson());
    }
}

void GraphView::setWorkspaceRoot(const QString &root) {
    m_root = root;
    clearGraph();
    if (root.isEmpty() || !QDir(root).exists()) {
        return;
    }

    auto *loadingText = scene()->addText(QStringLiteral("Calculating graph layout..."));
    loadingText->setDefaultTextColor(kTextColor);

    QFuture<void> future = QtConcurrent::run([this, root]() {
        QHash<QString, QPair<QPair<QPointF, bool>, qint64>> cache;
        const QString cachePath = root + "/.fasthub/graph_cache.json";
        QFile cacheFile(cachePath);
        if (cacheFile.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(cacheFile.readAll());
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                for (auto it = obj.begin(); it != obj.end(); ++it) {
                    QJsonObject val = it.value().toObject();
                    qreal x = val["x"].toDouble();
                    qreal y = val["y"].toDouble();
                    qint64 mtime = val["mtime"].toInt();
                    bool pinned = val["pinned"].toBool();
                    cache.insert(it.key(), {{QPointF(x, y), pinned}, mtime});
                }
            }
        }

        QVector<Node> nodes;
        QVector<Edge> edges;
        QHash<QString, int> byName;
        byName.reserve(64);

        QDirIterator it(root, {QStringLiteral("*.md"), QStringLiteral("*.markdown")}, QDir::Files,
                        QDirIterator::Subdirectories);
        int scanned = 0;
        while (it.hasNext() && scanned < kMaxFilesScanned && nodes.size() < kMaxNodes) {
            const QString path = it.next();
            ++scanned;
            const QString rel = QDir(root).relativeFilePath(path);
            if (rel.startsWith(QStringLiteral(".git")) ||
                rel.startsWith(QStringLiteral(".obsidian")) ||
                rel.contains(QStringLiteral("/node_modules/"))) {
                continue;
            }

            Node node;
            node.name = QFileInfo(path).completeBaseName();
            node.path = path;
            const qint64 mtime = QFileInfo(path).lastModified().toSecsSinceEpoch();
            if (cache.contains(path) && cache.value(path).second == mtime) {
                node.pos = cache.value(path).first.first;
                node.pinned = cache.value(path).first.second;
            } else {
                const qreal angle = (nodes.size() * 2.0 * M_PI) / qMax(1, nodes.size() + 1);
                const qreal r = 150.0 + (nodes.size() % 50);
                node.pos = QPointF(r * std::cos(angle), r * std::sin(angle));
                node.pinned = false;
            }
            byName.insert(node.name.toLower(), nodes.size());
            nodes.append(node);
        }

        static const QRegularExpression linkPattern(QStringLiteral("\\[\\[([^\\]\\|#]+)"));
        for (int i = 0; i < nodes.size(); ++i) {
            QFile file(nodes[i].path);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                continue;
            }
            const QString text = QString::fromUtf8(file.readAll());
            QRegularExpressionMatchIterator mit = linkPattern.globalMatch(text);
            while (mit.hasNext()) {
                const QString target = mit.next().captured(1).trimmed().toLower();
                const int j = byName.value(target, -1);
                if (j > i) {
                    Edge edge;
                    edge.from = i;
                    edge.to = j;
                    edges.append(edge);
                    nodes[i].degree++;
                    nodes[j].degree++;
                }
            }
        }

        for (int i = 0; i < nodes.size(); ++i) {
            const qreal angle = (i * 2.0 * M_PI) / qMax(1, nodes.size());
            const qreal r = 150.0 + (i % 50);
            nodes[i].pos = QPointF(r * std::cos(angle), r * std::sin(angle));
        }

        QMetaObject::invokeMethod(
            this,
            [this, root, nodes, edges]() {
                if (m_root != root)
                    return;
                if (scene())
                    scene()->clear();
                m_nodes = nodes;
                m_edges = edges;

                if (m_nodes.isEmpty()) {
                    auto *placeholder =
                        scene()->addText(QStringLiteral("No Markdown notes in this workspace."));
                    placeholder->setDefaultTextColor(kTextColor);
                    return;
                }
                buildScene();
                startSimulation();
            },
            Qt::QueuedConnection);
    });
}

void GraphView::clearGraph() {
    m_simTimer.stop();
    m_alpha = 1.0;
    if (scene()) {
        scene()->clear(); // Deletes node items and edge lines
    }
    m_nodes.clear();
    m_edges.clear();
}

void GraphView::scanNotes() {
    QHash<QString, int> byName; // Lower-cased note stem -> node index
    byName.reserve(64);

    QDirIterator it(m_root, {QStringLiteral("*.md"), QStringLiteral("*.markdown")}, QDir::Files,
                    QDirIterator::Subdirectories);
    int scanned = 0;
    while (it.hasNext() && scanned < kMaxFilesScanned && m_nodes.size() < kMaxNodes) {
        const QString path = it.next();
        ++scanned;
        // Skip service directories (VCS / app state / package managers).
        const QString rel = QDir(m_root).relativeFilePath(path);
        if (rel.startsWith(QStringLiteral(".git")) || rel.startsWith(QStringLiteral(".obsidian")) ||
            rel.contains(QStringLiteral("/node_modules/"))) {
            continue;
        }

        Node node;
        node.name = QFileInfo(path).completeBaseName();
        node.path = path;
        byName.insert(node.name.toLower(), m_nodes.size());
        m_nodes.append(node);
    }
    if (scanned >= kMaxFilesScanned) {
        infrastructure::Logger::instance().warn(
            "Workspace", QStringLiteral("GraphView: scan hit %1 files cap, graph may be "
                                        "partial.")
                             .arg(kMaxFilesScanned));
    }

    // Second pass: extract [[wiki links]] and wire edges between known nodes.
    static const QRegularExpression linkPattern(QStringLiteral("\\[\\[([^\\]\\|#]+)"));
    for (int i = 0; i < m_nodes.size(); ++i) {
        QFile file(m_nodes[i].path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }
        const QString text = QString::fromUtf8(file.readAll());
        QRegularExpressionMatchIterator mit = linkPattern.globalMatch(text);
        while (mit.hasNext()) {
            const QString target = mit.next().captured(1).trimmed().toLower();
            const int j = byName.value(target, -1);
            if (j > i) { // j <= i: self-loop or duplicate edge - skip
                Edge edge;
                edge.from = i;
                edge.to = j;
                ++m_nodes[i].degree;
                ++m_nodes[j].degree;
                m_edges.append(edge);
            }
        }
    }
}

void GraphView::buildScene() {
    auto *sc = scene();
    if (!sc || m_nodes.isEmpty()) {
        return;
    }

    for (int i = 0; i < m_nodes.size(); ++i) {
        Node &n = m_nodes[i];
        n.radius = 8.0f + std::min(n.degree, 12) * 1.5f;
        n.vel = QPointF(0, 0);
        n.item = new NoteNodeItem(n.name, i, n.radius);
        n.item->setPos(n.pos);
        sc->addItem(n.item);

        connect(n.item, &NoteNodeItem::clicked, this, [this](int index) {
            if (index >= 0 && index < m_nodes.size()) {
                emit noteActivated(m_nodes[index].path);
            }
        });

        connect(n.item, &NoteNodeItem::positionDropped, this, [this, i]() {
            if (i >= 0 && i < m_nodes.size()) {
                m_nodes[i].pinned = true;
                m_nodes[i].pos = m_nodes[i].item->pos();
                saveGraphCache();
            }
        });
    }

    QPen edgePen(kEdgeColor, 1.2);
    for (Edge &e : m_edges) {
        e.line = sc->addLine(QLineF(m_nodes[e.from].pos, m_nodes[e.to].pos), edgePen);
        e.line->setZValue(0.0);
        if (m_nodes[e.from].item) {
            m_nodes[e.from].item->addIncidentEdge(e.line, true);
        }
        if (m_nodes[e.to].item) {
            m_nodes[e.to].item->addIncidentEdge(e.line, false);
        }
    }

    sc->setSceneRect(sc->itemsBoundingRect().adjusted(-200, -200, 200, 200));
}

void GraphView::startSimulation() {
    m_alpha = 1.0;
    if (!m_simTimer.isActive()) {
        m_simTimer.start();
    }
}

void GraphView::applyPositions() {
    for (const Node &n : m_nodes) {
        if (n.item) {
            n.item->setPos(n.pos);
        }
    }
    for (const Edge &e : m_edges) {
        if (e.line) {
            e.line->setLine(QLineF(m_nodes[e.from].pos, m_nodes[e.to].pos));
        }
    }
}

void GraphView::simulationTick() {
    const int n = m_nodes.size();
    if (n == 0) {
        m_simTimer.stop();
        return;
    }

    // A node grabbed by the user follows the mouse only.
    int draggedIndex = -1;
    for (int i = 0; i < n; ++i) {
        if (m_nodes[i].item && m_nodes[i].item->isSelected()) {
            draggedIndex = i;
            break;
        }
    }
    if (draggedIndex >= 0 && m_nodes[draggedIndex].item) {
        m_nodes[draggedIndex].pos = m_nodes[draggedIndex].item->pos();
    }

    // Repulsion (O(n^2), bounded by kMaxNodes).
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            qreal dx = m_nodes[i].pos.x() - m_nodes[j].pos.x();
            qreal dy = m_nodes[i].pos.y() - m_nodes[j].pos.y();
            qreal d2 = dx * dx + dy * dy;
            if (d2 < 100.0) {
                d2 = 100.0; // Avoid division blowup on overlap
            }
            const qreal d = std::sqrt(d2);
            const qreal force = (kRepulsion / d2) * m_alpha;
            dx /= d;
            dy /= d;
            m_nodes[i].vel += QPointF(dx * force, dy * force);
            m_nodes[j].vel -= QPointF(dx * force, dy * force);
        }
    }

    // Springs along edges.
    for (const Edge &e : m_edges) {
        QPointF delta = m_nodes[e.to].pos - m_nodes[e.from].pos;
        const qreal d = std::sqrt(delta.x() * delta.x() + delta.y() * delta.y());
        if (d < 1.0) {
            continue;
        }
        const qreal force = (d - kSpringLength) * kSpringStiffness * m_alpha;
        delta *= force / d;
        m_nodes[e.from].vel += delta;
        m_nodes[e.to].vel -= delta;
    }

    // Gravity toward the viewport center keeps the graph from drifting away.
    const QPointF center = mapToScene(viewport()->rect().center());
    for (int i = 0; i < n; ++i) {
        m_nodes[i].vel += (center - m_nodes[i].pos) * kGravity * m_alpha;
    }

    // Integrate with damping and a step clamp.
    const qreal alphaStep = std::min<qreal>(1.0, m_alpha * 2.0);
    for (int i = 0; i < n; ++i) {
        if (i == draggedIndex || m_nodes[i].pinned) {
            continue; // User controls or pinned this node
        }
        QPointF step = m_nodes[i].vel * alphaStep;
        const double len = std::hypot(step.x(), step.y());
        if (len > kMaxStep) {
            step *= kMaxStep / len;
        }
        m_nodes[i].pos += step;
        m_nodes[i].vel *= 0.6;
    }

    applyPositions();

    m_alpha *= 0.99;
    if (m_alpha < kMinAlpha) {
        m_simTimer.stop();
    }
}

void GraphView::resizeEvent(QResizeEvent *event) {
    QGraphicsView::resizeEvent(event);
    // Keep everything reachable when the panel grows/shrinks.
    if (scene()) {
        scene()->setSceneRect(scene()->itemsBoundingRect().adjusted(-200, -200, 200, 200));
    }
}

void GraphView::wheelEvent(QWheelEvent *event) {
    // Ctrl+wheel zooms (plain wheel scrolls like everywhere else).
    if (event->modifiers() & Qt::ControlModifier) {
        constexpr double kZoomStep = 1.15;
        const double factor = event->angleDelta().y() > 0 ? kZoomStep : 1.0 / kZoomStep;
        scale(factor, factor);
        event->accept();
        return;
    }
    QGraphicsView::wheelEvent(event);
}

} // namespace fasthub::ui

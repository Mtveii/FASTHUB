#include "canvas_scene.h"

#include <QGraphicsScene>
#include <QGraphicsProxyWidget>
#include <QTextBrowser>
#include <QPainter>
#include <QPainterPath>
#include <QFileInfo>
#include <QDir>
#include <QStyleOptionGraphicsItem>

#include "ui/markdown_renderer.h"

namespace fasthub::ui {

// ---------------------------------------------------------------- Node ---

CanvasNodeItem::CanvasNodeItem(const core::canvas::CanvasNode &node, const QString &canvasFilePath,
                               QGraphicsItem *parent)
    : QGraphicsRectItem(parent), m_id(node.id), m_type(node.type), m_text(node.text),
      m_filePath(node.filePath), m_url(node.url), m_label(node.label), m_color(node.color()) {
    setRect(QRectF(0, 0, node.geometry.width(), node.geometry.height()));
    setPos(node.geometry.topLeft());
    setFlag(QGraphicsItem::ItemIsMovable, false); // edit mode later; MVP read-only
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setZValue(m_type == core::canvas::CanvasNodeType::Group ? 0 : 2);

    // Background styling via paint(), content via proxy for rich text
    if (m_type == core::canvas::CanvasNodeType::Text ||
        m_type == core::canvas::CanvasNodeType::Group) {
        auto *proxy = new QGraphicsProxyWidget(this);
        auto *browser = new QTextBrowser();
        browser->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        browser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        browser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        browser->setStyleSheet(
            "QTextBrowser { background: transparent; border: none; color: #E2E8F0; }");
        browser->setFrameStyle(QFrame::NoFrame);
        // Render markdown (if text contains md) else plain
        QString html = MarkdownRenderer::renderToHtml(
            m_type == core::canvas::CanvasNodeType::Group
                ? (m_label.isEmpty() ? m_text : m_label + "\n\n" + m_text)
                : m_text);
        browser->setHtml(html);
        browser->setFixedSize(int(rect().width()) - 16, int(rect().height()) - 16);
        proxy->setWidget(browser);
        proxy->setPos(8, 8);
        // LOD: hide rich content when zoomed far out
        m_proxy = proxy;
    } else if (m_type == core::canvas::CanvasNodeType::File) {
        auto *proxy = new QGraphicsProxyWidget(this);
        auto *label = new QTextBrowser();
        QString resolved =
            core::canvas::CanvasDocument::resolveFilePath(canvasFilePath, m_filePath);
        QString display = m_filePath.isEmpty() ? "Untitled" : QFileInfo(resolved).fileName();
        // Lazy: only show icon+name, content loaded on demand (not now)
        label->setHtml(
            QString("<b>📄 %1</b><br><span style='color:#94A3B8;font-size:10px'>%2</span>")
                .arg(display.toHtmlEscaped(), resolved.toHtmlEscaped()));
        label->setStyleSheet(
            "QTextBrowser { background: transparent; border: none; color: #E2E8F0; }");
        label->setFrameStyle(QFrame::NoFrame);
        label->setFixedSize(int(rect().width()) - 16, int(rect().height()) - 16);
        proxy->setWidget(label);
        proxy->setPos(8, 8);
        m_proxy = proxy;
    } else if (m_type == core::canvas::CanvasNodeType::Link) {
        auto *proxy = new QGraphicsProxyWidget(this);
        auto *label = new QTextBrowser();
        label->setHtml(
            QString("<a href='%1' style='color:#60A5FA'>%1</a>").arg(m_url.toHtmlEscaped()));
        label->setOpenExternalLinks(true);
        label->setStyleSheet("QTextBrowser { background: transparent; border: none; }");
        label->setFrameStyle(QFrame::NoFrame);
        label->setFixedSize(int(rect().width()) - 16, int(rect().height()) - 16);
        proxy->setWidget(label);
        proxy->setPos(8, 8);
        m_proxy = proxy;
    }
}

QRectF CanvasNodeItem::boundingRect() const { return rect(); }

void CanvasNodeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                           QWidget *widget) {
    Q_UNUSED(widget);
    const qreal lod = option ? option->levelOfDetailFromTransform(painter->worldTransform()) : 1.0;
    if (lod < 0.15 && m_proxy) {
        // Very far — don't render proxy content for perf (LOD culling)
        m_proxy->hide();
    } else if (m_proxy) {
        m_proxy->show();
    }

    painter->setRenderHint(QPainter::Antialiasing);
    QRectF r = rect();
    const qreal radius = 8.0;

    QColor bg = QColor("#1E293B");
    QColor border = QColor("#334155");
    if (m_color.isValid()) {
        border = m_color;
        if (m_type == core::canvas::CanvasNodeType::Group) {
            bg = QColor(m_color.red(), m_color.green(), m_color.blue(), 30);
        }
    }
    if (m_type == core::canvas::CanvasNodeType::Group) {
        painter->setBrush(bg);
        painter->setPen(QPen(border, 1.5, Qt::DashLine));
        painter->drawRoundedRect(r, radius, radius);
        if (!m_label.isEmpty()) {
            painter->setPen(QColor("#E2E8F0"));
            QFont f = painter->font();
            f.setBold(true);
            painter->setFont(f);
            painter->drawText(r.adjusted(10, 6, -10, 0), Qt::AlignTop | Qt::AlignLeft, m_label);
        }
    } else {
        // Card with left accent
        painter->setBrush(bg);
        painter->setPen(QPen(border, 1.2));
        painter->drawRoundedRect(r, radius, radius);
        if (m_color.isValid()) {
            QRectF accent = QRectF(r.left(), r.top(), 4, r.height());
            painter->fillRect(accent, m_color);
        }
    }

    if (isSelected()) {
        painter->setPen(QPen(QColor("#60A5FA"), 2, Qt::DotLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(r.adjusted(-1, -1, 1, 1), radius, radius);
    }
}

// ---------------------------------------------------------------- Edge ---

QPointF CanvasEdgeItem::sidePoint(const QRectF &rect, core::canvas::CanvasSide side) {
    switch (side) {
    case core::canvas::CanvasSide::Top:
        return QPointF(rect.center().x(), rect.top());
    case core::canvas::CanvasSide::Bottom:
        return QPointF(rect.center().x(), rect.bottom());
    case core::canvas::CanvasSide::Left:
        return QPointF(rect.left(), rect.center().y());
    case core::canvas::CanvasSide::Right:
        return QPointF(rect.right(), rect.center().y());
    default:
        break;
    }
    // Auto: closest side to other node is handled by caller; fallback center
    return rect.center();
}

CanvasEdgeItem::CanvasEdgeItem(const core::canvas::CanvasEdge &edge, QGraphicsItem *from,
                               QGraphicsItem *to)
    : QGraphicsPathItem(), m_fromSide(edge.fromSide), m_toSide(edge.toSide), m_fromItem(from),
      m_toItem(to), m_label(edge.label), m_color(edge.color()) {
    setZValue(1); // between groups (0) and nodes (2)
    setPen(QPen(m_color.isValid() ? m_color : QColor("#475569"), 1.8));
    setBrush(Qt::NoBrush);
    updatePath();
}

void CanvasEdgeItem::updatePath() {
    if (!m_fromItem || !m_toItem)
        return;
    QRectF fr = m_fromItem->sceneBoundingRect();
    QRectF tr = m_toItem->sceneBoundingRect();

    // Resolve None sides automatically: pick nearest edge
    auto resolveAuto = [](const QRectF &a, const QRectF &b,
                          core::canvas::CanvasSide s) -> core::canvas::CanvasSide {
        if (s != core::canvas::CanvasSide::None)
            return s;
        QPointF ac = a.center(), bc = b.center();
        qreal dx = bc.x() - ac.x();
        qreal dy = bc.y() - ac.y();
        if (qAbs(dx) > qAbs(dy))
            return dx > 0 ? core::canvas::CanvasSide::Right : core::canvas::CanvasSide::Left;
        return dy > 0 ? core::canvas::CanvasSide::Bottom : core::canvas::CanvasSide::Top;
    };

    core::canvas::CanvasSide fs = resolveAuto(fr, tr, m_fromSide);
    core::canvas::CanvasSide ts = resolveAuto(tr, fr, m_toSide);
    QPointF p1 = sidePoint(fr, fs);
    QPointF p2 = sidePoint(tr, ts);

    QPainterPath path(p1);
    // Cubic control points offset along side normal
    auto offsetForSide = [](core::canvas::CanvasSide s, qreal d) -> QPointF {
        switch (s) {
        case core::canvas::CanvasSide::Top:
            return QPointF(0, -d);
        case core::canvas::CanvasSide::Bottom:
            return QPointF(0, d);
        case core::canvas::CanvasSide::Left:
            return QPointF(-d, 0);
        case core::canvas::CanvasSide::Right:
            return QPointF(d, 0);
        default:
            return QPointF(0, 0);
        }
    };
    qreal dist = QLineF(p1, p2).length();
    qreal ctrl = qMax(40.0, dist * 0.35);
    QPointF c1 = p1 + offsetForSide(fs, ctrl);
    QPointF c2 = p2 + offsetForSide(ts, ctrl);
    path.cubicTo(c1, c2, p2);
    setPath(path);

    // Arrow head
    // (Simple: Qt doesn't draw arrow automatically; we could add polygon later)
}

// ---------------------------------------------------------------- Scene ---

CanvasScene::CanvasScene(QObject *parent) : QGraphicsScene(parent) {
    setBackgroundBrush(QColor("#121418"));
}

void CanvasScene::clearCanvas() {
    clear();
    m_nodeItems.clear();
    m_edgeItems.clear();
}

void CanvasScene::setDocument(const core::canvas::CanvasDocument &doc,
                              const QString &canvasFilePath) {
    clearCanvas();

    // 1) Groups first (background containers)
    for (const auto &n : doc.nodes) {
        if (n.type != core::canvas::CanvasNodeType::Group)
            continue;
        auto *item = new CanvasNodeItem(n, canvasFilePath);
        addItem(item);
        m_nodeItems.insert(n.id, item);
    }
    // Reserve scene rect early so edges have bounds
    setSceneRect(itemsBoundingRect().adjusted(-200, -200, 200, 200));

    // 2) Edges next (so they are under non-group nodes but over groups)
    // Need hash for lookup — but edges need both nodes exist; skip if missing
    // We'll create remaining nodes first to guarantee lookup? Instead create all nodes first in
    // order then edges. For correct z-order we inserted groups already; now create remaining nodes,
    // then edges.
    for (const auto &n : doc.nodes) {
        if (n.type == core::canvas::CanvasNodeType::Group)
            continue;
        auto *item = new CanvasNodeItem(n, canvasFilePath);
        addItem(item);
        m_nodeItems.insert(n.id, item);
    }

    for (const auto &e : doc.edges) {
        auto *from = m_nodeItems.value(e.fromNode, nullptr);
        auto *to = m_nodeItems.value(e.toNode, nullptr);
        if (!from || !to) {
            qWarning() << "CanvasScene: edge references missing node" << e.id << e.fromNode
                       << e.toNode;
            continue;
        }
        auto *edgeItem = new CanvasEdgeItem(e, from, to);
        addItem(edgeItem);
        m_edgeItems.push_back(edgeItem);
    }

    // Ensure groups stay at z 0, edges 1, nodes 2 (already set in items)
    // Adjust scene rect to fit all
    setSceneRect(itemsBoundingRect().adjusted(-300, -300, 300, 300));
}

} // namespace fasthub::ui

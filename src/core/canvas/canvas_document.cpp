#include "canvas_document.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QDebug>

namespace fasthub::core::canvas {

namespace {
CanvasNodeType nodeTypeFromString(const QString& s) {
    const QString v = s.trimmed().toLower();
    if (v == "file") return CanvasNodeType::File;
    if (v == "link") return CanvasNodeType::Link;
    if (v == "group") return CanvasNodeType::Group;
    return CanvasNodeType::Text;
}
} // namespace

CanvasDocument CanvasDocument::parse(const QByteArray& json, CanvasParseResult* result) {
    CanvasDocument doc;
    if (result) { result->ok = false; result->error.clear(); }

    if (json.trimmed().isEmpty()) {
        if (result) result->error = "Empty canvas JSON";
        return doc;
    }

    QJsonParseError err{};
    QJsonDocument jd = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError) {
        if (result) result->error = QString("JSON parse error at %1: %2").arg(err.offset).arg(err.errorString());
        qWarning() << "Canvas parse error:" << (result ? result->error : "unknown");
        return doc;
    }
    if (!jd.isObject()) {
        if (result) result->error = "Top-level JSON is not an object";
        return doc;
    }
    QJsonObject root = jd.object();

    // nodes — optional, default []
    QJsonArray jNodes = root.value("nodes").toArray();
    for (const auto& v : jNodes) {
        if (!v.isObject()) {
            qWarning() << "Canvas: skip non-object node";
            continue;
        }
        QJsonObject o = v.toObject();
        QString id = o.value("id").toString().trimmed();
        if (id.isEmpty()) {
            qWarning() << "Canvas: skip node without id";
            continue;
        }
        // mandatory geometry — if missing, skip per spec (forward-compat, don't fail whole file)
        if (!o.contains("x") || !o.contains("y") || !o.contains("width") || !o.contains("height")) {
            qWarning() << "Canvas: skip node without geometry:" << id;
            continue;
        }
        CanvasNode n;
        n.id = id;
        n.type = nodeTypeFromString(o.value("type").toString("text"));
        double x = o.value("x").toDouble();
        double y = o.value("y").toDouble();
        double w = o.value("width").toDouble();
        double h = o.value("height").toDouble();
        n.geometry = QRectF(x, y, w, h);
        if (o.contains("color")) {
            QString c = o.value("color").toString().trimmed();
            if (!c.isEmpty()) n.colorRaw = c;
        }
        n.text = o.value("text").toString();
        n.filePath = o.value("file").toString();
        n.url = o.value("url").toString();
        n.label = o.value("label").toString();
        doc.nodes.push_back(std::move(n));
    }

    QJsonArray jEdges = root.value("edges").toArray();
    for (const auto& v : jEdges) {
        if (!v.isObject()) continue;
        QJsonObject o = v.toObject();
        QString from = o.value("fromNode").toString().trimmed();
        QString to = o.value("toNode").toString().trimmed();
        if (from.isEmpty() || to.isEmpty()) {
            qWarning() << "Canvas: skip edge without fromNode/toNode";
            continue;
        }
        CanvasEdge e;
        e.id = o.value("id").toString();
        e.fromNode = from;
        e.toNode = to;
        e.fromSide = sideFromString(o.value("fromSide").toString());
        e.toSide = sideFromString(o.value("toSide").toString());
        if (o.contains("color")) {
            QString c = o.value("color").toString().trimmed();
            if (!c.isEmpty()) e.colorRaw = c;
        }
        e.label = o.value("label").toString();
        doc.edges.push_back(std::move(e));
    }

    if (result) { result->ok = true; result->error.clear(); }
    return doc;
}

CanvasDocument CanvasDocument::parseFile(const QString& canvasFilePath, CanvasParseResult* result) {
    QFile f(canvasFilePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (result) { result->ok = false; result->error = "Cannot open canvas file: " + canvasFilePath; }
        return {};
    }
    QByteArray data = f.readAll();
    // Handle UTF-8 BOM if present
    if (data.startsWith("\xEF\xBB\xBF")) data = data.mid(3);
    return parse(data, result);
}

QString CanvasDocument::resolveFilePath(const QString& canvasFilePath, const QString& nodeFilePath) {
    if (nodeFilePath.isEmpty()) return {};
    if (QDir::isAbsolutePath(nodeFilePath)) return QDir::cleanPath(nodeFilePath);
    // Handle both / and \ separators
    QString normalized = nodeFilePath;
    normalized.replace('\\', '/');
    QDir base = QFileInfo(canvasFilePath).absoluteDir();
    return QDir::cleanPath(base.filePath(normalized));
}

QByteArray CanvasDocument::toJson() const {
    QJsonArray jNodes;
    for (const auto& n : nodes) {
        QJsonObject o;
        o.insert("id", n.id);
        QString typeStr = "text";
        switch (n.type) {
            case CanvasNodeType::Text: typeStr = "text"; break;
            case CanvasNodeType::File: typeStr = "file"; break;
            case CanvasNodeType::Link: typeStr = "link"; break;
            case CanvasNodeType::Group: typeStr = "group"; break;
        }
        o.insert("type", typeStr);
        o.insert("x", n.geometry.x());
        o.insert("y", n.geometry.y());
        o.insert("width", n.geometry.width());
        o.insert("height", n.geometry.height());
        if (n.colorRaw && !n.colorRaw->isEmpty()) o.insert("color", *n.colorRaw);
        if (!n.text.isEmpty()) o.insert("text", n.text);
        if (!n.filePath.isEmpty()) o.insert("file", n.filePath);
        if (!n.url.isEmpty()) o.insert("url", n.url);
        if (!n.label.isEmpty()) o.insert("label", n.label);
        jNodes.append(o);
    }
    QJsonArray jEdges;
    for (const auto& e : edges) {
        QJsonObject o;
        o.insert("id", e.id);
        o.insert("fromNode", e.fromNode);
        o.insert("toNode", e.toNode);
        QString fs = sideToString(e.fromSide);
        QString ts = sideToString(e.toSide);
        if (!fs.isEmpty()) o.insert("fromSide", fs);
        if (!ts.isEmpty()) o.insert("toSide", ts);
        if (e.colorRaw && !e.colorRaw->isEmpty()) o.insert("color", *e.colorRaw);
        if (!e.label.isEmpty()) o.insert("label", e.label);
        jEdges.append(o);
    }
    QJsonObject root;
    root.insert("nodes", jNodes);
    root.insert("edges", jEdges);
    QJsonDocument doc(root);
    return doc.toJson(QJsonDocument::Indented);
}

} // namespace fasthub::core::canvas

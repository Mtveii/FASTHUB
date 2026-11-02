#pragma once

#include <QString>
#include <QColor>
#include <optional>
#include "canvas_node.h"

namespace fasthub::core::canvas {

/// @brief Side of a node where an edge connects.
enum class CanvasSide { None, Top, Right, Bottom, Left };

inline CanvasSide sideFromString(const QString &s) {
    const QString v = s.trimmed().toLower();
    if (v == "top")
        return CanvasSide::Top;
    if (v == "right")
        return CanvasSide::Right;
    if (v == "bottom")
        return CanvasSide::Bottom;
    if (v == "left")
        return CanvasSide::Left;
    return CanvasSide::None;
}

inline QString sideToString(CanvasSide s) {
    switch (s) {
    case CanvasSide::Top:
        return "top";
    case CanvasSide::Right:
        return "right";
    case CanvasSide::Bottom:
        return "bottom";
    case CanvasSide::Left:
        return "left";
    default:
        return "";
    }
}

/// @brief Canvas edge — POD.
struct CanvasEdge {
    QString id;
    QString fromNode;
    QString toNode;
    CanvasSide fromSide = CanvasSide::None;
    CanvasSide toSide = CanvasSide::None;
    std::optional<QString> colorRaw;
    QString label;

    QColor color() const { return resolveCanvasColor(colorRaw); }
};

} // namespace fasthub::core::canvas

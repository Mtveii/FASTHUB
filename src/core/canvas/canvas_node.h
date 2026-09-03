#pragma once

#include <QString>
#include <QRectF>
#include <QColor>
#include <QHash>
#include <optional>

namespace fasthub::core::canvas {

/// @brief Type of a canvas node per JSON Canvas 1.0 spec.
enum class CanvasNodeType {
    Text,
    File,
    Link,
    Group
};

/// @brief Preset color handling: "1".."6" or hex.
inline QColor resolveCanvasColor(const std::optional<QString>& raw) {
    if (!raw) return QColor();
    const QString v = raw->trimmed();
    if (v.isEmpty()) return QColor();
    static const QHash<QString, QColor> presets = {
        {"1", QColor("#FB464C")}, {"2", QColor("#FF8300")}, {"3", QColor("#FFC300")},
        {"4", QColor("#5DB300")}, {"5", QColor("#4472E0")}, {"6", QColor("#9A5CF0")},
    };
    if (presets.contains(v)) return presets.value(v);
    QColor c(v);
    return c.isValid() ? c : QColor();
}

/// @brief Single canvas node — POD, no QObject, no QGraphicsItem.
struct CanvasNode {
    QString id;
    CanvasNodeType type = CanvasNodeType::Text;
    QRectF geometry; ///< x,y,width,height
    std::optional<QString> colorRaw; ///< as in JSON ("1".."6" or "#RRGGBB")
    QString text;   ///< for type == Text
    QString filePath; ///< for type == File (relative to .canvas location)
    QString url;    ///< for type == Link
    QString label;  ///< for type == Group

    /// @brief Resolved QColor or invalid if none.
    QColor color() const { return resolveCanvasColor(colorRaw); }
};

} // namespace fasthub::core::canvas

#pragma once

#include <QVector>
#include <QByteArray>
#include <QString>
#include <QFileInfo>
#include <optional>

#include "canvas_node.h"
#include "canvas_edge.h"

namespace fasthub::core::canvas {

/// @brief Result of parsing a canvas file.
struct CanvasParseResult {
    bool ok = false;
    QString error; ///< non-empty if !ok
};

/// @brief Parsed canvas document — value type, no QObject, no scene dependency.
///        Forward-compatible: unknown fields are dropped, malformed nodes are skipped with warn.
class CanvasDocument {
public:
    QVector<CanvasNode> nodes;
    QVector<CanvasEdge> edges;

    /// @brief Parse from raw JSON bytes. Never throws. Skips invalid nodes/edges with warning.
    /// @param json UTF-8 JSON as per jsoncanvas.org/spec
    /// @param result out param with ok/error
    /// @return parsed document (may be empty on error)
    static CanvasDocument parse(const QByteArray& json, CanvasParseResult* result = nullptr);

    /// @brief Parse from file on disk. Resolves relative file-node paths against canvasFilePath.
    static CanvasDocument parseFile(const QString& canvasFilePath, CanvasParseResult* result = nullptr);

    /// @brief Resolve a file-node relative path against the .canvas location.
    static QString resolveFilePath(const QString& canvasFilePath, const QString& nodeFilePath);

    /// @brief Serialize to JSON Canvas 1.0 bytes (UTF-8, pretty-printed).
    QByteArray toJson() const;
};

} // namespace fasthub::core::canvas

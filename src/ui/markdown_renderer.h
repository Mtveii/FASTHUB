#pragma once

#include <QString>

namespace fasthub::ui {

/// @brief Markdown -> HTML conversion for note previews.
///
/// Backed by md4c (GitHub dialect + extensions: tables, task lists,
/// strikethrough, footnotes, wikilinks). Obsidian-style [[wiki links]]
/// are emitted as <a href="wiki:<target>"> anchors, ready for the
/// preview widget to intercept and resolve inside the workspace.
namespace MarkdownRenderer {

/// @brief Renders @p markdown to an HTML fragment (no <html> wrapper).
QString renderToHtml(const QString &markdown);

} // namespace MarkdownRenderer

} // namespace fasthub::ui

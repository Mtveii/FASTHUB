#pragma once

#include <QString>

namespace fasthub::workspace {

/// @brief Obsidian-style note resolution helpers (pure logic, no UI).
namespace FileLocator {

/// @brief Finds a file inside @p rootPath by name, recursively.
///
/// Matching rules (Obsidian-like):
/// - exact name match first ("note.md");
/// - then name without extension match against file base names;
/// - finally "name + .md" fallback;
/// - comparison is case-insensitive (Windows).
///
/// @return Absolute path of the best match, or empty QString if not found.
QString findFileByName(const QString& rootPath, const QString& name);

/// @brief Ensures the file exists; creates missing notes (with .md
///        extension when no suffix given) like Obsidian does when
///        following an unresolved wiki link.
/// @return Absolute path to the existing or newly created file.
QString createIfMissing(const QString& rootPath, const QString& name);

} // namespace FileLocator

} // namespace fasthub::workspace

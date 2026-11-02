#pragma once

#include "core/i_file_provider.h"

#include <string>

namespace fasthub::workspace {

/// @brief File provider restricted to a single workspace root directory.
///
/// Security: every operation resolves the requested path and rejects anything
/// that escapes the configured root (path traversal via "..", absolute paths
/// to other locations, etc.). Comparison is case-insensitive (Windows).
/// A provider constructed without a root is locked until setRootPath() is
/// called with an existing directory.
class LocalFileProvider : public core::IFileProvider {
public:
    LocalFileProvider() = default;
    ~LocalFileProvider() override = default;

    /// @brief Sets/limits all operations to an existing directory.
    /// @return true if the directory exists and was accepted.
    bool setRootPath(const std::string &rootPath);

    /// @brief Returns true if @p path points inside the workspace root.
    bool isInsideRoot(const std::string &path) const;

    std::vector<core::FileInfo> listDirectory(const std::string &path) override;
    std::optional<std::string> readFile(const std::string &path) override;
    bool writeFile(const std::string &path, const std::string &content) override;
    bool createFile(const std::string &path, bool isDirectory) override;
    bool deleteFile(const std::string &path) override;
    bool renameFile(const std::string &oldPath, const std::string &newPath) override;

private:
    /// @brief Cleans and normalizes @p path lexically (".." resolved).
    QString normalizedAbsolute(const std::string &path) const;

    /// @brief Checks membership against m_rootPath. Assumes both are cleaned.
    bool containsNormalized(const QString &candidate) const;

    std::string m_rootPath;
};

} // namespace fasthub::workspace

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <QtGlobal>

namespace fasthub::core {

/// @brief Metadata representing a file or directory on disk.
struct FileInfo {
    std::string path;
    std::string name;
    bool isDirectory = false;
    qint64 size = 0;
};

/// @brief Abstract interface for file system operations in Workspace.
class IFileProvider {
public:
    virtual ~IFileProvider() = default;

    /// @brief Lists contents of a directory.
    virtual std::vector<FileInfo> listDirectory(const std::string& path) = 0;

    /// @brief Reads text content of a file.
    virtual std::optional<std::string> readFile(const std::string& path) = 0;

    /// @brief Writes text content to a file directly on disk.
    virtual bool writeFile(const std::string& path, const std::string& content) = 0;

    /// @brief Creates a new file or directory.
    virtual bool createFile(const std::string& path, bool isDirectory) = 0;

    /// @brief Deletes a file or directory.
    virtual bool deleteFile(const std::string& path) = 0;

    /// @brief Renames or moves a file or directory.
    virtual bool renameFile(const std::string& oldPath, const std::string& newPath) = 0;
};

} // namespace fasthub::core

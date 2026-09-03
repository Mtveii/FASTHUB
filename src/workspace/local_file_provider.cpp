#include "local_file_provider.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QTextStream>

#include "infrastructure/logger.h"

namespace fasthub::workspace {

namespace {
constexpr qint64 kMaxReadFileBytes = 20 * 1024 * 1024; // 20 MB guard for the text reader.
}

bool LocalFileProvider::setRootPath(const std::string& rootPath) {
    QString cleaned = QDir::cleanPath(QString::fromStdString(rootPath));
    if (cleaned.isEmpty() || !QDir(cleaned).exists()) {
        infrastructure::Logger::instance().error("Workspace", "Rejected workspace root (missing or invalid): " + cleaned);
        return false;
    }
    m_rootPath = cleaned.toStdString();
    infrastructure::Logger::instance().info("Workspace", "Workspace root set to: " + cleaned);
    return true;
}

QString LocalFileProvider::normalizedAbsolute(const std::string& path) const {
    return QDir::cleanPath(QDir::fromNativeSeparators(QString::fromStdString(path)));
}

bool LocalFileProvider::containsNormalized(const QString& candidate) const {
    if (m_rootPath.empty()) {
        return false; // No root configured -> everything is rejected.
    }
    const QString root = QDir::cleanPath(QString::fromStdString(m_rootPath));
    if (candidate.compare(root, Qt::CaseInsensitive) == 0) {
        return true;
    }
    const QString prefix = root + QLatin1Char('/');
    return candidate.startsWith(prefix, Qt::CaseInsensitive);
}

bool LocalFileProvider::isInsideRoot(const std::string& path) const {
    return containsNormalized(normalizedAbsolute(path));
}

std::vector<core::FileInfo> LocalFileProvider::listDirectory(const std::string& path) {
    std::vector<core::FileInfo> result;

    QString qPath = normalizedAbsolute(path);
    if (!containsNormalized(qPath)) {
        infrastructure::Logger::instance().warn("Workspace", "listDirectory rejected (outside workspace): " + qPath);
        return result;
    }

    QDir dir(qPath);
    if (!dir.exists()) {
        return result;
    }

    QFileInfoList list = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::DirsFirst | QDir::Name);
    for (const QFileInfo& info : list) {
        core::FileInfo item;
        item.path = info.absoluteFilePath().toStdString();
        item.name = info.fileName().toStdString();
        item.isDirectory = info.isDir();
        item.size = info.isDir() ? 0 : info.size();
        result.push_back(item);
    }

    return result;
}

std::optional<std::string> LocalFileProvider::readFile(const std::string& path) {
    QString qPath = normalizedAbsolute(path);
    if (!containsNormalized(qPath)) {
        infrastructure::Logger::instance().warn("Workspace", "readFile rejected (outside workspace): " + qPath);
        return std::nullopt;
    }

    QFile file(qPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return std::nullopt;
    }
    // Guard after hydration: cloud files report 0 before open, real size after.
    if (file.size() > kMaxReadFileBytes) {
        infrastructure::Logger::instance().warn("Workspace", QString("readFile rejected, file too large (%1 bytes): %2")
            .arg(file.size()).arg(qPath));
        return std::nullopt;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QString content = in.readAll();
    return content.toStdString();
}

bool LocalFileProvider::writeFile(const std::string& path, const std::string& content) {
    QString qPath = normalizedAbsolute(path);
    if (!containsNormalized(qPath)) {
        infrastructure::Logger::instance().warn("Workspace", "writeFile rejected (outside workspace): " + qPath);
        return false;
    }

    // Ensure parent dir exists (iCloud vault subfolders may be dehydrated).
    QFileInfo fi(qPath);
    QDir().mkpath(fi.absolutePath());

    // Atomic save via QSaveFile: write to temp then rename.
    // This hydrates iCloud placeholders (removes ReparsePoint dehydration)
    // and avoids 0-byte truncation if app crashes mid-write.
    QSaveFile file(qPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        infrastructure::Logger::instance().error("Workspace", "Failed to open file for writing: " + qPath + " error: " + file.errorString());
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out.setGenerateByteOrderMark(false);
    out << QString::fromStdString(content);
    out.flush();
    if (out.status() != QTextStream::Ok) {
        infrastructure::Logger::instance().error("Workspace", "QTextStream write failed for: " + qPath);
        return false;
    }
    if (!file.commit()) {
        infrastructure::Logger::instance().error("Workspace", "Failed to commit file (QSaveFile): " + qPath + " error: " + file.errorString());
        return false;
    }
    return true;
}

bool LocalFileProvider::createFile(const std::string& path, bool isDirectory) {
    QString qPath = normalizedAbsolute(path);
    if (!containsNormalized(qPath)) {
        infrastructure::Logger::instance().warn("Workspace", "createFile rejected (outside workspace): " + qPath);
        return false;
    }

    if (isDirectory) {
        return QDir().mkpath(qPath);
    } else {
        // Ensure parent directory exists before creating the file.
        QFileInfo info(qPath);
        if (!info.absoluteDir().exists() && !QDir().mkpath(info.absolutePath())) {
            return false;
        }

        QFile file(qPath);
        if (file.exists()) {
            return true;
        }
        if (file.open(QIODevice::WriteOnly)) {
            file.close();
            return true;
        }
        return false;
    }
}

bool LocalFileProvider::deleteFile(const std::string& path) {
    QString qPath = normalizedAbsolute(path);
    if (!containsNormalized(qPath)) {
        infrastructure::Logger::instance().warn("Workspace", "deleteFile rejected (outside workspace): " + qPath);
        return false;
    }

    // Never allow deleting the workspace root itself through this API.
    if (qPath.compare(QDir::cleanPath(QString::fromStdString(m_rootPath)), Qt::CaseInsensitive) == 0) {
        infrastructure::Logger::instance().warn("Workspace", "Refusing to delete the workspace root itself: " + qPath);
        return false;
    }

    QFileInfo info(qPath);
    if (info.isDir()) {
        return QDir(info.absoluteFilePath()).removeRecursively();
    } else {
        return QFile::remove(info.absoluteFilePath());
    }
}

bool LocalFileProvider::renameFile(const std::string& oldPath, const std::string& newPath) {
    QString qOld = normalizedAbsolute(oldPath);
    QString qNew = normalizedAbsolute(newPath);

    if (!containsNormalized(qOld) || !containsNormalized(qNew)) {
        infrastructure::Logger::instance().warn("Workspace", QString("rename rejected (outside workspace): %1 -> %2").arg(qOld, qNew));
        return false;
    }

    if (!QFile::rename(qOld, qNew)) {
        infrastructure::Logger::instance().error("Workspace", QString("rename failed: %1 -> %2").arg(qOld, qNew));
        return false;
    }
    return true;
}

} // namespace fasthub::workspace

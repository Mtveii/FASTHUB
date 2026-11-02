#include "file_locator.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>

namespace fasthub::workspace {

namespace {

bool namesEqual(const QString &a, const QString &b) {
    return QString::compare(a, b, Qt::CaseInsensitive) == 0;
}

/// @brief Walks @p rootPath once, scoring candidates for @p target.
QString searchRoot(const QString &rootPath, const QString &target) {
    const QFileInfo targetInfo(target);
    const QString targetFullName = targetInfo.fileName();
    const QString targetBaseName = targetInfo.completeBaseName();

    QString exactMatch;
    QString baseMatch;
    QString mdMatch;

    QDirIterator it(rootPath, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString candidate = it.next();
        const QFileInfo info(candidate);

        if (namesEqual(info.fileName(), targetFullName)) {
            exactMatch = candidate;
            break; // cannot do better
        }
        if (baseMatch.isEmpty() && namesEqual(info.completeBaseName(), targetFullName)) {
            baseMatch = candidate;
            continue;
        }
        if (mdMatch.isEmpty() &&
            namesEqual(info.fileName(), targetFullName + QStringLiteral(".md"))) {
            mdMatch = candidate;
        }
    }

    if (!exactMatch.isEmpty())
        return exactMatch;
    if (!baseMatch.isEmpty())
        return baseMatch;
    return mdMatch;
}

} // namespace

QString FileLocator::findFileByName(const QString &rootPath, const QString &name) {
    if (rootPath.isEmpty() || name.isEmpty()) {
        return {};
    }
    // Reject path separators: wiki targets are plain note names.
    if (name.contains('/') || name.contains('\\')) {
        return {};
    }
    const QString cleaned = rootPath.trimmed();
    if (cleaned.isEmpty() || !QDir(cleaned).exists()) {
        return {};
    }
    return searchRoot(cleaned, name.trimmed());
}

QString FileLocator::createIfMissing(const QString &rootPath, const QString &name) {
    const QString existing = findFileByName(rootPath, name);
    if (!existing.isEmpty()) {
        return existing;
    }

    QString fileName = name.trimmed();
    if (!fileName.contains('.')) {
        fileName += QStringLiteral(".md");
    }

    // Sanitize against traversal: only the file name component is used.
    fileName.replace('/', '_').replace('\\', '_');

    const QString path = QDir(rootPath).filePath(fileName);
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.close();
        return path;
    }
    return {};
}

} // namespace fasthub::workspace

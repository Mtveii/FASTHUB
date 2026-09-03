#pragma once

#include <QString>
#include <QVector>

namespace workspace {

struct WikiLink {
    QString target;   // note reference, trimmed, e.g. "Project Ideas"
    QString heading;   // optional "#heading" part, empty if none
    QString alias;      // optional "|alias" part, empty if none
    int start = -1;   // offset of '[[' in source
    int end   = -1;   // offset just past ']]'
};

class LinkParser
{
public:
    // Parses all [[wiki links]] in `content`.
    // Skips links located inside fenced (```) or inline (`) code spans.
    static QVector<WikiLink> parse(const QString &content);
};

} // namespace workspace

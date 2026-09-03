#include "link_parser.h"
#include <QRegularExpression>

namespace workspace {
namespace {

struct Range { int start; int end; }; // [start, end)

bool insideAny(int pos, const QVector<Range> &ranges)
{
    for (const auto &r : ranges)
        if (pos >= r.start && pos < r.end)
            return true;
    return false;
}

QVector<Range> fencedCodeRanges(const QString &content)
{
    QVector<Range> ranges;
    static const QRegularExpression fence(
        QStringLiteral("```[^\\n]*\\n[\\s\\S]*?```"));
    auto it = fence.globalMatch(content);
    while (it.hasNext()) {
        const auto m = it.next();
        ranges.append({int(m.capturedStart()), int(m.capturedEnd())});
    }
    return ranges;
}

QVector<Range> inlineCodeRanges(const QString &content, const QVector<Range> &fenced)
{
    QVector<Range> ranges;
    static const QRegularExpression inlineCode(QStringLiteral("`[^`\\n]+`"));
    auto it = inlineCode.globalMatch(content);
    while (it.hasNext()) {
        const auto m = it.next();
        const int s = int(m.capturedStart());
        if (!insideAny(s, fenced))
            ranges.append({s, int(m.capturedEnd())});
    }
    return ranges;
}

} // namespace

QVector<WikiLink> LinkParser::parse(const QString &content)
{
    QVector<WikiLink> result;

    const QVector<Range> fenced = fencedCodeRanges(content);
    QVector<Range> codeRanges = fenced;
    codeRanges += inlineCodeRanges(content, fenced);

    // target: everything up to ] | # ; heading and alias are optional,
    // in that order — mirrors Obsidian's [[target#heading|alias]] syntax
    static const QRegularExpression linkRe(
        QStringLiteral("\\[\\[([^\\]\\|#]+)(?:#([^\\]\\|]+))?(?:\\|([^\\]]+))?\\]\\]"));

    auto it = linkRe.globalMatch(content);
    while (it.hasNext()) {
        const auto m = it.next();
        const int start = int(m.capturedStart());
        const int end = int(m.capturedEnd());

        if (insideAny(start, codeRanges))
            continue;

        WikiLink link;
        link.target  = m.captured(1).trimmed();
        link.heading = m.captured(2).trimmed();
        link.alias   = m.captured(3).trimmed();
        link.start   = start;
        link.end     = end;

        if (link.target.isEmpty())
            continue; // "[[]]" or "[[#heading]]" — not a valid link

        result.append(link);
    }

    return result;
}

} // namespace workspace

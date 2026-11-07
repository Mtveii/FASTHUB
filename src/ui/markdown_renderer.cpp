#include "markdown_renderer.h"

#include <QHash>
#include <QRegularExpression>

#include "theme_manager.h"

#include "md4c-html.h"

namespace fasthub::ui {

namespace {

/// @brief md4c output callback: appends rendered chunks to a QByteArray.
void processOutput(const MD_CHAR *data, MD_SIZE size, void *userdata) {
    auto *buffer = static_cast<QByteArray *>(userdata);
    buffer->append(data, static_cast<int>(size));
}

/// @brief Color role of the active theme with fallbacks for tests/early UI.
QString themeColor(const QString &role, const char *fallback) {
    static QHash<QString, QString> cache;
    const QString key = role + QStringLiteral("|") + ThemeManager::instance().currentThemeId();
    const auto it = cache.constFind(key);
    if (it != cache.constEnd()) {
        return it.value();
    }
    QString result = QString::fromLatin1(fallback);
    for (const auto &t : ThemeManager::instance().themes()) {
        if (t.id == ThemeManager::instance().currentThemeId()) {
            result = t.colors.value(role, QString::fromLatin1(fallback));
            break;
        }
    }
    cache.insert(key, result);
    return result;
}

} // namespace

QString MarkdownRenderer::renderToHtml(const QString &markdown) {
    const QByteArray utf8 = markdown.toUtf8();
    QByteArray html;
    html.reserve(utf8.size() * 2);

    // GitHub-like dialect plus Obsidian-style wiki links.
    const unsigned parserFlags = MD_DIALECT_GITHUB | MD_FLAG_WIKILINKS | MD_FLAG_UNDERLINE;
    const unsigned renderFlags = 0;

    if (md_html(utf8.constData(), static_cast<MD_SIZE>(utf8.size()), &processOutput, &html,
                parserFlags, renderFlags) != 0) {
        return QStringLiteral("<p style='color:#e05555'>Markdown render error.</p>");
    }

    QString out = QString::fromUtf8(html);

    const QString accent = themeColor("accent", "#6E7BFF");
    const QString accentSoft = themeColor("accentHover", "#8B96FF");
    const QString panel = themeColor("panel", "#12161F");
    const QString text = themeColor("text", "#E6E9F0");

    // md4c emits wikilinks as <x-wikilink data-target="T">label</x-wikilink>.
    // Convert them into interceptable anchors styled as pill chips
    // (design spec: accent 12% background, accent text).
    // QTextDocument CSS subset: only background-color/color are honored.
    static const QRegularExpression linkOpen(
        QStringLiteral("<x-wikilink data-target=\"([^\"]*)\">"));
    out.replace(
        linkOpen,
        QStringLiteral(
            "<a href=\"wiki:\\1\" style=\"background-color:%1; color:%2; text-decoration:none;\">")
            .arg(accent + QStringLiteral("22"), accentSoft));
    out.replace(QStringLiteral("</x-wikilink>"), QStringLiteral("</a>"));

    // Typography for the reading view: Manrope headings in accent,
    // mono code blocks on panel background.
    const QString css =
        QStringLiteral(
            "<style>"
            "h1, h2, h3 { font-family: 'Manrope','Inter'; color:%1; }"
            "h1 { font-size: 26px; }"
            "h2 { font-size: 21px; }"
            "code, pre { font-family: 'JetBrains Mono','Consolas'; background-color:%2; }"
            "body { color:%3; }"
            "</style>")
            .arg(accentSoft, panel, text);

    return css + out;
}

} // namespace fasthub::ui

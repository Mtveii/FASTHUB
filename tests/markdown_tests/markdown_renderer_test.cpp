#include <QtTest/QtTest>

#include "ui/markdown_renderer.h"

// MSVC rejects `using fasthub::ui::MarkdownRenderer;` for namespaces.
namespace MarkdownRenderer = fasthub::ui::MarkdownRenderer;

class MarkdownRendererTest : public QObject {
    Q_OBJECT

private slots:
    void rendersBasicMarkdown();
    void rendersTablesAndTaskLists();
    void wikiLinksBecomeAnchors();
    void wikiLinksIgnoredInsideCode();

private:
};

void MarkdownRendererTest::rendersBasicMarkdown()
{
    const QString html = MarkdownRenderer::renderToHtml(
        QStringLiteral("# Title\n\nSome **bold** and *italic* text.\n"));

    QVERIFY(html.contains("<h1"));
    QVERIFY(html.contains("Title"));
    QVERIFY(html.contains("<strong>bold</strong>"));
    QVERIFY(html.contains("<em>italic</em>"));
}

void MarkdownRendererTest::rendersTablesAndTaskLists()
{
    const QString md = QStringLiteral(
        "| a | b |\n|---|---|\n| 1 | 2 |\n\n- [x] done task\n- [ ] open task\n");
    const QString html = MarkdownRenderer::renderToHtml(md);

    QVERIFY2(html.contains("<table"), qPrintable(html.left(200)));
    QVERIFY(html.contains("<input")); // task list checkboxes
}

void MarkdownRendererTest::wikiLinksBecomeAnchors()
{
    const QString html = MarkdownRenderer::renderToHtml(
        QStringLiteral("See [[My Note]] here.\n"));

    // Obsidian-style link must become an interceptable wiki: anchor
    // (styled as a pill chip, so only the href is asserted).
    QVERIFY2(html.contains(QLatin1String("href=\"wiki:My Note\"")),
             qPrintable(html));
    QVERIFY(html.contains(">My Note<") || html.contains("My Note</a>"));
}

void MarkdownRendererTest::wikiLinksIgnoredInsideCode()
{
    const QString html = MarkdownRenderer::renderToHtml(
        QStringLiteral("Example: `[[not a link]]` code.\n"));

    // Inside inline code the brackets stay literal - no wiki anchor emitted.
    QVERIFY(!html.contains(QLatin1String("href=\"wiki:not a link\"")));
    QVERIFY(html.contains("[[not a link]]"));
}

QTEST_MAIN(MarkdownRendererTest)
#include "markdown_renderer_test.moc"

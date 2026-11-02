#include <QtTest>
#include "parsers/link_parser.h"

using namespace workspace;

class LinkParserTest : public QObject {
    Q_OBJECT

private slots:
    void basicLink();
    void linkWithAlias();
    void linkWithHeading();
    void linkWithHeadingAndAlias();
    void cyrillicTarget();
    void multipleLinksInText();
    void unclosedBracketsIgnored();
    void emptyTargetIgnored();
    void whitespaceTrimmed();
    void ignoresInlineCode();
    void ignoresFencedCodeBlock();
    void matchesOutsideCodeBlock();
    void reportsCorrectOffsets();
};

void LinkParserTest::basicLink() {
    const auto links = LinkParser::parse(QStringLiteral("See [[Project Ideas]] for details."));
    QCOMPARE(links.size(), 1);
    QCOMPARE(links[0].target, QStringLiteral("Project Ideas"));
    QVERIFY(links[0].heading.isEmpty());
    QVERIFY(links[0].alias.isEmpty());
}

void LinkParserTest::linkWithAlias() {
    const auto links = LinkParser::parse(QStringLiteral("[[Note|Custom Text]]"));
    QCOMPARE(links.size(), 1);
    QCOMPARE(links[0].target, QStringLiteral("Note"));
    QCOMPARE(links[0].alias, QStringLiteral("Custom Text"));
    QVERIFY(links[0].heading.isEmpty());
}

void LinkParserTest::linkWithHeading() {
    const auto links = LinkParser::parse(QStringLiteral("[[Note#Section One]]"));
    QCOMPARE(links.size(), 1);
    QCOMPARE(links[0].target, QStringLiteral("Note"));
    QCOMPARE(links[0].heading, QStringLiteral("Section One"));
    QVERIFY(links[0].alias.isEmpty());
}

void LinkParserTest::linkWithHeadingAndAlias() {
    const auto links = LinkParser::parse(QStringLiteral("[[Note#Section One|See here]]"));
    QCOMPARE(links.size(), 1);
    QCOMPARE(links[0].target, QStringLiteral("Note"));
    QCOMPARE(links[0].heading, QStringLiteral("Section One"));
    QCOMPARE(links[0].alias, QStringLiteral("See here"));
}

void LinkParserTest::cyrillicTarget() {
    const auto links = LinkParser::parse(QStringLiteral("Смотри [[Заметка проекта]] тут."));
    QCOMPARE(links.size(), 1);
    QCOMPARE(links[0].target, QStringLiteral("Заметка проекта"));
}

void LinkParserTest::multipleLinksInText() {
    const auto links = LinkParser::parse(QStringLiteral("[[A]] and [[B|Bee]] and [[C#H]]"));
    QCOMPARE(links.size(), 3);
    QCOMPARE(links[0].target, QStringLiteral("A"));
    QCOMPARE(links[1].target, QStringLiteral("B"));
    QCOMPARE(links[1].alias, QStringLiteral("Bee"));
    QCOMPARE(links[2].target, QStringLiteral("C"));
    QCOMPARE(links[2].heading, QStringLiteral("H"));
}

void LinkParserTest::unclosedBracketsIgnored() {
    const auto links = LinkParser::parse(QStringLiteral("This is [[unclosed and stays as text"));
    QVERIFY(links.isEmpty());
}

void LinkParserTest::emptyTargetIgnored() {
    const auto links = LinkParser::parse(QStringLiteral("[[]] and [[#OnlyHeading]]"));
    QVERIFY(links.isEmpty());
}

void LinkParserTest::whitespaceTrimmed() {
    const auto links = LinkParser::parse(QStringLiteral("[[  Padded Note  ]]"));
    QCOMPARE(links.size(), 1);
    QCOMPARE(links[0].target, QStringLiteral("Padded Note"));
}

void LinkParserTest::ignoresInlineCode() {
    const auto links = LinkParser::parse(QStringLiteral("Text `[[NotALink]]` more text"));
    QVERIFY(links.isEmpty());
}

void LinkParserTest::ignoresFencedCodeBlock() {
    const QString text = QStringLiteral("```cpp\n"
                                        "auto x = \"[[NotALink]]\";\n"
                                        "```\n");
    const auto links = LinkParser::parse(text);
    QVERIFY(links.isEmpty());
}

void LinkParserTest::matchesOutsideCodeBlock() {
    const QString text = QStringLiteral("[[Real Link]]\n"
                                        "```\n"
                                        "[[Fake Link]]\n"
                                        "```\n"
                                        "[[Another Real]]\n");
    const auto links = LinkParser::parse(text);
    QCOMPARE(links.size(), 2);
    QCOMPARE(links[0].target, QStringLiteral("Real Link"));
    QCOMPARE(links[1].target, QStringLiteral("Another Real"));
}

void LinkParserTest::reportsCorrectOffsets() {
    const QString text = QStringLiteral("abc [[Note]] def");
    const auto links = LinkParser::parse(text);
    QCOMPARE(links.size(), 1);
    QCOMPARE(links[0].start, 4);
    QCOMPARE(links[0].end, 12);
}

QTEST_APPLESS_MAIN(LinkParserTest)
#include "link_parser_test.moc"

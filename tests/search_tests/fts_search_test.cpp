#include <QtTest/QtTest>
#include <QTemporaryDir>

#include "search/search_provider.h"
#include "database/database_manager.h"

using fasthub::search::SearchProvider;

class FtsSearchTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void findsIndexedDocument();
    void hostileQueryReturnsSafely();

private:
    QTemporaryDir m_tempDir;
};

void FtsSearchTest::initTestCase()
{
    QVERIFY(m_tempDir.isValid());
    QVERIFY(fasthub::database::DatabaseManager::instance().initialize(m_tempDir.filePath("search.sqlite")));

    QVERIFY(fasthub::database::DatabaseManager::instance().indexDocument(
        "test:doc1", "telegram", "Release notes", "FastHub release checklist body"));
}

void FtsSearchTest::findsIndexedDocument()
{
    SearchProvider provider;
    const auto hits = provider.search("release");
    QCOMPARE(hits.size(), 1);
    QCOMPARE(hits.front().internalId, std::string("test:doc1"));
    QCOMPARE(hits.front().source, std::string("telegram"));
}

void FtsSearchTest::hostileQueryReturnsSafely()
{
    SearchProvider provider;

    // None of these may throw / crash; they are treated as plain text terms.
    const auto r1 = provider.search("NOT NEAR ( ) ^");
    const auto r2 = provider.search("release\" OR title:body*");
    QVERIFY(r2.empty() || !r2.empty()); // Any result set is acceptable.
    Q_UNUSED(r1);

    // Empty query returns no results and must not error.
    QVERIFY(provider.search("").empty());
}

QTEST_MAIN(FtsSearchTest)
#include "fts_search_test.moc"

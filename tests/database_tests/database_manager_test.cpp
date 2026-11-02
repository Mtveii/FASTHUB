#include <QtTest/QtTest>
#include <QTemporaryDir>

#include "database/database_manager.h"
#include "core/id_utils.h"

using fasthub::core::Conversation;
using fasthub::core::Message;
using fasthub::database::DatabaseManager;

namespace {

Conversation makeConversation(const std::string &externalId, const std::string &title, qint64 ts) {
    Conversation c;
    c.internalId = fasthub::core::IdUtils::makeInternalId("test", externalId);
    c.title = title;
    c.lastMessageText = "last";
    c.lastMessageTimestamp = ts;
    c.unreadCount = 1;
    return c;
}

Message makeMessage(const std::string &convId, const std::string &text, qint64 ts) {
    Message m;
    m.internalId = fasthub::core::IdUtils::makeInternalId("test", std::to_string(ts));
    m.conversationId = convId;
    m.senderName = "Alice";
    m.text = text;
    m.timestamp = ts;
    m.isOutgoing = false;
    m.isPending = false;
    return m;
}

} // namespace

class DatabaseManagerTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void migrationsAndSettings();
    void conversationCrudRoundTrip();
    void messageSaveLoadWithFtsIndexing();
    void ftsSanitizedQueryDoesNotError();

private:
    QTemporaryDir m_tempDir;
};

void DatabaseManagerTest::initTestCase() {
    QVERIFY2(m_tempDir.isValid(), "QTemporaryDir must be valid");
    const QString dbPath = m_tempDir.filePath("test.sqlite");
    QVERIFY(DatabaseManager::instance().initialize(dbPath));
    // Idempotent second call must succeed too.
    QVERIFY(DatabaseManager::instance().initialize(dbPath));
}

void DatabaseManagerTest::migrationsAndSettings() {
    auto &db = DatabaseManager::instance();
    QVERIFY(db.setSetting("k", "v"));
    QCOMPARE(db.getSetting("k").value_or(QString()), QString("v"));
    QVERIFY(!db.getSetting("missing").has_value());
}

void DatabaseManagerTest::conversationCrudRoundTrip() {
    auto &db = DatabaseManager::instance();
    const auto conv = makeConversation("c1", "Chat One", 1000);
    QVERIFY(db.saveConversations({conv}));

    const auto all = db.getConversations();
    QCOMPARE(all.size(), 1);
    QCOMPARE(all.front().internalId, conv.internalId);
    QCOMPARE(all.front().title, QString("Chat One"));
    QCOMPARE(all.front().unreadCount, 1);
}

void DatabaseManagerTest::messageSaveLoadWithFtsIndexing() {
    auto &db = DatabaseManager::instance();
    const std::string convId = fasthub::core::IdUtils::makeInternalId("test", "c1");

    QVERIFY(db.saveMessage(makeMessage(convId, "hello world from telegram", 100), "telegram"));

    const auto messages = db.getMessages(convId);
    QCOMPARE(messages.size(), 1);
    QCOMPARE(messages.front().text, QString("hello world from telegram"));
    QVERIFY(!messages.front().isOutgoing);

    // The message must be searchable through the FTS index.
    const auto hits = db.searchFts("hello");
    QVERIFY(!hits.empty());
}

void DatabaseManagerTest::ftsSanitizedQueryDoesNotError() {
    auto &db = DatabaseManager::instance();

    // FTS operators and quote characters must not produce errors or crash.
    const auto r1 = db.searchFts("\"hello\" OR * NEAR (");
    const auto r2 = db.searchFts("hello; DROP TABLE messages;--");
    const auto r3 = db.searchFts("");
    QVERIFY(r3.empty());
    // Sanity: normal query still finds the indexed document.
    QVERIFY(!db.searchFts("world").empty());
    Q_UNUSED(r1);
    Q_UNUSED(r2);
}

QTEST_MAIN(DatabaseManagerTest)
#include "database_manager_test.moc"

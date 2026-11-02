#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QElapsedTimer>

#include "sync/sync_service.h"
#include "database/database_manager.h"

#include "fake_chat_provider.h"

using fasthub::sync::SyncService;

class SyncServiceTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void syncPersistsConversationsAndSignals();
    void performSyncDoesNotBlockCaller();
    void overlappingTicksAreSkipped();

private:
    QTemporaryDir m_tempDir;
};

void SyncServiceTest::initTestCase() {
    QVERIFY(m_tempDir.isValid());
    QVERIFY(fasthub::database::DatabaseManager::instance().initialize(
        m_tempDir.filePath("sync.sqlite")));
}

void SyncServiceTest::syncPersistsConversationsAndSignals() {
    auto provider = std::make_shared<FakeChatProvider>();
    SyncService service(provider);

    QSignalSpy completedSpy(&service, &SyncService::syncCompleted);
    QVERIFY(completedSpy.isValid());

    service.syncNow();

    // Completion is delivered asynchronously on this thread's event loop.
    QVERIFY(completedSpy.wait(5000));
    QCOMPARE(completedSpy.count(), 1);

    const auto convs = fasthub::database::DatabaseManager::instance().getConversations();
    QCOMPARE(convs.size(), 1);
    QCOMPARE(convs.front().internalId, std::string("fake:1"));
}

void SyncServiceTest::performSyncDoesNotBlockCaller() {
    auto provider = std::make_shared<FakeChatProvider>(300); // slow provider
    SyncService service(provider);

    QElapsedTimer timer;
    timer.start();
    service.syncNow();
    const qint64 elapsed = timer.elapsed();

    // The call must return almost immediately; the work happens in background.
    QVERIFY2(elapsed < 100, qPrintable(QString("performSync blocked for %1 ms").arg(elapsed)));

    QSignalSpy completedSpy(&service, &SyncService::syncCompleted);
    QVERIFY(completedSpy.wait(5000));
}

void SyncServiceTest::overlappingTicksAreSkipped() {
    auto provider = std::make_shared<FakeChatProvider>(250);
    SyncService service(provider);

    // Fire two ticks back to back: the second must be a no-op while the first
    // is still running (no task pile-up).
    service.syncNow();
    service.syncNow();

    QSignalSpy completedSpy(&service, &SyncService::syncCompleted);
    QVERIFY(completedSpy.wait(5000));
    QTest::qWait(150); // allow any (unexpected) second completion to arrive

    QCOMPARE(completedSpy.count(), 1);
    QCOMPARE(provider->callCount(), 1);
}

QTEST_MAIN(SyncServiceTest)
#include "sync_service_test.moc"

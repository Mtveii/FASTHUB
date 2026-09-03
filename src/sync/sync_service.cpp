#include "sync_service.h"

#include <QFutureWatcher>
#include <QtConcurrent>

#include "database/database_manager.h"
#include "infrastructure/logger.h"

namespace fasthub::sync {

SyncService::SyncService(std::shared_ptr<core::IChatProvider> provider, QObject* parent)
    : QObject(parent), m_provider(provider) {
    connect(&m_timer, &QTimer::timeout, this, &SyncService::performSync);
}

SyncService::~SyncService() {
    stopSync();
}

void SyncService::startSync(int intervalMs) {
    m_timer.start(intervalMs);
    infrastructure::Logger::instance().info("Sync", "SyncService started with interval " + QString::number(intervalMs) + "ms");
}

void SyncService::stopSync() {
    if (m_timer.isActive()) {
        m_timer.stop();
        infrastructure::Logger::instance().info("Sync", "SyncService stopped.");
    }
}

void SyncService::syncNow() {
    performSync();
}

void SyncService::performSync() {
    if (!m_provider) return;

    // Skip ticks while the previous sync is still running (no task pile-up).
    bool expected = false;
    if (!m_syncInProgress.compare_exchange_strong(expected, true)) {
        return;
    }

    // Copy the provider so the worker never touches `this`.
    auto provider = m_provider;
    auto* database = &database::DatabaseManager::instance();

    auto* watcher = new QFutureWatcher<void>(this);
    connect(watcher, &QFutureWatcher<void>::finished, this, [this, watcher]() {
        watcher->deleteLater();
        m_syncInProgress.store(false);
        emit syncCompleted();
    });

    watcher->setFuture(QtConcurrent::run([provider, database]() {
        try {
            auto convs = provider->getConversations();
            database->saveConversations(convs);
        } catch (const std::exception& e) {
            infrastructure::Logger::instance().error("Sync", QString("Background sync failed: %1").arg(e.what()));
        } catch (...) {
            infrastructure::Logger::instance().error("Sync", "Background sync failed with unknown exception.");
        }
    }));
}

} // namespace fasthub::sync

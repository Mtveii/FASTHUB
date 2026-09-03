#pragma once

#include <QObject>
#include <QTimer>
#include <atomic>
#include <memory>
#include "core/i_chat_provider.h"

namespace fasthub::sync {

/// @brief Periodically pulls conversations from a chat provider into the
///        local cache without blocking the UI thread.
///
/// Lifetime safety notes:
/// - The provider is captured by shared_ptr inside the worker task, so the
///   task never dereferences a dangling provider even if SyncService dies.
/// - A QFutureWatcher parented to `this` delivers completion; if SyncService
///   is destroyed mid-sync, the watcher is destroyed too and no signal is
///   delivered to a dead object.
class SyncService : public QObject {
    Q_OBJECT
public:
    explicit SyncService(std::shared_ptr<core::IChatProvider> provider, QObject* parent = nullptr);
    ~SyncService() override;

    void startSync(int intervalMs = 30000);
    void stopSync();

    /// @brief Triggers one background sync immediately (public test/UX hook).
    void syncNow();

    /// @brief Returns true while a background sync task is still running.
    bool isSyncInProgress() const { return m_syncInProgress.load(); }

signals:
    void syncCompleted();

private slots:
    void performSync();

private:
    std::shared_ptr<core::IChatProvider> m_provider;
    QTimer m_timer;
    std::atomic_bool m_syncInProgress{false};
};

} // namespace fasthub::sync

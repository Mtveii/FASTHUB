#pragma once

#include <QString>
#include <QSqlDatabase>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include "core/models/conversation.h"
#include "core/models/message.h"
#include "core/i_search_provider.h"

namespace fasthub::database {

/// @brief Thread-safe facade over application SQLite storage.
///
/// Qt forbids sharing a single QSqlDatabase handle across threads
/// ("requested database does not belong to the calling thread"), therefore
/// this manager opens ONE connection PER THREAD on demand (all pointing at
/// the same database file) and additionally serializes its own bookkeeping
/// with a mutex. WAL journal mode plus busy_timeout make concurrent access
/// safe and wait-free for short reads.
class DatabaseManager {
public:
    static DatabaseManager &instance();

    /// @brief Opens the database and applies migrations. Idempotent.
    /// @param dbPath File path of the SQLite database.
    /// @return true on success (or if already initialized).
    bool initialize(const QString &dbPath);

    bool setSetting(const QString &key, const QString &value);
    std::optional<QString> getSetting(const QString &key);

    bool saveConversations(const std::vector<core::Conversation> &conversations);
    std::vector<core::Conversation> getConversations();

    /// @brief Saves a message and indexes it into the FTS table atomically.
    bool saveMessage(const core::Message &message, const std::string &source);
    std::vector<core::Message> getMessages(const std::string &conversationId);

    bool indexDocument(const std::string &internalId, const std::string &source,
                       const std::string &title, const std::string &body);

    /// @brief Full-text search. The user query is sanitized before MATCH,
    ///        so arbitrary FTS5 syntax / injection attempts are treated as
    ///        plain text terms instead of causing errors.
    std::vector<core::SearchResult> searchFts(const std::string &query);

private:
    DatabaseManager() = default;
    ~DatabaseManager() = default;
    DatabaseManager(const DatabaseManager &) = delete;
    DatabaseManager &operator=(const DatabaseManager &) = delete;

    /// @brief Returns (creating if needed) the caller-thread SQL connection.
    QSqlDatabase connectionForThread();

    bool runMigrations(QSqlDatabase db);
    bool indexDocumentLocked(const std::string &internalId, const std::string &source,
                             const std::string &title, const std::string &body);

    void applyPragmas(QSqlDatabase db);

    QString m_dbPath;
    bool m_initialized = false;

    std::mutex m_mutex; // guards bookkeeping below (NOT held during queries)
    std::map<unsigned long long, QString> m_threadConnections;
};

} // namespace fasthub::database

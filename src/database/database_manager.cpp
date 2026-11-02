#include "database_manager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

#include <thread>

#include "infrastructure/logger.h"

namespace fasthub::database {

namespace {

/// @brief Sanitizes a free-form user string into a safe FTS5 MATCH expression.
///
/// Each whitespace-separated token is wrapped in double quotes (phrase match,
/// implicit AND between tokens); embedded quotes are doubled per FTS5 rules.
/// This prevents user input from being interpreted as FTS5 operators
/// (AND/OR/NOT/NEAR/*, column filters, etc.) which would otherwise either
/// error out or change semantics.
QString sanitizeFtsQuery(const std::string &raw) {
    const QString text = QString::fromStdString(raw).simplified();
    if (text.isEmpty()) {
        return {};
    }

    QStringList phrases;
    const QStringList tokens = text.split(' ', Qt::SkipEmptyParts);
    for (const QString &token : tokens) {
        QString escaped = token;
        escaped.replace('"', QStringLiteral("\"\""));
        // Cap token length to avoid pathological inputs.
        if (escaped.size() > 64) {
            escaped = escaped.left(64);
        }
        phrases.append(QStringLiteral("\"%1\"").arg(escaped));
    }
    return phrases.join(QLatin1Char(' '));
}

unsigned long long currentThreadId() {
    return static_cast<unsigned long long>(
        std::hash<std::thread::id>{}(std::this_thread::get_id()));
}

} // namespace

DatabaseManager &DatabaseManager::instance() {
    static DatabaseManager s_instance;
    return s_instance;
}

void DatabaseManager::applyPragmas(QSqlDatabase db) {
    QSqlQuery pragma(db);
    // WAL: concurrent readers + one writer across per-thread connections.
    pragma.exec("PRAGMA journal_mode=WAL;");
    pragma.exec("PRAGMA busy_timeout=5000;");
    pragma.exec("PRAGMA foreign_keys=ON;");
}

QSqlDatabase DatabaseManager::connectionForThread() {
    const auto tid = currentThreadId();

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_threadConnections.find(tid);
    if (it != m_threadConnections.end()) {
        QSqlDatabase existing = QSqlDatabase::database(it->second, false);
        if (existing.isValid()) {
            return existing;
        }
    }

    const QString name = QStringLiteral("FastHubConn-%1").arg(tid);
    // Recreate if Qt removed a stale registration for this thread.
    QSqlDatabase::removeDatabase(name);
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", name);
    db.setDatabaseName(m_dbPath);
    if (!db.open()) {
        infrastructure::Logger::instance().error("DB", "Failed to open thread connection: " +
                                                           db.lastError().text());
        return db; // invalid/unopened; callers check m_initialized and exec results
    }
    applyPragmas(db);

    m_threadConnections[tid] = name;
    return db;
}

bool DatabaseManager::initialize(const QString &dbPath) {
    QFileInfo info(dbPath);
    QDir().mkpath(info.absolutePath());
    m_dbPath = dbPath;

    if (!m_initialized) {
        QSqlDatabase db = connectionForThread();
        if (!db.isOpen()) {
            infrastructure::Logger::instance().error("DB", "Failed to open database at " + dbPath);
            return false;
        }
        if (!runMigrations(db)) {
            return false;
        }
        m_initialized = true;
        infrastructure::Logger::instance().info("DB",
                                                "Database initialized successfully at " + dbPath);
    } else {
        // Idempotent call: make sure this thread has a live connection too.
        QSqlDatabase db = connectionForThread();
        if (!db.isOpen()) {
            return false;
        }
    }
    return true;
}

bool DatabaseManager::runMigrations(QSqlDatabase db) {
    QSqlQuery query(db);

    QStringList migrationQueries = {
        "CREATE TABLE IF NOT EXISTS settings ("
        "  key TEXT PRIMARY KEY,"
        "  value TEXT NOT NULL"
        ");",
        "CREATE TABLE IF NOT EXISTS conversations ("
        "  internal_id TEXT PRIMARY KEY,"
        "  title TEXT NOT NULL,"
        "  last_message_text TEXT,"
        "  last_message_timestamp INTEGER,"
        "  unread_count INTEGER"
        ");",
        "CREATE TABLE IF NOT EXISTS messages ("
        "  internal_id TEXT PRIMARY KEY,"
        "  conversation_id TEXT NOT NULL,"
        "  sender_name TEXT,"
        "  text TEXT,"
        "  timestamp INTEGER,"
        "  is_outgoing INTEGER,"
        "  is_pending INTEGER"
        ");",
        "CREATE INDEX IF NOT EXISTS idx_messages_conversation ON messages(conversation_id, "
        "timestamp);",
        "CREATE VIRTUAL TABLE IF NOT EXISTS search_index_fts USING fts5("
        "  internal_id UNINDEXED,"
        "  source,"
        "  title,"
        "  body"
        ");"};

    for (const QString &qStr : migrationQueries) {
        if (!query.exec(qStr)) {
            infrastructure::Logger::instance().error("DB", "Migration failed: " +
                                                               query.lastError().text());
            return false;
        }
    }

    return true;
}

bool DatabaseManager::setSetting(const QString &key, const QString &value) {
    if (!m_initialized)
        return false;
    QSqlQuery query(connectionForThread());
    query.prepare("INSERT OR REPLACE INTO settings (key, value) VALUES (:key, :value);");
    query.bindValue(":key", key);
    query.bindValue(":value", value);
    return query.exec();
}

std::optional<QString> DatabaseManager::getSetting(const QString &key) {
    if (!m_initialized)
        return std::nullopt;
    QSqlQuery query(connectionForThread());
    query.prepare("SELECT value FROM settings WHERE key = :key;");
    query.bindValue(":key", key);

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }
    return query.value(0).toString();
}

bool DatabaseManager::saveConversations(const std::vector<core::Conversation> &conversations) {
    if (!m_initialized || conversations.empty())
        return false;

    QSqlDatabase db = connectionForThread();
    if (!db.transaction()) {
        infrastructure::Logger::instance().error("DB", "Failed to start transaction: " +
                                                           db.lastError().text());
        return false;
    }

    QSqlQuery query(db);
    query.prepare("INSERT OR REPLACE INTO conversations (internal_id, title, last_message_text, "
                  "last_message_timestamp, unread_count) "
                  "VALUES (:internal_id, :title, :last_message_text, :last_message_timestamp, "
                  ":unread_count);");

    for (const auto &conv : conversations) {
        query.bindValue(":internal_id", QString::fromStdString(conv.internalId));
        query.bindValue(":title", QString::fromStdString(conv.title));
        query.bindValue(":last_message_text", QString::fromStdString(conv.lastMessageText));
        query.bindValue(":last_message_timestamp", static_cast<qint64>(conv.lastMessageTimestamp));
        query.bindValue(":unread_count", conv.unreadCount);
        if (!query.exec()) {
            infrastructure::Logger::instance().error("DB", "saveConversations failed: " +
                                                               query.lastError().text());
            db.rollback();
            return false;
        }
    }

    if (!db.commit()) {
        infrastructure::Logger::instance().error("DB", "saveConversations commit failed: " +
                                                           db.lastError().text());
        db.rollback();
        return false;
    }
    return true;
}

std::vector<core::Conversation> DatabaseManager::getConversations() {
    std::vector<core::Conversation> result;
    if (!m_initialized)
        return result;

    QSqlQuery query(connectionForThread());
    if (!query.exec("SELECT internal_id, title, last_message_text, last_message_timestamp, "
                    "unread_count FROM conversations ORDER BY last_message_timestamp DESC;")) {
        infrastructure::Logger::instance().error("DB", "getConversations failed: " +
                                                           query.lastError().text());
        return result;
    }

    while (query.next()) {
        core::Conversation conv;
        conv.internalId = query.value(0).toString().toStdString();
        conv.title = query.value(1).toString().toStdString();
        conv.lastMessageText = query.value(2).toString().toStdString();
        conv.lastMessageTimestamp = query.value(3).toLongLong();
        conv.unreadCount = query.value(4).toInt();
        result.push_back(conv);
    }

    return result;
}

bool DatabaseManager::saveMessage(const core::Message &message, const std::string &source) {
    if (!m_initialized)
        return false;

    QSqlQuery query(connectionForThread());
    query.prepare("INSERT OR REPLACE INTO messages (internal_id, conversation_id, sender_name, "
                  "text, timestamp, is_outgoing, is_pending) "
                  "VALUES (:internal_id, :conversation_id, :sender_name, :text, :timestamp, "
                  ":is_outgoing, :is_pending);");
    query.bindValue(":internal_id", QString::fromStdString(message.internalId));
    query.bindValue(":conversation_id", QString::fromStdString(message.conversationId));
    query.bindValue(":sender_name", QString::fromStdString(message.senderName));
    query.bindValue(":text", QString::fromStdString(message.text));
    query.bindValue(":timestamp", static_cast<qint64>(message.timestamp));
    query.bindValue(":is_outgoing", message.isOutgoing ? 1 : 0);
    query.bindValue(":is_pending", message.isPending ? 1 : 0);

    if (!query.exec()) {
        infrastructure::Logger::instance().error("DB",
                                                 "saveMessage failed: " + query.lastError().text());
        return false;
    }

    // Keep FTS index consistent with the messages table. Failure is logged
    // but does not invalidate the saved message itself.
    if (!indexDocumentLocked(message.internalId, source, message.senderName, message.text)) {
        infrastructure::Logger::instance().warn(
            "DB", "FTS indexing failed for message: " + QString::fromStdString(message.internalId));
    }
    return true;
}

std::vector<core::Message> DatabaseManager::getMessages(const std::string &conversationId) {
    std::vector<core::Message> result;
    if (!m_initialized)
        return result;

    QSqlQuery query(connectionForThread());
    query.prepare("SELECT internal_id, conversation_id, sender_name, text, timestamp, is_outgoing, "
                  "is_pending "
                  "FROM messages WHERE conversation_id = :conversation_id ORDER BY timestamp ASC;");
    query.bindValue(":conversation_id", QString::fromStdString(conversationId));

    if (!query.exec()) {
        infrastructure::Logger::instance().error("DB",
                                                 "getMessages failed: " + query.lastError().text());
        return result;
    }

    while (query.next()) {
        core::Message msg;
        msg.internalId = query.value(0).toString().toStdString();
        msg.conversationId = query.value(1).toString().toStdString();
        msg.senderName = query.value(2).toString().toStdString();
        msg.text = query.value(3).toString().toStdString();
        msg.timestamp = query.value(4).toLongLong();
        msg.isOutgoing = query.value(5).toInt() != 0;
        msg.isPending = query.value(6).toInt() != 0;
        result.push_back(msg);
    }

    return result;
}

bool DatabaseManager::indexDocument(const std::string &internalId, const std::string &source,
                                    const std::string &title, const std::string &body) {
    if (!m_initialized)
        return false;
    return indexDocumentLocked(internalId, source, title, body);
}

bool DatabaseManager::indexDocumentLocked(const std::string &internalId, const std::string &source,
                                          const std::string &title, const std::string &body) {
    QSqlQuery query(connectionForThread());
    query.prepare("INSERT OR REPLACE INTO search_index_fts (internal_id, source, title, body) "
                  "VALUES (:id, :source, :title, :body);");
    query.bindValue(":id", QString::fromStdString(internalId));
    query.bindValue(":source", QString::fromStdString(source));
    query.bindValue(":title", QString::fromStdString(title));
    query.bindValue(":body", QString::fromStdString(body));
    return query.exec();
}

std::vector<core::SearchResult> DatabaseManager::searchFts(const std::string &query) {
    std::vector<core::SearchResult> results;
    if (!m_initialized || query.empty())
        return results;

    const QString sanitized = sanitizeFtsQuery(query);
    if (sanitized.isEmpty()) {
        return results;
    }

    QSqlQuery q(connectionForThread());
    q.prepare("SELECT internal_id, source, title, body FROM search_index_fts WHERE "
              "search_index_fts MATCH :query LIMIT 50;");
    q.bindValue(":query", sanitized);

    if (!q.exec()) {
        infrastructure::Logger::instance().warn("DB", "FTS query failed: " + q.lastError().text());
        return results;
    }

    while (q.next()) {
        core::SearchResult res;
        res.internalId = q.value(0).toString().toStdString();
        res.source = q.value(1).toString().toStdString();
        res.title = q.value(2).toString().toStdString();
        res.snippet = q.value(3).toString().toStdString();
        results.push_back(res);
    }

    return results;
}

} // namespace fasthub::database

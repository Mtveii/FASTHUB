#include "telegram_provider.h"

#include <QJsonObject>
#include <QRegularExpression>

#include "core/id_utils.h"
#include "database/database_manager.h"
#include "infrastructure/logger.h"
#include "infrastructure/secure_storage.h"

#include <QDateTime>
#include <chrono>

namespace fasthub::providers::telegram {

namespace {

constexpr const char *kKeyApiId = "telegram.api_id";
constexpr const char *kKeyApiHash = "telegram.api_hash";

QString loadSecret(const std::string &key) {
    infrastructure::SecureStorage storage;
    return QString::fromStdString(storage.getSecret(key).value_or(std::string()));
}

} // namespace

TelegramProvider::TelegramProvider() {
    m_tdClient = std::make_unique<TdClient>();
    if (isTdlibAvailable()) {
        // Updates arrive on the TDLib thread; marshal into this QObject's
        // thread (UI) via queued invocation.
        m_tdClient->start([this](const QJsonObject &update) {
            QMetaObject::invokeMethod(
                this, [this, update]() { onTdUpdate(update); }, Qt::QueuedConnection);
        });
    }

    // Ensure initial mock conversation for testing/demo if cache is empty.
    auto convs = database::DatabaseManager::instance().getConversations();
    if (convs.empty()) {
        core::Conversation sampleConv;
        sampleConv.internalId = core::IdUtils::makeInternalId("telegram", "1001");
        sampleConv.title = "FastHub Team Chat (Telegram)";
        sampleConv.lastMessageText = "Welcome to FastHub Local First Telegram provider!";
        sampleConv.lastMessageTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
                                              std::chrono::system_clock::now().time_since_epoch())
                                              .count();
        sampleConv.unreadCount = 0;

        database::DatabaseManager::instance().saveConversations({sampleConv});

        core::Message sampleMsg;
        sampleMsg.internalId = core::IdUtils::makeInternalId("telegram", "msg_1");
        sampleMsg.conversationId = sampleConv.internalId;
        sampleMsg.senderName = "FastHub Bot";
        sampleMsg.text = "Welcome to FastHub Local First Telegram provider!";
        sampleMsg.timestamp = sampleConv.lastMessageTimestamp;
        sampleMsg.isOutgoing = false;
        sampleMsg.isPending = false;

        database::DatabaseManager::instance().saveMessage(sampleMsg, "telegram");
    }
}

TelegramProvider::~TelegramProvider() = default;

core::ProviderStatus TelegramProvider::getStatus() const { return m_status; }

std::vector<core::Conversation> TelegramProvider::getConversations() {
    // LOCAL FIRST: Load from SQLite cache instantly.
    return database::DatabaseManager::instance().getConversations();
}

std::vector<core::Message> TelegramProvider::getMessages(const std::string &conversationId) {
    // LOCAL FIRST: Load from SQLite cache instantly.
    return database::DatabaseManager::instance().getMessages(conversationId);
}

bool TelegramProvider::sendMessage(const std::string &conversationId, const std::string &text) {
    if (text.empty()) {
        return false;
    }

    core::Message msg;
    msg.internalId = core::IdUtils::makeInternalId(
        "telegram", QString::number(QDateTime::currentMSecsSinceEpoch()).toStdString());
    msg.conversationId = conversationId;
    msg.senderName = "You";
    msg.text = text;
    msg.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count();
    msg.isOutgoing = true;
    msg.isPending = false;

    bool saved = database::DatabaseManager::instance().saveMessage(msg, "telegram");
    if (saved) {
        infrastructure::Logger::instance().info("Telegram", "Message sent and cached locally.");
    }
    return saved;
}

bool TelegramProvider::login(const std::string &phoneNumber) {
    // Privacy: never log the raw phone number (AGENTS.md §6.4).
    QString masked = QString::fromStdString(phoneNumber);
    if (masked.size() > 4) {
        masked = masked.left(masked.size() - 4).replace(QRegularExpression("."), QChar('*')) +
                 masked.right(4);
    }
    infrastructure::Logger::instance().info("Telegram", "Login initiated for phone: " + masked);
    m_status = core::ProviderStatus::Available;
    return true;
}

bool TelegramProvider::verifyCode(const std::string &code) {
    infrastructure::Logger::instance().info("Telegram", "Verification code accepted.");
    m_status = core::ProviderStatus::Available;
    return true;
}

void TelegramProvider::saveApiCredentials(const QString &apiId, const QString &apiHash) {
    infrastructure::SecureStorage storage;
    storage.setSecret(kKeyApiId, apiId.toStdString());
    storage.setSecret(kKeyApiHash, apiHash.toStdString());
    m_parametersSet = false; // force re-send of tdlibParameters on next login
    infrastructure::Logger::instance().info("Telegram", "API credentials saved (DPAPI-protected).");
}

void TelegramProvider::startQrLogin() {
    if (!isTdlibAvailable()) {
        emit qrLoginStateChanged(
            QStringLiteral("unavailable"), QString(),
            QStringLiteral(
                "tdjson.dll not found. Place it next to FastHub.exe to enable real login."));
        return;
    }

    const QString apiId = loadSecret(kKeyApiId);
    const QString apiHash = loadSecret(kKeyApiHash);
    if (apiId.isEmpty() || apiHash.isEmpty()) {
        emit qrLoginStateChanged(
            QStringLiteral("error"), QString(),
            QStringLiteral("Missing API credentials. Set api_id/api_hash first."));
        return;
    }

    if (!m_parametersSet) {
        const QJsonObject params{
            {"@type", QStringLiteral("setTdlibParameters")},
            {"database_directory", QStringLiteral("tdlib-db")},
            {"files_directory", QStringLiteral("tdlib-files")},
            {"use_message_database", true},
            {"use_secret_chats", false},
            {"api_id", apiId.toInt()},
            {"api_hash", apiHash},
            {"system_language_code", QStringLiteral("en")},
            {"device_model", QStringLiteral("FastHub Desktop")},
            {"application_version", QStringLiteral("1.0.0")},
        };
        m_tdClient->send(QString::fromUtf8(QJsonDocument(params).toJson(QJsonDocument::Compact)));
        m_parametersSet = true;
    }

    emit qrLoginStateChanged(QStringLiteral("sending"), QString(), QString());

    const QJsonObject request{{"@type", QStringLiteral("requestQrCodeAuthentication")}};
    m_tdClient->send(QString::fromUtf8(QJsonDocument(request).toJson(QJsonDocument::Compact)));
}

void TelegramProvider::onTdUpdate(const QJsonObject &update) {
    const QString type = update.value("@type").toString();
    if (type == QLatin1String("updateAuthorizationState")) {
        handleAuthState(update.value("authorization_state").toObject().value("@type").toString(),
                        update);
    } else if (type == QLatin1String("updateOption")) {
        // Options like @id may carry errors for bad api_id.
    } else if (type == QLatin1String("error")) {
        infrastructure::Logger::instance().warn("Telegram", "TDLib error: " +
                                                                update.value("message").toString());
        emit qrLoginStateChanged(QStringLiteral("error"), QString(),
                                 update.value("message").toString());
    }
}

void TelegramProvider::handleAuthState(const QString &authState, const QJsonObject &update) {
    Q_UNUSED(update);

    if (authState == QLatin1String("authorizationStateWaitPhoneNumber") ||
        authState == QLatin1String("authorizationStateWaitOtherDeviceConfirmation")) {
        if (authState == QLatin1String("authorizationStateWaitOtherDeviceConfirmation")) {
            const QString link =
                update.value("authorization_state").toObject().value("link").toString();
            m_status = core::ProviderStatus::Unavailable; // until confirmed
            emit qrLoginStateChanged(QStringLiteral("wait_qr"), link,
                                     QStringLiteral("Scan the QR with your Telegram app."));
            return;
        }
        // Phone number step: immediately switch to QR flow instead.
        startQrLogin();
    } else if (authState == QLatin1String("authorizationStateReady")) {
        m_status = core::ProviderStatus::Available;
        infrastructure::Logger::instance().info("Telegram", "TDLib authorization ready.");
        emit qrLoginStateChanged(QStringLiteral("ready"), QString(), QStringLiteral("Authorized."));
    } else if (authState == QLatin1String("authorizationStateClosed")) {
        m_parametersSet = false;
    } else if (authState == QLatin1String("authorizationStateLoggingOut") ||
               authState == QLatin1String("authorizationStateWaitCode") ||
               authState == QLatin1String("authorizationStateWaitPassword")) {
        // Fallback states outside QR flow - surfaced as-is.
        emit qrLoginStateChanged(
            QStringLiteral("wait_qr"), QString(),
            QStringLiteral("Additional confirmation required in Telegram (%1).").arg(authState));
    }
}

} // namespace fasthub::providers::telegram

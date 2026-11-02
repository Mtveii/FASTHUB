#include "td_client.h"

#include <windows.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>

#include "infrastructure/logger.h"

namespace fasthub::providers::telegram {

namespace {

// tdjson C API signatures (https://core.telegram.org/tdlib/docs/td__json__client_8h.html)
using td_create_client_id_fn = int();
using td_send_fn = void(int, const char *);
using td_receive_fn = const char *(double);
using td_receive_timeout_fn = const char *(double);
using td_execute_fn = const char *(const char *);

constexpr const char *kTdJsonDllName = "tdjson.dll";

} // namespace

TdClient::TdClient() {
    m_available.store(loadLibrary());
    if (!isAvailable()) {
        infrastructure::Logger::instance().warn(
            "Telegram",
            "tdjson.dll not found - TDLib integration disabled, provider stays in mock mode.");
    }
}

TdClient::~TdClient() {
    m_running.store(false);
    // The receive loop uses a 1s timeout, so the thread exits promptly.
    if (m_receiveThread && m_receiveThread->joinable()) {
        m_receiveThread->join();
    }
    if (m_lib) {
        FreeLibrary(static_cast<HMODULE>(m_lib));
    }
}

bool TdClient::loadLibrary() {
    HMODULE lib = ::LoadLibraryA(kTdJsonDllName);
    if (!lib) {
        return false;
    }

    auto resolve = [lib](const char *name) -> void * {
        void *fn = reinterpret_cast<void *>(::GetProcAddress(lib, name));
        if (!fn) {
            infrastructure::Logger::instance().error(
                "Telegram", QString("tdjson export missing: %1").arg(name));
        }
        return fn;
    };

    m_createClientId = resolve("td_create_client_id");
    m_sendFn = resolve("td_send");
    m_receiveFn = resolve("td_receive");
    m_executeFn = resolve("td_execute");

    if (!m_createClientId || !m_sendFn || !m_receiveFn || !m_executeFn) {
        FreeLibrary(lib);
        return false;
    }

    m_lib = lib;
    return true;
}

void TdClient::start(std::function<void(const QJsonObject &)> updateCallback) {
    if (!isAvailable() || m_running.load()) {
        return;
    }
    m_updateCallback = std::move(updateCallback);

    const int clientId = reinterpret_cast<td_create_client_id_fn *>(m_createClientId)();
    m_clientIdValue = clientId;

    m_running.store(true);
    m_receiveThread = std::make_unique<std::thread>([this]() { receiveLoop(); });
}

void TdClient::send(const QString &jsonRequest) {
    if (!isAvailable())
        return;
    std::lock_guard<std::mutex> lock(m_sendMutex);
    reinterpret_cast<td_send_fn *>(m_sendFn)(m_clientIdValue, jsonRequest.toUtf8().constData());
}

void TdClient::execute(const QString &jsonRequest) {
    if (!isAvailable())
        return;
    reinterpret_cast<td_execute_fn *>(m_executeFn)(jsonRequest.toUtf8().constData());
}

void TdClient::receiveLoop() {
    auto receive = reinterpret_cast<td_receive_timeout_fn *>(m_receiveFn);
    while (m_running.load()) {
        // 1s timeout keeps shutdown latency bounded.
        const char *raw = receive(1.0);
        if (!raw) {
            continue;
        }

        const QJsonObject update = QJsonDocument::fromJson(QByteArray(raw)).object();
        if (m_updateCallback) {
            try {
                m_updateCallback(update);
            } catch (const std::exception &e) {
                infrastructure::Logger::instance().error(
                    "Telegram", QString("Update handler exception: %1").arg(e.what()));
            } catch (...) {
                infrastructure::Logger::instance().error("Telegram",
                                                         "Update handler unknown exception.");
            }
        }
    }
}

} // namespace fasthub::providers::telegram

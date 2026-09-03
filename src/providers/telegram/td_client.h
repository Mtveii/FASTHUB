#pragma once

#include <QString>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

class QJsonObject;

namespace fasthub::providers::telegram {

/// @brief Thin async wrapper around TDLib's tdjson C API.
///
/// The DLL is loaded at runtime (no link-time dependency): the app starts
/// even if tdjson.dll is missing, and the Telegram UI degrades gracefully
/// to an "integration unavailable" state.
///
/// Threading: a dedicated worker thread owns the receive loop; updates are
/// delivered via @p updateCallback on that thread - consumers must marshal
/// to the UI thread themselves (e.g. queued signal).
class TdClient {
public:
    TdClient();
    ~TdClient();

    TdClient(const TdClient&) = delete;
    TdClient& operator=(const TdClient&) = delete;

    /// @brief true if tdjson.dll was found and all entry points resolved.
    bool isAvailable() const { return m_available.load(); }

    /// @brief Creates the TDLib client and starts the receive loop.
    void start(std::function<void(const QJsonObject&)> updateCallback);

    /// @brief Sends an asynchronous request (JSON string with "@extra").
    void send(const QString& jsonRequest);

    /// @brief Executes a synchronous TDLib method.
    void execute(const QString& jsonRequest);

private:
    void receiveLoop();
    bool loadLibrary();

    std::function<void(const QJsonObject&)> m_updateCallback;
    std::atomic_bool m_available{false};
    std::atomic_bool m_running{false};

    void* m_lib = nullptr;
    int m_clientIdValue = 0;
    void* m_createClientId = nullptr;
    void* m_sendFn = nullptr;
    void* m_receiveFn = nullptr;
    void* m_executeFn = nullptr;

    std::unique_ptr<std::thread> m_receiveThread;
    std::mutex m_sendMutex;
};

} // namespace fasthub::providers::telegram

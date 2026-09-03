#pragma once

#include "core/i_chat_provider.h"

#include <atomic>
#include <thread>
#include <vector>

namespace {

/// @brief Deterministic provider stub for SyncService tests.
///
/// Optionally sleeps in getConversations() to emulate slow work so tests can
/// verify that a sync tick does not block the caller and that overlapping
/// ticks are skipped.
class FakeChatProvider final : public fasthub::core::IChatProvider {
public:
    explicit FakeChatProvider(int workMs = 0) : m_workMs(workMs) {}

    fasthub::core::ProviderStatus getStatus() const override {
        return fasthub::core::ProviderStatus::Available;
    }

    std::vector<fasthub::core::Conversation> getConversations() override {
        ++m_calls;
        if (m_workMs > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(m_workMs));
        }
        fasthub::core::Conversation c;
        c.internalId = "fake:1";
        c.title = "Fake chat";
        c.lastMessageTimestamp = 42;
        return {c};
    }

    std::vector<fasthub::core::Message> getMessages(const std::string& conversationId) override {
        Q_UNUSED(conversationId);
        return {};
    }

    bool sendMessage(const std::string& conversationId, const std::string& text) override {
        Q_UNUSED(conversationId);
        Q_UNUSED(text);
        return true;
    }

    int callCount() const { return m_calls.load(); }

private:
    int m_workMs = 0;
    std::atomic_int m_calls{0};
};

} // namespace

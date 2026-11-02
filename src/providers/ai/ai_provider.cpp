#include "ai_provider.h"

#include "infrastructure/logger.h"

namespace fasthub::providers::ai {

AIProvider::AIProvider(core::AIProviderType type) : m_type(type) {}

void AIProvider::setApiKey(const std::string &apiKey) { m_apiKey = QString::fromStdString(apiKey); }

std::string AIProvider::ask(const std::string &context, const std::string &prompt) {
    if (prompt.empty()) {
        return {};
    }

    infrastructure::Logger::instance().info(
        "AI", QString("Request received (context: %1 chars, provider: %2).")
                  .arg(context.size())
                  .arg(static_cast<int>(m_type)));

    // NOTE (release blocker): real HTTP integration is not implemented yet.
    // The network module and per-provider request formats are a separate
    // work item; until then the provider answers with an explicit mock so
    // the UI is never silently lied to about having contacted a service.
    if (m_apiKey.isEmpty()) {
        return "[FastHub AI Mock Response] No API key configured. Received context length: " +
               std::to_string(context.size()) + " chars.";
    }

    return "[FastHub AI Mock Response] API key configured but HTTP integration is not implemented "
           "yet.";
}

} // namespace fasthub::providers::ai

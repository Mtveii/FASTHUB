#pragma once

#include <string>

namespace fasthub::core {

enum class AIProviderType {
    OpenAI,
    Anthropic,
    Gemini
};

/// @brief Abstract interface for AI assistant providers.
class IAIProvider {
public:
    virtual ~IAIProvider() = default;

    /// @brief Sends a prompt with explicit context to the AI model.
    /// @param context Selected message, file content, or text selection.
    /// @param prompt User prompt or instruction.
    /// @return AI response string.
    virtual std::string ask(const std::string& context, const std::string& prompt) = 0;
};

} // namespace fasthub::core

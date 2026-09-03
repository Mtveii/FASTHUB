#pragma once

#include "core/i_ai_provider.h"
#include <QString>

namespace fasthub::providers::ai {

class AIProvider : public core::IAIProvider {
public:
    explicit AIProvider(core::AIProviderType type);
    ~AIProvider() override = default;

    std::string ask(const std::string& context, const std::string& prompt) override;

    void setApiKey(const std::string& apiKey);

private:
    core::AIProviderType m_type;
    QString m_apiKey;
};

} // namespace fasthub::providers::ai

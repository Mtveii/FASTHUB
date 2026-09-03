#pragma once

#include "core/i_secure_storage.h"
#include <QString>

namespace fasthub::infrastructure {

class SecureStorage : public core::ISecureStorage {
public:
    SecureStorage();
    ~SecureStorage() override = default;

    bool setSecret(const std::string& key, const std::string& value) override;
    std::optional<std::string> getSecret(const std::string& key) override;
    bool removeSecret(const std::string& key) override;

private:
    QString getStoragePath() const;
};

} // namespace fasthub::infrastructure

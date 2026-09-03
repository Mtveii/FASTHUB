#include "secure_storage.h"

#include <windows.h>
#include <wincrypt.h>

#include <QDir>
#include <QStandardPaths>
#include <QSettings>
#include <QByteArray>

namespace fasthub::infrastructure {

SecureStorage::SecureStorage() = default;

QString SecureStorage::getStoragePath() const {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/secrets.ini";
}

bool SecureStorage::setSecret(const std::string& key, const std::string& value) {
    if (key.empty()) {
        return false;
    }

    DATA_BLOB inBlob;
    inBlob.pbData = const_cast<BYTE*>(reinterpret_cast<const BYTE*>(value.data()));
    inBlob.cbData = static_cast<DWORD>(value.size());

    DATA_BLOB outBlob;
    if (!CryptProtectData(&inBlob, L"FastHub Secret", nullptr, nullptr, nullptr, 0, &outBlob)) {
        return false;
    }

    QByteArray encrypted(reinterpret_cast<const char*>(outBlob.pbData), outBlob.cbData);
    LocalFree(outBlob.pbData);

    QSettings settings(getStoragePath(), QSettings::IniFormat);
    settings.setValue(QString::fromStdString(key), encrypted.toBase64());
    return true;
}

std::optional<std::string> SecureStorage::getSecret(const std::string& key) {
    if (key.empty()) {
        return std::nullopt;
    }

    QSettings settings(getStoragePath(), QSettings::IniFormat);
    if (!settings.contains(QString::fromStdString(key))) {
        return std::nullopt;
    }

    QByteArray encrypted = QByteArray::fromBase64(settings.value(QString::fromStdString(key)).toByteArray());
    if (encrypted.isEmpty()) {
        return std::nullopt;
    }

    DATA_BLOB inBlob;
    inBlob.pbData = reinterpret_cast<BYTE*>(encrypted.data());
    inBlob.cbData = static_cast<DWORD>(encrypted.size());

    DATA_BLOB outBlob;
    if (!CryptUnprotectData(&inBlob, nullptr, nullptr, nullptr, nullptr, 0, &outBlob)) {
        return std::nullopt;
    }

    std::string result(reinterpret_cast<char*>(outBlob.pbData), outBlob.cbData);
    LocalFree(outBlob.pbData);

    return result;
}

bool SecureStorage::removeSecret(const std::string& key) {
    if (key.empty()) {
        return false;
    }

    QSettings settings(getStoragePath(), QSettings::IniFormat);
    settings.remove(QString::fromStdString(key));
    return true;
}

} // namespace fasthub::infrastructure

#pragma once

#include <QString>
#include <QMutex>

namespace fasthub::infrastructure {

enum class LogLevel { Debug, Info, Warning, Error, Fatal };

class Logger {
public:
    static Logger &instance();

    void init(const QString &logFilePath);
    void log(LogLevel level, const QString &category, const QString &message);

    void debug(const QString &category, const QString &message);
    void info(const QString &category, const QString &message);
    void warn(const QString &category, const QString &message);
    void error(const QString &category, const QString &message);

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    void rotateIfNeeded();

    QString m_logFilePath;
    QMutex m_mutex;
    const qint64 m_maxFileSize = 5 * 1024 * 1024; // 5MB
};

} // namespace fasthub::infrastructure

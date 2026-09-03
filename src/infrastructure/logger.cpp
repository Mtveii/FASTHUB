#include "logger.h"

#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

namespace fasthub::infrastructure {

Logger& Logger::instance() {
    static Logger s_instance;
    return s_instance;
}

void Logger::init(const QString& logFilePath) {
    QMutexLocker locker(&m_mutex);
    m_logFilePath = logFilePath;
    QFileInfo info(m_logFilePath);
    QDir().mkpath(info.absolutePath());
}

QString levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO ";
        case LogLevel::Warning: return "WARN ";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
    }
    return "UNKNOWN";
}

void Logger::log(LogLevel level, const QString& category, const QString& message) {
    QMutexLocker locker(&m_mutex);

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString formattedMessage = QString("[%1] [%2] [%3] %4\n")
        .arg(timestamp)
        .arg(levelToString(level))
        .arg(category)
        .arg(message);

    // Also output to debug stream
    qDebug().noquote() << formattedMessage.trimmed();

    if (m_logFilePath.isEmpty()) {
        return;
    }

    rotateIfNeeded();

    QFile file(m_logFilePath);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << formattedMessage;
    }
}

void Logger::debug(const QString& category, const QString& message) {
    log(LogLevel::Debug, category, message);
}

void Logger::info(const QString& category, const QString& message) {
    log(LogLevel::Info, category, message);
}

void Logger::warn(const QString& category, const QString& message) {
    log(LogLevel::Warning, category, message);
}

void Logger::error(const QString& category, const QString& message) {
    log(LogLevel::Error, category, message);
}

void Logger::rotateIfNeeded() {
    QFileInfo info(m_logFilePath);
    if (info.exists() && info.size() > m_maxFileSize) {
        QString backupPath = m_logFilePath + ".old";
        if (QFile::exists(backupPath)) {
            QFile::remove(backupPath);
        }
        QFile::rename(m_logFilePath, backupPath);
    }
}

} // namespace fasthub::infrastructure

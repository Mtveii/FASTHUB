#include "context_broker.h"

namespace fasthub::ui {

ContextBroker& ContextBroker::instance() {
    static ContextBroker s;
    return s;
}

void ContextBroker::setSelection(const QString& source, const QString& text) {
    if (m_selectionSource == source && m_selectionText == text) {
        return;
    }
    m_selectionSource = source;
    m_selectionText = text;
    emit contextChanged();
}

void ContextBroker::setCurrentFile(const QString& path, std::function<QString()> contentProvider) {
    if (m_filePath == path) {
        m_fileProvider = std::move(contentProvider);
        return;
    }
    m_filePath = path;
    m_fileProvider = std::move(contentProvider);
    emit contextChanged();
}

QString ContextBroker::buildAiContext() const {
    QString out;
    if (!m_selectionText.isEmpty()) {
        out += QStringLiteral("=== Selection from %1 ===\n%2\n")
                   .arg(m_selectionSource, m_selectionText);
    } else if (!m_filePath.isEmpty()) {
        out += QStringLiteral("=== Current file: %1 (use 'Load full file' to attach) ===\n")
                   .arg(m_filePath);
    }
    return out;
}

} // namespace fasthub::ui

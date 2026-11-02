#pragma once

#include <QObject>
#include <QString>
#include <functional>

namespace fasthub::ui {

/// @brief Cross-module selection/context hub for the AI assistant.
///
/// Modules publish what the user is looking at / has selected:
/// - WorkspaceWidget: selected text in the editor + current file path
/// - WebEmbedWidget (Discord/Steam/web-TG): page selection via JS
/// - AyuGram (separate process): NOT accessible - honest limitation.
///
/// When the user opens the AI tab, AIWidget pulls the freshest context
/// from here (owner request: "выделенное переносится в ИИ при переходе
/// на вкладку").
class ContextBroker : public QObject {
    Q_OBJECT
public:
    static ContextBroker &instance();

    /// @brief Publishes a text selection from a module.
    /// @param source Human-readable origin ("workspace: note.md", "discord").
    /// @param text Selected text (may be empty -> clears selection part).
    void setSelection(const QString &source, const QString &text);

    /// @brief Publishes the currently open file (path + content provider).
    ///        The provider is called lazily (file may be huge).
    void setCurrentFile(const QString &path, std::function<QString()> contentProvider);

    QString selectionText() const { return m_selectionText; }
    QString selectionSource() const { return m_selectionSource; }
    QString currentFilePath() const { return m_filePath; }

    /// @brief Full content of the current file (lazy, size-capped by caller).
    QString currentFileContent() const { return m_fileProvider ? m_fileProvider() : QString(); }

    /// @brief Ready-to-paste context block for the AI input.
    QString buildAiContext() const;

signals:
    void contextChanged();

private:
    ContextBroker() = default;

    QString m_selectionSource;
    QString m_selectionText;
    QString m_filePath;
    std::function<QString()> m_fileProvider;
};

} // namespace fasthub::ui

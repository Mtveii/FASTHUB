#pragma once

#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QTabWidget>
#include <QPushButton>
#include <QSplitter>
#include <QTimer>
#include <QHash>

#include "workspace/local_file_provider.h"
#include "workspace/top_bar.h"
#include "workspace/icon_rail.h"
#include "workspace/file_list_toolbar.h"

class QTextEdit;
class QTextBrowser;
class QStackedWidget;
class QPushButton;
class QAbstractFileIconProvider;

namespace fasthub::ui {

class GraphView;

/// @brief Per-tab state of an opened note.
struct NoteTab {
    QString path;
    QTextEdit* editor = nullptr;      // raw text (edit mode + autosave source)
    QTextBrowser* preview = nullptr;  // rendered markdown/html (read mode)
    QWidget* container = nullptr;
    QStackedWidget* stack = nullptr;  // editor <-> preview switcher
    QPushButton* modeButton = nullptr;// Edit/Preview toggle
    bool isRenderable = false;        // .md/.html/.htm -> preview available
};

/// @brief Obsidian-style workspace: file tree, tabs, edit/reading modes,
///        rendered Markdown & HTML notes, [[wiki link]] navigation.
class WorkspaceWidget : public QWidget {
    Q_OBJECT
public:
    explicit WorkspaceWidget(QWidget* parent = nullptr);
    ~WorkspaceWidget() override;

    /// @brief Opens @p path as the workspace root programmatically
    ///        (used by first-run onboarding / saved settings).
    void openWorkspacePath(const QString& path);

private slots:
    void onOpenFolderClicked();
    void onFileDoubleClicked(const QModelIndex& index);
    void onFileClicked(const QModelIndex& index);
    void onFileContextMenuRequested(const QPoint& pos);
    void closeTab(int index);
    void toggleCurrentPreview();
    void saveCurrentNow();
    void toggleGraphView();

private:
    void setupUi();
    void openFileAtPath(const QString& filePath, bool activateTab);
    void togglePreview(NoteTab& tab);
    void refreshPreview(const NoteTab& tab);
    void handleAnchorClick(const QUrl& url, const QTextBrowser* source);
    void flushPendingSave(int index);
    void writeTabToFile(int index);

    NoteTab* currentNoteTab();
    NoteTab* findNoteByPath(const QString& path);

    QPushButton* m_openFolderBtn;
    GraphView* m_graphView = nullptr;   // Obsidian-style link graph (toggleable)
    QTreeView* m_fileTreeView;
    QFileSystemModel* m_fileModel;
    QTabWidget* m_editorTabWidget;
    QSplitter* m_splitter;
    QString m_currentWorkspacePath;
    workspace::LocalFileProvider m_fileProvider;
    QAbstractFileIconProvider* m_iconProvider = nullptr;

    QHash<QWidget*, NoteTab> m_tabs; // key: tab page widget

    TopBar* m_topBar = nullptr;
    IconRail* m_iconRail = nullptr;
    FileListToolbar* m_fileListToolbar = nullptr;
    QWidget* m_sidebarContainer = nullptr;
    QStackedWidget* m_sidebarPages = nullptr;

public:
    void createNewNote();
    void createNewFolder();
    void createNewCanvas();
    void setFileTreeVisible(bool visible);
    bool fileTreeVisible() const;
    void openOrFocusTab(const QString& title, QWidget* widget);
    void openCommandPalette();
};

} // namespace fasthub::ui

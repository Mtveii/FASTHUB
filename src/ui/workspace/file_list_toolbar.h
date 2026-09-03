#pragma once
#include <QWidget>
#include <QToolButton>

class FileListToolbar : public QWidget
{
    Q_OBJECT
public:
    explicit FileListToolbar(QWidget *parent = nullptr);

public slots:
    void updateTheme();

signals:
    void newNoteRequested();
    void newFolderRequested();
    void sortMenuRequested();     // откроет QMenu с вариантами сортировки
    void sortRequested(const QString& mode);
    void layoutToggleRequested(); // переключение вида списка
    void closeRequested();
    void quickSwitcherRequested();
    void graphRequested();
    void canvasRequested();
    void dailyNoteRequested();
    void commandPaletteRequested();
    void baseTableRequested();

private:
    QToolButton *m_newNote = nullptr;
    QToolButton *m_newFolder = nullptr;
    QToolButton *m_sort = nullptr;
    QToolButton *m_layout = nullptr;
    QToolButton *m_close = nullptr;
    QToolButton *m_obsidianMenuBtn = nullptr;
};

#include "file_list_toolbar.h"
#include <QHBoxLayout>
#include <QMenu>
#include "icon_util.h"
#include "theme_manager.h"
#include "../style_tokens.h"

namespace {
QColor getThemeIconColor() {
    for (const auto &t : fasthub::ui::ThemeManager::instance().themes()) {
        if (t.id == fasthub::ui::ThemeManager::instance().currentThemeId()) {
            return QColor(t.isDark ? "#FFFFFF" : "#1A1A1A");
        }
    }
    return QColor("#FFFFFF");
}
} // namespace

FileListToolbar::FileListToolbar(QWidget *parent) : QWidget(parent) {
    setStyleSheet("QToolButton { background: transparent; border: none; border-radius: 4px; "
                  "padding: 3px; color: palette(text); }"
                  "QToolButton:hover { background-color: rgba(128, 128, 128, 0.2); }");

    m_newNote = new QToolButton();
    m_newNote->setObjectName("newNote");
    m_newFolder = new QToolButton();
    m_newFolder->setObjectName("newFolder");
    m_sort = new QToolButton();
    m_sort->setObjectName("sortBtn");
    m_layout = new QToolButton();
    m_layout->setObjectName("layoutBtn");
    m_obsidianMenuBtn = new QToolButton();
    m_obsidianMenuBtn->setObjectName("obsidianMenuBtn");
    m_close = new QToolButton();
    m_close->setObjectName("closeBtn");

    for (auto *b : {m_newNote, m_newFolder, m_sort, m_layout, m_obsidianMenuBtn, m_close}) {
        b->setIconSize({16, 16});
        b->setAutoRaise(true);
    }

    auto *box = new QHBoxLayout(this);
    box->setContentsMargins(8, 4, 8, 4);
    box->addWidget(m_obsidianMenuBtn); // Dedicated Obsidian menu button on the left of toolbar
    box->addStretch();
    box->addWidget(m_newNote);
    box->addWidget(m_newFolder);
    box->addWidget(m_sort);
    box->addWidget(m_layout);
    box->addWidget(m_close);

    updateTheme();
    connect(&fasthub::ui::ThemeManager::instance(), &fasthub::ui::ThemeManager::themeApplied, this,
            &FileListToolbar::updateTheme);

    connect(m_newNote, &QToolButton::clicked, this, &FileListToolbar::newNoteRequested);
    connect(m_newFolder, &QToolButton::clicked, this, &FileListToolbar::newFolderRequested);

    connect(m_obsidianMenuBtn, &QToolButton::clicked, this, [this]() {
        QMenu menu(this);
        menu.setStyleSheet(
            "QMenu { background: palette(base); color: palette(text); border: 1px solid "
            "palette(mid); border-radius: 6px; padding: 4px; } QMenu::item { padding: 6px 24px; "
            "border-radius: 4px; } QMenu::item:selected { background-color: palette(highlight); "
            "color: palette(highlighted-text); }");

        QAction *actQuickSwitcher = menu.addAction("1. Quick Switcher (Ctrl+O)");
        QAction *actGraph = menu.addAction("2. Граф (Graph view)");
        QAction *actCanvas = menu.addAction("3. Холст (Canvas)");
        QAction *actDailyNote = menu.addAction("4. Ежедневная заметка (2026-08-29)");
        QAction *actCommandPalette = menu.addAction("5. Палитра команд (Ctrl+P)");
        QAction *actBaseTable = menu.addAction("6. Базы / Таблица (Base)");

        QAction *selected =
            menu.exec(m_obsidianMenuBtn->mapToGlobal(QPoint(0, m_obsidianMenuBtn->height())));
        if (selected == actQuickSwitcher)
            emit quickSwitcherRequested();
        else if (selected == actGraph)
            emit graphRequested();
        else if (selected == actCanvas)
            emit canvasRequested();
        else if (selected == actDailyNote)
            emit dailyNoteRequested();
        else if (selected == actCommandPalette)
            emit commandPaletteRequested();
        else if (selected == actBaseTable)
            emit baseTableRequested();
    });

    connect(m_sort, &QToolButton::clicked, this, [this]() {
        QMenu menu(this);
        menu.setStyleSheet(
            "QMenu { background: palette(base); color: palette(text); border: 1px solid "
            "palette(mid); border-radius: 6px; padding: 4px; } QMenu::item { padding: 6px 24px; "
            "border-radius: 4px; } QMenu::item:selected { background-color: palette(highlight); "
            "color: palette(highlighted-text); }");
        QAction *byName = menu.addAction("По имени (А-Я)");
        QAction *byDate = menu.addAction("По дате изменения");
        QAction *bySize = menu.addAction("По размеру");
        QAction *selected = menu.exec(m_sort->mapToGlobal(QPoint(0, m_sort->height())));
        if (selected == byName) {
            emit sortRequested("name");
        } else if (selected == byDate) {
            emit sortRequested("date");
        } else if (selected == bySize) {
            emit sortRequested("size");
        }
    });
    connect(m_layout, &QToolButton::clicked, this, [this]() {
        QMenu menu(this);
        menu.setStyleSheet(
            "QMenu { background: palette(base); color: palette(text); border: 1px solid "
            "palette(mid); border-radius: 6px; padding: 4px; } QMenu::item { padding: 6px 24px; "
            "border-radius: 4px; } QMenu::item:selected { background-color: palette(highlight); "
            "color: palette(highlighted-text); }");
        QAction *treeView = menu.addAction("Дерево файлов");
        QAction *listView = menu.addAction("Плоский список");
        QAction *selected = menu.exec(m_layout->mapToGlobal(QPoint(0, m_layout->height())));
        if (selected) {
            emit layoutToggleRequested();
        }
    });
    connect(m_close, &QToolButton::clicked, this, &FileListToolbar::closeRequested);
}

void FileListToolbar::updateTheme() {
    const QColor color = getThemeIconColor();
    m_newNote->setIcon(fasthub::ui::IconUtil::tintedIcon(":/icons/pencil.svg", color, 16));
    m_newFolder->setIcon(fasthub::ui::IconUtil::tintedIcon(":/icons/folder-plus.svg", color, 16));
    m_sort->setIcon(fasthub::ui::IconUtil::tintedIcon(":/icons/sort.svg", color, 16));
    m_layout->setIcon(fasthub::ui::IconUtil::tintedIcon(":/icons/columns.svg", color, 16));
    m_obsidianMenuBtn->setIcon(fasthub::ui::IconUtil::tintedIcon(":/icons/list.svg", color, 16));
    m_close->setIcon(fasthub::ui::IconUtil::tintedIcon(":/icons/x.svg", color, 16));
}

#include "top_bar.h"
#include <QHBoxLayout>
#include <QButtonGroup>
#include "icon_util.h"
#include "theme_manager.h"
#include "../style_tokens.h"
#include "../animation_util.h"

namespace {
QColor getThemeIconColor() {
    for (const auto &t : fasthub::ui::ThemeManager::instance().themes()) {
        if (t.id == fasthub::ui::ThemeManager::instance().currentThemeId()) {
            return QColor(t.isDark ? "#FFFFFF" : "#1A1A1A");
        }
    }
    return QColor("#FFFFFF");
}

QToolButton *makeIconButton(bool checkable = false) {
    auto *btn = new QToolButton();
    btn->setIconSize({20, 20});
    btn->setAutoRaise(true); // плоские кнопки, как в Obsidian
    btn->setCheckable(checkable);
    return btn;
}
} // namespace

TopBar::TopBar(QWidget *parent) : QWidget(parent) {
    setStyleSheet("QToolButton { background: transparent; border: none; border-radius: 4px; "
                  "padding: 4px; color: palette(text); }"
                  "QToolButton:hover { background-color: rgba(128, 128, 128, 0.2); }"
                  "QToolButton:checked { background-color: rgba(128, 128, 128, 0.3); border: 1px "
                  "solid rgba(128, 128, 128, 0.4); }");

    m_sidebarToggle = makeIconButton();
    m_sidebarToggle->setObjectName("sidebarToggle");
    m_filesTab = makeIconButton(true);
    m_filesTab->setObjectName("filesTab");
    m_searchTab = makeIconButton(true);
    m_searchTab->setObjectName("searchTab");
    m_bookmarksTab = makeIconButton(true);
    m_bookmarksTab->setObjectName("bookmarksTab");

    m_filesTab->setChecked(true); // активна по умолчанию

    for (auto *btn : {m_sidebarToggle, m_filesTab, m_searchTab, m_bookmarksTab}) {
        btn->installEventFilter(new fasthub::ui::SmoothHoverFilter(btn));
    }

    // Files/Search/Bookmarks — взаимоисключающие вкладки
    auto *group = new QButtonGroup(this);
    group->setExclusive(true);
    group->addButton(m_filesTab);
    group->addButton(m_searchTab);
    group->addButton(m_bookmarksTab);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->addWidget(m_sidebarToggle);
    layout->addStretch();
    layout->addWidget(m_filesTab);
    layout->addWidget(m_searchTab);
    layout->addWidget(m_bookmarksTab);

    updateTheme();
    connect(&fasthub::ui::ThemeManager::instance(), &fasthub::ui::ThemeManager::themeApplied, this,
            &TopBar::updateTheme);

    connect(m_sidebarToggle, &QToolButton::clicked, this, &TopBar::sidebarToggleRequested);
    connect(m_filesTab, &QToolButton::clicked, this, &TopBar::filesTabRequested);
    connect(m_searchTab, &QToolButton::clicked, this, &TopBar::searchTabRequested);
    connect(m_bookmarksTab, &QToolButton::clicked, this, &TopBar::bookmarksTabRequested);
}

void TopBar::updateTheme() {
    const QColor color = getThemeIconColor();
    m_sidebarToggle->setIcon(fasthub::ui::IconUtil::tintedIcon(":/icons/sidebar.svg", color, 20));
    m_filesTab->setIcon(fasthub::ui::IconUtil::tintedIcon(":/icons/folder.svg", color, 20));
    m_searchTab->setIcon(fasthub::ui::IconUtil::tintedIcon(":/icons/search.svg", color, 20));
    m_bookmarksTab->setIcon(fasthub::ui::IconUtil::tintedIcon(":/icons/bookmark.svg", color, 20));
}

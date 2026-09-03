#include "icon_rail.h"
#include <QVBoxLayout>
#include "icon_util.h"
#include "theme_manager.h"
#include "../style_tokens.h"

namespace {
QColor getThemeIconColor() {
    for (const auto& t : fasthub::ui::ThemeManager::instance().themes()) {
        if (t.id == fasthub::ui::ThemeManager::instance().currentThemeId()) {
            return QColor(t.isDark ? "#FFFFFF" : "#1A1A1A");
        }
    }
    return QColor("#FFFFFF");
}
}

IconRail::IconRail(QWidget *parent) : QWidget(parent)
{
    setStyleSheet(
        "QToolButton { background: transparent; border: none; border-radius: 4px; padding: 6px; color: palette(text); }"
        "QToolButton:hover { background-color: rgba(128, 128, 128, 0.2); }"
        "QToolButton:checked { background-color: rgba(128, 128, 128, 0.3); border: 1px solid rgba(128, 128, 128, 0.4); }"
    );

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 8, 4, 8); // kSpace1, kSpace2
    layout->setSpacing(4); // kSpace1

    auto *group = new QButtonGroup(this);
    group->setExclusive(true);

    addButton(":/icons/file.svg",     Section::Files,     group, layout);
    addButton(":/icons/graph.svg",    Section::Graph,     group, layout);
    addButton(":/icons/grid.svg",     Section::Canvas,    group, layout);
    addButton(":/icons/calendar.svg", Section::Calendar,  group, layout);
    addButton(":/icons/copy.svg",     Section::Templates, group, layout);
    addButton(":/icons/terminal.svg", Section::Terminal,  group, layout);
    addButton(":/icons/list.svg",     Section::Tags,      group, layout);

    layout->addStretch(); // прижать кнопки к верху

    updateTheme();
    connect(&fasthub::ui::ThemeManager::instance(), &fasthub::ui::ThemeManager::themeApplied,
            this, &IconRail::updateTheme);
}

QToolButton *IconRail::addButton(const QString &iconPath, Section section,
                                  QButtonGroup *group, QVBoxLayout *layout)
{
    auto *btn = new QToolButton();
    btn->setIconSize({20, 20});
    btn->setAutoRaise(true);
    btn->setCheckable(true);

    switch (section) {
        case Section::Files:     btn->setObjectName("railFiles"); break;
        case Section::Graph:     btn->setObjectName("railGraph"); break;
        case Section::Canvas:    btn->setObjectName("railCanvas"); break;
        case Section::Calendar:  btn->setObjectName("railCalendar"); break;
        case Section::Templates: btn->setObjectName("railTemplates"); break;
        case Section::Terminal:  btn->setObjectName("railTerminal"); break;
        case Section::Tags:      btn->setObjectName("railTags"); break;
    }

    connect(btn, &QToolButton::clicked, this, [this, section] {
        emit sectionRequested(section);
    });

    group->addButton(btn);
    layout->addWidget(btn);
    m_buttons.append({btn, iconPath});
    return btn;
}

void IconRail::updateTheme()
{
    const QColor color = getThemeIconColor();
    for (const auto &pair : m_buttons) {
        pair.first->setIcon(fasthub::ui::IconUtil::tintedIcon(pair.second, color, 20));
    }
}

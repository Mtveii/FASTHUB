#pragma once
#include <QToolButton>
#include <QIcon>
#include "icon_util.h"
#include "theme_manager.h"

// Разворачивающаяся стрелка для узла папки: ▶ -> ▼
class TreeChevronButton : public QToolButton
{
    Q_OBJECT
public:
    explicit TreeChevronButton(QWidget *parent = nullptr) : QToolButton(parent)
    {
        setIconSize({14, 14});
        setAutoRaise(true);
        setCheckable(true);

        updateTheme();
        connect(&fasthub::ui::ThemeManager::instance(), &fasthub::ui::ThemeManager::themeApplied,
                this, &TreeChevronButton::updateTheme);

        connect(this, &QToolButton::toggled, this, [this](bool expanded) {
            updateIconState(expanded);
            emit expandedChanged(expanded);
        });
    }

public slots:
    void updateTheme()
    {
        updateIconState(isChecked());
    }

private:
    void updateIconState(bool expanded)
    {
        QColor color("#FFFFFF");
        for (const auto& t : fasthub::ui::ThemeManager::instance().themes()) {
            if (t.id == fasthub::ui::ThemeManager::instance().currentThemeId()) {
                color = QColor(t.isDark ? "#FFFFFF" : "#1A1A1A");
                break;
            }
        }
        const QString iconPath = expanded ? ":/icons/chevron-down.svg" : ":/icons/chevron-right.svg";
        setIcon(fasthub::ui::IconUtil::tintedIcon(iconPath, color, 14));
    }

signals:
    void expandedChanged(bool expanded);
};

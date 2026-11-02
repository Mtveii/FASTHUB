#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
#include "animation_util.h"

namespace fasthub::ui {

class AppearanceWidget : public QWidget {
    Q_OBJECT
public:
    explicit AppearanceWidget(QWidget *parent = nullptr);

private slots:
    void refreshList();
    void onFilterChanged(const QString &text);
    void onCardClicked(QListWidgetItem *item);
    void onCreateCustomTheme();

private:
    void setupUi();
    QWidget *buildCard(const QString &themeId, bool isCreateCard);
    void markCurrentCard();

    QListWidget *m_grid = nullptr;
    QLineEdit *m_search = nullptr;
    FadeStackedWidget *m_settingsStack = nullptr;
    QListWidget *m_navMenu = nullptr;
};

} // namespace fasthub::ui

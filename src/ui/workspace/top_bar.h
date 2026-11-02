#pragma once
#include <QWidget>
#include <QToolButton>

class TopBar : public QWidget {
    Q_OBJECT
public:
    explicit TopBar(QWidget *parent = nullptr);

public slots:
    void updateTheme();

signals:
    void sidebarToggleRequested();
    void filesTabRequested();
    void searchTabRequested();
    void bookmarksTabRequested();

private:
    QToolButton *m_sidebarToggle;
    QToolButton *m_filesTab;
    QToolButton *m_searchTab;
    QToolButton *m_bookmarksTab;
};

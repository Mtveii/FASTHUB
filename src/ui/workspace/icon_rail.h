#pragma once
#include <QWidget>
#include <QToolButton>
#include <QButtonGroup>
#include <QVBoxLayout>
#include <QVector>
#include <QPair>

class IconRail : public QWidget {
    Q_OBJECT
public:
    enum class Section { Files, Graph, Canvas, Calendar, Templates, Terminal, Tags };
    explicit IconRail(QWidget *parent = nullptr);

public slots:
    void updateTheme();

signals:
    void sectionRequested(Section section);

private:
    QToolButton *addButton(const QString &iconPath, Section section, QButtonGroup *group,
                           QVBoxLayout *layout);

    QVector<QPair<QToolButton *, QString>> m_buttons;
};

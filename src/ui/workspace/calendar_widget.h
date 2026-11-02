#pragma once

#include <QWidget>
#include <QCalendarWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QSplitter>

namespace fasthub::ui {

class CalendarWidget : public QWidget {
    Q_OBJECT
public:
    explicit CalendarWidget(QString workspacePath, QWidget *parent = nullptr);

private slots:
    void onDateSelected(const QDate &date);
    void onAddTask();

private:
    void loadTasksForDate(const QDate &date);
    QString dailyNotePath(const QDate &date) const;

    QString m_workspacePath;
    QCalendarWidget *m_calendar;
    QLabel *m_selectedDateLabel;
    QListWidget *m_taskListWidget;
    QLineEdit *m_taskInput;
    QPushButton *m_addTaskBtn;
    QDate m_currentDate;
};

} // namespace fasthub::ui

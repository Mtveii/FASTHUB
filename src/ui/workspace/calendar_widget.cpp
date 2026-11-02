#include "calendar_widget.h"

#include <QFileInfo>
#include <QMessageBox>
#include "../style_tokens.h"

namespace fasthub::ui {

CalendarWidget::CalendarWidget(QString workspacePath, QWidget *parent)
    : QWidget(parent), m_workspacePath(std::move(workspacePath)),
      m_currentDate(QDate::currentDate()) {

    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(16);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(1);

    // Left: Calendar
    m_calendar = new QCalendarWidget(this);
    m_calendar->setGridVisible(true);
    m_calendar->setStyleSheet(
        "QCalendarWidget QWidget { background-color: palette(window); color: palette(text); }"
        "QCalendarWidget QToolButton { color: palette(text); background-color: palette(button); "
        "border-radius: 4px; margin: 2px; }"
        "QCalendarWidget QToolButton:hover { background-color: palette(alternate-base); }"
        "QCalendarWidget QMenu { background-color: palette(window); color: palette(text); }"
        "QCalendarWidget QSpinBox { background-color: palette(base); color: palette(text); "
        "selection-background-color: palette(highlight); }"
        "QAbstractItemView:enabled { background-color: palette(base); color: palette(text); "
        "selection-background-color: palette(highlight); selection-color: "
        "palette(highlighted-text); }");
    connect(m_calendar, &QCalendarWidget::clicked, this, &CalendarWidget::onDateSelected);

    // Right: Tasks / Notes for selected date
    auto *rightPanel = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);

    m_selectedDateLabel = new QLabel(this);
    QFont f = m_selectedDateLabel->font();
    f.setPointSize(f.pointSize() + 2);
    f.setBold(true);
    m_selectedDateLabel->setFont(f);
    rightLayout->addWidget(m_selectedDateLabel);

    m_taskListWidget = new QListWidget(this);
    m_taskListWidget->setStyleSheet(
        "QListWidget { background-color: palette(base); color: palette(text); border: 1px solid "
        "palette(mid); border-radius: 6px; padding: 4px; }"
        "QListWidget::item { padding: 6px; border-bottom: 1px solid palette(alternate-base); "
        "border-radius: 4px; }"
        "QListWidget::item:hover { background-color: palette(alternate-base); }");
    rightLayout->addWidget(m_taskListWidget, 1);

    auto *inputLayout = new QHBoxLayout();
    inputLayout->setSpacing(6);
    m_taskInput = new QLineEdit(this);
    m_taskInput->setPlaceholderText("Add a task or note for this day...");
    m_taskInput->setStyleSheet(
        "QLineEdit { background-color: palette(base); color: palette(text); border: 1px solid "
        "palette(mid); border-radius: 4px; padding: 6px; }");
    connect(m_taskInput, &QLineEdit::returnPressed, this, &CalendarWidget::onAddTask);
    inputLayout->addWidget(m_taskInput, 1);

    m_addTaskBtn = new QPushButton("Add", this);
    m_addTaskBtn->setStyleSheet(
        "QPushButton { background-color: palette(button); color: palette(button-text); border: 1px "
        "solid palette(mid); border-radius: 4px; padding: 6px 12px; } QPushButton:hover { "
        "background-color: palette(alternate-base); }");
    connect(m_addTaskBtn, &QPushButton::clicked, this, &CalendarWidget::onAddTask);
    inputLayout->addWidget(m_addTaskBtn);

    rightLayout->addLayout(inputLayout);

    splitter->addWidget(m_calendar);
    splitter->addWidget(rightPanel);
    splitter->setSizes({450, 450});

    mainLayout->addWidget(splitter);
    setStyleSheet("background-color: palette(window);");

    onDateSelected(m_currentDate);
}

QString CalendarWidget::dailyNotePath(const QDate &date) const {
    if (m_workspacePath.isEmpty())
        return QString();
    return m_workspacePath + "/" + date.toString("yyyy-MM-dd") + ".md";
}

void CalendarWidget::onDateSelected(const QDate &date) {
    m_currentDate = date;
    m_selectedDateLabel->setText(date.toString("dddd, MMMM d, yyyy"));
    loadTasksForDate(date);
}

void CalendarWidget::loadTasksForDate(const QDate &date) {
    m_taskListWidget->clear();
    const QString path = dailyNotePath(date);
    if (path.isEmpty() || !QFile::exists(path)) {
        return;
    }
    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty()) {
                m_taskListWidget->addItem(line);
            }
        }
    }
}

void CalendarWidget::onAddTask() {
    const QString text = m_taskInput->text().trimmed();
    if (text.isEmpty())
        return;

    if (m_workspacePath.isEmpty()) {
        QMessageBox::information(this, "Workspace", "Please open a workspace folder first.");
        return;
    }

    const QString path = dailyNotePath(m_currentDate);
    QFile file(path);
    bool exists = QFile::exists(path);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        if (!exists) {
            out << "# " << m_currentDate.toString("yyyy-MM-dd") << "\n\n";
        }
        out << "- [ ] " << text << "\n";
        file.close();
        m_taskInput->clear();
        loadTasksForDate(m_currentDate);
    }
}

} // namespace fasthub::ui

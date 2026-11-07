#include "search_dialog.h"

#include <QVBoxLayout>
#include <QListWidgetItem>
#include <QLabel>

namespace fasthub::ui {

SearchDialog::SearchDialog(std::shared_ptr<core::ISearchProvider> searchProvider, QWidget *parent)
    : QDialog(parent), m_searchProvider(searchProvider) {
    setupUi();
}

void SearchDialog::setupUi() {
    setWindowTitle("Quick Switcher");
    resize(640, 420);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    auto *container = new QWidget(this);
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto *headerLabel = new QLabel("Введите имя файла для создания или перехода...", container);
    headerLabel->setStyleSheet("font-size: 14px; font-weight: 500; color: palette(text);");
    layout->addWidget(headerLabel);

    m_searchLineEdit = new QLineEdit(container);
    m_searchLineEdit->setPlaceholderText("Введите имя файла для создания или перехода...");
    m_searchLineEdit->setStyleSheet(
        "QLineEdit { font-size: 15px; padding: 8px; border-radius: 6px; background-color: "
        "palette(base); color: palette(text); border: 1px solid palette(mid); }");
    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(250);
    connect(m_debounceTimer, &QTimer::timeout, this, &SearchDialog::runSearch);
    connect(m_searchLineEdit, &QLineEdit::textChanged, this, &SearchDialog::onSearchTextChanged);
    layout->addWidget(m_searchLineEdit);

    m_resultsListWidget = new QListWidget(container);
    m_resultsListWidget->setStyleSheet(
        "QListWidget { background-color: palette(base); border: 1px solid palette(mid); "
        "border-radius: 6px; outline: 0; }"
        "QListWidget::item { padding: 8px 12px; border-radius: 4px; }"
        "QListWidget::item:selected { background-color: palette(highlight); color: "
        "palette(highlighted-text); }");
    layout->addWidget(m_resultsListWidget, 1);

    auto *footerLabel =
        new QLabel("↑↓ навигация    ↵ открыть    ctrl ↵ открыть в новой вкладке    ctrl alt "
                   "открыть в области справа    shift ↵ создать    esc отмена",
                   container);
    footerLabel->setStyleSheet("font-size: 11px; color: palette(placeholder-text);");
    layout->addWidget(footerLabel);

    mainLayout->addWidget(container);

    m_searchLineEdit->setFocus();
}

void SearchDialog::onSearchTextChanged(const QString &text) {
    Q_UNUSED(text);
    m_debounceTimer->stop();
    m_debounceTimer->start();
}

void SearchDialog::runSearch() {
    const QString text = m_searchLineEdit->text();
    m_resultsListWidget->clear();
    if (!m_searchProvider || text.trimmed().isEmpty()) {
        return;
    }

    auto results = m_searchProvider->search(text.trimmed().toStdString());
    for (const auto &res : results) {
        QString itemText =
            QString("%1  [%2]")
                .arg(QString::fromStdString(res.title), QString::fromStdString(res.source));
        QListWidgetItem *item = new QListWidgetItem(itemText, m_resultsListWidget);
        item->setData(Qt::UserRole, QString::fromStdString(res.internalId));
    }
}

} // namespace fasthub::ui

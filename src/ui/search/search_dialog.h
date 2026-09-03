#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <memory>
#include "core/i_search_provider.h"

namespace fasthub::ui {

class SearchDialog : public QDialog {
    Q_OBJECT
public:
    explicit SearchDialog(std::shared_ptr<core::ISearchProvider> searchProvider, QWidget* parent = nullptr);
    ~SearchDialog() override = default;

private slots:
    void onSearchTextChanged(const QString& text);

private:
    void setupUi();

    std::shared_ptr<core::ISearchProvider> m_searchProvider;
    QLineEdit* m_searchLineEdit;
    QListWidget* m_resultsListWidget;
};

} // namespace fasthub::ui

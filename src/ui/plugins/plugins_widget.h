#pragma once

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QTabWidget>
#include "plugins/plugin_manager.h"

namespace fasthub::ui {

class AppearanceWidget;

/// @brief Plugins & appearance hub. Tab 1: installed plugins + install flow.
///        Tab 2: theme gallery / dark-light switching / custom themes.
class PluginsWidget : public QWidget {
    Q_OBJECT
public:
    explicit PluginsWidget(QWidget* parent = nullptr);
    ~PluginsWidget() override = default;

private slots:
    void onInstallPluginClicked();

private:
    void setupUi();
    void refreshList();

    QTabWidget* m_tabs;
    QListWidget* m_pluginsListWidget;
    QPushButton* m_installButton;
};

} // namespace fasthub::ui

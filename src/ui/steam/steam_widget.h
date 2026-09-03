#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <memory>
#include "providers/steam/steam_provider.h"

namespace fasthub::ui {

class SteamSettingsDialog;

/// @brief Steam friends presence panel (real Web API polling with diff
///        cache; honest mock when no key is configured).
class SteamWidget : public QWidget {
    Q_OBJECT
public:
    explicit SteamWidget(std::shared_ptr<providers::steam::SteamProvider> steamProvider, QWidget* parent = nullptr);
    ~SteamWidget() override = default;

private slots:
    void loadFriends();
    void onPresenceUpdated(const QString& status);
    void onSettingsClicked();

private:
    void setupUi();
    QIcon statusDot(const fasthub::providers::steam::SteamPresence& p) const;

    std::shared_ptr<providers::steam::SteamProvider> m_steamProvider;
    QListWidget* m_friendsListWidget;
    QLabel* m_statusLabel;
    QTimer m_uiRefreshTimer; // debounces bursts of presenceUpdated signals
};

} // namespace fasthub::ui

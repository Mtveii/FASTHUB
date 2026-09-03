#pragma once

#include <QDialog>
#include <memory>

#include "providers/steam/steam_provider.h"

class QLineEdit;
class QLabel;

namespace fasthub::ui {

/// @brief Steam Web API setup: API key + own SteamID64 (or vanity name).
class SteamSettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SteamSettingsDialog(std::shared_ptr<providers::steam::SteamProvider> provider, QWidget* parent = nullptr);

private slots:
    void onSave();

private:
    void setupUi();

    std::shared_ptr<providers::steam::SteamProvider> m_provider;
    QLineEdit* m_apiKeyEdit;
    QLineEdit* m_steamIdEdit;
    QLabel* m_hintLabel;
};

} // namespace fasthub::ui

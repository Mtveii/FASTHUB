#pragma once

#include <QDialog>
#include <QComboBox>
#include <QLabel>
#include "infrastructure/performance_manager.h"

namespace fasthub::ui {

class DeveloperDialog : public QDialog {
    Q_OBJECT
public:
    explicit DeveloperDialog(QWidget* parent = nullptr);
    ~DeveloperDialog() override = default;

private slots:
    void onModeChanged(int index);

private:
    void setupUi();

    QComboBox* m_modeComboBox;
    QLabel* m_metricsLabel;
};

} // namespace fasthub::ui

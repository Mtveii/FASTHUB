#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <memory>
#include "providers/ai/ai_provider.h"

namespace fasthub::ui {

class AIWidget : public QWidget {
    Q_OBJECT
public:
    AIWidget(std::shared_ptr<providers::ai::AIProvider> aiProvider, QWidget *parent = nullptr);

    /// @brief Pulls the freshest cross-module context into the context box.
    ///        Called by MainWindow when the user switches to the AI tab.
    void pullContext();

private slots:
    void onAskClicked();
    void onSummarizeClicked();
    void onExplainCodeClicked();

private:
    void setupUi();

    std::shared_ptr<providers::ai::AIProvider> m_aiProvider;
    QComboBox *m_modelSelector;
    QTextEdit *m_contextInput;
    QTextEdit *m_promptInput;
    QTextEdit *m_responseDisplay;
    QPushButton *m_askButton;
};

} // namespace fasthub::ui

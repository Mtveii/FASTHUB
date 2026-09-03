#include "ai_widget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSplitter>
#include <QPushButton>

#include "context_broker.h"

namespace fasthub::ui {

AIWidget::AIWidget(std::shared_ptr<providers::ai::AIProvider> aiProvider, QWidget* parent)
    : QWidget(parent), m_aiProvider(aiProvider) {
    setupUi();
}

void AIWidget::pullContext() {
    const QString ctx = ContextBroker::instance().buildAiContext();
    if (!ctx.isEmpty()) {
        m_contextInput->setPlainText(ctx);
    }
}

void AIWidget::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Top bar: Model selector
    QHBoxLayout* topLayout = new QHBoxLayout();
    topLayout->addWidget(new QLabel("AI Provider:", this));
    m_modelSelector = new QComboBox(this);
    m_modelSelector->addItem("OpenAI GPT-4o", static_cast<int>(core::AIProviderType::OpenAI));
    m_modelSelector->addItem("Anthropic Claude 3.5", static_cast<int>(core::AIProviderType::Anthropic));
    m_modelSelector->addItem("Google Gemini 1.5", static_cast<int>(core::AIProviderType::Gemini));
    topLayout->addWidget(m_modelSelector);
    topLayout->addStretch();

    // Context & Prompt splitter
    QSplitter* splitter = new QSplitter(Qt::Vertical, this);

    QWidget* inputWidget = new QWidget(this);
    QVBoxLayout* inputLayout = new QVBoxLayout(inputWidget);
    inputLayout->setContentsMargins(0, 0, 0, 0);

    inputLayout->addWidget(new QLabel("Explicit Context (Selected Message / Code / File):", this));
    m_contextInput = new QTextEdit(this);
    m_contextInput->setPlaceholderText("Paste or select text/code context here (AI has no automatic access to full history)...");
    inputLayout->addWidget(m_contextInput);

    inputLayout->addWidget(new QLabel("Prompt / Question:", this));
    m_promptInput = new QTextEdit(this);
    m_promptInput->setMaximumHeight(100);
    inputLayout->addWidget(m_promptInput);

    QHBoxLayout* actionsLayout = new QHBoxLayout();
    m_askButton = new QPushButton("Ask AI", this);
    connect(m_askButton, &QPushButton::clicked, this, &AIWidget::onAskClicked);

    QPushButton* summarizeBtn = new QPushButton("Summarize", this);
    connect(summarizeBtn, &QPushButton::clicked, this, &AIWidget::onSummarizeClicked);

    QPushButton* explainBtn = new QPushButton("Explain Code", this);
    connect(explainBtn, &QPushButton::clicked, this, &AIWidget::onExplainCodeClicked);

    actionsLayout->addWidget(m_askButton);
    actionsLayout->addWidget(summarizeBtn);
    actionsLayout->addWidget(explainBtn);

    // Cross-module context: attach the whole current file (size-capped).
    auto* loadFileBtn = new QPushButton("Load full file", this);
    loadFileBtn->setToolTip("Attach the entire current workspace file as context (capped at 20k chars)");
    connect(loadFileBtn, &QPushButton::clicked, this, [this]() {
        QString content = ContextBroker::instance().currentFileContent();
        if (content.isEmpty()) {
            m_responseDisplay->setPlainText("No file is open in the Workspace tab.");
            return;
        }
        if (content.size() > 20000) {
            content = content.left(20000) + "\n... [truncated]";
        }
        m_contextInput->setPlainText(
            QString("=== Full file: %1 ===\n%2")
                .arg(ContextBroker::instance().currentFilePath(), content));
    });
    actionsLayout->addWidget(loadFileBtn);
    actionsLayout->addStretch();
    inputLayout->addLayout(actionsLayout);

    // Response display
    QWidget* responseWidget = new QWidget(this);
    QVBoxLayout* responseLayout = new QVBoxLayout(responseWidget);
    responseLayout->setContentsMargins(0, 0, 0, 0);
    responseLayout->addWidget(new QLabel("AI Response:", this));
    m_responseDisplay = new QTextEdit(this);
    m_responseDisplay->setReadOnly(true);
    responseLayout->addWidget(m_responseDisplay);

    splitter->addWidget(inputWidget);
    splitter->addWidget(responseWidget);
    splitter->setSizes({350, 350});

    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(splitter);
}

void AIWidget::onAskClicked() {
    if (!m_aiProvider) return;
    std::string context = m_contextInput->toPlainText().toStdString();
    std::string prompt = m_promptInput->toPlainText().toStdString();

    std::string response = m_aiProvider->ask(context, prompt);
    m_responseDisplay->setPlainText(QString::fromStdString(response));
}

void AIWidget::onSummarizeClicked() {
    if (!m_aiProvider) return;
    std::string context = m_contextInput->toPlainText().toStdString();
    std::string response = m_aiProvider->ask(context, "Please summarize the above context concisely.");
    m_responseDisplay->setPlainText(QString::fromStdString(response));
}

void AIWidget::onExplainCodeClicked() {
    if (!m_aiProvider) return;
    std::string context = m_contextInput->toPlainText().toStdString();
    std::string response = m_aiProvider->ask(context, "Please explain how this code works step by step.");
    m_responseDisplay->setPlainText(QString::fromStdString(response));
}

} // namespace fasthub::ui

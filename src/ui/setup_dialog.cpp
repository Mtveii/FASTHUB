#include "setup_dialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>
#include <QFileInfo>

namespace fasthub::ui {

SetupDialog::SetupDialog(QWidget* parent) : QDialog(parent) {
    setupUi();
}

void SetupDialog::setupUi() {
    setWindowTitle("Welcome to FastHub");
    setModal(true);
    resize(560, 200);

    QVBoxLayout* layout = new QVBoxLayout(this);

    QLabel* title = new QLabel("Welcome! Let's set up your workspace.", this);
    title->setStyleSheet("font-size: 16px; font-weight: bold;");
    layout->addWidget(title);

    QLabel* description = new QLabel(
        "Choose the folder where your notes and files live.\n"
        "You can pick any directory on any drive of this PC.", this);
    description->setWordWrap(true);
    layout->addWidget(description);

    QHBoxLayout* row = new QHBoxLayout();
    m_pathEdit = new QLineEdit(this);
    m_pathEdit->setPlaceholderText(QDir::homePath());
    m_browseButton = new QPushButton("Browse...", this);
    connect(m_browseButton, &QPushButton::clicked, this, &SetupDialog::browse);
    row->addWidget(m_pathEdit, 1);
    row->addWidget(m_browseButton);
    layout->addLayout(row);

    layout->addStretch();

    QHBoxLayout* buttons = new QHBoxLayout();
    buttons->addStretch();
    m_okButton = new QPushButton("Start using FastHub", this);
    m_okButton->setDefault(true);
    connect(m_okButton, &QPushButton::clicked, this, &SetupDialog::acceptIfNeeded);
    buttons->addWidget(m_okButton);
    layout->addLayout(buttons);
}

QString SetupDialog::workspacePath() const {
    const QString typed = m_pathEdit->text().trimmed();
    return typed.isEmpty() ? QDir::homePath() : typed;
}

void SetupDialog::browse() {
    // Native dialog: starts at "This PC" so every local/removable drive is
    // reachable, not just C:.
    const QString dir = QFileDialog::getExistingDirectory(
        this,
        "Select notes/workspace folder",
        QString(), // empty root -> shell shows This PC with all drives
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        m_pathEdit->setText(dir);
    }
}

void SetupDialog::acceptIfNeeded() {
    const QString path = workspacePath();

    if (!QFileInfo::exists(path)) {
        const auto answer = QMessageBox::question(this, "Create folder?",
            QString("The folder does not exist:\n%1\n\nCreate it?").arg(path));
        if (answer != QMessageBox::Yes) {
            return;
        }
        if (!QDir().mkpath(path)) {
            QMessageBox::warning(this, "Error", "Could not create folder: " + path);
            return;
        }
    }

    if (!QFileInfo(path).isDir()) {
        QMessageBox::warning(this, "Error", "The selected path is a file, not a folder.");
        return;
    }

    accept();
}

} // namespace fasthub::ui

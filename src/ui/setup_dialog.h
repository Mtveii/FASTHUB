#pragma once

#include <QDialog>
class QLineEdit;
class QPushButton;

namespace fasthub::ui {

/// @brief First-run onboarding dialog.
///
/// Asks the user where their notes/workspace directory lives. The native
/// directory browser starts at "This PC", so any drive (C:, D:, network
/// mounts, ...) is reachable - not only C:. The chosen path is persisted in
/// the settings table ("workspace.path") and marked done ("onboarding.done"),
/// so the dialog appears exactly once unless the user clears settings.
class SetupDialog : public QDialog {
    Q_OBJECT
public:
    explicit SetupDialog(QWidget *parent = nullptr);

    /// @brief Directory chosen by the user (valid after exec() returns Accepted).
    QString workspacePath() const;

private slots:
    void browse();
    void acceptIfNeeded();

private:
    void setupUi();

    QLineEdit *m_pathEdit;
    QPushButton *m_browseButton;
    QPushButton *m_okButton;
};

} // namespace fasthub::ui

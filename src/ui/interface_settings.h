#pragma once

#include <QObject>
#include <QString>

namespace fasthub::ui {

/// @brief User-adjustable interface metrics AND behavior toggles.
///
/// Values persist via DatabaseManager settings and are applied live through
/// signals. Edited from Plugins -> Appearance (Interface / Behavior groups).
class InterfaceSettings : public QObject {
    Q_OBJECT
public:
    static InterfaceSettings& instance();

    /// @brief Loads persisted values from the settings table.
    void initialize();

    // --- Layout ---
    int tabIconSize() const { return m_tabIconSize; }
    void setTabIconSize(int px);

    int fontSize() const { return m_fontSize; }
    void setFontSize(int pt);

    int treePanelWidth() const { return m_treeWidth; }
    void setTreePanelWidth(int px);

    // --- Behavior ---
    /// true = one click opens files in the tree (default), false = double click.
    bool treeSingleClick() const { return m_treeSingleClick; }
    void setTreeSingleClick(bool on);

    int autoSaveDelayMs() const { return m_autoSaveDelay; }
    void setAutoSaveDelayMs(int ms);

    QString editorFontFamily() const { return m_editorFont; }
    void setEditorFontFamily(const QString& family);

    bool ayuAutoLaunch() const { return m_ayuAutoLaunch; }
    void setAyuAutoLaunch(bool on);

    bool ayuStripChrome() const { return m_ayuStripChrome; }
    void setAyuStripChrome(bool on);

    int webZoomPercent() const { return m_webZoom; }
    void setWebZoomPercent(int pct);

signals:
    void tabIconSizeChanged(int px);
    void fontSizeChanged(int pt);
    void treePanelWidthChanged(int px);
    void treeSingleClickChanged(bool on);
    void autoSaveDelayChanged(int ms);
    void editorFontChanged(const QString& family);
    void webZoomChanged(int pct);

private:
    InterfaceSettings() = default;

    int m_tabIconSize = 20;
    int m_fontSize = 10;
    int m_treeWidth = 300;
    bool m_treeSingleClick = true;
    int m_autoSaveDelay = 500;
    QString m_editorFont = QStringLiteral("Consolas");
    bool m_ayuAutoLaunch = true;
    bool m_ayuStripChrome = true;
    int m_webZoom = 100;
};

} // namespace fasthub::ui

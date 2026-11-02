#include "interface_settings.h"

#include "database/database_manager.h"

namespace fasthub::ui {

namespace {
constexpr const char *kKeyTabIcon = "ui.tab_icon_size";
constexpr const char *kKeyFont = "ui.font_size";
constexpr const char *kKeyTree = "ui.tree_width";
constexpr const char *kKeySingleClick = "ui.tree_single_click";
constexpr const char *kKeyAutoSave = "ui.autosave_ms";
constexpr const char *kKeyEditorFont = "ui.editor_font";
constexpr const char *kKeyAyuLaunch = "ui.ayu_autolaunch";
constexpr const char *kKeyAyuChrome = "ui.ayu_strip_chrome";
constexpr const char *kKeyWebZoom = "ui.web_zoom";
} // namespace

InterfaceSettings &InterfaceSettings::instance() {
    static InterfaceSettings s;
    return s;
}

void InterfaceSettings::initialize() {
    auto &db = database::DatabaseManager::instance();
    bool ok = false;

    if (auto v = db.getSetting(kKeyTabIcon)) {
        const int n = v->toInt(&ok);
        if (ok && n >= 14 && n <= 32)
            m_tabIconSize = n;
    }
    if (auto v = db.getSetting(kKeyFont)) {
        const int n = v->toInt(&ok);
        if (ok && n >= 9 && n <= 16)
            m_fontSize = n;
    }
    if (auto v = db.getSetting(kKeyTree)) {
        const int n = v->toInt(&ok);
        if (ok && n >= 180 && n <= 600)
            m_treeWidth = n;
    }
    if (auto v = db.getSetting(kKeySingleClick)) {
        m_treeSingleClick = *v != QStringLiteral("0");
    }
    if (auto v = db.getSetting(kKeyAutoSave)) {
        const int n = v->toInt(&ok);
        if (ok && n >= 150 && n <= 2000)
            m_autoSaveDelay = n;
    }
    if (auto v = db.getSetting(kKeyEditorFont)) {
        if (!v->isEmpty())
            m_editorFont = *v;
    }
    if (auto v = db.getSetting(kKeyAyuLaunch)) {
        m_ayuAutoLaunch = *v != QStringLiteral("0");
    }
    if (auto v = db.getSetting(kKeyAyuChrome)) {
        m_ayuStripChrome = *v != QStringLiteral("0");
    }
    if (auto v = db.getSetting(kKeyWebZoom)) {
        const int n = v->toInt(&ok);
        if (ok && n >= 70 && n <= 150)
            m_webZoom = n;
    }
}

void InterfaceSettings::setTabIconSize(int px) {
    const int v = qBound(14, px, 32);
    if (v == m_tabIconSize)
        return;
    m_tabIconSize = v;
    database::DatabaseManager::instance().setSetting(kKeyTabIcon, QString::number(v));
    emit tabIconSizeChanged(v);
}

void InterfaceSettings::setFontSize(int pt) {
    const int v = qBound(9, pt, 16);
    if (v == m_fontSize)
        return;
    m_fontSize = v;
    database::DatabaseManager::instance().setSetting(kKeyFont, QString::number(v));
    emit fontSizeChanged(v);
}

void InterfaceSettings::setTreePanelWidth(int px) {
    const int v = qBound(180, px, 600);
    if (v == m_treeWidth)
        return;
    m_treeWidth = v;
    database::DatabaseManager::instance().setSetting(kKeyTree, QString::number(v));
    emit treePanelWidthChanged(v);
}

void InterfaceSettings::setTreeSingleClick(bool on) {
    if (on == m_treeSingleClick)
        return;
    m_treeSingleClick = on;
    database::DatabaseManager::instance().setSetting(kKeySingleClick, on ? "1" : "0");
    emit treeSingleClickChanged(on);
}

void InterfaceSettings::setAutoSaveDelayMs(int ms) {
    const int v = qBound(150, ms, 2000);
    if (v == m_autoSaveDelay)
        return;
    m_autoSaveDelay = v;
    database::DatabaseManager::instance().setSetting(kKeyAutoSave, QString::number(v));
    emit autoSaveDelayChanged(v);
}

void InterfaceSettings::setEditorFontFamily(const QString &family) {
    if (family.isEmpty() || family == m_editorFont)
        return;
    m_editorFont = family;
    database::DatabaseManager::instance().setSetting(kKeyEditorFont, family);
    emit editorFontChanged(family);
}

void InterfaceSettings::setAyuAutoLaunch(bool on) {
    if (on == m_ayuAutoLaunch)
        return;
    m_ayuAutoLaunch = on;
    database::DatabaseManager::instance().setSetting(kKeyAyuLaunch, on ? "1" : "0");
}

void InterfaceSettings::setAyuStripChrome(bool on) {
    if (on == m_ayuStripChrome)
        return;
    m_ayuStripChrome = on;
    database::DatabaseManager::instance().setSetting(kKeyAyuChrome, on ? "1" : "0");
}

void InterfaceSettings::setWebZoomPercent(int pct) {
    const int v = qBound(70, pct, 150);
    if (v == m_webZoom)
        return;
    m_webZoom = v;
    database::DatabaseManager::instance().setSetting(kKeyWebZoom, QString::number(v));
    emit webZoomChanged(v);
}

} // namespace fasthub::ui

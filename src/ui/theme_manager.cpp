#include "theme_manager.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

#include "database/database_manager.h"
#include "infrastructure/logger.h"
#include "icon_util.h"
#include "style_tokens.h"

namespace fasthub::ui {

namespace {

constexpr const char* kSettingThemeId = "theme.id";

// QSS template shared by every theme. Color roles are substituted from
// ThemeInfo::colors so all themes stay visually consistent.
// Typography roles: Manrope = headers/UI labels, Inter = body,
// JetBrains Mono = file names/code (bundled via resources).
QString buildStyleSheet(const QHash<QString, QString>& c) {
    return QString(QStringLiteral(
        "QWidget { background-color: %1; color: %2; font-size: 13px; font-family: 'Inter','Segoe UI'; }"
        "QMainWindow, QDialog { background-color: %1; }"
        "QLabel[heading='true'] { font-family: 'Manrope','Inter'; font-weight: 600; font-size: 15px; }"
        "QListWidget, QTreeWidget, QListView, QTreeView, QTextEdit, QLineEdit, QPlainTextEdit { "
        "background-color: %3; color: %2; border: 1px solid %7; border-radius: %8; padding: %10; selection-background-color: %5; selection-color: #ffffff; }"
        "QTreeView::item, QListWidget::item { padding: %10 %11; border-left: 2px solid transparent; border-radius: %8; }"
        "QTreeView::item:hover { background-color: %7; }"
        "QTreeView::item:selected { border-left: 2px solid %5; background-color: %5; }"
        "QListWidget#navRail { background-color: %3; border: none; border-radius: 0; outline: 0; }"
        "QListWidget#navRail::item { padding: 0; margin: %10 %11; border-radius: %9; border-left: 3px solid transparent; }"
        "QListWidget#navRail::item:selected { background-color: %7; border-left: 3px solid %5; }"
        "QListWidget#navRail::item:hover { background-color: %7; }"
        "QPushButton { background-color: transparent; color: %2; border: 1px solid %7; border-radius: %8; padding: %11 %13; }"
        "QPushButton:hover { border-color: %5; color: %5; }"
        "QPushButton:pressed { background-color: %7; }"
        "QPushButton:disabled { color: %4; border-color: %7; }"
        "QPushButton[primary='true'] { background-color: %5; color: #ffffff; border: none; font-family: 'Manrope','Inter'; font-weight: 600; }"
        "QPushButton[primary='true']:hover { background-color: %6; }"
        "QPushButton[primary='true']:pressed { background-color: %5; }"
        "QPushButton#iconToolBtn { padding: %10; min-width: 20px; min-height: 20px; border-radius: %8; font-size: 14px; }"
        "QPushButton#iconToolBtn:hover { border-color: %5; }"
        "QLabel { background: transparent; color: %2; }"
        "QLabel[role='muted'] { color: %4; }"
        "QTabWidget::pane { border: 1px solid %7; border-top: 2px solid %5; border-radius: %9; }"
        "QTabBar::tab { background: %3; color: %4; padding: %11 %13; border-top-left-radius: %9; border-top-right-radius: %9; font-family: 'Manrope','Inter'; }"
        "QTabBar::tab:selected { background: %5; color: #ffffff; }"
        "QSplitter::handle { background-color: %7; }"
        "QScrollBar:vertical { background: %3; width: 10px; margin: 0; }"
        "QScrollBar::handle:vertical { background: %7; border-radius: %8; min-height: 24px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar:horizontal { background: %3; height: 10px; margin: 0; }"
        "QScrollBar::handle:horizontal { background: %7; border-radius: %8; min-width: 24px; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }"
        "QComboBox { background-color: %3; color: %2; border: 1px solid %7; border-radius: %8; padding: %10 %11; }"
        "QComboBox QAbstractItemView { background-color: %3; color: %2; selection-background-color: %5; }"
        "QMenuBar { background: %3; color: %2; }"
        "QMenuBar::item:selected { background: %5; color: #ffffff; }"
        "QMenu { background: %3; color: %2; border: 1px solid %7; border-radius: %9; }"
        "QMenu::item:selected { background: %5; color: #ffffff; }"
        "QMessageBox { background: %1; }"
        "QLineEdit { font-family: 'JetBrains Mono','Consolas'; }"
        "QFrame[frameShape=\"4\"], QFrame[frameShape=\"5\"] { border: none; background-color: %7; max-height: 1px; margin-top: %12; margin-bottom: %12; }"
        "QPushButton:focus, QLineEdit:focus, QComboBox:focus, QCheckBox:focus, QSlider:focus { border-color: %5; }"
    ))
        .arg(c.value("bg"), c.value("text"), c.value("panel"), c.value("muted"),
             c.value("accent"), c.value("accentHover"), c.value("border"),
             QString::fromLatin1(tokens::kRadiusSm), QString::fromLatin1(tokens::kRadiusMd),
             QString::fromLatin1(tokens::kSpace1), QString::fromLatin1(tokens::kSpace2),
             QString::fromLatin1(tokens::kSpace3), QString::fromLatin1(tokens::kSpace4));
}

struct PaletteSpec {
    const char* baseName;
    int hue;
};

/// 25 base palettes; each gets a dark and a light variant -> 50 themes.
const std::vector<PaletteSpec> kPalettes = {
    {"Ocean", 210}, {"Forest", 130}, {"Sunset", 20}, {"Violet", 270}, {"Rose", 340},
    {"Teal", 175}, {"Amber", 40}, {"Crimson", 355}, {"Emerald", 150}, {"Sapphire", 225},
    {"Coral", 10}, {"Lavender", 285}, {"Mint", 160}, {"Sky", 200}, {"Wine", 330},
    {"Olive", 80}, {"Indigo", 245}, {"Peach", 30}, {"Steel", 215}, {"Chocolate", 55},
    {"Neon", 300}, {"Arctic", 190}, {"Magma", 350}, {"Jungle", 110}, {"Graphite", 220},
};

QString hsl(int h, int s, int l) {
    return QStringLiteral("hsl(%1, %2%, %3%)").arg(h).arg(s).arg(l);
}

} // namespace

QString ThemeInfo::styleSheet() const {
    return buildStyleSheet(colors);
}

ThemeManager& ThemeManager::instance() {
    static ThemeManager s_instance;
    return s_instance;
}

void ThemeManager::initialize() {
    if (!m_themes.empty()) {
        return;
    }
    buildBuiltinThemes();
    loadCustomThemes();

    const QString saved = database::DatabaseManager::instance()
                              .getSetting(kSettingThemeId)
                              .value_or(QString());
    if (saved.isEmpty()) {
        // First run: the FastHub brand theme (design spec palette).
        m_currentId = QStringLiteral("fasthub-dark");
    } else {
        m_currentId = saved;
    }
    applyTheme(m_currentId);
}

void ThemeManager::buildBuiltinThemes() {
    // ---- Brand identity themes (design spec tokens) --------------------
    ThemeInfo hubDark;
    hubDark.id = QStringLiteral("fasthub-dark");
    hubDark.name = QStringLiteral("FastHub Dark");
    hubDark.isDark = true;
    hubDark.colors = {
        {"bg", QStringLiteral("#0B0E14")},
        {"panel", QStringLiteral("#12161F")},
        {"text", QStringLiteral("#E6E9F0")},
        {"muted", QStringLiteral("#8A93A6")},
        {"accent", QStringLiteral("#6E7BFF")},
        {"accentHover", QStringLiteral("#8B96FF")},
        {"border", QStringLiteral("#1E2430")},
    };
    m_themes.push_back(hubDark);

    ThemeInfo hubLight;
    hubLight.id = QStringLiteral("fasthub-light");
    hubLight.name = QStringLiteral("FastHub Light");
    hubLight.isDark = false;
    hubLight.colors = {
        {"bg", QStringLiteral("#F7F8FC")},
        {"panel", QStringLiteral("#EEF0F7")},
        {"text", QStringLiteral("#1A1D26")},
        {"muted", QStringLiteral("#5C6577")},
        {"accent", QStringLiteral("#5A66F0")},
        {"accentHover", QStringLiteral("#6E7BFF")},
        {"border", QStringLiteral("#D8DCE8")},
    };
    m_themes.push_back(hubLight);

    // ---- 25 generated palettes x dark/light ----------------------------
    for (const auto& spec : kPalettes) {
        const QString base = QString::fromUtf8(spec.baseName);

        ThemeInfo dark;
        dark.id = base.toLower() + QStringLiteral("-dark");
        dark.name = base + QStringLiteral(" Dark");
        dark.isDark = true;
        dark.colors = {
            {"bg", hsl(spec.hue, 28, 10)},
            {"panel", hsl(spec.hue, 26, 15)},
            {"text", hsl(spec.hue, 12, 90)},
            {"muted", hsl(spec.hue, 14, 58)},
            {"accent", hsl(spec.hue, 62, 48)},
            {"accentHover", hsl(spec.hue, 68, 56)},
            {"border", hsl(spec.hue, 22, 26)},
        };
        m_themes.push_back(dark);

        ThemeInfo light;
        light.id = base.toLower() + QStringLiteral("-light");
        light.name = base + QStringLiteral(" Light");
        light.isDark = false;
        light.colors = {
            {"bg", hsl(spec.hue, 30, 97)},
            {"panel", hsl(spec.hue, 25, 92)},
            {"text", hsl(spec.hue, 30, 18)},
            {"muted", hsl(spec.hue, 15, 45)},
            {"accent", hsl(spec.hue, 60, 42)},
            {"accentHover", hsl(spec.hue, 65, 50)},
            {"border", hsl(spec.hue, 22, 80)},
        };
        m_themes.push_back(light);
    }
    m_builtinCount = static_cast<qint64>(m_themes.size());
}

QString ThemeManager::customThemesDir() const {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/themes";
    QDir().mkpath(dir);
    return dir;
}

bool ThemeManager::loadThemeFromJson(const QString& path, ThemeInfo* out) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
    if (obj.isEmpty()) {
        return false;
    }
    out->id = obj.value("id").toString();
    out->name = obj.value("name").toString();
    out->isDark = obj.value("dark").toBool(true);
    const QJsonObject cols = obj.value("colors").toObject();
    for (auto it = cols.begin(); it != cols.end(); ++it) {
        out->colors.insert(it.key(), it.value().toString());
    }
    // All roles must be present for the QSS template to render correctly.
    static const QStringList required = {"bg", "panel", "text", "muted", "accent", "accentHover", "border"};
    for (const QString& role : required) {
        if (!out->colors.contains(role) || out->colors.value(role).isEmpty()) {
            return false;
        }
    }
    return !out->id.isEmpty() && !out->name.isEmpty();
}

bool ThemeManager::writeThemeToJson(const QString& path, const ThemeInfo& info) {
    QJsonObject cols;
    for (auto it = info.colors.begin(); it != info.colors.end(); ++it) {
        cols.insert(it.key(), it.value());
    }
    QJsonObject obj;
    obj.insert("id", info.id);
    obj.insert("name", info.name);
    obj.insert("dark", info.isDark);
    obj.insert("colors", cols);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(QJsonDocument(obj).toJson());
    return true;
}

void ThemeManager::loadCustomThemes() {
    const QString dirPath = customThemesDir();
    const auto entries = QDir(dirPath).entryList({"*.json"}, QDir::Files, QDir::Name);
    for (const QString& entry : entries) {
        ThemeInfo info;
        if (loadThemeFromJson(dirPath + "/" + entry, &info)) {
            m_themes.push_back(info);
        }
    }
}

bool ThemeManager::applyTheme(const QString& id) {
    for (const auto& theme : m_themes) {
        if (theme.id == id) {
            qApp->setStyleSheet(theme.styleSheet());
            m_currentId = id;
            database::DatabaseManager::instance().setSetting(kSettingThemeId, id);
            IconUtil::clearCache();
            emit themeApplied(id);
            return true;
        }
    }
    infrastructure::Logger::instance().warn("Themes", "Unknown theme id requested: " + id);
    return false;
}

bool ThemeManager::saveCustomTheme(const ThemeInfo& info) {
    if (info.id.isEmpty() || info.name.isEmpty() || info.id.startsWith(QStringLiteral("__builtin"))) {
        return false;
    }

    // Refuse to overwrite a built-in theme.
    for (qint64 i = 0; i < m_builtinCount && i < static_cast<qint64>(m_themes.size()); ++i) {
        if (m_themes[i].id == info.id) {
            infrastructure::Logger::instance().warn("Themes", "Cannot overwrite built-in theme: " + info.id);
            return false;
        }
    }

    const QString path = customThemesDir() + "/" + info.id + ".json";
    if (!writeThemeToJson(path, info)) {
        infrastructure::Logger::instance().error("Themes", "Failed to write theme file: " + path);
        return false;
    }

    // Replace existing custom entry or append.
    for (auto& existing : m_themes) {
        if (existing.id == info.id) {
            existing = info;
            applyTheme(info.id);
            return true;
        }
    }
    m_themes.push_back(info);
    applyTheme(info.id);
    return true;
}

bool ThemeManager::removeCustomTheme(const QString& id) {
    for (qint64 i = 0; i < m_builtinCount && i < static_cast<qint64>(m_themes.size()); ++i) {
        if (m_themes[i].id == id) {
            infrastructure::Logger::instance().warn("Themes", "Refusing to remove built-in theme: " + id);
            return false;
        }
    }

    QFile::remove(customThemesDir() + "/" + id + ".json");

    bool wasCurrent = (m_currentId == id);
    for (auto it = m_themes.begin(); it != m_themes.end(); ++it) {
        if (it->id == id) {
            m_themes.erase(it);
            break;
        }
    }
    if (wasCurrent) {
        applyTheme(QStringLiteral("ocean-dark"));
    }
    return true;
}

} // namespace fasthub::ui

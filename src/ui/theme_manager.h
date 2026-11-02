#pragma once

#include <QObject>
#include <QString>
#include <QHash>
#include <vector>

namespace fasthub::ui {

/// @brief Single visual theme definition.
struct ThemeInfo {
    QString id;   ///< Stable identifier (e.g. "ocean-dark").
    QString name; ///< Human-readable display name.
    bool isDark = true;
    /// QSS color roles: bg, panel, text, muted, accent, accentHover, border.
    QHash<QString, QString> colors;

    QString styleSheet() const;
};

/// @brief Central theme registry and applier.
///
/// Provides 50 built-in themes (25 base palettes x {dark, light}) plus
/// user-defined themes stored as JSON in AppData/themes. The selected theme
/// is persisted via DatabaseManager settings ("theme.id") and applied as an
/// application-wide stylesheet. Pure presentation logic: no business rules.
class ThemeManager : public QObject {
    Q_OBJECT
public:
    static ThemeManager &instance();

    /// @brief Loads custom themes from disk and the saved selection from DB.
    void initialize();

    /// @brief All themes available for display (built-in + custom).
    const std::vector<ThemeInfo> &themes() const { return m_themes; }

    /// @brief Applies a theme by id (persists the choice).
    /// @return false if no such theme exists.
    bool applyTheme(const QString &id);

    /// @brief Currently applied theme id ("" = system default, no QSS).
    QString currentThemeId() const { return m_currentId; }

    /// @brief Adds a user theme and persists it to disk as JSON.
    bool saveCustomTheme(const ThemeInfo &info);

    /// @brief Deletes a user-defined theme by id (built-ins are protected).
    bool removeCustomTheme(const QString &id);

signals:
    void themeApplied(const QString &id);

private:
    ThemeManager() = default;

    void buildBuiltinThemes();
    void loadCustomThemes();
    static bool loadThemeFromJson(const QString &path, ThemeInfo *out);
    static bool writeThemeToJson(const QString &path, const ThemeInfo &info);
    QString customThemesDir() const;

    std::vector<ThemeInfo> m_themes;
    QString m_currentId;
    qint64 m_builtinCount = 0;
};

} // namespace fasthub::ui

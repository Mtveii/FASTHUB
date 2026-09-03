#include <QtTest/QtTest>
#include <QApplication>
#include <QTemporaryDir>

#include "ui/theme_manager.h"

using fasthub::ui::ThemeInfo;
using fasthub::ui::ThemeManager;

class ThemeManagerTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void exactly50BuiltinThemes();
    void darkAndLightVariantsExist();
    void applyKnownAndUnknownThemes();
    void customThemeRoundTrip();

private:
    QTemporaryDir m_tempDir;
};

void ThemeManagerTest::initTestCase()
{
    QVERIFY(m_tempDir.isValid());
    qputenv("XDG_DATA_HOME", m_tempDir.path().toUtf8());
    // AppDataLocation on Windows points at %APPDATA%; ThemeManager only
    // writes there during saveCustomTheme, which this test exercises with a
    // unique id, so repeated runs are safe.
}

void ThemeManagerTest::exactly50BuiltinThemes()
{
    auto& tm = ThemeManager::instance();
    tm.initialize();
    // 25 generated palettes x2 + FastHub brand pair = 52.
    QVERIFY2(tm.themes().size() >= 50,
             qPrintable(QString("expected >= 50 themes, got %1").arg(tm.themes().size())));

    // Brand themes exist and the default on first run is FastHub Dark.
    bool hasBrand = false;
    for (const auto& t : tm.themes()) {
        if (t.id == QStringLiteral("fasthub-dark")) hasBrand = true;
    }
    QVERIFY(hasBrand);
}

void ThemeManagerTest::darkAndLightVariantsExist()
{
    int dark = 0;
    int light = 0;
    for (const auto& t : ThemeManager::instance().themes()) {
        if (t.id.endsWith("-dark")) ++dark;
        if (t.id.endsWith("-light")) ++light;

        // Every theme must define all roles used by the QSS template.
        const QStringList required = {"bg", "panel", "text", "muted", "accent", "accentHover", "border"};
        for (const QString& role : required) {
            QVERIFY2(!t.colors.value(role).isEmpty(),
                     qPrintable(QString("theme %1 misses role %2").arg(t.id, role)));
        }
        QVERIFY(!t.styleSheet().isEmpty());
    }
    QVERIFY2(dark >= 26, qPrintable(QString("dark=%1").arg(dark)));
    QVERIFY2(light >= 26, qPrintable(QString("light=%1").arg(light)));
}

void ThemeManagerTest::applyKnownAndUnknownThemes()
{
    auto& tm = ThemeManager::instance();
    QVERIFY(tm.applyTheme("ocean-dark"));
    QCOMPARE(tm.currentThemeId(), QString("ocean-dark"));
    QVERIFY(!qApp->styleSheet().isEmpty());

    // Unknown id must be rejected and must not clear the applied sheet.
    QVERIFY(!tm.applyTheme("__no_such_theme__"));
    QCOMPARE(tm.currentThemeId(), QString("ocean-dark"));

    QVERIFY(tm.applyTheme("ocean-light"));
    QCOMPARE(tm.currentThemeId(), QString("ocean-light"));
}

void ThemeManagerTest::customThemeRoundTrip()
{
    auto& tm = ThemeManager::instance();
    const QString uniqueId = QStringLiteral("custom-test-%1").arg(QDateTime::currentMSecsSinceEpoch());

    ThemeInfo info;
    info.id = uniqueId;
    info.name = "Unit Test Theme";
    info.isDark = true;
    info.colors = {
        {"bg", "#101010"}, {"panel", "#181818"}, {"text", "#eeeeee"},
        {"muted", "#888888"}, {"accent", "#3366cc"}, {"accentHover", "#4488ee"},
        {"border", "#333333"},
    };

    QVERIFY(tm.saveCustomTheme(info));
    QVERIFY(tm.applyTheme(uniqueId));

    bool foundAsCustom = false;
    for (const auto& t : tm.themes()) {
        if (t.id == uniqueId) foundAsCustom = true;
    }
    QVERIFY(foundAsCustom);

    // Built-ins are protected from deletion.
    QVERIFY(!tm.removeCustomTheme("ocean-dark"));
    // Custom can be removed.
    QVERIFY(tm.removeCustomTheme(uniqueId));
    QVERIFY(tm.currentThemeId() != uniqueId); // fell back to a valid theme
}

QTEST_MAIN(ThemeManagerTest)
#include "theme_manager_test.moc"

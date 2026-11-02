#include <QtTest>
#include <QToolButton>
#include <QButtonGroup>
#include <QSignalSpy>
#include "ui/workspace/top_bar.h"
#include "ui/workspace/icon_rail.h"
#include "ui/workspace/file_list_toolbar.h"

class UiLayoutTest : public QObject {
    Q_OBJECT

private slots:
    void topBarLayoutAndState();
    void iconRailLayoutAndState();
    void fileListToolbarLayoutAndState();
};

void UiLayoutTest::topBarLayoutAndState() {
    TopBar bar;
    bar.show();
    QVERIFY(QTest::qWaitForWindowExposed(&bar));

    auto *toggle = bar.findChild<QToolButton *>("sidebarToggle");
    auto *files = bar.findChild<QToolButton *>("filesTab");
    auto *search = bar.findChild<QToolButton *>("searchTab");
    auto *marks = bar.findChild<QToolButton *>("bookmarksTab");

    QVERIFY(toggle);
    QVERIFY(files);
    QVERIFY(search);
    QVERIFY(marks);

    // 1.1 First element
    QCOMPARE(bar.layout()->indexOf(toggle), 0);

    // 1.3 Left to right order
    QVERIFY(toggle->geometry().x() < files->geometry().x());
    QVERIFY(files->geometry().x() < search->geometry().x());
    QVERIFY(search->geometry().x() < marks->geometry().x());

    // 1.6 Default checked state
    QVERIFY(files->isChecked());
    QVERIFY(!search->isChecked());
    QVERIFY(!marks->isChecked());

    // 1.7 Mutually exclusive tabs
    search->click();
    QVERIFY(!files->isChecked());
    QVERIFY(search->isChecked());
    QVERIFY(!marks->isChecked());

    // 1.8 Toggle not in exclusive group
    toggle->click(); // should not uncheck search
    QVERIFY(search->isChecked());

    // 1.10 Icon size
    QCOMPARE(files->iconSize(), QSize(20, 20));
}

void UiLayoutTest::iconRailLayoutAndState() {
    IconRail rail;
    rail.show();
    QVERIFY(QTest::qWaitForWindowExposed(&rail));

    auto *files = rail.findChild<QToolButton *>("railFiles");
    auto *graph = rail.findChild<QToolButton *>("railGraph");
    auto *tags = rail.findChild<QToolButton *>("railTags");

    QVERIFY(files);
    QVERIFY(graph);
    QVERIFY(tags);

    // 2.1 Order top to bottom
    QCOMPARE(rail.layout()->indexOf(files), 0);
    QVERIFY(files->geometry().y() < graph->geometry().y());

    // 2.2 Same vertical X axis
    QCOMPARE(files->geometry().x(), graph->geometry().x());
    QCOMPARE(files->geometry().x(), tags->geometry().x());

    // 2.4 Exclusive selection test via signal spy
    QSignalSpy spy(&rail, &IconRail::sectionRequested);
    graph->click();
    QVERIFY(graph->isChecked());
    QVERIFY(!files->isChecked());
    QCOMPARE(spy.count(), 1);
}

void UiLayoutTest::fileListToolbarLayoutAndState() {
    FileListToolbar toolbar;
    toolbar.show();
    QVERIFY(QTest::qWaitForWindowExposed(&toolbar));

    auto *newNote = toolbar.findChild<QToolButton *>("newNote");
    auto *close = toolbar.findChild<QToolButton *>("closeBtn");

    QVERIFY(newNote);
    QVERIFY(close);

    // 3.4 Icon size 16x16
    QCOMPARE(newNote->iconSize(), QSize(16, 16));

    // 3.5 Signal test
    QSignalSpy spy(&toolbar, &FileListToolbar::closeRequested);
    close->click();
    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(UiLayoutTest)
#include "ui_layout_test.moc"

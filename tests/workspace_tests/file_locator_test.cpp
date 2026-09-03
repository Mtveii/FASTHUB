#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>

#include "workspace/file_locator.h"

// MSVC rejects `using fasthub::workspace::FileLocator;` for namespaces.
namespace FileLocator = fasthub::workspace::FileLocator;

namespace {

bool writeFileRaw(const QString& path, const QByteArray& content) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return false;
    return f.write(content) == content.size();
}

} // namespace

class FileLocatorTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void findsExactNameRecursively();
    void fallsBackToBaseNameAndMd();
    void rejectsPathsAndMissingRoots();
    void createIfMissingCreatesMd();

private:
    QTemporaryDir m_root;
};

void FileLocatorTest::initTestCase()
{
    QVERIFY(m_root.isValid());
    QVERIFY(QDir(m_root.path()).mkpath("projects/nested"));
    QVERIFY(writeFileRaw(m_root.filePath("top.md"), "# Top"));
    QVERIFY(writeFileRaw(m_root.filePath("projects/deep.html"), "<p>hi</p>"));
    QVERIFY(writeFileRaw(m_root.filePath("projects/nested/idea.md"), "idea body"));
    QVERIFY(writeFileRaw(m_root.filePath("plain.txt"), "plain"));
}

void FileLocatorTest::findsExactNameRecursively()
{
    // Exact full name anywhere under the root (nested dirs included).
    QCOMPARE(FileLocator::findFileByName(m_root.path(), "idea.md"),
             m_root.filePath("projects/nested/idea.md"));
    // Case-insensitive.
    QCOMPARE(FileLocator::findFileByName(m_root.path(), "IDEA.MD"),
             m_root.filePath("projects/nested/idea.md"));
}

void FileLocatorTest::fallsBackToBaseNameAndMd()
{
    // Obsidian-style: [[deep]] -> deep.html by complete base name match.
    QCOMPARE(FileLocator::findFileByName(m_root.path(), "deep"),
             m_root.filePath("projects/deep.html"));
    // name -> name + ".md" fallback when nothing matches better.
    QCOMPARE(FileLocator::findFileByName(m_root.path(), "top"),
             m_root.filePath("top.md"));
    // Unknown note returns empty.
    QVERIFY(FileLocator::findFileByName(m_root.path(), "no-such-note").isEmpty());
}

void FileLocatorTest::rejectsPathsAndMissingRoots()
{
    // Wiki targets are plain names: path separators must be rejected.
    QVERIFY(FileLocator::findFileByName(m_root.path(), "../outside").isEmpty());
    QVERIFY(FileLocator::findFileByName(m_root.path(), "sub/dir.md").isEmpty());
    QVERIFY(FileLocator::findFileByName("", "x.md").isEmpty());
    QVERIFY(FileLocator::findFileByName("Z:/missing/root", "x.md").isEmpty());
}

void FileLocatorTest::createIfMissingCreatesMd()
{
    // Existing notes resolve without creation.
    QCOMPARE(FileLocator::createIfMissing(m_root.path(), "top.md"),
             m_root.filePath("top.md"));

    // Missing note is created as .md in the workspace root.
    const QString created = FileLocator::createIfMissing(m_root.path(), "brand new");
    QCOMPARE(created, m_root.filePath("brand new.md"));
    QVERIFY(QFile::exists(created));

    // Traversal attempts cannot escape the root.
    const QString sneaky = FileLocator::createIfMissing(m_root.path(), "../../escape");
    QVERIFY(!sneaky.isEmpty());
    // The created file must live INSIDE the workspace root directory.
    QCOMPARE(QFileInfo(sneaky).absolutePath(), QDir(m_root.path()).absolutePath());
    QFile::remove(sneaky);
}

QTEST_MAIN(FileLocatorTest)
#include "file_locator_test.moc"

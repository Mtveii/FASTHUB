#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>

#include "workspace/local_file_provider.h"

using fasthub::workspace::LocalFileProvider;

namespace {

bool writeFileRaw(const QString &path, const QByteArray &content) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return false;
    return f.write(content) == content.size();
}

} // namespace

class LocalFileProviderTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void rootIsRequiredBeforeUse();
    void listDirectoryInsideRoot();
    void readWriteRoundTrip();
    void pathTraversalIsRejected();
    void deleteProtectsRoot();
    void renameStaysInsideRoot();

private:
    QTemporaryDir m_root;
};

void LocalFileProviderTest::initTestCase() {
    QVERIFY(m_root.isValid());
    QVERIFY(QDir(m_root.path()).mkpath("sub"));
    QVERIFY(writeFileRaw(m_root.filePath("note.md"), "# Hello"));
    QVERIFY(writeFileRaw(m_root.filePath("sub/deep.txt"), "deep content"));
}

void LocalFileProviderTest::rootIsRequiredBeforeUse() {
    LocalFileProvider locked;
    QVERIFY(!locked.readFile(m_root.filePath("note.md").toStdString()).has_value());
    QVERIFY(locked.listDirectory(m_root.path().toStdString()).empty());
    QVERIFY(!locked.writeFile(m_root.filePath("x.txt").toStdString(), "nope"));

    QVERIFY(locked.setRootPath(m_root.path().toStdString()));
    QVERIFY(!locked.setRootPath("Z:/definitely/missing/dir")); // invalid root rejected
}

void LocalFileProviderTest::listDirectoryInsideRoot() {
    LocalFileProvider p;
    QVERIFY(p.setRootPath(m_root.path().toStdString()));

    const auto entries = p.listDirectory(m_root.path().toStdString());
    QCOMPARE(static_cast<int>(entries.size()), 2); // note.md + sub

    bool sawDir = false;
    for (const auto &e : entries) {
        if (e.name == "sub") {
            sawDir = e.isDirectory;
        }
    }
    QVERIFY(sawDir);
}

void LocalFileProviderTest::readWriteRoundTrip() {
    LocalFileProvider p;
    QVERIFY(p.setRootPath(m_root.path().toStdString()));

    const std::string target = m_root.filePath("roundtrip.txt").toStdString();
    QVERIFY(p.createFile(target, false));
    QVERIFY(p.writeFile(target, "line1\nline2\n"));
    const auto content = p.readFile(target);
    QVERIFY(content.has_value());
    QCOMPARE(QString::fromStdString(*content), QString("line1\nline2\n"));
}

void LocalFileProviderTest::pathTraversalIsRejected() {
    LocalFileProvider p;
    QVERIFY(p.setRootPath(m_root.path().toStdString()));

    const std::string outside =
        QDir::toNativeSeparators(m_root.filePath("../outside.txt")).toStdString();
    QVERIFY(!p.isInsideRoot(outside));
    QVERIFY(!p.writeFile(outside, "evil"));
    QVERIFY(!p.readFile(outside).has_value());
    QVERIFY(!p.deleteFile(outside));
    QVERIFY(!p.createFile(outside, true));

    const std::string traversal =
        m_root.filePath("..\\..\\windows\\system32\\evil.txt").toStdString();
    QVERIFY(!p.isInsideRoot(traversal));
    QVERIFY(!p.writeFile(traversal, "evil"));

    // Absolute path to a location outside the root is also rejected.
    QVERIFY(!p.isInsideRoot(std::string("C:\\Windows\\notepad.exe")));
}

void LocalFileProviderTest::deleteProtectsRoot() {
    LocalFileProvider p;
    QVERIFY(p.setRootPath(m_root.path().toStdString()));

    QVERIFY(p.deleteFile(m_root.filePath("roundtrip.txt").toStdString()));
    QVERIFY(!QFile::exists(m_root.filePath("roundtrip.txt")));

    // Deleting the workspace root itself must be refused.
    QVERIFY(!p.deleteFile(m_root.path().toStdString()));
    QVERIFY(QDir(m_root.path()).exists());
}

void LocalFileProviderTest::renameStaysInsideRoot() {
    LocalFileProvider p;
    QVERIFY(p.setRootPath(m_root.path().toStdString()));

    QVERIFY(p.renameFile(m_root.filePath("sub/deep.txt").toStdString(),
                         m_root.filePath("sub/renamed.txt").toStdString()));
    QVERIFY(QFile::exists(m_root.filePath("sub/renamed.txt")));

    QVERIFY(!p.renameFile(m_root.filePath("sub/renamed.txt").toStdString(),
                          m_root.filePath("../../renamed_outside.txt").toStdString()));
}

QTEST_MAIN(LocalFileProviderTest)
#include "local_file_provider_test.moc"

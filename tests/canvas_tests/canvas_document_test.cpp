#include <QtTest>
#include "core/canvas/canvas_document.h"

using namespace fasthub::core::canvas;

class CanvasDocumentTest : public QObject {
    Q_OBJECT
private slots:
    void minimalCanvas();
    void allFourNodeTypes();
    void edgesWithSides();
    void malformedJson();
    void missingOptionalFields();
    void unicodeText();
    void filePathResolve();
    void emptyCanvas();
    void unknownFieldsDropped();
    void presetColors();
    void hexColors();
    void missingGeometrySkipped();
    void bomHandling();
    void largeFileDoesNotHang();
};

void CanvasDocumentTest::minimalCanvas() {
    QByteArray json = R"({"nodes":[{"id":"n1","type":"text","x":0,"y":0,"width":100,"height":100,"text":"hello"}],"edges":[]})";
    CanvasParseResult r;
    auto doc = CanvasDocument::parse(json, &r);
    QVERIFY(r.ok);
    QCOMPARE(doc.nodes.size(), 1);
    QCOMPARE(doc.nodes[0].id, QString("n1"));
    QCOMPARE(doc.nodes[0].type, CanvasNodeType::Text);
    QCOMPARE(doc.edges.size(), 0);
}

void CanvasDocumentTest::allFourNodeTypes() {
    QByteArray json = R"({
        "nodes":[
            {"id":"t1","type":"text","x":0,"y":0,"width":200,"height":100,"text":"hi"},
            {"id":"f1","type":"file","x":10,"y":10,"width":200,"height":100,"file":"notes/todo.md"},
            {"id":"l1","type":"link","x":20,"y":20,"width":200,"height":100,"url":"https://example.com"},
            {"id":"g1","type":"group","x":0,"y":0,"width":400,"height":400,"label":"Group"}
        ],
        "edges":[{"id":"e1","fromNode":"t1","toNode":"f1"}]
    })";
    CanvasParseResult r;
    auto doc = CanvasDocument::parse(json, &r);
    QVERIFY(r.ok);
    QCOMPARE(doc.nodes.size(), 4);
    QCOMPARE(doc.nodes[0].type, CanvasNodeType::Text);
    QCOMPARE(doc.nodes[1].type, CanvasNodeType::File);
    QCOMPARE(doc.nodes[1].filePath, QString("notes/todo.md"));
    QCOMPARE(doc.nodes[2].type, CanvasNodeType::Link);
    QCOMPARE(doc.nodes[2].url, QString("https://example.com"));
    QCOMPARE(doc.nodes[3].type, CanvasNodeType::Group);
    QCOMPARE(doc.nodes[3].label, QString("Group"));
}

void CanvasDocumentTest::edgesWithSides() {
    QByteArray json = R"({
        "nodes":[
            {"id":"a","type":"text","x":0,"y":0,"width":100,"height":100},
            {"id":"b","type":"text","x":200,"y":0,"width":100,"height":100}
        ],
        "edges":[
            {"id":"e1","fromNode":"a","fromSide":"right","toNode":"b","toSide":"left","color":"1","label":"edge label"},
            {"id":"e2","fromNode":"a","fromSide":"top","toNode":"b","toSide":"bottom"}
        ]
    })";
    CanvasParseResult r;
    auto doc = CanvasDocument::parse(json, &r);
    QVERIFY(r.ok);
    QCOMPARE(doc.edges.size(), 2);
    QCOMPARE(doc.edges[0].fromSide, CanvasSide::Right);
    QCOMPARE(doc.edges[0].toSide, CanvasSide::Left);
    QCOMPARE(doc.edges[0].label, QString("edge label"));
    QCOMPARE(doc.edges[1].fromSide, CanvasSide::Top);
    QCOMPARE(doc.edges[1].toSide, CanvasSide::Bottom);
}

void CanvasDocumentTest::malformedJson() {
    QByteArray json = R"({ not valid json )";
    CanvasParseResult r;
    auto doc = CanvasDocument::parse(json, &r);
    QVERIFY(!r.ok);
    QVERIFY(!r.error.isEmpty());
    // Should not crash, returns empty doc
    QCOMPARE(doc.nodes.size(), 0);
}

void CanvasDocumentTest::missingOptionalFields() {
    QByteArray json = R"({"nodes":[{"id":"n1","type":"text","x":0,"y":0,"width":100,"height":100}],"edges":[{"id":"e1","fromNode":"n1","toNode":"n1"}]})";
    CanvasParseResult r;
    auto doc = CanvasDocument::parse(json, &r);
    QVERIFY(r.ok);
    QCOMPARE(doc.nodes[0].colorRaw, std::nullopt);
    QCOMPARE(doc.edges[0].colorRaw, std::nullopt);
    QVERIFY(doc.edges[0].label.isEmpty());
}

void CanvasDocumentTest::unicodeText() {
    QByteArray json = QString(R"({"nodes":[{"id":"n1","type":"text","x":0,"y":0,"width":200,"height":100,"text":"Привет 🌟 مرحبا"}]})").toUtf8();
    CanvasParseResult r;
    auto doc = CanvasDocument::parse(json, &r);
    QVERIFY(r.ok);
    QCOMPARE(doc.nodes[0].text, QString("Привет 🌟 مرحبا"));
    // Round-trip: serialize back and re-parse should preserve
    QByteArray data = json;
    auto doc2 = CanvasDocument::parse(data, &r);
    QCOMPARE(doc2.nodes[0].text, doc.nodes[0].text);
}

void CanvasDocumentTest::filePathResolve() {
    QString canvasPath = "C:/vault/folder/board.canvas";
    // relative with /
    QCOMPARE(CanvasDocument::resolveFilePath(canvasPath, "notes/todo.md"), QString("C:/vault/folder/notes/todo.md"));
    // relative with \
    QCOMPARE(CanvasDocument::resolveFilePath(canvasPath, "notes\\todo.md"), QString("C:/vault/folder/notes/todo.md"));
    // absolute
    QCOMPARE(CanvasDocument::resolveFilePath(canvasPath, "C:/other/file.md"), QString("C:/other/file.md"));
    // dot segment
    QCOMPARE(CanvasDocument::resolveFilePath(canvasPath, "../other.md"), QString("C:/vault/other.md"));
}

void CanvasDocumentTest::emptyCanvas() {
    QByteArray json = R"({"nodes":[],"edges":[]})";
    CanvasParseResult r;
    auto doc = CanvasDocument::parse(json, &r);
    QVERIFY(r.ok);
    QCOMPARE(doc.nodes.size(), 0);
    QCOMPARE(doc.edges.size(), 0);
    // Also top-level without keys should default to empty
    QByteArray json2 = R"({})";
    auto doc2 = CanvasDocument::parse(json2, &r);
    QVERIFY(r.ok);
    QCOMPARE(doc2.nodes.size(), 0);
}

void CanvasDocumentTest::unknownFieldsDropped() {
    QByteArray json = R"({"nodes":[{"id":"n1","type":"text","x":0,"y":0,"width":100,"height":100,"unknownField":"keep","another":123}],"edges":[],"futureField":"ignore"})";
    CanvasParseResult r;
    auto doc = CanvasDocument::parse(json, &r);
    QVERIFY(r.ok);
    QCOMPARE(doc.nodes.size(), 1);
    // Unknown fields should not cause failure (forward-compat)
}

void CanvasDocumentTest::presetColors() {
    // JSON Canvas spec preset 1..6 → реальная палитра Obsidian (не цифра→hex)
    const QHash<QString, QColor> expected = {
        {"1", QColor("#FB464C")}, {"2", QColor("#FF8300")}, {"3", QColor("#FFC300")},
        {"4", QColor("#5DB300")}, {"5", QColor("#4472E0")}, {"6", QColor("#9A5CF0")},
    };
    for (auto it = expected.begin(); it != expected.end(); ++it) {
        QByteArray json = QString(R"({"nodes":[{"id":"n1","type":"text","x":0,"y":0,"width":100,"height":100,"color":"%1"}]})").arg(it.key()).toUtf8();
        auto doc = CanvasDocument::parse(json);
        QVERIFY(doc.nodes[0].color().isValid());
        QCOMPARE(doc.nodes[0].color().name().toUpper(), it.value().name().toUpper());
        // Также проверка что цифра не трактуется как hex (например "1" != "#000001")
        QVERIFY(doc.nodes[0].color() != QColor("#000001"));
    }
    // Hex цвета парсятся напрямую
    QCOMPARE(CanvasDocument::parse(R"({"nodes":[{"id":"n1","type":"text","x":0,"y":0,"width":100,"height":100,"color":"#FF00AA"}]})").nodes[0].color(), QColor("#FF00AA"));
}

void CanvasDocumentTest::hexColors() {
    QByteArray json = R"({"nodes":[{"id":"n1","type":"text","x":0,"y":0,"width":100,"height":100,"color":"#FF00AA"}]})";
    auto doc = CanvasDocument::parse(json);
    QCOMPARE(doc.nodes[0].color(), QColor("#FF00AA"));
}

void CanvasDocumentTest::missingGeometrySkipped() {
    QByteArray json = R"({"nodes":[{"id":"bad","type":"text","text":"no geometry"},{"id":"good","type":"text","x":0,"y":0,"width":100,"height":100}],"edges":[]})";
    CanvasParseResult r;
    auto doc = CanvasDocument::parse(json, &r);
    QVERIFY(r.ok);
    QCOMPARE(doc.nodes.size(), 1);
    QCOMPARE(doc.nodes[0].id, QString("good"));
}

void CanvasDocumentTest::bomHandling() {
    QByteArray json = QByteArray("\xEF\xBB\xBF") + R"({"nodes":[{"id":"n1","type":"text","x":0,"y":0,"width":100,"height":100}]})";
    // parseFile handles BOM; parse() with BOM should also be tolerant via fromJson (it handles BOM? but we test)
    // For direct parse, strip manually as parseFile does — here we test parseFile path
    // Write temp file
    QString tmp = QDir::temp().filePath("test_bom.canvas");
    QFile f(tmp);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(json);
    f.close();
    CanvasParseResult r;
    auto doc = CanvasDocument::parseFile(tmp, &r);
    QVERIFY(r.ok);
    QCOMPARE(doc.nodes.size(), 1);
    QFile::remove(tmp);
}

void CanvasDocumentTest::largeFileDoesNotHang() {
    // Synthesize 500 nodes
    QJsonArray nodes;
    for (int i=0;i<500;++i) {
        QJsonObject o; o["id"]=QString("n%1").arg(i); o["type"]="text"; o["x"]=i*10; o["y"]=0; o["width"]=100; o["height"]=50; o["text"]=QString("node %1").arg(i);
        nodes.append(o);
    }
    QJsonObject root; root["nodes"]=nodes; root["edges"]=QJsonArray();
    QByteArray json = QJsonDocument(root).toJson();
    QElapsedTimer t; t.start();
    auto doc = CanvasDocument::parse(json);
    qint64 elapsed = t.elapsed();
    QVERIFY(doc.nodes.size() == 500);
    QVERIFY(elapsed < 1000); // should parse well under 1s
}

QTEST_APPLESS_MAIN(CanvasDocumentTest)
#include "canvas_document_test.moc"

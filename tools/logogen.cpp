#include <QGuiApplication>
#include <QSvgRenderer>
#include <QImage>
#include <QPainter>
#include <QFile>
#include <QBuffer>
#include <QDataStream>
#include <QDir>

#include <iostream>

/// @brief One-shot brand asset generator.
///
/// Renders assets/icons/fasthub-logo.svg into:
///  - <root>/лого.png          (512x512, project root, per product request)
///  - <root>/assets/fasthub.ico (256x256 PNG-compressed ICO for the exe)
int main(int argc, char* argv[]) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app(argc, argv);

    // Static-lib resources must be initialized explicitly in tools that
    // don't pull the initializer through other references.
    Q_INIT_RESOURCE(resources);

    QFile svg(QStringLiteral(":/icons/fasthub-logo.svg"));
    if (!svg.open(QIODevice::ReadOnly)) {
        std::cerr << "logo svg missing\n";
        return 1;
    }
    QSvgRenderer renderer(svg.readAll());
    if (!renderer.isValid()) {
        std::cerr << "logo svg invalid\n";
        return 1;
    }

    auto render = [&renderer](int size) {
        QImage img(size, size, QImage::Format_ARGB32);
        img.fill(Qt::transparent);
        QPainter p(&img);
        p.setRenderHint(QPainter::Antialiasing);
        renderer.render(&p, QRect(0, 0, size, size));
        return img;
    };

    // 1) лого.png in the repository root (build/tools -> build -> repo root).
    const QString root = QDir::cleanPath(QGuiApplication::applicationDirPath() + QStringLiteral("/../..")) + QStringLiteral("/");
    if (!render(512).save(root + QStringLiteral("лого.png"), "PNG")) {
        std::cerr << "failed to write logo png\n";
        return 1;
    }

    // 2) fasthub.ico: ICONDIR + one 256px PNG entry (Vista+ PNG-in-ICO).
    QImage icon256 = render(256);
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    if (!icon256.save(&buffer, "PNG")) {
        std::cerr << "png encode failed\n";
        return 1;
    }

    QFile ico(root + QStringLiteral("assets/fasthub.ico"));
    if (!ico.open(QIODevice::WriteOnly)) {
        std::cerr << "failed to open ico: " << ico.errorString().toStdString()
                  << " (path: " << ico.fileName().toStdString() << ")\n";
        return 1;
    }
    QDataStream out(&ico);
    out.setByteOrder(QDataStream::LittleEndian);
    out << quint16(0) << quint16(1) << quint16(1);           // ICONDIR
    out << quint8(0) << quint8(0)                            // 256 => 0
        << quint8(0) << quint8(0)
        << quint16(1) << quint16(32)
        << quint32(static_cast<quint32>(png.size()))
        << quint32(6 + 16);                                   // offset
    ico.write(png);
    ico.close();

    std::cout << "logo assets generated\n";
    return 0;
}

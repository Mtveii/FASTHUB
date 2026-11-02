#include <QtTest/QtTest>
#include <QImage>

#include "ui/qr_login_dialog.h"

using fasthub::ui::QrLoginDialog;

class QrLoginDialogTest : public QObject {
    Q_OBJECT

private slots:
    void generatesValidMatrixForUrls();
    void rejectsGarbagePayloads();
    void deterministicOutput();

private:
    static QImage pixmapToImage(const QPixmap &pm) {
        return pm.toImage().convertToFormat(QImage::Format_Grayscale8);
    }
};

void QrLoginDialogTest::generatesValidMatrixForUrls() {
    const auto urls = {
        "https://web.telegram.org",
        "https://discord.com/login",
        "https://store.steampowered.com/login/",
    };

    for (const QString &url : urls) {
        const QPixmap pm = QrLoginDialog::makeQrPixmap(url, 240);
        QVERIFY2(!pm.isNull(), qPrintable(QString("QR generation failed for %1").arg(url)));
        QCOMPARE(pm.width(), 240);

        // A rendered QR must contain both black and white pixels.
        const QImage img = pixmapToImage(pm);
        int black = 0;
        int white = 0;
        for (int y = 0; y < img.height(); ++y) {
            for (int x = 0; x < img.width(); ++x) {
                (qGray(img.pixel(x, y)) < 128 ? black : white)++;
            }
        }
        QVERIFY2(black > 100, qPrintable(QString("too few black modules for %1").arg(url)));
        QVERIFY2(white > 100, qPrintable(QString("too few white modules for %1").arg(url)));
    }
}

void QrLoginDialogTest::rejectsGarbagePayloads() {
    // Empty payload must not crash and should yield a null pixmap.
    QVERIFY(QrLoginDialog::makeQrPixmap("", 120).isNull());
}

void QrLoginDialogTest::deterministicOutput() {
    const QPixmap a = QrLoginDialog::makeQrPixmap("https://example.com", 210);
    const QPixmap b = QrLoginDialog::makeQrPixmap("https://example.com", 210);
    QCOMPARE(pixmapToImage(a), pixmapToImage(b));
}

QTEST_MAIN(QrLoginDialogTest)
#include "qr_login_dialog_test.moc"

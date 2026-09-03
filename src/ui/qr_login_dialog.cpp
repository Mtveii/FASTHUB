#include "qr_login_dialog.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPainter>
#include <QClipboard>
#include <QApplication>

#include "qrcodegen.hpp"

namespace fasthub::ui {

QPixmap QrLoginDialog::makeQrPixmap(const QString& text, int sizePx) {
    if (text.isEmpty()) {
        return QPixmap();
    }

    // 4-module quiet zone per QR specification.
    const int border = 4;
    QImage image;

    try {
        const qrcodegen::QrCode qr =
            qrcodegen::QrCode::encodeText(text.toUtf8().constData(), qrcodegen::QrCode::Ecc::MEDIUM);
        image = QImage(qr.getSize() + 2 * border, qr.getSize() + 2 * border, QImage::Format_ARGB32);
        image.fill(Qt::white);
        for (int y = 0; y < qr.getSize(); ++y) {
            for (int x = 0; x < qr.getSize(); ++x) {
                if (qr.getModule(x, y)) {
                    image.setPixel(x + border, y + border, Qt::black);
                }
            }
        }
    } catch (const std::exception&) {
        return QPixmap(); // encoding error
    }

    return QPixmap::fromImage(image.scaled(sizePx, sizePx, Qt::KeepAspectRatio, Qt::FastTransformation));
}

QrLoginDialog::QrLoginDialog(const QString& title, const QString& loginUrl, const QString& note, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(title);
    setModal(false);
    setupUi(note, makeQrPixmap(loginUrl, 240));
}

void QrLoginDialog::setupUi(const QString& note, const QPixmap& qr) {
    QVBoxLayout* layout = new QVBoxLayout(this);

    if (!qr.isNull()) {
        QLabel* qrLabel = new QLabel(this);
        qrLabel->setPixmap(qr);
        qrLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(qrLabel);
    } else {
        QLabel* errorLabel = new QLabel("Failed to generate QR code.", this);
        errorLabel->setStyleSheet("color: #e05555;");
        layout->addWidget(errorLabel);
    }

    QLabel* noteLabel = new QLabel(note, this);
    noteLabel->setWordWrap(true);
    noteLabel->setProperty("role", "muted");
    layout->addWidget(noteLabel);

    QHBoxLayout* buttons = new QHBoxLayout();
    QPushButton* copyButton = new QPushButton("Copy link", this);
    connect(copyButton, &QPushButton::clicked, this, [note]() {
        QApplication::clipboard()->setText(note.section('\n', -1).trimmed());
    });
    buttons->addWidget(copyButton);
    buttons->addStretch();
    QPushButton* closeButton = new QPushButton("Close", this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttons->addWidget(closeButton);
    layout->addLayout(buttons);
}

} // namespace fasthub::ui

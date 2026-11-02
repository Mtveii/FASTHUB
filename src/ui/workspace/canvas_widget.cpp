#include "canvas_widget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QResizeEvent>
#include <QShowEvent>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QMessageBox>
#include <QDateTime>

#include "canvas_scene.h"
#include "canvas_view.h"
#include "../icon_util.h"
#include "../theme_manager.h"

namespace fasthub::ui {

CanvasWidget::CanvasWidget(QWidget *parent) : QWidget(parent) { setupUi(); }

CanvasWidget::CanvasWidget(const QString &filePath, QWidget *parent)
    : QWidget(parent), m_filePath(filePath) {
    setupUi();
    loadFromFile(filePath);
}

bool CanvasWidget::loadFromFile(const QString &filePath) {
    m_filePath = filePath;
    core::canvas::CanvasParseResult res;
    auto doc = core::canvas::CanvasDocument::parseFile(filePath, &res);
    if (!res.ok) {
        // Non-crash: show user-friendly error, keep empty canvas
        if (m_hintLabel) {
            m_hintLabel->setText(QString("Ошибка загрузки canvas:\n%1").arg(res.error));
            m_hintLabel->adjustSize();
            m_hintLabel->show();
            m_hintLabel->raise();
        }
        // Still show empty scene so toolbars remain usable
        if (m_scene)
            m_scene->clearCanvas();
        // Distinguish malformed vs empty: don't treat empty as error if file was empty nodes
        // If error is "Empty canvas JSON" and file contains {"nodes":[],"edges":[]} it's not empty
        // — parse would succeed So only show message box for truly malformed JSON
        if (!res.error.isEmpty() && res.error.contains("JSON parse error")) {
            QMessageBox::warning(this, "Canvas", res.error);
        }
        m_document = doc;
        updateWidgetPositions();
        return false;
    }
    m_document = doc;
    if (m_scene) {
        m_scene->setDocument(doc, filePath);
        // Center view on content
        if (m_view)
            m_view->centerOn(m_scene->itemsBoundingRect().center());
    }
    if (m_hintLabel) {
        if (!doc.nodes.isEmpty())
            m_hintLabel->hide();
        else {
            m_hintLabel->show();
            m_hintLabel->raise();
        }
    }
    if (m_backlinksLabel) {
        m_backlinksLabel->setText(QString("Обратных ссылок: %1").arg(doc.edges.size()));
        m_backlinksLabel->adjustSize();
    }
    updateWidgetPositions();
    return true;
}

bool CanvasWidget::saveToFile() const {
    if (m_filePath.isEmpty())
        return false;
    QSaveFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QByteArray json = m_document.toJson();
    if (file.write(json) != json.size())
        return false;
    return file.commit();
}

void CanvasWidget::createCardWidget(const QString &text, int x, int y, int w, int h,
                                    const QString &colorId) {
    // MVP: add a new Text node to the document and refresh scene
    // This keeps model as source of truth (no direct QWidget create)
    core::canvas::CanvasNode n;
    n.id = QString("node_%1").arg(QDateTime::currentMSecsSinceEpoch());
    n.type = core::canvas::CanvasNodeType::Text;
    n.geometry = QRectF(x, y, w, h);
    n.colorRaw = colorId;
    n.text = text;
    m_document.nodes.push_back(n);
    if (m_scene) {
        m_scene->setDocument(m_document, m_filePath);
    }
    if (m_hintLabel)
        m_hintLabel->hide();
    // Persist immediately (atomic) to avoid loss on close/crash; works for iCloud too.
    if (!saveToFile()) {
        qWarning() << "CanvasWidget: failed to auto-save" << m_filePath;
    }
}

void CanvasWidget::setupUi() {
    setAttribute(Qt::WA_StyledBackground, true);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_scene = new CanvasScene(this);
    m_view = new CanvasView(this);
    m_view->setScene(m_scene);
    layout->addWidget(m_view);

    // Hint overlay (center)
    m_hintLabel = new QLabel("Перетащите снизу или дважды щелкните\n"
                             "Пробел + перетаскивание для позиционирования\n"
                             "Ctrl + Прокрутка для масштабирования",
                             this);
    m_hintLabel->setAlignment(Qt::AlignCenter);
    m_hintLabel->setWordWrap(false);
    m_hintLabel->setStyleSheet(
        "QLabel { color: #8A93A6; font-size: 13px; background: rgba(30, 41, 59, 0.85); "
        "padding: 14px 22px; border-radius: 8px; border: 1px solid #334155; }");
    m_hintLabel->adjustSize();
    m_hintLabel->show();
    m_hintLabel->raise();

    // Top-right toolbar
    m_topRightToolbar = new QWidget(this);
    m_topRightToolbar->setObjectName("canvasTopRight");
    auto *trLayout = new QVBoxLayout(m_topRightToolbar);
    trLayout->setContentsMargins(4, 4, 4, 4);
    trLayout->setSpacing(2);
    auto makeToolBtn = [this](const QString &iconName, const QString &fallback,
                              const QString &tip) {
        auto *btn = new QToolButton(m_topRightToolbar);
        btn->setToolTip(tip);
        btn->setFixedSize(28, 28);
        btn->setIconSize(QSize(16, 16));
        if (!iconName.isEmpty())
            btn->setIcon(IconUtil::tintedIcon(iconName, QColor("#E2E8F0"), 16));
        else
            btn->setText(fallback);
        btn->setStyleSheet(
            "QToolButton { background: #1E293B; border: 1px solid #334155; border-radius: 4px; "
            "color: #E2E8F0; font-size: 12px; font-weight: 600; } QToolButton:hover { background: "
            "#334155; } QToolButton:pressed { background: #0F172A; }");
        return btn;
    };
    trLayout->addWidget(makeToolBtn(":/icons/gear.svg", "⚙", "Настройки холста"));
    trLayout->addWidget(makeToolBtn("", "+", "Добавить карточку"));
    trLayout->addWidget(makeToolBtn("", "↻", "Сбросить масштаб"));
    trLayout->addWidget(makeToolBtn("", "⛶", "Показать весь холст"));
    trLayout->addWidget(makeToolBtn("", "−", "Уменьшить"));
    trLayout->addWidget(makeToolBtn("", "+", "Увеличить"));
    trLayout->addWidget(makeToolBtn("", "↩", "Отменить"));
    trLayout->addWidget(makeToolBtn("", "↪", "Повторить"));
    trLayout->addWidget(makeToolBtn("", "?", "Помощь"));
    m_topRightToolbar->adjustSize();
    m_topRightToolbar->setStyleSheet("QWidget#canvasTopRight { background: #0F172A; border: 1px "
                                     "solid #334155; border-radius: 6px; }");
    m_topRightToolbar->show();
    m_topRightToolbar->raise();
    {
        auto btns = m_topRightToolbar->findChildren<QToolButton *>();
        if (!btns.isEmpty()) {
            connect(btns.last(), &QToolButton::clicked, this, [this]() {
                if (m_hintLabel) {
                    m_hintLabel->show();
                    m_hintLabel->raise();
                }
            });
        }
        // Fit-to-view button (4th)
        if (btns.size() >= 4) {
            connect(btns[3], &QToolButton::clicked, this, [this]() {
                if (m_view && m_scene)
                    m_view->centerOn(m_scene->itemsBoundingRect().center());
            });
        }
    }

    // Bottom-center toolbar
    m_bottomCenterToolbar = new QWidget(this);
    m_bottomCenterToolbar->setObjectName("canvasBottomCenter");
    auto *bcLayout = new QHBoxLayout(m_bottomCenterToolbar);
    bcLayout->setContentsMargins(6, 6, 6, 6);
    bcLayout->setSpacing(6);
    auto makeCardBtn = [this](const QString &iconName, const QString &tip) {
        auto *btn = new QToolButton(m_bottomCenterToolbar);
        btn->setToolTip(tip);
        btn->setFixedSize(34, 34);
        btn->setIconSize(QSize(18, 18));
        if (!iconName.isEmpty())
            btn->setIcon(IconUtil::tintedIcon(iconName, QColor("#E2E8F0"), 18));
        btn->setStyleSheet("QToolButton { background: #1E293B; border: 1px solid #334155; "
                           "border-radius: 6px; } QToolButton:hover { background: #334155; }");
        return btn;
    };
    auto *textCardBtn = makeCardBtn(":/icons/file.svg", "Добавить текстовую карточку");
    auto *fileCardBtn = makeCardBtn(":/icons/file-earmark-richtext.svg", "Добавить карточку файла");
    auto *mediaCardBtn =
        makeCardBtn(":/icons/file-earmark-image.svg", "Добавить медиа / изображение");
    bcLayout->addWidget(textCardBtn);
    bcLayout->addWidget(fileCardBtn);
    bcLayout->addWidget(mediaCardBtn);
    m_bottomCenterToolbar->adjustSize();
    m_bottomCenterToolbar->setStyleSheet("QWidget#canvasBottomCenter { background: #0F172A; "
                                         "border: 1px solid #334155; border-radius: 8px; }");
    m_bottomCenterToolbar->show();
    m_bottomCenterToolbar->raise();

    connect(textCardBtn, &QToolButton::clicked, this, [this]() {
        int cx = m_view ? int(m_view->mapToScene(m_view->viewport()->rect().center()).x()) - 90 : 0;
        int cy = m_view ? int(m_view->mapToScene(m_view->viewport()->rect().center()).y()) - 45 : 0;
        createCardWidget("Новая текстовая карточка. Двойной клик для редактирования.", cx, cy, 180,
                         90, "1");
    });
    connect(fileCardBtn, &QToolButton::clicked, this, [this]() {
        int cx = m_view ? int(m_view->mapToScene(m_view->viewport()->rect().center()).x()) - 90 : 0;
        int cy = m_view ? int(m_view->mapToScene(m_view->viewport()->rect().center()).y()) - 45 : 0;
        createCardWidget("Карточка файла — перетащите файл с диска.", cx, cy, 180, 90, "5");
    });
    connect(mediaCardBtn, &QToolButton::clicked, this, [this]() {
        int cx = m_view ? int(m_view->mapToScene(m_view->viewport()->rect().center()).x()) - 90 : 0;
        int cy = m_view ? int(m_view->mapToScene(m_view->viewport()->rect().center()).y()) - 45 : 0;
        createCardWidget("Карточка изображения / медиа.", cx, cy, 180, 90, "3");
    });

    m_backlinksLabel = new QLabel("Обратных ссылок: 0", this);
    m_backlinksLabel->setAlignment(Qt::AlignCenter);
    m_backlinksLabel->setStyleSheet(
        "QLabel { color: #CBD5E1; background: #0F172A; border: 1px solid #334155; border-radius: "
        "6px; padding: 5px 10px; font-size: 11px; }");
    m_backlinksLabel->adjustSize();
    m_backlinksLabel->show();
    m_backlinksLabel->raise();

    updateWidgetPositions();
}

void CanvasWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    updateWidgetPositions();
}

void CanvasWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    updateWidgetPositions();
    if (m_topRightToolbar)
        m_topRightToolbar->raise();
    if (m_bottomCenterToolbar)
        m_bottomCenterToolbar->raise();
    if (m_backlinksLabel)
        m_backlinksLabel->raise();
    if (m_hintLabel)
        m_hintLabel->raise();
}

void CanvasWidget::updateWidgetPositions() {
    const int w = width();
    const int h = height();
    if (w <= 0 || h <= 0)
        return;
    if (m_hintLabel && m_hintLabel->isVisible()) {
        m_hintLabel->adjustSize();
        m_hintLabel->move((w - m_hintLabel->width()) / 2, (h - m_hintLabel->height()) / 2);
    }
    if (m_topRightToolbar) {
        m_topRightToolbar->adjustSize();
        m_topRightToolbar->move(w - m_topRightToolbar->width() - 12, 12);
    }
    if (m_bottomCenterToolbar) {
        m_bottomCenterToolbar->adjustSize();
        m_bottomCenterToolbar->move((w - m_bottomCenterToolbar->width()) / 2,
                                    h - m_bottomCenterToolbar->height() - 18);
    }
    if (m_backlinksLabel) {
        m_backlinksLabel->adjustSize();
        m_backlinksLabel->move(w - m_backlinksLabel->width() - 12,
                               h - m_backlinksLabel->height() - 12);
    }
}

} // namespace fasthub::ui

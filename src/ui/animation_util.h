#pragma once

#include <QWidget>
#include <QVariantAnimation>
#include <QEvent>
#include <QEasingCurve>
#include <QStackedWidget>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include "style_tokens.h"

namespace fasthub::ui {

/// @brief Helper to attach smooth hover background animation to QToolButton / QPushButton.
class SmoothHoverFilter : public QObject {
public:
    explicit SmoothHoverFilter(QWidget *parent) : QObject(parent), m_progress(0.0) {
        m_anim = new QVariantAnimation(this);
        m_anim->setDuration(tokens::kDurationFast);
        m_anim->setEasingCurve(QEasingCurve::OutCubic);
        connect(m_anim, &QVariantAnimation::valueChanged, this,
                [this, parent](const QVariant &val) {
                    m_progress = val.toReal();
                    parent->setProperty("hoverProgress", m_progress);
                    parent->update();
                });
    }

protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (obj == parent()) {
            if (event->type() == QEvent::Enter) {
                m_anim->stop();
                m_anim->setStartValue(m_progress);
                m_anim->setEndValue(1.0);
                m_anim->start();
            } else if (event->type() == QEvent::Leave) {
                m_anim->stop();
                m_anim->setStartValue(m_progress);
                m_anim->setEndValue(0.0);
                m_anim->start();
            }
        }
        return QObject::eventFilter(obj, event);
    }

private:
    QVariantAnimation *m_anim = nullptr;
    qreal m_progress = 0.0;
};

/// @brief Лёгкий fade для QStackedWidget — отключён для QWebEngine (native window не поддерживает
/// QGraphicsEffect). Держим класс для совместимости, но без эффекта — instant switch, 0ms, без
/// риска краша.
class FadeStackedWidget : public QStackedWidget {
public:
    using QStackedWidget::QStackedWidget;
};

} // namespace fasthub::ui

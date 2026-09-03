#include "icon_util.h"

#include <QFile>
#include <QFontDatabase>
#include <QSvgRenderer>
#include <QPainter>
#include <QHash>

#include "infrastructure/logger.h"

// Q_INIT_RESOURCE must expand at the GLOBAL namespace scope, otherwise it
// declares a new local symbol instead of referencing the qrc initializer
// compiled into the static ui library.
inline void initHubResources() {
    Q_INIT_RESOURCE(resources);
}

namespace fasthub::ui {

namespace {

bool s_resourcesInitialized = false;

void ensureResources() {
    if (!s_resourcesInitialized) {
        initHubResources();
        s_resourcesInitialized = true;
    }
}

QHash<QString, QIcon> &iconCache() {
    static QHash<QString, QIcon> instance;
    return instance;
}

QPixmap renderSvg(const QByteArray& svg, const QColor& color, int size) {
    QByteArray tinted = svg;
    if (color.isValid()) {
        tinted.replace("currentColor", color.name().toUtf8());
    }

    QSvgRenderer renderer(tinted);
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    renderer.render(&p, QRect(0, 0, size, size));
    return pm;
}

} // namespace

void IconUtil::loadBundledFonts() {
    ensureResources();
    const QStringList fontResources = {
        QStringLiteral(":/fonts/Manrope-SemiBold.ttf"),
        QStringLiteral(":/fonts/Inter-Regular.ttf"),
        QStringLiteral(":/fonts/JetBrainsMono-Regular.ttf"),
    };
    for (const QString& path : fontResources) {
        const int id = QFontDatabase::addApplicationFont(path);
        if (id < 0) {
            infrastructure::Logger::instance().warn("UI", "Failed to load bundled font: " + path);
        }
    }
}

QPixmap IconUtil::logoPixmap(int size) {
    ensureResources();
    // Prefer PNG (user-provided, e.g. 1330x1182 from лого.png) if present.
    QFile pngFile(QStringLiteral(":/icons/fasthub-logo.png"));
    if (pngFile.exists()) {
        if (pngFile.open(QIODevice::ReadOnly)) {
            QByteArray data = pngFile.readAll();
            QPixmap pm;
            if (pm.loadFromData(data)) {
                if (pm.width() != size || pm.height() != size) {
                    // Square crop + smooth scale for non-square source (iCloud logo 1330x1182)
                    int s = qMin(pm.width(), pm.height());
                    QRect crop((pm.width() - s) / 2, (pm.height() - s) / 2, s, s);
                    pm = pm.copy(crop).scaled(size, size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                }
                return pm;
            }
        }
    }
    QFile file(QStringLiteral(":/icons/fasthub-logo.svg"));
    if (!file.open(QIODevice::ReadOnly)) {
        infrastructure::Logger::instance().warn("UI", "Logo resource missing: fasthub-logo.svg");
        return QPixmap();
    }
    return renderSvg(file.readAll(), QColor(), size);
}

QIcon IconUtil::logoIcon() {
    QIcon icon;
    for (int size : {16, 24, 32, 48, 64, 128}) {
        icon.addPixmap(logoPixmap(size));
    }
    return icon;
}

QIcon IconUtil::tintedIcon(const QString& resourcePath, const QColor& color, int size) {
    ensureResources();
    const QString key = resourcePath + QLatin1Char('@') + color.name() + QLatin1Char('@') + QString::number(size);
    auto &c = iconCache();
    if (auto it = c.constFind(key); it != c.constEnd()) {
        return it.value();
    }

    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly)) {
        infrastructure::Logger::instance().warn("UI", "Icon resource missing: " + resourcePath);
        return QIcon();
    }
    QIcon icon(renderSvg(file.readAll(), color, size));
    c.insert(key, icon);
    return icon;
}

void IconUtil::clearCache() {
    iconCache().clear();
}

} // namespace fasthub::ui

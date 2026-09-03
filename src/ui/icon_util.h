#pragma once

#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QString>

namespace fasthub::ui {

/// @brief Brand identity helpers: font loading, SVG icon tinting, logo.
namespace IconUtil {

/// @brief Loads bundled fonts (Manrope / Inter / JetBrains Mono) from
///        resources. Safe to call once at startup.
void loadBundledFonts();

/// @brief Application-wide logo QIcon (from the bundled SVG).
QIcon logoIcon();

/// @brief Renders the logo SVG to a pixmap of @p size.
QPixmap logoPixmap(int size);

/// @brief Renders a bundled SVG icon recolored to @p color, @p size x @p size, with caching.
QIcon tintedIcon(const QString& resourcePath, const QColor& color, int size = 24);

/// @brief Clears the icon render cache (call on theme change).
void clearCache();

} // namespace IconUtil

} // namespace fasthub::ui

#include "HeaderModel.h"
#include <QDebug>
#include <algorithm>
#include <QSvgRenderer>
#include <QFile>
#include <QPainter>

/**
 * @brief Calculates the scale factor to fit an SVG into the target size.
 *
 * @param svgFilePath Path to the SVG file.
 * @param targetWidth The target width for scaling.
 * @param targetHeight The target height for scaling.
 * @return qreal Scale factor (0 to 1) to fit the SVG into the target size.
 */
qreal HeaderModel::calculateSvgScaleFactor(const QString &svgFilePath, const qreal &targetWidth, const qreal &targetHeight) {
    QSvgRenderer renderer;
    QFile file(svgFilePath);

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open SVG file:" << svgFilePath;
        return 0.0;
    }

    if (!renderer.load(file.readAll())) {
        qWarning() << "Failed to load SVG file:" << svgFilePath;
        return 0.0;
    }

    QSizeF svgSize = renderer.defaultSize();
    QRectF viewBox = renderer.viewBoxF();

    if (svgSize.isEmpty() && !viewBox.isEmpty()) {
        svgSize = viewBox.size();
    }

    qreal scaleWidth = targetWidth / svgSize.width();
    qreal scaleHeight = targetHeight / svgSize.height();
    qreal scaleFactor = qMin(scaleWidth, scaleHeight);

    return qBound(0.0, scaleFactor, 1.0);
}

/**
 * @brief Renders an SVG file into a QImage with the specified size and scale.
 *
 * @param svgFilePath Path to the SVG file.
 * @param targetSize The target size for the rendered image.
 * @return QImage The rendered SVG as a QImage. Returns an empty QImage if rendering fails.
 */
QImage HeaderModel::renderSvgToImage(const QString &svgFilePath, const QSize &targetSize) {
    QSvgRenderer renderer;
    QFile file(svgFilePath);

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open SVG file:" << svgFilePath;
        return QImage();
    }

    if (!renderer.load(file.readAll())) {
        qWarning() << "Failed to load SVG file:" << svgFilePath;
        return QImage();
    }

    renderer.setProperty("fill", "white");

    // Calculate the scale factor
    qreal scaleFactor = calculateSvgScaleFactor(svgFilePath, targetSize.width(), targetSize.height());

    // Create a QImage with the target size
    QImage image(targetSize, QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.scale(scaleFactor, scaleFactor);
    renderer.render(&painter);

    return image;
}

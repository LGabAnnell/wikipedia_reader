#ifndef HEADERMODEL_H
#define HEADERMODEL_H

#include <QObject>
#include <QString>
#include <QQmlEngine>
#include <QList>
#include <QImage>
class HeaderModel : public QObject
{
    Q_OBJECT
        QML_ELEMENT

public:
    explicit HeaderModel(QObject *parent = nullptr) : QObject(parent) {};

    /**
     * @brief Calculates the scale factor to fit an SVG into a 24x24 bounding box.
     *
     * @param svgFilePath Path to the SVG file.
     * @param targetWidth The target width for scaling.
     * @param targetHeight The target height for scaling.
     * @return qreal Scale factor (0 to 1) to fit the SVG into the target size.
     */
    Q_INVOKABLE qreal calculateSvgScaleFactor(const QString &svgFilePath, const qreal &targetWidth, const qreal &targetHeight);

    /**
     * @brief Renders an SVG file into a QImage with the specified size and scale.
     *
     * @param svgFilePath Path to the SVG file.
     * @param targetSize The target size for the rendered image.
     * @return QImage The rendered SVG as a QImage. Returns an empty QImage if rendering fails.
     */
    Q_INVOKABLE QImage renderSvgToImage(const QString &svgFilePath, const QSize &targetSize);
};

#endif // HEADERMODEL_H


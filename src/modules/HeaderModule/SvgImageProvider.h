#ifndef SVGIMAGEPROVIDER_H
#define SVGIMAGEPROVIDER_H

#include <QQuickImageProvider>
#include <QImage>
#include <QSize>
#include <QColor>
#include <QByteArray>

class HeaderModel;
namespace tinyxml2 {
    class XMLElement;
}

class SvgImageProvider : public QQuickImageProvider {
public:
    SvgImageProvider(HeaderModel *model);
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;
    void setFillColor(const QColor &color);

private:
    QByteArray modifySvgFillColor(const QByteArray &svgData);
    void updateAttribute(tinyxml2::XMLElement *element, const QString &attribute);

    QPointer<HeaderModel> m_model;
    QColor m_fillColor;
};

#endif // SVGIMAGEPROVIDER_H
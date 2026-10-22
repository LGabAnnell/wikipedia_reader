#ifndef SVGIMAGEPROVIDER_H
#define SVGIMAGEPROVIDER_H

#include <QByteArray>
#include <QColor>
#include <QImage>
#include <QQuickImageProvider>
#include <QSize>

namespace tinyxml2 {
class XMLElement;
}

class SvgImageProvider : public QQuickImageProvider {
  public:
    SvgImageProvider();
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

  private:
    QByteArray modifySvgFillColor(const QByteArray &svgData);
    void updateAttribute(tinyxml2::XMLElement *element, const QString &attribute);

    const QColor m_fillColor;
};

#endif // SVGIMAGEPROVIDER_H

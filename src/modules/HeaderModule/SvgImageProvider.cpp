#include "SvgImageProvider.h"
#include "HeaderModel.h"
#include <QPainter>
#include <QSvgRenderer>
#include <QFile>
#include <QPalette>
#include <QPainterPath>
#include <tinyxml2.h>
#include <QDebug>

SvgImageProvider::SvgImageProvider(HeaderModel *model)
    : QQuickImageProvider(QQuickImageProvider::Image), m_model(model) {
    QPalette palette;
    m_fillColor = palette.color(QPalette::Active, QPalette::Text);
}

void SvgImageProvider::setFillColor(const QColor &color) {
    m_fillColor = color;
}

QByteArray SvgImageProvider::modifySvgFillColor(const QByteArray &svgData) {
    tinyxml2::XMLDocument doc;
    if (doc.Parse(svgData.constData()) != tinyxml2::XML_SUCCESS) {
        qWarning() << "Failed to parse SVG data";
        return svgData;
    }

    // Traverse all elements in the SVG
    tinyxml2::XMLElement *root = doc.RootElement();
    if (!root) {
        qWarning() << "No root element in SVG";
        return svgData;
    }

    // Recursively update fill attributes
    updateAttribute(root, "fill");
    updateAttribute(root, "stroke");

    // Save the modified XML back to a QByteArray
    tinyxml2::XMLPrinter printer;
    doc.Print(&printer);
    return QByteArray(printer.CStr());
}

void SvgImageProvider::updateAttribute(tinyxml2::XMLElement *element, const QString &attribute) {
    if (!element) {
        return;
    }

    // Update fill attribute if it exists
    element->SetAttribute(attribute.toStdString().c_str(), m_fillColor.name().toUtf8().constData());

    // Recursively process child elements
    for (tinyxml2::XMLElement *child = element->FirstChildElement(); child != nullptr; child = child->NextSiblingElement()) {
        updateAttribute(child, attribute);
    }
}

QImage SvgImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize) {
    QSize targetSize = requestedSize;
    if (targetSize.isEmpty()) {
        targetSize = QSize(24, 24);
    }

    if (size) {
        *size = targetSize;
    }

    // Load the SVG file
    QString svgFilePath = ":/icons/" + id + ".svg";
    QFile file(svgFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open SVG file:" << svgFilePath;
        return QImage();
    }

    QByteArray svgData = file.readAll();
    file.close();

    // Modify the SVG fill color if a valid color is set
    if (m_fillColor.isValid()) {
        svgData = modifySvgFillColor(svgData);
    }

    // Render the modified SVG
    QSvgRenderer renderer;
    if (!renderer.load(svgData)) {
        qWarning() << "Failed to load SVG data for:" << svgFilePath;
        return QImage();
    }

    // Create a QImage with the target size
    QImage image(targetSize, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    renderer.render(&painter);

    return image;
}
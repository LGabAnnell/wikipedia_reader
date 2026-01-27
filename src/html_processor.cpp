// html_processor.cpp

#include "html_processor.h"
#include <tinyxml2.h>
#include <QFile>
#include <QPalette>

void HtmlProcessor::removeImgNodes(tinyxml2::XMLElement *element) {
    if (element == nullptr) return;

    // Remove img nodes
    for (tinyxml2::XMLElement *img = element->FirstChildElement("img");
         img != nullptr; img = element->FirstChildElement("img")) {
        element->DeleteChild(img);
    }

    // Recursively process child elements
    for (tinyxml2::XMLElement *child = element->FirstChildElement();
         child != nullptr; child = child->NextSiblingElement()) {
        removeImgNodes(child);
    }
}

void HtmlProcessor::removeStyleNodes(tinyxml2::XMLElement *element) {
    if (element == nullptr) return;

    // Remove style nodes
    for (tinyxml2::XMLElement *style = element->FirstChildElement("style");
         style != nullptr; style = element->FirstChildElement("style")) {
        element->DeleteChild(style);
    }

    // Recursively process child elements
    for (tinyxml2::XMLElement *child = element->FirstChildElement();
         child != nullptr; child = child->NextSiblingElement()) {
        removeStyleNodes(child);
    }
}

void HtmlProcessor::removeStyleAttributes(tinyxml2::XMLElement *node) {
    if (node == nullptr) return;

    // Remove style attribute from the current node
    node->DeleteAttribute("style");

    // Recursively process child elements
    for (tinyxml2::XMLElement *child = node->FirstChildElement();
         child != nullptr; child = child->NextSiblingElement()) {
        removeStyleAttributes(child);
    }
}

QString HtmlProcessor::processHtml(const QString &htmlContent) {
    tinyxml2::XMLDocument doc;
    doc.Parse(htmlContent.toStdString().c_str());

    removeImgNodes(doc.RootElement());
    removeStyleNodes(doc.RootElement());
    removeStyleAttributes(doc.RootElement());

    tinyxml2::XMLPrinter printer;
    doc.Print(&printer);

    QFile css("styles/table_style.css");
    if (!css.open(QIODevice::ReadOnly)) {
        return QString(printer.CStr());
    }

    QString style = QString(css.readAll()).arg(QPalette().text().color().name());
    QString processedHtml = QString(R"""(
            <style>
                %1
            </style>
    )""").arg(style) +
                            QString(printer.CStr());

    return processedHtml;
}

// html_processor.cpp

#include "html_processor.h"
#include <tinyxml2.h>
#include <QFile>
#include <QPalette>

void HtmlProcessor::removeImgNodes(tinyxml2::XMLElement *element) {
    if (element == nullptr) return;

    // Remove img nodes
    for (tinyxml2::XMLElement *img = element->FirstChildElement("img");
         img != nullptr; ) {
        tinyxml2::XMLElement *next = img->NextSiblingElement("img");
        element->DeleteChild(img);
        img = next;
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
         style != nullptr; ) {
        tinyxml2::XMLElement *next = style->NextSiblingElement("style");
        element->DeleteChild(style);
        style = next;
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

void HtmlProcessor::processImageNodes(tinyxml2::XMLElement *element) {
    if (element == nullptr) return;

    // Process img nodes
    for (tinyxml2::XMLElement *img = element->FirstChildElement("img");
         img != nullptr; img = img->NextSiblingElement("img")) {
        
        const char* src = img->Attribute("src");
        if (src) {
            QString srcStr = QString::fromUtf8(src);
            
            // Wikipedia images often use //upload.wikimedia.org - convert to https
            if (srcStr.startsWith("//")) {
                QString httpsSrc = "https:" + srcStr;
                img->SetAttribute("src", httpsSrc.toStdString().c_str());
            }
            // Wikipedia images sometimes use /wiki/Special:FilePath - these won't work
            // but the parse API should return proper URLs
        }
        
        // Ensure img has proper display attributes
        // Add max-width for responsive images
        const char* style = img->Attribute("style");
        QString styleStr = style ? QString::fromUtf8(style) : "";
        if (!styleStr.contains("max-width")) {
            if (styleStr.isEmpty()) {
                img->SetAttribute("style", "max-width: 100%; height: auto;");
            } else {
                QString newStyle = styleStr + " max-width: 100%; height: auto;";
                img->SetAttribute("style", newStyle.toStdString().c_str());
            }
        }
    }

    // Recursively process child elements
    for (tinyxml2::XMLElement *child = element->FirstChildElement();
         child != nullptr; child = child->NextSiblingElement()) {
        processImageNodes(child);
    }
}

QString HtmlProcessor::processHtml(const QString &htmlContent) {
    tinyxml2::XMLDocument doc;
    doc.Parse(htmlContent.toStdString().c_str());

    for (tinyxml2::XMLElement* child = doc.FirstChildElement(); child != nullptr; child = child->NextSiblingElement()) {
        removeStyleNodes(child);
        removeStyleAttributes(child);
        removeImgNodes(child);
    }

    tinyxml2::XMLPrinter printer;
    doc.Print(&printer);

    QFile css(":/styles/table_style.css");
    if (!css.open(QIODevice::ReadOnly)) {
        return QString(printer.CStr());
    }

    QString style = QString(css.readAll()).arg(QPalette().text().color().name()); // Default to black if palette is unavailable
    QString processedHtml = QString(R"""(
            <style>
                %1
            </style>
    )""").arg(style) + QString(printer.CStr());

    return processedHtml;
}

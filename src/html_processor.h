// html_processor.h

#ifndef HTML_PROCESSOR_H
#define HTML_PROCESSOR_H

#include <QString>
#include <tinyxml2.h>

class HtmlProcessor {
public:
    static void removeImgNodes(tinyxml2::XMLElement *element);
    static void removeStyleNodes(tinyxml2::XMLElement *element);
    static void removeStyleAttributes(tinyxml2::XMLElement *node);
    static QString processHtml(const QString &htmlContent);
};

#endif // HTML_PROCESSOR_H
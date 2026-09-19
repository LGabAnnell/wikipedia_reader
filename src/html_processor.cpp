// html_processor.cpp

#include "html_processor.h"
#include <QFile>
#include <QPalette>
#include <QRegularExpression>
#include <tinyxml2.h>

namespace {
int findTagEnd(const QString &html, int start) {
    QChar quote;
    for (int position = start + 1; position < html.size(); ++position) {
        const QChar character = html.at(position);
        if (!quote.isNull()) {
            if (character == quote) {
                quote = QChar();
            }
        } else if (character == QLatin1Char('"') || character == QLatin1Char('\'')) {
            quote = character;
        } else if (character == QLatin1Char('>')) {
            return position;
        }
    }
    return -1;
}

QString processHtmlFragment(const QString &htmlContent) {
    static const QRegularExpression styleElementRegex(
        R"(<style\b(?:[^>"']|"[^"]*"|'[^']*')*>[\s\S]*?</style\s*>)",
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression imageTagRegex(R"(^<\s*img\b)",
                                                  QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression protocolRelativeSourceRegex(
        R"((\bsrc\s*=\s*["'])//)", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression styleAttributeRegex(
        R"(\s+style\s*=\s*(?:"[^"]*"|'[^']*'|[^\s>]+))",
        QRegularExpression::CaseInsensitiveOption);

    QString withoutStyleElements = htmlContent;
    withoutStyleElements.remove(styleElementRegex);
    QString processedHtml;
    processedHtml.reserve(withoutStyleElements.size());

    int position = 0;
    while (position < withoutStyleElements.size()) {
        const int tagStart = withoutStyleElements.indexOf(QLatin1Char('<'), position);
        if (tagStart < 0) {
            break;
        }
        const int tagEnd = findTagEnd(withoutStyleElements, tagStart);
        if (tagEnd < 0) {
            break;
        }
        processedHtml += withoutStyleElements.sliced(position, tagStart - position);

        QString tag = withoutStyleElements.sliced(tagStart, tagEnd - tagStart + 1);
        if (imageTagRegex.match(tag).hasMatch()) {
            // Keep images in their original article position. The parse API's
            // src attribute is the relation between the HTML and the fetched
            // thumbnail, so removing the element loses that information.
            tag.replace(protocolRelativeSourceRegex, R"(\1https://)");
            tag.remove(styleAttributeRegex);
            if (!tag.contains("mwe-math-fallback-image", Qt::CaseInsensitive)) {
                int attributePosition = tag.lastIndexOf('>');
                if (attributePosition > 0 && tag.at(attributePosition - 1) == '/') {
                    --attributePosition;
                }
                if (attributePosition >= 0) {
                    tag.insert(attributePosition, " style=\"max-width: 100%; height: auto;\"");
                }
            }
            processedHtml += tag;
        } else {
            tag.remove(styleAttributeRegex);
            processedHtml += tag;
        }

        position = tagEnd + 1;
    }
    processedHtml += withoutStyleElements.sliced(position);
    return processedHtml;
}

QString removeMathMlAccessibilityMarkup(const QString &htmlContent) {
    static const QRegularExpression mathMlRegex(
        R"(<span\b(?=[^>]*\bmwe-math-mathml-[^\s"']+)[^>]*>[\s\S]*?</span\s*>)",
        QRegularExpression::CaseInsensitiveOption);

    QString processedHtml = htmlContent;
    processedHtml.remove(mathMlRegex);
    return processedHtml;
}
} // namespace

void HtmlProcessor::removeStyleNodes(tinyxml2::XMLElement *element) {
    if (element == nullptr)
        return;

    // Remove style nodes
    for (tinyxml2::XMLElement *style = element->FirstChildElement("style"); style != nullptr;) {
        tinyxml2::XMLElement *next = style->NextSiblingElement("style");
        element->DeleteChild(style);
        style = next;
    }

    // Recursively process child elements
    for (tinyxml2::XMLElement *child = element->FirstChildElement(); child != nullptr;
         child = child->NextSiblingElement()) {
        removeStyleNodes(child);
    }
}

void HtmlProcessor::removeStyleAttributes(tinyxml2::XMLElement *node) {
    if (node == nullptr)
        return;

    node->DeleteAttribute("style");

    // Recursively process child elements
    for (tinyxml2::XMLElement *child = node->FirstChildElement(); child != nullptr;
         child = child->NextSiblingElement()) {
        removeStyleAttributes(child);
    }
}

void HtmlProcessor::processImageNodes(tinyxml2::XMLElement *element) {
    if (element == nullptr)
        return;

    // Process img nodes
    for (tinyxml2::XMLElement *img = element->FirstChildElement("img"); img != nullptr;
         img = img->NextSiblingElement("img")) {

        const char *src = img->Attribute("src");
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

        const char *className = img->Attribute("class");
        const bool isMathFallback =
            className != nullptr && QString::fromUtf8(className).contains("mwe-math-fallback-image");
        if (isMathFallback) {
            continue;
        }

        // Keep article images within the text column when Qt supports the
        // corresponding rich-text CSS.
        const char *style = img->Attribute("style");
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
    for (tinyxml2::XMLElement *child = element->FirstChildElement(); child != nullptr;
         child = child->NextSiblingElement()) {
        processImageNodes(child);
    }
}

QString HtmlProcessor::processHtml(const QString &htmlContent) {
    tinyxml2::XMLDocument doc;
    // Wrap content in a dummy root element so that root-level <style>/<img>
    // nodes are treated as children and caught by the recursive removers.
    const QString contentWithoutMathMl = removeMathMlAccessibilityMarkup(htmlContent);
    std::string wrapped = "<root>" + contentWithoutMathMl.toStdString() + "</root>";
    const tinyxml2::XMLError parseResult = doc.Parse(wrapped.c_str());

    QString processedContent;
    if (parseResult != tinyxml2::XML_SUCCESS) {
        // MediaWiki returns HTML, which may contain HTML void elements such as
        // <img> rather than XML's <img/>. Preserve that valid HTML instead of
        // serializing tinyxml2's incomplete parse tree.
        processedContent = processHtmlFragment(contentWithoutMathMl);
    } else {
        tinyxml2::XMLElement *root = doc.FirstChildElement("root");
        if (root) {
            removeStyleNodes(root);
            removeStyleAttributes(root);
            processImageNodes(root);
        } else {
            for (tinyxml2::XMLElement *child = doc.FirstChildElement(); child != nullptr;
                 child = child->NextSiblingElement()) {
                removeStyleNodes(child);
                removeStyleAttributes(child);
                processImageNodes(child);
            }
        }

        tinyxml2::XMLPrinter printer;
        if (root) {
            // Print only the children of <root>, not the wrapper itself
            for (tinyxml2::XMLNode *child = root->FirstChild(); child != nullptr;
                 child = child->NextSibling()) {
                child->Accept(&printer);
            }
        } else {
            doc.Print(&printer);
        }

        processedContent = QString::fromUtf8(printer.CStr());
    }

    QFile css(":/styles/table_style.css");
    if (!css.open(QIODevice::ReadOnly)) {
        return processedContent;
    }

    QString style =
        QString(css.readAll()).arg(QPalette().text().color().name()); // Default to black if palette is unavailable
    QString processedHtml = QString(R"""(
            <style>
                %1
            </style>
    )""")
                                .arg(style) +
                            processedContent;

    return processedHtml;
}

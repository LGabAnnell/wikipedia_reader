// html_processor.cpp

#include "html_processor.h"
#include <QFile>
#include <QPalette>
#include <QRegularExpression>
#include <tinyxml2.h>

namespace {
bool isMathFallbackImage(const tinyxml2::XMLElement *element) {
    if (element == nullptr || QString::fromUtf8(element->Name()) != "img") {
        return false;
    }

    const char *className = element->Attribute("class");
    return className != nullptr && QString::fromUtf8(className).contains("mwe-math-fallback-image");
}

QString processHtmlFragment(const QString &htmlContent) {
    static const QRegularExpression styleElementRegex(
        R"(<style\b[^>]*>[\s\S]*?</style\s*>)",
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression htmlTagRegex(R"(<[^>]+>)");
    static const QRegularExpression imageTagRegex(R"(^<\s*img\b)",
                                                  QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression styleAttributeRegex(
        R"(\s+style\s*=\s*(?:"[^"]*"|'[^']*'|[^\s>]+))",
        QRegularExpression::CaseInsensitiveOption);

    QString withoutStyleElements = htmlContent;
    withoutStyleElements.remove(styleElementRegex);
    QString processedHtml;
    processedHtml.reserve(withoutStyleElements.size());

    int position = 0;
    QRegularExpressionMatchIterator tags = htmlTagRegex.globalMatch(withoutStyleElements);
    while (tags.hasNext()) {
        const QRegularExpressionMatch match = tags.next();
        processedHtml += withoutStyleElements.sliced(position, match.capturedStart() - position);

        QString tag = match.captured();
        if (imageTagRegex.match(tag).hasMatch()) {
            // Preserve the PNG fallback that the page client generates from
            // MediaWiki's MathML SVG.
            if (tag.contains("mwe-math-fallback-image", Qt::CaseInsensitive)) {
                // SVG's ex-based dimensions are not interpreted reliably by
                // QQuickTextEdit. The generated PNG has intrinsic dimensions.
                tag.remove(styleAttributeRegex);
                processedHtml += tag;
            }
        } else {
            tag.remove(styleAttributeRegex);
            processedHtml += tag;
        }

        position = match.capturedEnd();
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

void HtmlProcessor::removeImgNodes(tinyxml2::XMLElement *element) {
    if (element == nullptr)
        return;

    // Remove img nodes
    for (tinyxml2::XMLElement *img = element->FirstChildElement("img"); img != nullptr;) {
        tinyxml2::XMLElement *next = img->NextSiblingElement("img");
        // MediaWiki provides LaTeX equations as MathML plus an SVG fallback
        // image. Qt rich text does not render MathML, so retain that fallback
        // while continuing to remove regular article images for the gallery.
        if (!isMathFallbackImage(img)) {
            element->DeleteChild(img);
        }
        img = next;
    }

    // Recursively process child elements
    for (tinyxml2::XMLElement *child = element->FirstChildElement(); child != nullptr;
         child = child->NextSiblingElement()) {
        removeImgNodes(child);
    }
}

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

        // Ensure img has proper display attributes
        // Add max-width for responsive images
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
            removeImgNodes(root);
        } else {
            for (tinyxml2::XMLElement *child = doc.FirstChildElement(); child != nullptr;
                 child = child->NextSiblingElement()) {
                removeStyleNodes(child);
                removeStyleAttributes(child);
                removeImgNodes(child);
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

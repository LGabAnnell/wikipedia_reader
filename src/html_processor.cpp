// html_processor.cpp

#include "html_processor.h"
#include <QFile>
#include <QPalette>
#include <QRegularExpression>
#include <QTextDocument>
#include <QUrl>
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

QString attributeValue(const QString &tag, const QString &name) {
    const QRegularExpression attributeRegex(
        QString(R"(\b%1\s*=\s*(["'])(.*?)\1)").arg(QRegularExpression::escape(name)),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    return attributeRegex.match(tag).captured(2);
}

bool hasClass(const QString &tag, const QString &className) {
    return attributeValue(tag, QStringLiteral("class"))
        .split(QRegularExpression(QStringLiteral(R"(\s+)")), Qt::SkipEmptyParts)
        .contains(className);
}

// Return the end of an element, including its closing tag. This also handles
// the nested divs used by MediaWiki's older thumbnail markup.
int elementEnd(const QString &html, int start, const QString &name) {
    const QRegularExpression tagRegex(
        QString(R"(<\s*(/?)\s*%1\b)").arg(QRegularExpression::escape(name)),
        QRegularExpression::CaseInsensitiveOption);
    int depth = 0;
    auto matches = tagRegex.globalMatch(html, start);
    while (matches.hasNext()) {
        const auto match = matches.next();
        const int end = findTagEnd(html, match.capturedStart());
        if (end < 0)
            return -1;
        if (match.captured(1).isEmpty()) {
            ++depth;
        } else if (--depth == 0) {
            return end + 1;
        }
    }
    return -1;
}

QString imageTagIn(const QString &html) {
    static const QRegularExpression imageStart(R"(<\s*img\b)",
                                               QRegularExpression::CaseInsensitiveOption);
    const auto match = imageStart.match(html);
    if (!match.hasMatch())
        return {};
    const int end = findTagEnd(html, match.capturedStart());
    return end < 0 ? QString() : html.sliced(match.capturedStart(), end - match.capturedStart() + 1);
}

struct Caption {
    QString html;
    int start = -1;
    int end = -1;
};

Caption captionIn(const QString &html, const QString &name, const QString &className = {}) {
    const QRegularExpression startRegex(
        QString(R"(<\s*%1\b)").arg(QRegularExpression::escape(name)),
        QRegularExpression::CaseInsensitiveOption);
    auto matches = startRegex.globalMatch(html);
    while (matches.hasNext()) {
        const auto match = matches.next();
        const int openingEnd = findTagEnd(html, match.capturedStart());
        if (openingEnd < 0)
            return {};
        const QString opening = html.sliced(match.capturedStart(), openingEnd - match.capturedStart() + 1);
        if (!className.isEmpty() && !hasClass(opening, className))
            continue;
        const int closingEnd = elementEnd(html, match.capturedStart(), name);
        if (closingEnd < 0)
            return {};
        const int closingStart = html.lastIndexOf(QLatin1String("</"), closingEnd - 1);
        if (closingStart < openingEnd)
            return {};
        return {html.sliced(openingEnd + 1, closingStart - openingEnd - 1).trimmed(),
                int(match.capturedStart()), closingEnd};
    }
    return {};
}

QString imageWithLink(const QString &html, const QString &imageTag) {
    const int imageStart = html.indexOf(imageTag);
    if (imageStart < 0)
        return imageTag;
    static const QRegularExpression openingAnchor(R"(<\s*a\b)",
                                                  QRegularExpression::CaseInsensitiveOption);
    const int anchorStart = html.lastIndexOf(openingAnchor, imageStart);
    if (anchorStart < 0)
        return imageTag;
    const int anchorOpeningEnd = findTagEnd(html, anchorStart);
    static const QRegularExpression closingAnchor(R"(</\s*a\s*>)",
                                                  QRegularExpression::CaseInsensitiveOption);
    const int anchorClosingStart = html.indexOf(closingAnchor, imageStart);
    if (anchorOpeningEnd >= imageStart || anchorClosingStart < 0)
        return imageTag;
    const int anchorClosingEnd = findTagEnd(html, anchorClosingStart);
    return anchorClosingEnd < 0 ? imageTag
                                : html.sliced(anchorStart, anchorClosingEnd - anchorStart + 1);
}

QString imageTable(const QString &body, const QString &caption) {
    const QString imageTag = imageTagIn(body);
    if (imageTag.isEmpty() || imageTag.contains(QLatin1String("mwe-math-fallback-image"),
                                                 Qt::CaseInsensitive))
        return {};
    static const QRegularExpression imageStart(R"(<\s*img\b)",
                                               QRegularExpression::CaseInsensitiveOption);
    if (imageStart.match(body, body.indexOf(imageTag) + imageTag.size()).hasMatch())
        return {}; // A gallery or multi-image figure must retain its own layout.

    QString visibleCaption = caption;
    QTextDocument captionDocument;
    captionDocument.setHtml(visibleCaption);
    if (captionDocument.toPlainText().trimmed().isEmpty()) {
        visibleCaption = attributeValue(imageTag, QStringLiteral("alt"));
        visibleCaption.replace(QLatin1Char('<'), QLatin1String("&lt;"));
        visibleCaption.replace(QLatin1Char('>'), QLatin1String("&gt;"));
        captionDocument.setHtml(visibleCaption);
    }
    const QByteArray encodedDescription = QUrl::toPercentEncoding(captionDocument.toPlainText().trimmed());
    QString linkedImage = imageWithLink(body, imageTag);
    const int imagePosition = linkedImage.indexOf(imageTag);
    if (imagePosition < 0)
        return {};
    QString annotatedImage = imageTag;
    int insertionPoint = annotatedImage.lastIndexOf(QLatin1Char('>'));
    if (insertionPoint > 0 && annotatedImage.at(insertionPoint - 1) == QLatin1Char('/'))
        --insertionPoint;
    annotatedImage.insert(insertionPoint,
                          QStringLiteral(" data-article-description=\"%1\"")
                              .arg(QString::fromLatin1(encodedDescription)));
    linkedImage.replace(imagePosition, imageTag.size(), annotatedImage);

    bool hasWidth = false;
    const int imageWidth = attributeValue(imageTag, QStringLiteral("width")).toInt(&hasWidth);
    const QString tableWidth = hasWidth && imageWidth > 0
                                   ? QStringLiteral(" width=\"%1\"").arg(qint64(imageWidth) + 20)
                                   : QString();
    return QStringLiteral("<table class=\"article-image-table\" border=\"1\"%1 cellspacing=\"0\">"
                          "<tr><td align=\"center\">%2</td></tr>"
                          "<tr><td>%3</td></tr></table>")
        .arg(tableWidth, linkedImage, visibleCaption);
}

QString normalizeBlockImages(const QString &html) {
    static const QRegularExpression tagNameRegex(R"(^<\s*(/?)\s*([a-z][a-z0-9:-]*)\b)",
                                                 QRegularExpression::CaseInsensitiveOption);
    QString result;
    int copyStart = 0;
    int position = 0;
    int tableDepth = 0;
    while ((position = html.indexOf(QLatin1Char('<'), position)) >= 0) {
        const int openingEnd = findTagEnd(html, position);
        if (openingEnd < 0)
            break;
        const QString tag = html.sliced(position, openingEnd - position + 1);
        const auto match = tagNameRegex.match(tag);
        if (!match.hasMatch()) {
            position = openingEnd + 1;
            continue;
        }
        const QString name = match.captured(2).toLower();
        if (name == QLatin1String("table")) {
            tableDepth += match.captured(1).isEmpty() ? 1 : -1;
        } else if (tableDepth == 0 && match.captured(1).isEmpty()
                   && (name == QLatin1String("figure") || name == QLatin1String("p")
                       || name == QLatin1String("div"))) {
            const int end = elementEnd(html, position, name);
            if (end > 0) {
                const QString body = html.sliced(openingEnd + 1,
                                                 html.lastIndexOf(QLatin1String("</"), end - 1)
                                                     - openingEnd - 1);
                const bool isThumbnail = name == QLatin1String("div")
                                         && hasClass(tag, QStringLiteral("thumb"));
                bool isStandalone = name == QLatin1String("figure") || isThumbnail;
                if (!isStandalone) {
                    QString remainder = body;
                    remainder.remove(QRegularExpression(R"(<\s*img\b(?:[^>"']|"[^"]*"|'[^']*')*>)",
                                                        QRegularExpression::CaseInsensitiveOption));
                    remainder.remove(QRegularExpression(R"(</?\s*a\b[^>]*>)",
                                                        QRegularExpression::CaseInsensitiveOption));
                    if (name == QLatin1String("div")) {
                        remainder.remove(QRegularExpression(R"(</?\s*div\b[^>]*>)",
                                                            QRegularExpression::CaseInsensitiveOption));
                    }
                    isStandalone = remainder.trimmed().isEmpty();
                }
                const Caption caption = name == QLatin1String("figure")
                                            ? captionIn(body, QStringLiteral("figcaption"))
                                            : name == QLatin1String("div")
                                                  ? captionIn(body, QStringLiteral("div"),
                                                              QStringLiteral("thumbcaption"))
                                                  : Caption();
                QString mediaBody = body;
                if (caption.start >= 0)
                    mediaBody.remove(caption.start, caption.end - caption.start);
                QString captionHtml = caption.html;
                if (isThumbnail) {
                    const Caption magnify = captionIn(captionHtml, QStringLiteral("div"),
                                                      QStringLiteral("magnify"));
                    if (magnify.start >= 0)
                        captionHtml.remove(magnify.start, magnify.end - magnify.start);
                }
                const QString table = isStandalone ? imageTable(mediaBody, captionHtml.trimmed())
                                                   : QString();
                if (!table.isEmpty()) {
                    result += html.sliced(copyStart, position - copyStart);
                    result += table;
                    copyStart = end;
                    position = end;
                    continue;
                }
                if (name == QLatin1String("figure")) {
                    position = end;
                    continue;
                }
            }
        }
        position = openingEnd + 1;
    }
    result += html.sliced(copyStart);
    return result;
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
    const QString normalizedContent = normalizeBlockImages(contentWithoutMathMl);
    std::string wrapped = "<root>" + normalizedContent.toStdString() + "</root>";
    const tinyxml2::XMLError parseResult = doc.Parse(wrapped.c_str());

    QString processedContent;
    if (parseResult != tinyxml2::XML_SUCCESS) {
        // MediaWiki returns HTML, which may contain HTML void elements such as
        // <img> rather than XML's <img/>. Preserve that valid HTML instead of
        // serializing tinyxml2's incomplete parse tree.
        processedContent = processHtmlFragment(normalizedContent);
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

// wikipedia_page_client.cpp
#include "wikipedia_page_client.h"
#include "html_processor.h"
#include "wikipedia_network_access_manager.h"
#include <QBuffer>
#include <QPalette>
#include <QPainter>
#include <QRegularExpression>
#include <QSharedPointer>
#include <QSvgRenderer>
#include <QUrl>
#include <QUrlQuery>
#include <algorithm>

static const QRegularExpression htmlTagRegex("<[^>]*>");

namespace {
struct MathImageRequest {
    QString htmlContent;
    QHash<QString, QString> dataUrls;
    int pendingRequests = 0;
    std::function<void(const QString &)> completion;
};

// Strip HTML tags and decode common entities so image descriptions render as
// plain text.
QString stripHtml(const QString &html) {
    if (html.isEmpty()) {
        return html;
    }
    QString text = html;
    text.remove(htmlTagRegex);
    text.replace("&amp;", "&");
    text.replace("&lt;", "<");
    text.replace("&gt;", ">");
    text.replace("&quot;", "\"");
    text.replace("&#39;", "'");
    text.replace("&nbsp;", " ");
    return text.trimmed();
}

QString rasterizeSvg(const QByteArray &svgData) {
    static const QRegularExpression widthRegex(R"(\bwidth="([0-9.]+)ex")");
    static const QRegularExpression heightRegex(R"(\bheight="([0-9.]+)ex")");
    constexpr double pixelsPerEx = 7.0;

    QByteArray svgWithTextColor = svgData;
    if (svgWithTextColor.startsWith("<svg")) {
        const QByteArray color = QPalette().text().color().name().toUtf8();
        svgWithTextColor.insert(4, " color=\"" + color + "\"");
    }

    QSvgRenderer renderer(svgWithTextColor);
    if (!renderer.isValid()) {
        return {};
    }

    QSize imageSize = renderer.defaultSize();
    const QString svg = QString::fromUtf8(svgData);
    const QRegularExpressionMatch widthMatch = widthRegex.match(svg);
    const QRegularExpressionMatch heightMatch = heightRegex.match(svg);
    if (widthMatch.hasMatch() && heightMatch.hasMatch()) {
        imageSize = QSize(qRound(widthMatch.captured(1).toDouble() * pixelsPerEx),
                          qRound(heightMatch.captured(1).toDouble() * pixelsPerEx));
    }
    if (imageSize.isEmpty()) {
        return {};
    }

    imageSize.setWidth(std::min(imageSize.width(), 4096));
    imageSize.setHeight(std::min(imageSize.height(), 4096));
    QImage image(imageSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    renderer.render(&painter);
    painter.end();

    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    if (!image.save(&buffer, "PNG")) {
        return {};
    }

    return "data:image/png;base64," + QString::fromLatin1(buffer.data().toBase64());
}
} // namespace

WikipediaPageClient::WikipediaPageClient(QObject *parent)
    : QObject(parent), networkManager(WikipediaNetwork::createNetworkAccessManager(this)) {
    setLanguage("en");
}

WikipediaPageClient::~WikipediaPageClient() = default;

void WikipediaPageClient::setLanguage(const QString &langCode) {
    m_requests.setLanguage(langCode);
    baseUrl = QString("https://%1.wikipedia.org/w/api.php").arg(langCode);
}

void WikipediaPageClient::cancelPendingRequests() { m_requests.cancelPendingRequests(); }

void WikipediaPageClient::getPage(const QString &title) {
    QUrl url(baseUrl);
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("action", "query");
    urlQuery.addQueryItem("format", "json");
    urlQuery.addQueryItem("prop", "extracts");
    urlQuery.addQueryItem("titles", title);
    urlQuery.addQueryItem("explaintext", "1");
    url.setQuery(urlQuery);

    const auto context = m_requests.context();
    QNetworkReply *reply = networkManager->get(QNetworkRequest(url));
    m_requests.track(reply, context);
    connect(reply, &QNetworkReply::finished, this, [this, reply, title, context]() { this->onPageReply(reply, title, context); });
}

void WikipediaPageClient::onPageReply(QNetworkReply *reply, const QString &title, const WikipediaRequestContext &context) {
    if (m_requests.isCurrent(context) && reply->error() == QNetworkReply::NoError) {
        page p = parsePage(reply->readAll(), title);
        if (!p.title.isEmpty()) {
            emit pageReceived(p);
        }
    } else if (m_requests.isCurrent(context) && reply->error() != QNetworkReply::OperationCanceledError) {
        emit errorOccurred(reply->errorString());
    }
    reply->deleteLater();
}

page WikipediaPageClient::parsePage(const QByteArray &responseData, const QString &title) {
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
    QJsonObject jsonObj = jsonDoc.object();
    QJsonObject pages = jsonObj["query"].toObject()["pages"].toObject();

    page result;
    for (auto it = pages.begin(); it != pages.end(); ++it) {
        if (it.value().toObject()["title"].toString() == title) {
            result.title = it.value().toObject()["title"].toString();
            result.extract = it.value().toObject()["extract"].toString();
            result.pageid = it.key().toInt();
            result.imageUrls = QStringList();
            break;
        }
    }
    return result;
}

void WikipediaPageClient::getPageById(int pageid) {
    QUrl url(baseUrl);
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("action", "query");
    urlQuery.addQueryItem("format", "json");
    urlQuery.addQueryItem("pageids", QString::number(pageid));
    url.setQuery(urlQuery);

    const auto context = m_requests.context();
    QNetworkReply *reply = networkManager->get(QNetworkRequest(url));
    m_requests.track(reply, context);
    connect(reply, &QNetworkReply::finished, this, [this, reply, pageid, context]() {
        if (!m_requests.isCurrent(context)) { reply->deleteLater(); return; }
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
            QJsonObject jsonObj = jsonDoc.object();
            QJsonObject pages = jsonObj["query"].toObject()["pages"].toObject();

            page page;

            for (auto it = pages.begin(); it != pages.end(); ++it) {
                if (it.key() == QString::number(pageid)) {
                    QJsonObject pageObj = it.value().toObject();
                    if (pageObj.contains("missing") || pageObj.contains("invalid")) {
                        break;
                    }
                    page.title = pageObj["title"].toString();
                    page.pageid = pageid;
                    break;
                }
            }

            if (page.title.isEmpty()) {
                emit errorOccurred(tr("Unable to load article: page %1 was not found in the response.").arg(pageid));
            } else {
                fetchPageContentWithImages(pageid, page, context);
            }
        } else if (reply->error() != QNetworkReply::OperationCanceledError) {
            emit errorOccurred(reply->errorString());
        }
        reply->deleteLater();
    });
}

void WikipediaPageClient::getPageWithImages(int pageid) {
    QUrl url(baseUrl);
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("action", "query");
    urlQuery.addQueryItem("format", "json");
    urlQuery.addQueryItem("prop", "extracts|images");
    urlQuery.addQueryItem("pageids", QString::number(pageid));
    urlQuery.addQueryItem("explaintext", "1");
    urlQuery.addQueryItem("imlimit", "50");
    url.setQuery(urlQuery);

    const auto context = m_requests.context();
    QNetworkReply *reply = networkManager->get(QNetworkRequest(url));
    m_requests.track(reply, context);
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, pageid, context]() { this->onPageWithImagesReply(reply, pageid, context); });
}

void WikipediaPageClient::resolveTitleToPageId(const QString &title) {
    QUrl url(baseUrl);
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("action", "query");
    urlQuery.addQueryItem("format", "json");
    urlQuery.addQueryItem("titles", title);
    urlQuery.addQueryItem("prop", "pageprops|pageids");
    url.setQuery(urlQuery);

    const auto context = m_requests.context();
    QNetworkReply *reply = networkManager->get(QNetworkRequest(url));
    m_requests.track(reply, context);
    connect(reply, &QNetworkReply::finished, this, [this, reply, context]() {
        if (!m_requests.isCurrent(context)) { reply->deleteLater(); return; }
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
            QJsonObject jsonObj = jsonDoc.object();
            QJsonObject pages = jsonObj["query"].toObject()["pages"].toObject();

            bool resolved = false;
            for (auto it = pages.begin(); it != pages.end(); ++it) {
                int pageid = it.value().toObject()["pageid"].toInt();
                if (pageid > 0) {
                    emit pageIdResolved(pageid);
                    resolved = true;
                    break;
                }
            }
            if (!resolved) {
                emit errorOccurred(tr("Unable to resolve article title."));
            }
        } else if (reply->error() != QNetworkReply::OperationCanceledError) {
            emit errorOccurred(reply->errorString());
        }
        reply->deleteLater();
    });
}

void WikipediaPageClient::onPageWithImagesReply(QNetworkReply *reply, int pageid, const WikipediaRequestContext &context) {
    if (!m_requests.isCurrent(context)) { reply->deleteLater(); return; }
    if (reply->error() == QNetworkReply::NoError) {
        page p = parsePageWithImages(reply->readAll(), pageid);

        // parsePageWithImages stores image titles in imageUrls temporarily;
        // extract them and fetch the actual URLs
        const QStringList imageTitles = p.imageUrls;
        p.imageUrls.clear();
        fetchImageUrlsFromTitles(imageTitles, p, context, [this, context](page completedPage) {
            if (m_requests.isCurrent(context))
                emit pageWithImagesReceived(completedPage);
        });
    } else if (reply->error() != QNetworkReply::OperationCanceledError) {
        emit errorOccurred(reply->errorString());
    }
    reply->deleteLater();
}

page WikipediaPageClient::parsePageWithImages(const QByteArray &responseData, int pageid) {
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
    QJsonObject jsonObj = jsonDoc.object();
    QJsonObject pages = jsonObj["query"].toObject()["pages"].toObject();

    page result;
    for (auto it = pages.begin(); it != pages.end(); ++it) {
        if (it.key().toInt() == pageid) {
            QJsonObject pageObj = it.value().toObject();
            result.title = pageObj["title"].toString();
            result.extract = pageObj["extract"].toString();
            result.pageid = pageid;
            result.imageUrls = QStringList();

            // Extract image titles into imageUrls temporarily (caller fetches URLs)
            if (pageObj.contains("images")) {
                QJsonArray images = pageObj["images"].toArray();
                for (const QJsonValueConstRef &image : std::as_const(images)) {
                    result.imageUrls.append(image.toObject().value("title").toString());
                }
            }
            break;
        }
    }
    return result;
}

void WikipediaPageClient::getImageDetails(const QString &title) {
    const auto context = m_requests.context();
    fetchImageUrlsFromTitles({title}, {}, context, [this, title, context](page result) {
        if (m_requests.isCurrent(context))
            emit imageDetailsReceived(title, result.imageUrls.value(0), result.imageDescriptions.value(0));
    }, true);
}

void WikipediaPageClient::fetchImageUrlsFromTitles(const QStringList &imageTitles, page pageData,
                                                   const WikipediaRequestContext &context,
                                                   std::function<void(page)> completion, bool details) {
    if (!m_requests.isCurrent(context)) return;
    if (imageTitles.isEmpty()) {
        completion(pageData);
        return;
    }

    QUrl url(QString("https://%1.wikipedia.org/w/api.php").arg(context.language));
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("action", "query");
    urlQuery.addQueryItem("format", "json");
    urlQuery.addQueryItem("prop", "imageinfo");
    urlQuery.addQueryItem("iiprop", "url|extmetadata|mime");
    urlQuery.addQueryItem("iiurlheight", details ? "2048" : "330");
    if (details)
        urlQuery.addQueryItem("iiextmetadatafilter", "ImageDescription");
    urlQuery.addQueryItem("iiurlwidth", details ? "2048" : "330");
    urlQuery.addQueryItem("titles", imageTitles.join("|"));
    url.setQuery(urlQuery);

    QNetworkReply *reply = networkManager->get(QNetworkRequest(url));
    m_requests.track(reply, context);
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, imageTitles, pageData = std::move(pageData), context, completion = std::move(completion)]() mutable {
    if (!m_requests.isCurrent(context)) { reply->deleteLater(); return; }
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
        QJsonObject jsonObj = jsonDoc.object();
        if (jsonDoc.isNull() || jsonObj.contains("error")) {
            emit errorOccurred(jsonObj.contains("error")
                ? jsonObj["error"].toObject()["info"].toString()
                : tr("Invalid image metadata response."));
            reply->deleteLater();
            return;
        }
        QJsonObject pages = jsonObj["query"].toObject()["pages"].toObject();
        QHash<QString, QJsonObject> pagesByTitle;
        for (auto it = pages.constBegin(); it != pages.constEnd(); ++it) {
            const QJsonObject pageObj = it.value().toObject();
            pagesByTitle.insert(pageObj["title"].toString(), pageObj);
        }

        // Response page IDs do not preserve the requested gallery sequence.
        for (const QString &title : imageTitles) {
            const QJsonObject pageObj = pagesByTitle.value(title);
            if (pageObj.contains("imageinfo")) {
                QJsonArray imageInfo = pageObj["imageinfo"].toArray();
                if (!imageInfo.isEmpty()) {
                    QJsonObject info = imageInfo[0].toObject();
                    QString imageUrl = info["thumburl"].toString();
                    if (imageUrl.isEmpty())
                        imageUrl = info["url"].toString();
                    if (!imageUrl.isEmpty()) {
                        pageData.imageUrls.append(imageUrl);
                        pageData.imageTitles.append(pageObj["title"].toString());

                        // Extract the plain-text image description from
                        // extmetadata, if present.
                        QString description;
                        QJsonObject extmetadata = info["extmetadata"].toObject();
                        if (extmetadata.contains("ImageDescription")) {
                            description =
                                stripHtml(extmetadata["ImageDescription"].toObject().value("value").toString());
                        }
                        pageData.imageDescriptions.append(description);
                    }
                }
            }
        }
    } else {
        if (reply->error() != QNetworkReply::OperationCanceledError)
            emit errorOccurred(reply->errorString());
        reply->deleteLater();
        return;
    }
    completion(std::move(pageData));
    reply->deleteLater();
    });
}

void WikipediaPageClient::fetchPageContentWithImages(int pageid, const page &pageData, const WikipediaRequestContext &context) {
    if (!m_requests.isCurrent(context)) return;
    QUrl url(QString("https://%1.wikipedia.org/w/api.php").arg(context.language));
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("action", "parse");
    urlQuery.addQueryItem("format", "json");
    urlQuery.addQueryItem("prop", "text");
    urlQuery.addQueryItem("disableeditsection", "true");
    urlQuery.addQueryItem("formatversion", "2");
    urlQuery.addQueryItem("pageid", QString::number(pageid));
    url.setQuery(urlQuery);

    QNetworkReply *reply = networkManager->get(QNetworkRequest(url));
    m_requests.track(reply, context);
    connect(reply, &QNetworkReply::finished, this, [this, reply, pageData, context] {
        if (!m_requests.isCurrent(context)) { reply->deleteLater(); return; }
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonParseError parseError;
            const QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &parseError);
            if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
                emit errorOccurred(tr("Unable to load article: invalid parse response."));
                reply->deleteLater();
                return;
            }

            const QJsonObject jsonObj = jsonDoc.object();
            if (jsonObj.contains("error")) {
                const QJsonObject apiError = jsonObj.value("error").toObject();
                const QString code = apiError.value("code").toString();
                const QString info = apiError.value("info").toString();
                QString message = tr("Unable to load article: the Wikipedia API returned an error.");
                if (!code.isEmpty() || !info.isEmpty())
                    message = tr("Unable to load article: Wikipedia API error (%1): %2").arg(code, info);
                emit errorOccurred(message);
                reply->deleteLater();
                return;
            }

            const QJsonValue parseValue = jsonObj.value("parse");
            if (!parseValue.isObject() || !parseValue.toObject().value("text").isString()) {
                emit errorOccurred(tr("Unable to load article: parse response is missing article text."));
                reply->deleteLater();
                return;
            }
            const QString htmlContent = parseValue.toObject().value("text").toString();

            fetchMathImageDataUrls(htmlContent, context, [this, pageData, context](const QString &htmlContent) {
                if (!m_requests.isCurrent(context)) return;
                page page = pageData;
                page.extract = HtmlProcessor::processHtml(htmlContent);
                emit pageReceived(page);
            });
        } else if (reply->error() != QNetworkReply::OperationCanceledError) {
            emit errorOccurred(reply->errorString());
        }
        reply->deleteLater();
    });
}

void WikipediaPageClient::fetchMathImageDataUrls(const QString &htmlContent,
                                                  const WikipediaRequestContext &context,
                                                  std::function<void(const QString &)> completion) {
    if (!m_requests.isCurrent(context)) return;
    static const QRegularExpression imageTagRegex(R"(<img\b[^>]*>)",
                                                  QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression sourceRegex(R"(\bsrc\s*=\s*(["'])(.*?)\1)",
                                                 QRegularExpression::CaseInsensitiveOption);

    auto request = QSharedPointer<MathImageRequest>::create();
    request->htmlContent = htmlContent;
    request->completion = std::move(completion);

    QRegularExpressionMatchIterator imageTags = imageTagRegex.globalMatch(htmlContent);
    while (imageTags.hasNext()) {
        const QString imageTag = imageTags.next().captured();
        if (!imageTag.contains("mwe-math-fallback-image", Qt::CaseInsensitive)) {
            continue;
        }

        const QRegularExpressionMatch sourceMatch = sourceRegex.match(imageTag);
        if (!sourceMatch.hasMatch()) {
            continue;
        }

        const QString source = sourceMatch.captured(2);
        if (mathImageDataUrls.contains(source)) {
            request->dataUrls.insert(source, mathImageDataUrls.value(source));
            continue;
        }
        if (request->dataUrls.contains(source)) {
            continue;
        }

        request->dataUrls.insert(source, {});
        ++request->pendingRequests;

        QNetworkReply *mathReply = networkManager->get(QNetworkRequest(QUrl(source)));
        m_requests.track(mathReply, context);
        connect(mathReply, &QNetworkReply::finished, this, [this, mathReply, request, source, context] {
            if (!m_requests.isCurrent(context)) { mathReply->deleteLater(); return; }
            if (mathReply->error() == QNetworkReply::NoError) {
                const QString dataUrl = rasterizeSvg(mathReply->readAll());
                if (!dataUrl.isEmpty()) {
                    mathImageDataUrls.insert(source, dataUrl);
                    request->dataUrls[source] = dataUrl;
                }
            }
            mathReply->deleteLater();

            if (--request->pendingRequests != 0) {
                return;
            }

            for (auto it = request->dataUrls.cbegin(); it != request->dataUrls.cend(); ++it) {
                if (!it.value().isEmpty()) {
                    request->htmlContent.replace(it.key(), it.value());
                }
            }
            request->completion(request->htmlContent);
        });
    }

    if (request->pendingRequests == 0) {
        for (auto it = request->dataUrls.cbegin(); it != request->dataUrls.cend(); ++it) {
            if (!it.value().isEmpty()) {
                request->htmlContent.replace(it.key(), it.value());
            }
        }
        request->completion(request->htmlContent);
    }
}

void WikipediaPageClient::getSections(const QString &title) {
    QUrl url(baseUrl);
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("action", "parse");
    urlQuery.addQueryItem("format", "json");
    urlQuery.addQueryItem("prop", "tocdata");
    urlQuery.addQueryItem("page", title);
    url.setQuery(urlQuery);

    const auto context = m_requests.context();
    QNetworkReply *reply = networkManager->get(QNetworkRequest(url));
    m_requests.track(reply, context);
    connect(reply, &QNetworkReply::finished, this, [this, reply, context]() { this->onSectionsReply(reply, context); });
}

void WikipediaPageClient::onSectionsReply(QNetworkReply *reply, const WikipediaRequestContext &context) {
    if (m_requests.isCurrent(context) && reply->error() == QNetworkReply::NoError) {
        emit sectionsReceived(parseSections(reply->readAll()));
    } else if (m_requests.isCurrent(context) && reply->error() != QNetworkReply::OperationCanceledError) {
        emit errorOccurred(reply->errorString());
    }
    reply->deleteLater();
}

QVector<section> WikipediaPageClient::parseSections(const QByteArray &responseData) {
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
    QJsonObject jsonObj = jsonDoc.object();

    QVector<section> sections;

    if (jsonObj.contains("parse") && jsonObj["parse"].toObject().contains("tocdata")) {
        QJsonObject tocdata = jsonObj["parse"].toObject()["tocdata"].toObject();

        if (tocdata.contains("sections") && tocdata["sections"].isArray()) {
            QJsonArray sectionsArray = tocdata["sections"].toArray();

            for (const QJsonValueConstRef &sectionValue : std::as_const(sectionsArray)) {
                QJsonObject sectionObj = sectionValue.toObject();

                section sec;
                sec.title = sectionObj["line"].toString();
                sec.level = sectionObj["tocLevel"].toInt();
                sec.anchor = QUrl::fromPercentEncoding(sectionObj["anchor"].toString().toUtf8());
                sec.index = sectionObj["index"].toString().toInt();

                sections.append(sec);
            }
        }
    }

    return sections;
}

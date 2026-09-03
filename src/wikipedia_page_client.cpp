// wikipedia_page_client.cpp
#include "wikipedia_page_client.h"
#include "html_processor.h"
#include <QEventLoop>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>

static const QRegularExpression htmlTagRegex("<[^>]*>");

// Strip HTML tags and decode common entities so image descriptions render as
// plain text.
static QString stripHtml(const QString &html) {
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

WikipediaPageClient::WikipediaPageClient(QObject *parent)
    : QObject(parent), networkManager(new QNetworkAccessManager(this)) {
    baseUrl = "https://en.wikipedia.org/w/api.php";
}

WikipediaPageClient::~WikipediaPageClient() = default;

void WikipediaPageClient::getPage(const QString &title) {
    QUrl url(baseUrl);
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("action", "query");
    urlQuery.addQueryItem("format", "json");
    urlQuery.addQueryItem("prop", "extracts");
    urlQuery.addQueryItem("titles", title);
    urlQuery.addQueryItem("explaintext", "1");
    url.setQuery(urlQuery);

    QNetworkReply *reply = networkManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, title]() { this->onPageReply(reply, title); });
}

void WikipediaPageClient::onPageReply(QNetworkReply *reply, const QString &title) {
    if (reply->error() == QNetworkReply::NoError) {
        page p = parsePage(reply->readAll(), title);
        if (!p.title.isEmpty()) {
            emit pageReceived(p);
        }
    } else {
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
    urlQuery.addQueryItem("prop", "extracts|images");
    urlQuery.addQueryItem("pageids", QString::number(pageid));
    urlQuery.addQueryItem("explaintext", "1");
    urlQuery.addQueryItem("imlimit", "50");
    url.setQuery(urlQuery);

    QNetworkReply *reply = networkManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, pageid]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
            QJsonObject jsonObj = jsonDoc.object();
            QJsonObject pages = jsonObj["query"].toObject()["pages"].toObject();

            page page;
            QStringList imageTitles;

            for (auto it = pages.begin(); it != pages.end(); ++it) {
                if (it.key().toInt() == pageid) {
                    QJsonObject pageObj = it.value().toObject();
                    page.title = pageObj["title"].toString();
                    page.extract = pageObj["extract"].toString();
                    page.pageid = pageid;
                    page.imageUrls = QStringList();

                    if (pageObj.contains("images")) {
                        QJsonArray images = pageObj["images"].toArray();
                        for (const QJsonValue &image : std::as_const(images)) {
                            imageTitles.append(image.toObject().value("title").toString());
                        }
                    }
                    break;
                }
            }

            fetchPageContentWithImages(pageid, page);
        } else {
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

    QNetworkReply *reply = networkManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, pageid]() { this->onPageWithImagesReply(reply, pageid); });
}

void WikipediaPageClient::resolveTitleToPageId(const QString &title) {
    QUrl url(baseUrl);
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("action", "query");
    urlQuery.addQueryItem("format", "json");
    urlQuery.addQueryItem("titles", title);
    urlQuery.addQueryItem("prop", "pageprops|pageids");
    url.setQuery(urlQuery);

    QNetworkReply *reply = networkManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
            QJsonObject jsonObj = jsonDoc.object();
            QJsonObject pages = jsonObj["query"].toObject()["pages"].toObject();

            for (auto it = pages.begin(); it != pages.end(); ++it) {
                int pageid = it.value().toObject()["pageid"].toInt();
                if (pageid > 0) {
                    emit pageIdResolved(pageid);
                    break;
                }
            }
        } else {
            emit errorOccurred(reply->errorString());
        }
        reply->deleteLater();
    });
}

void WikipediaPageClient::onPageWithImagesReply(QNetworkReply *reply, int pageid) {
    if (reply->error() == QNetworkReply::NoError) {
        page p = parsePageWithImages(reply->readAll(), pageid);

        // parsePageWithImages stores image titles in imageUrls temporarily;
        // extract them and fetch the actual URLs
        QStringList imageTitles = p.imageUrls;
        p.imageUrls = QStringList();
        fetchImageUrlsFromTitles(imageTitles, p.imageUrls, p.imageDescriptions);
        emit pageWithImagesReceived(p);
    } else {
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
                for (const QJsonValue &image : std::as_const(images)) {
                    result.imageUrls.append(image.toObject().value("title").toString());
                }
            }
            break;
        }
    }
    return result;
}

void WikipediaPageClient::fetchImageUrlsFromTitles(const QStringList &imageTitles, QStringList &imageUrls,
                                                   QStringList &imageDescriptions) {
    if (imageTitles.isEmpty()) {
        return;
    }

    QEventLoop loop;
    QUrl url(baseUrl);
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("action", "query");
    urlQuery.addQueryItem("format", "json");
    urlQuery.addQueryItem("prop", "imageinfo");
    urlQuery.addQueryItem("iiprop", "url|extmetadata");
    urlQuery.addQueryItem("iiextmetadata", "ImageDescription");
    urlQuery.addQueryItem("titles", imageTitles.join("|"));
    url.setQuery(urlQuery);

    QNetworkReply *reply = networkManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
        QJsonObject jsonObj = jsonDoc.object();
        QJsonObject pages = jsonObj["query"].toObject()["pages"].toObject();

        for (auto it = pages.begin(); it != pages.end(); ++it) {
            QJsonObject pageObj = it.value().toObject();
            if (pageObj.contains("imageinfo")) {
                QJsonArray imageInfo = pageObj["imageinfo"].toArray();
                if (!imageInfo.isEmpty()) {
                    QJsonObject info = imageInfo[0].toObject();
                    QString imageUrl = info["url"].toString();
                    if (!imageUrl.isEmpty()) {
                        imageUrls.append(imageUrl);

                        // Extract the plain-text image description from
                        // extmetadata, if present.
                        QString description;
                        QJsonObject extmetadata = info["extmetadata"].toObject();
                        if (extmetadata.contains("ImageDescription")) {
                            description =
                                stripHtml(extmetadata["ImageDescription"].toObject().value("value").toString());
                        }
                        imageDescriptions.append(description);
                    }
                }
            }
        }
    }
    reply->deleteLater();
}

void WikipediaPageClient::fetchPageContentWithImages(int pageid, const page &pageData) {
    QUrl url(baseUrl);
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("action", "parse");
    urlQuery.addQueryItem("format", "json");
    urlQuery.addQueryItem("prop", "text");
    urlQuery.addQueryItem("disableeditsection", "true");
    urlQuery.addQueryItem("formatversion", "2");
    urlQuery.addQueryItem("pageid", QString::number(pageid));
    url.setQuery(urlQuery);

    QNetworkReply *reply = networkManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, pageData] {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
            QJsonObject jsonObj = jsonDoc.object();
            QJsonObject pages = jsonObj["parse"].toObject();

            page page = pageData;
            page.extract = HtmlProcessor::processHtml(pages["text"].toString());
            emit pageReceived(page);
        } else {
            emit errorOccurred(reply->errorString());
        }
        reply->deleteLater();
    });
}

void WikipediaPageClient::getSections(const QString &title) {
    QUrl url(baseUrl);
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("action", "parse");
    urlQuery.addQueryItem("format", "json");
    urlQuery.addQueryItem("prop", "tocdata");
    urlQuery.addQueryItem("page", title);
    url.setQuery(urlQuery);

    QNetworkReply *reply = networkManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, title]() { this->onSectionsReply(reply); });
}

void WikipediaPageClient::onSectionsReply(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        emit sectionsReceived(parseSections(reply->readAll()));
    } else {
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

            for (const QJsonValue &sectionValue : std::as_const(sectionsArray)) {
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

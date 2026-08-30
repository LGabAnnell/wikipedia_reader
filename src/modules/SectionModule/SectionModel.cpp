#include "SectionModel.h"
#include "wikipedia_page_client.h"

#include <iostream>

SectionModel::SectionModel(QObject *parent) : QObject(parent), m_isLoading(false) {
    // Get the WikipediaPageClient instance from GlobalState
    // We'll create it here for now, but ideally it should be shared
    m_pageClient = new WikipediaPageClient(this);

    connect(m_pageClient, &WikipediaPageClient::sectionsReceived, // NOLINT(clang-diagnostic-error)
            this, &SectionModel::handleSectionsReceived);
    connect(m_pageClient, &WikipediaPageClient::errorOccurred,
            this, &SectionModel::handleError);
}

QVector<section> SectionModel::sections() const {

  return m_sections;
}

bool SectionModel::isLoading() const {
    return m_isLoading;
}

QString SectionModel::errorMessage() const {
    return m_errorMessage;
}

void SectionModel::fetchSections(const QString &title) {
    if (!title.isEmpty()) {
        m_isLoading = true;
        m_errorMessage.clear();
        emit loadingChanged();
        emit errorChanged();
        
        m_pageClient->getSections(title);
    }
}

void SectionModel::clearSections() {
    m_sections.clear();
    m_errorMessage.clear();
    emit sectionsChanged();
    emit errorChanged();
}

void SectionModel::handleSectionsReceived(const QVector<section> &sections) {
    m_sections = sections;
    m_isLoading = false;
    emit sectionsChanged();
    emit loadingChanged();
}

void SectionModel::handleError(const QString &error) {
    m_errorMessage = error;
    m_isLoading = false;
    emit errorChanged();
    emit loadingChanged();
}

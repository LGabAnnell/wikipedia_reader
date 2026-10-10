#include "SectionModel.h"
#include "wikipedia_page_client.h"
#include "SettingsState.h"

SectionModel::SectionModel(QObject *parent) : QObject(parent), m_isLoading(false) {
    m_pageClient = new WikipediaPageClient(this);

    m_settings = SettingsState::instance();
    if (m_settings) {
        m_pageClient->setLanguage(m_settings->language());
        connect(m_settings, &SettingsState::languageChanged, this, [this]() {
            m_pageClient->setLanguage(m_settings->language());
        });
    }

    connect(m_pageClient, &WikipediaPageClient::sectionsReceived, // NOLINT(clang-diagnostic-error)
            this, &SectionModel::handleSectionsReceived);
    connect(m_pageClient, &WikipediaPageClient::errorOccurred, this, &SectionModel::handleError);
}

QVector<section> SectionModel::sections() const { return m_sections; }

bool SectionModel::isLoading() const { return m_isLoading; }

QString SectionModel::errorMessage() const { return m_errorMessage; }

void SectionModel::fetchSections(const QString &title) {
    if (title.isEmpty()) {
        clearSections();
        return;
    }
    m_sections.clear();
    m_errorMessage.clear();
    m_isLoading = true;
    m_acceptResponses = true;
    emit sectionsChanged();
    emit loadingChanged();
    emit errorChanged();
    m_pageClient->getSections(title);
}

void SectionModel::clearSections() {
    m_sections.clear();
    m_errorMessage.clear();
    m_isLoading = false;
    m_acceptResponses = false;
    emit loadingChanged();
    emit sectionsChanged();
    emit errorChanged();
}

void SectionModel::handleSectionsReceived(const QVector<section> &sections) {
    if (!m_acceptResponses)
        return;
    m_sections = sections;
    m_isLoading = false;
    emit sectionsChanged();
    emit loadingChanged();
}

void SectionModel::handleError(const QString &error) {
    if (!m_acceptResponses)
        return;
    m_errorMessage = error;
    m_isLoading = false;
    emit errorChanged();
    emit loadingChanged();
}

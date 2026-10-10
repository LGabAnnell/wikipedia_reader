#include "SettingsState.h"

QPointer<SettingsState> SettingsState::m_instance;

SettingsState::SettingsState(QObject *parent) : QObject(parent) {
    Q_ASSERT(!m_instance);
    m_instance = this;
}

void SettingsState::setLanguage(const QString &language) {
    if (m_language == language)
        return;
    m_language = language;
    emit languageChanged();
}

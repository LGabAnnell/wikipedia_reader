#include "SettingsState.h"
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

namespace {
QString settingsFilePath() {
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation))
        .filePath(QStringLiteral("settings.ini"));
}
} // namespace

QPointer<SettingsState> SettingsState::m_instance;

SettingsState::SettingsState(QObject *parent) : QObject(parent) {
    Q_ASSERT(!m_instance);
    m_instance = this;
    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    m_linkPreviewsEnabled = settings.value(QStringLiteral("content/linkPreviewsEnabled"), true).toBool();
}

void SettingsState::setLinkPreviewsEnabled(bool enabled) {
    if (m_linkPreviewsEnabled == enabled)
        return;
    const QString settingsFile = settingsFilePath();
    const QString configDirectory = QFileInfo(settingsFile).absolutePath();
    if (!QDir().mkpath(configDirectory))
        qWarning() << "Could not create settings directory:" << configDirectory;
    QSettings settings(settingsFile, QSettings::IniFormat);
    settings.setValue(QStringLiteral("content/linkPreviewsEnabled"), enabled);
    settings.sync();
    if (settings.status() != QSettings::NoError)
        qWarning() << "Could not save link preview preference:" << settings.fileName();
    m_linkPreviewsEnabled = enabled;
    emit linkPreviewsEnabledChanged();
}

void SettingsState::setLanguage(const QString &language) {
    if (m_language == language)
        return;
    m_language = language;
    emit languageTransitionStarted();
    emit languageChanged();
}

#ifndef SETTINGSSTATE_H
#define SETTINGSSTATE_H

#include <QObject>
#include <QPointer>
#include <QQmlEngine>

/** @brief Shared Wikipedia language and link-preview preferences. */
class SettingsState : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Singleton")
    Q_PROPERTY(QString language READ language NOTIFY languageChanged)
    Q_PROPERTY(bool linkPreviewsEnabled READ linkPreviewsEnabled WRITE setLinkPreviewsEnabled NOTIFY linkPreviewsEnabledChanged)

  public:
    explicit SettingsState(QObject *parent = nullptr);
    QString language() const { return m_language; }
    bool linkPreviewsEnabled() const { return m_linkPreviewsEnabled; }
    /** @brief Enable or disable article link-preview popups and persist the choice. */
    Q_INVOKABLE void setLinkPreviewsEnabled(bool enabled);
    static QPointer<SettingsState> instance() { return m_instance; }
    /** @brief Change the language without automatically reloading content. */
    Q_INVOKABLE void setLanguage(const QString &language);

  signals:
    void languageTransitionStarted();
    void languageChanged();
    void linkPreviewsEnabledChanged();

  private:
    QString m_language = "en";
    bool m_linkPreviewsEnabled = true;
    static QPointer<SettingsState> m_instance;
};

#endif // SETTINGSSTATE_H

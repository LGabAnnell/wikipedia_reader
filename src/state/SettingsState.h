#ifndef SETTINGSSTATE_H
#define SETTINGSSTATE_H

#include <QObject>
#include <QPointer>
#include <QQmlEngine>

/** @brief Shared Wikipedia language setting. */
class SettingsState : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Singleton")
    Q_PROPERTY(QString language READ language NOTIFY languageChanged)

  public:
    explicit SettingsState(QObject *parent = nullptr);
    QString language() const { return m_language; }
    static QPointer<SettingsState> instance() { return m_instance; }
    /** @brief Change the language without automatically reloading content. */
    Q_INVOKABLE void setLanguage(const QString &language);

  signals:
    void languageChanged();

  private:
    QString m_language = "en";
    static QPointer<SettingsState> m_instance;
};

#endif // SETTINGSSTATE_H

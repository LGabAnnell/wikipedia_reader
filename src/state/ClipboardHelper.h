#ifndef CLIPBOARDHELPER_H
#define CLIPBOARDHELPER_H

#include <QObject>
#include <QQmlEngine>

/** @brief Stateless clipboard operations callable from QML. */
class ClipboardHelper : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Singleton")

  public:
    explicit ClipboardHelper(QObject *parent = nullptr) : QObject(parent) {}
    /** @brief Copy text to the application's clipboard. */
    Q_INVOKABLE void copyToClipboard(const QString &text);
};

#endif // CLIPBOARDHELPER_H

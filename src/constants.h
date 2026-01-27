// Constants.h
#ifndef CONSTANTS_H
#define CONSTANTS_H
#include <QObject>
#include <QQmlEngine>

class Constants : public QObject {
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT
    Q_PROPERTY(QString historyView READ historyView CONSTANT)
    Q_PROPERTY(QString contentView READ contentView CONSTANT)
    Q_PROPERTY(QString searchView READ searchView CONSTANT)
    Q_PROPERTY(QString homeView READ homeView CONSTANT)

public:
    static Constants* create(QQmlEngine *, QJSEngine *) {
        if (!m_instance) {
            m_instance = new Constants();
        }
        return m_instance;
    }

    static QString historyView() {
        return "history";
    }

    static QString contentView() {
        return "content";
    }

    static QString searchView() {
        return "search";
    }

    static QString homeView() {
        return "home";
    }

    // Delete copy constructor and assignment operator to prevent copies
    Constants(const Constants&) = delete;
    Constants& operator=(const Constants&) = delete;

private:
    Constants() {};
    static Constants *m_instance;
};
#endif // CONSTANTS_H
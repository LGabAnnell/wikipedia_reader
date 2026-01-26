#ifndef CONTENTDISPLAY_H
#define CONTENTDISPLAY_H

#include <QObject>
#include <QString>
#include <QQmlEngine>
#include <QList>

struct SearchIndices {
    Q_GADGET
        QML_ELEMENT
        Q_PROPERTY(qsizetype start MEMBER start);
        Q_PROPERTY(qsizetype end MEMBER end);

public:
    qsizetype start;
    qsizetype end;
};

class ContentDisplayModel : public QObject
{
    Q_OBJECT
        QML_ELEMENT

public:
    ContentDisplayModel(QObject *parent = nullptr);

signals:
    /**
     * @brief Signal emitted when a search is performed.
     * @param searchText The text to search for.
     */
    void searchRequested(const QString &searchText, const QString &text);

    /**
     * @brief Signal emitted when search results are available.
     * @param results The list of search results.
     */
    void searchResultsAvailable(const QList<SearchIndices> &results);

private:
    // Helper method to connect search signals and slots
    void connectSearchSignals();

    // Helper method to perform the search
    QList<SearchIndices> searchForText(const QString &searchText, const QString &text);

    // List of items to search through
    QStringList m_items;
};

#endif // CONTENTDISPLAY_H
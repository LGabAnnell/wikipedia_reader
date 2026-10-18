// src/modules/HistoryModule/HistoryModel.h
#ifndef HISTORYMODEL_H
#define HISTORYMODEL_H

#include <QDateTime>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QVector>

#include "wikipedia_models.h"

class HistoryModel : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVector<history_item> history READ history NOTIFY historyChanged)

  public:
    explicit HistoryModel(QObject *parent = nullptr);

    // History property accessor
    QVector<history_item> history() const;

  public slots:
    /** @brief Record a visit identified by a nonblank Wikipedia language and page ID. */
    void addToHistory(const QString &title, int pageId, const QString &language);
    void clearHistory();
    void removeFromHistory(int index);

  signals:
    void historyChanged();

  private:
    QVector<history_item> m_history;
    static const int MAX_HISTORY_ITEMS = 50;

    // Helper method to check if item already exists in history
    bool itemExistsInHistory(int pageId, const QString &language);
};

#endif // HISTORYMODEL_H
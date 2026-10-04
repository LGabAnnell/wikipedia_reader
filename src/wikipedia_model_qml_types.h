#ifndef WIKIPEDIA_MODEL_QML_TYPES_H
#define WIKIPEDIA_MODEL_QML_TYPES_H

#include "wikipedia_models.h"
#include <QtQml/qqmlregistration.h>

// Register the backend value types in wikipedia_qt without compiling their
// meta-objects again or registering the explicitly managed state singletons.
struct SearchResultQmlType {
    Q_GADGET
    QML_FOREIGN(search_result)
    QML_NAMED_ELEMENT(search_result)
};

struct PageQmlType {
    Q_GADGET
    QML_FOREIGN(page)
    QML_NAMED_ELEMENT(page)
};

struct FeaturedArticleQmlType {
    Q_GADGET
    QML_FOREIGN(featured_article)
    QML_NAMED_ELEMENT(featured_article)
};

struct NewsItemQmlType {
    Q_GADGET
    QML_FOREIGN(news_item)
    QML_NAMED_ELEMENT(news_item)
};

struct OnThisDayEventQmlType {
    Q_GADGET
    QML_FOREIGN(on_this_day_event)
    QML_NAMED_ELEMENT(on_this_day_event)
};

struct DidYouKnowItemQmlType {
    Q_GADGET
    QML_FOREIGN(did_you_know_item)
    QML_NAMED_ELEMENT(did_you_know_item)
};

struct HistoryItemQmlType {
    Q_GADGET
    QML_FOREIGN(history_item)
    QML_NAMED_ELEMENT(history_item)
};

struct SectionQmlType {
    Q_GADGET
    QML_FOREIGN(section)
    QML_NAMED_ELEMENT(section)
};

#endif // WIKIPEDIA_MODEL_QML_TYPES_H

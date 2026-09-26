#include "html_processor.h"

#include <QFile>
#include <QPalette>
#include <QTextDocument>
#include <QUrl>

#include <lexbor/html/html.h>
#include <lexbor/dom/interfaces/character_data.h>

#include <memory>
#include <limits>
#include <new>
#include <vector>

namespace {
using Node = lxb_dom_node_t;
using Element = lxb_dom_element_t;

struct DocumentDeleter {
    void operator()(lxb_html_document_t *document) const {
        if (document != nullptr)
            lxb_html_document_destroy(document);
    }
};
using Document = std::unique_ptr<lxb_html_document_t, DocumentDeleter>;

QString tagName(Node *node) {
    if (node == nullptr || node->type != LXB_DOM_NODE_TYPE_ELEMENT)
        return {};
    size_t length = 0;
    const lxb_char_t *name = lxb_dom_element_local_name(lxb_dom_interface_element(node), &length);
    return name == nullptr ? QString() : QString::fromUtf8(reinterpret_cast<const char *>(name),
                                                            qsizetype(length));
}

QString attribute(Element *element, const char *name) {
    size_t length = 0;
    const auto *value = lxb_dom_element_get_attribute(
        element, reinterpret_cast<const lxb_char_t *>(name), std::char_traits<char>::length(name),
        &length);
    return value == nullptr ? QString() : QString::fromUtf8(reinterpret_cast<const char *>(value),
                                                             qsizetype(length));
}

bool setAttribute(Element *element, const char *name, const QByteArray &value) {
    return lxb_dom_element_set_attribute(element, reinterpret_cast<const lxb_char_t *>(name),
                                         std::char_traits<char>::length(name),
                                         reinterpret_cast<const lxb_char_t *>(value.constData()),
                                         size_t(value.size())) != nullptr;
}

void collectNodes(Node *root, std::vector<Node *> &nodes) {
    for (Node *child = root == nullptr ? nullptr : root->first_child; child != nullptr;) {
        Node *next = child->next;
        nodes.push_back(child);
        collectNodes(child, nodes);
        child = next;
    }
}

bool hasClassToken(Element *element, const QString &token) {
    const QString classes = attribute(element, "class");
    qsizetype index = 0;
    while (index < classes.size()) {
        while (index < classes.size() && classes.at(index).isSpace())
            ++index;
        const qsizetype start = index;
        while (index < classes.size() && !classes.at(index).isSpace())
            ++index;
        if (classes.mid(start, index - start) == token)
            return true;
    }
    return false;
}

bool hasClassPrefix(Element *element, const QString &prefix) {
    const QString classes = attribute(element, "class");
    qsizetype index = 0;
    while (index < classes.size()) {
        while (index < classes.size() && classes.at(index).isSpace())
            ++index;
        const qsizetype start = index;
        while (index < classes.size() && !classes.at(index).isSpace())
            ++index;
        if (classes.mid(start, index - start).startsWith(prefix))
            return true;
    }
    return false;
}

bool isMathImage(Element *image) {
    return hasClassPrefix(image, QStringLiteral("mwe-math-fallback-image"));
}

bool containsMathA11yClass(Element *element) {
    const QString classes = attribute(element, "class");
    qsizetype index = 0;
    while (index < classes.size()) {
        while (index < classes.size() && classes.at(index).isSpace())
            ++index;
        const qsizetype start = index;
        while (index < classes.size() && !classes.at(index).isSpace())
            ++index;
        const QString name = classes.mid(start, index - start);
        if (name.startsWith(QStringLiteral("mwe-math-mathml-")))
            return true;
    }
    return false;
}

bool hasAncestorTag(Node *node, const QString &name) {
    for (Node *parent = node->parent; parent != nullptr; parent = parent->parent) {
        if (tagName(parent) == name)
            return true;
    }
    return false;
}

bool isConnected(Node *node, Node *root) {
    for (Node *current = node; current != nullptr; current = current->parent) {
        if (current == root)
            return true;
    }
    return false;
}

bool removeNodes(Node *root) {
    std::vector<Node *> nodes;
    collectNodes(root, nodes);
    for (Node *node : nodes) {
        if (node->parent == nullptr || node->type != LXB_DOM_NODE_TYPE_ELEMENT)
            continue;
        const QString name = tagName(node);
        Element *element = lxb_dom_interface_element(node);
        if (name == QStringLiteral("style")
            || (name == QStringLiteral("span") && containsMathA11yClass(element))) {
            lxb_dom_node_remove(node);
        } else {
            if (lxb_dom_element_remove_attribute(element,
                    reinterpret_cast<const lxb_char_t *>("style"), 5) != LXB_STATUS_OK)
                return false;
        }
    }
    return true;
}

int countImages(Node *root, Element **found = nullptr) {
    int count = 0;
    std::vector<Node *> nodes;
    nodes.push_back(root);
    while (!nodes.empty()) {
        Node *node = nodes.back();
        nodes.pop_back();
        if (tagName(node) == QStringLiteral("img")) {
            auto *image = lxb_dom_interface_element(node);
            bool magnifyControl = false;
            for (Node *parent = node->parent; parent != nullptr; parent = parent->parent) {
                if (tagName(parent) == QStringLiteral("div")
                    && hasClassToken(lxb_dom_interface_element(parent), QStringLiteral("magnify"))) {
                    magnifyControl = true;
                    break;
                }
            }
            if (!magnifyControl) {
                ++count;
                if (found != nullptr && !isMathImage(image))
                    *found = image;
            }
        }
        for (Node *child = node->first_child; child != nullptr; child = child->next)
            nodes.push_back(child);
    }
    return count;
}

bool onlyImageWrappers(Node *node, Element *targetImage) {
    const QString name = tagName(node);
    if (node == lxb_dom_interface_node(targetImage))
        return true;
    if (node->type == LXB_DOM_NODE_TYPE_TEXT) {
        auto *text = reinterpret_cast<lxb_dom_character_data_t *>(node);
        return text->data.length == 0
               || QString::fromUtf8(reinterpret_cast<const char *>(text->data.data),
                                    qsizetype(text->data.length)).trimmed().isEmpty();
    }
    if (node->type == LXB_DOM_NODE_TYPE_COMMENT)
        return true;
    if (name != QStringLiteral("a") && name != QStringLiteral("div")
        && name != QStringLiteral("p"))
        return false;
    for (Node *child = node->first_child; child != nullptr; child = child->next) {
        if (!onlyImageWrappers(child, targetImage))
            return false;
    }
    return true;
}

QString serialize(Node *node, bool deep = true) {
    lexbor_str_t output = {0};
    const lxb_status_t status = deep ? lxb_html_serialize_deep_str(node, &output)
                                     : lxb_html_serialize_tree_str(node, &output);
    if (status != LXB_STATUS_OK || output.data == nullptr) {
        if (output.data != nullptr)
            lexbor_str_destroy(&output, node->owner_document->text, false);
        return {};
    }
    QString result = QString::fromUtf8(reinterpret_cast<const char *>(output.data),
                                       qsizetype(output.length));
    lexbor_str_destroy(&output, node->owner_document->text, false);
    return result;
}

bool visibleText(Node *node, QString &result) {
    const QString markup = serialize(node);
    if (markup.isNull())
        return false;
    QTextDocument document;
    document.setHtml(markup);
    result = document.toPlainText().trimmed();
    return true;
}

Node *makeElement(lxb_html_document_t *document, const char *name) {
    auto *element = lxb_html_document_create_element(
        document, reinterpret_cast<const lxb_char_t *>(name), std::char_traits<char>::length(name),
        nullptr);
    return element == nullptr ? nullptr : lxb_dom_interface_node(element);
}

Node *makeText(lxb_html_document_t *document, const QString &text) {
    const QByteArray utf8 = text.toUtf8();
    return reinterpret_cast<Node *>(lxb_dom_document_create_text_node(&document->dom_document,
        reinterpret_cast<const lxb_char_t *>(utf8.constData()), size_t(utf8.size())));
}

bool attach(Node *parent, Node *child) {
    if (parent == nullptr || child == nullptr)
        return false;
    lxb_dom_node_insert_child_wo_events(parent, child);
    return true;
}

Node *linkedImageRoot(Element *image) {
    Node *node = lxb_dom_interface_node(image);
    Node *parent = node->parent;
    while (parent != nullptr && tagName(parent) != QStringLiteral("a")) {
        if (tagName(parent) != QStringLiteral("div"))
            break;
        parent = parent->parent;
    }
    return parent != nullptr && tagName(parent) == QStringLiteral("a") ? parent : node;
}

Node *findCaption(Node *root, const QString &tag, const QString &classToken = {}) {
    std::vector<Node *> nodes;
    nodes.push_back(root);
    while (!nodes.empty()) {
        Node *node = nodes.back();
        nodes.pop_back();
        if (tagName(node) == tag
            && (classToken.isEmpty() || hasClassToken(lxb_dom_interface_element(node), classToken)))
            return node;
        for (Node *child = node->first_child; child != nullptr; child = child->next)
            nodes.push_back(child);
    }
    return nullptr;
}

bool replaceWithImageTable(lxb_html_document_t *document, Node *candidate, Element *image,
                           Node *caption) {
    Node *parent = candidate->parent;
    if (parent == nullptr)
        return false;

    if (caption != nullptr) {
        std::vector<Node *> captionNodes;
        collectNodes(caption, captionNodes);
        for (Node *node : captionNodes) {
            if (tagName(node) == QStringLiteral("div")
                && hasClassToken(lxb_dom_interface_element(node), QStringLiteral("magnify")))
                lxb_dom_node_remove(node);
        }
    }

    QString captionText;
    if (caption != nullptr && !visibleText(caption, captionText))
        return false;
    bool useAlt = captionText.isEmpty();
    if (useAlt)
        captionText = attribute(image, "alt").trimmed();
    QString captionMarkup = caption == nullptr || useAlt ? QString() : serialize(caption);
    if (caption != nullptr && !useAlt && captionMarkup.isNull())
        return false;

    const QByteArray encodedDescription = QUrl::toPercentEncoding(captionText);
    if (!setAttribute(image, "data-article-description", encodedDescription))
        return false;

    bool widthOk = false;
    const qint64 imageWidth = attribute(image, "width").toLongLong(&widthOk);
    QByteArray tableClass("article-image-table");
    Node *table = makeElement(document, "table");
    Node *imageRow = makeElement(document, "tr");
    Node *imageCell = makeElement(document, "td");
    Node *captionRow = makeElement(document, "tr");
    Node *captionCell = makeElement(document, "td");
    if (!table || !imageRow || !imageCell || !captionRow || !captionCell)
        return false;

    Element *tableElement = lxb_dom_interface_element(table);
    if (!setAttribute(tableElement, "class", tableClass)
        || !setAttribute(tableElement, "border", QByteArray("1"))
        || !setAttribute(tableElement, "cellspacing", QByteArray("0")))
        return false;
    if (widthOk && imageWidth > 0
        && imageWidth <= std::numeric_limits<qint64>::max() - 20)
        if (!setAttribute(tableElement, "width", QByteArray::number(imageWidth + 20)))
            return false;
    if (!setAttribute(lxb_dom_interface_element(imageCell), "align", QByteArray("center")))
        return false;

    Node *media = linkedImageRoot(image);
    lxb_dom_node_remove(media);
    if (!attach(imageCell, media) || !attach(imageRow, imageCell)
        || !attach(captionRow, captionCell) || !attach(table, imageRow)
        || !attach(table, captionRow))
        return false;

    if (useAlt && !captionText.isEmpty()) {
        if (!attach(captionCell, makeText(document, captionText)))
            return false;
    } else if (!captionMarkup.isEmpty()) {
        // Keep the actual caption nodes, preserving links and emphasis markup.
        while (caption->first_child != nullptr) {
            Node *child = caption->first_child;
            lxb_dom_node_remove(child);
            if (!attach(captionCell, child))
                return false;
        }
    }

    if (caption != nullptr && caption->parent != nullptr)
        lxb_dom_node_remove(caption);
    if (candidate->parent == nullptr)
        return false;
    lxb_dom_node_insert_before(candidate, table);
    QString remainingText;
    if (!visibleText(candidate, remainingText))
        return false;
    if (remainingText.isEmpty() && countImages(candidate) == 0)
        lxb_dom_node_remove(candidate);
    return true;
}

bool normalizeBlockImages(lxb_html_document_t *document, Node *root) {
    std::vector<Node *> nodes;
    collectNodes(root, nodes);
    for (Node *node : nodes) {
        if (!isConnected(node, root) || hasAncestorTag(node, QStringLiteral("table")))
            continue;
        const QString name = tagName(node);
        if (name != QStringLiteral("figure") && name != QStringLiteral("p")
            && name != QStringLiteral("div"))
            continue;

        const bool figure = name == QStringLiteral("figure");
        const bool thumbnail = name == QStringLiteral("div")
            && hasClassToken(lxb_dom_interface_element(node), QStringLiteral("thumb"));
        Element *image = nullptr;
        if (countImages(node, &image) != 1 || image == nullptr)
            continue;

        Node *caption = figure ? findCaption(node, QStringLiteral("figcaption"))
                              : thumbnail ? findCaption(node, QStringLiteral("div"),
                                                         QStringLiteral("thumbcaption"))
                                          : nullptr;
        if (figure || thumbnail) {
            if (!replaceWithImageTable(document, node, image, caption))
                return false;
            continue;
        }

        if (isMathImage(image) || !onlyImageWrappers(node, image))
            continue;
        if (!replaceWithImageTable(document, node, image, nullptr))
            return false;
    }
    return true;
}

bool updateImages(Node *root) {
    std::vector<Node *> nodes;
    collectNodes(root, nodes);
    for (Node *node : nodes) {
        if (tagName(node) != QStringLiteral("img"))
            continue;
        Element *image = lxb_dom_interface_element(node);
        QString source = attribute(image, "src");
        if (source.startsWith(QStringLiteral("//"))) {
            source.prepend(QStringLiteral("https:"));
            if (!setAttribute(image, "src", source.toUtf8()))
                return false;
        }
        if (isMathImage(image))
            continue;
        if (!setAttribute(image, "style", QByteArray("max-width: 100%; height: auto;")))
            return false;
    }
    return true;
}

QString withApplicationStyle(const QString &content) {
    QFile css(QStringLiteral(":/styles/table_style.css"));
    if (!css.open(QIODevice::ReadOnly))
        return content;
    const QString style = QString::fromUtf8(css.readAll()).arg(QPalette().text().color().name());
    return QStringLiteral("<style>\n%1\n</style>").arg(style) + content;
}

QString processWithLexbor(const QString &html) {
    const QByteArray utf8 = html.toUtf8();
    Document document(lxb_html_document_create());
    if (!document)
        return {};
    lxb_html_document_scripting_set(document.get(), false);
    Node *contextNode = makeElement(document.get(), "body");
    if (contextNode == nullptr)
        return {};
    Node *root = lxb_html_document_parse_fragment(document.get(),
        lxb_dom_interface_element(contextNode),
        reinterpret_cast<const lxb_char_t *>(utf8.constData()), size_t(utf8.size()));
    if (root == nullptr)
        return {};
    if (!removeNodes(root))
        return {};
    if (!normalizeBlockImages(document.get(), root) || !updateImages(root))
        return {};
    lexbor_str_t output = {0};
    if (lxb_html_serialize_deep_str(root, &output) != LXB_STATUS_OK || output.data == nullptr) {
        if (output.data != nullptr)
            lexbor_str_destroy(&output, document->dom_document.text, false);
        return {};
    }
    const QString serialized = QString::fromUtf8(reinterpret_cast<const char *>(output.data),
                                                 qsizetype(output.length));
    lexbor_str_destroy(&output, document->dom_document.text, false);
    return serialized;
}
} // namespace

QString HtmlProcessor::processHtml(const QString &htmlContent) {
    QString processed;
    try {
        processed = processWithLexbor(htmlContent);
    } catch (const std::bad_alloc &) {
        qWarning("Lexbor allocation failed while processing article HTML");
        return withApplicationStyle(htmlContent);
    }
    if (processed.isNull()) {
        qWarning("Lexbor failed to process article HTML; returning the original fragment");
        return withApplicationStyle(htmlContent);
    }
    return withApplicationStyle(processed);
}

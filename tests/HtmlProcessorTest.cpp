#include <QtTest/QtTest>

#include <memory>
#include <vector>

#include <lexbor/html/html.h>

#include "html_processor.h"

namespace {
using Node = lxb_dom_node_t;

struct DocumentDeleter {
    void operator()(lxb_html_document_t *document) const {
        if (document)
            lxb_html_document_destroy(document);
    }
};

struct ParsedFragment {
    std::unique_ptr<lxb_html_document_t, DocumentDeleter> document;
    Node *root = nullptr;
};

ParsedFragment parse(const QString &html) {
    ParsedFragment parsed{std::unique_ptr<lxb_html_document_t, DocumentDeleter>(
                              lxb_html_document_create()), nullptr};
    if (!parsed.document)
        return parsed;
    lxb_html_document_scripting_set(parsed.document.get(), false);
    auto *context = lxb_html_document_create_element(parsed.document.get(),
        reinterpret_cast<const lxb_char_t *>("body"), 4, nullptr);
    if (!context)
        return parsed;
    const QByteArray utf8 = html.toUtf8();
    parsed.root = lxb_html_document_parse_fragment(parsed.document.get(),
        lxb_dom_interface_element(context),
        reinterpret_cast<const lxb_char_t *>(utf8.constData()), size_t(utf8.size()));
    return parsed;
}

QString name(Node *node) {
    if (!node || node->type != LXB_DOM_NODE_TYPE_ELEMENT)
        return {};
    size_t length = 0;
    const auto *tag = lxb_dom_element_local_name(lxb_dom_interface_element(node), &length);
    return QString::fromUtf8(reinterpret_cast<const char *>(tag), qsizetype(length));
}

QString attr(Node *node, const char *key) {
    if (!node || node->type != LXB_DOM_NODE_TYPE_ELEMENT)
        return {};
    size_t length = 0;
    const auto *value = lxb_dom_element_get_attribute(lxb_dom_interface_element(node),
        reinterpret_cast<const lxb_char_t *>(key), qstrlen(key), &length);
    return value ? QString::fromUtf8(reinterpret_cast<const char *>(value), qsizetype(length))
                 : QString();
}

QString text(Node *node) {
    size_t length = 0;
    const auto *value = lxb_dom_node_text_content(node, &length);
    return value ? QString::fromUtf8(reinterpret_cast<const char *>(value), qsizetype(length))
                 : QString();
}

void descendants(Node *root, const QString &tag, std::vector<Node *> &result) {
    for (Node *child = root ? root->first_child : nullptr; child; child = child->next) {
        if (name(child) == tag)
            result.push_back(child);
        descendants(child, tag, result);
    }
}

int count(Node *root, const QString &tag) {
    std::vector<Node *> found;
    descendants(root, tag, found);
    return int(found.size());
}

Node *first(Node *root, const QString &tag) {
    std::vector<Node *> found;
    descendants(root, tag, found);
    return found.empty() ? nullptr : found.front();
}

ParsedFragment processed(const QString &source) {
    return parse(HtmlProcessor::processHtml(source));
}
} // namespace

class HtmlProcessorTest : public QObject {
    Q_OBJECT

private slots:
    void removesSourceStylesAndPreservesArticleText() {
        const auto result = processed(QStringLiteral(
            "<style>body{color:red}</style><p style=color:red>Before</p>"
            "<div><span style='font-weight:bold'>After</span></div>"));
        QVERIFY(result.root);
        QCOMPARE(count(result.root, QStringLiteral("style")), 0);
        QCOMPARE(count(result.root, QStringLiteral("p")), 1);
        QCOMPARE(text(result.root).trimmed(), QStringLiteral("BeforeAfter"));
    }

    void normalizesLegacyHtmlHexColorAttributes() {
        const auto result = processed(QStringLiteral(
            "<table bgcolor=F7F6A8 bordercolor=123456><tr><td>Cell</td></tr></table>"
            "<font color=abcdef>Text</font>"));
        QVERIFY(result.root);
        QCOMPARE(attr(first(result.root, QStringLiteral("table")), "bgcolor"),
                 QStringLiteral("#F7F6A8"));
        QCOMPARE(attr(first(result.root, QStringLiteral("table")), "bordercolor"),
                 QStringLiteral("#123456"));
        QCOMPARE(attr(first(result.root, QStringLiteral("font")), "color"),
                 QStringLiteral("#abcdef"));
    }

    void removesLinkAndScriptElementsThatTruncateRichTextImport() {
        const auto result = processed(QStringLiteral(
            "<p>Before</p>"
            "<link rel=\"mw-deduplicated-inline-style\" href=\"mw-data:TemplateStyles:r1\">"
            "<script>ignore()</script>"
            "<p>After</p>"));
        QVERIFY(result.root);
        QCOMPARE(count(result.root, QStringLiteral("link")), 0);
        QCOMPARE(count(result.root, QStringLiteral("script")), 0);
        QCOMPARE(text(result.root).trimmed(), QStringLiteral("BeforeAfter"));
    }

    void convertsFigureAndKeepsLinkDimensionsAndCaptionMarkup() {
        const auto result = processed(QStringLiteral(
            "<h2>Heading</h2><figure><a href=/wiki/File:House.jpg>"
            "<img alt='House alt' src=//upload.wikimedia.org/house.jpg width=250 height=180>"
            "</a><figcaption>The <b>old</b> house &amp; garden</figcaption></figure><p>Next</p>"));
        Node *table = first(result.root, QStringLiteral("table"));
        QVERIFY(table);
        QCOMPARE(attr(table, "class"), QStringLiteral("article-image-table"));
        QCOMPARE(attr(table, "width"), QStringLiteral("270"));
        QCOMPARE(count(table, QStringLiteral("tr")), 2);
        QCOMPARE(count(table, QStringLiteral("td")), 2);
        Node *image = first(table, QStringLiteral("img"));
        QVERIFY(image);
        QCOMPARE(attr(image, "src"), QStringLiteral("https://upload.wikimedia.org/house.jpg"));
        QCOMPARE(attr(image, "width"), QStringLiteral("250"));
        QCOMPARE(attr(image, "height"), QStringLiteral("180"));
        QCOMPARE(attr(first(table, QStringLiteral("a")), "href"),
                 QStringLiteral("/wiki/File:House.jpg"));
        QCOMPARE(text(table).trimmed(), QStringLiteral("The old house & garden"));
        QVERIFY(first(table, QStringLiteral("b")));
        QCOMPARE(text(result.root).indexOf(QStringLiteral("Heading")) <
                     text(result.root).indexOf(QStringLiteral("The old house")), true);
        QVERIFY(text(result.root).endsWith(QStringLiteral("Next")));
    }

    void captionFallbackIsLiteralAndHasEncodedDescription() {
        const auto result = processed(QStringLiteral(
            "<figure><img alt='A < B &amp; C' src=x.jpg><figcaption></figcaption></figure>"));
        Node *table = first(result.root, QStringLiteral("table"));
        QVERIFY(table);
        Node *image = first(table, QStringLiteral("img"));
        QCOMPARE(attr(image, "alt"), QStringLiteral("A < B & C"));
        QCOMPARE(attr(image, "data-article-description"), QStringLiteral("A%20%3C%20B%20%26%20C"));
        QCOMPARE(text(table).trimmed(), QStringLiteral("A < B & C"));
    }

    void leavesEmptyCaptionRowWhenAltIsMissing() {
        const auto result = processed(QStringLiteral(
            "<figure><img src=uncaptioned.jpg><figcaption></figcaption></figure>"));
        Node *table = first(result.root, QStringLiteral("table"));
        QVERIFY(table);
        QCOMPARE(count(table, QStringLiteral("tr")), 2);
        QCOMPARE(attr(first(table, QStringLiteral("img")), "data-article-description"), QString());
        QCOMPARE(text(table).trimmed(), QString());
    }

    void convertsLegacyThumbnailAndOmitsMagnificationControl() {
        const auto result = processed(QStringLiteral(
            "<div class='thumb tright'><div class=thumbinner><a href=/wiki/File:Old.jpg>"
            "<img alt='Old alt' src=old.jpg></a><div class=thumbcaption>Older caption"
            "<div class=magnify><img src=zoom.svg></div></div></div></div>"));
        Node *table = first(result.root, QStringLiteral("table"));
        QVERIFY(table);
        QCOMPARE(count(table, QStringLiteral("img")), 1);
        QCOMPARE(text(table).trimmed(), QStringLiteral("Older caption"));
    }

    void standaloneImageBlocksConvertButInlineAndTableImagesDoNot() {
        const auto result = processed(QStringLiteral(
            "<div class=center><div class=floatnone><a href=/wiki/File:C.jpg>"
            "<img alt=Centered src=center.jpg></a></div></div>"
            "<p>Text <img src=inline.jpg> continues</p>"
            "<table><tr><td><img src=table.jpg></td></tr></table>"));
        QCOMPARE(count(result.root, QStringLiteral("table")), 2); // Image table + source table.
        QVERIFY(text(result.root).contains(QStringLiteral("Text  continues")));
        QCOMPARE(count(result.root, QStringLiteral("img")), 3);
    }

    void multipleMediaImagesKeepFigureLayout() {
        const auto result = processed(QStringLiteral(
            "<figure><img src=one.jpg><img src=two.jpg><figcaption>Gallery</figcaption></figure>"));
        QCOMPARE(count(result.root, QStringLiteral("table")), 0);
        QCOMPARE(count(result.root, QStringLiteral("figure")), 1);
        QCOMPARE(count(result.root, QStringLiteral("img")), 2);
    }

    void cleansNestedMathAccessibilityAndKeepsFallbackImageUnstyled() {
        const auto result = processed(QStringLiteral(
            "<span class='mwe-math-element'><span class='mwe-math-mathml-inline'>"
            "<span class='mwe-math-mathml-a11y'><math><annotation>x^2</annotation></math>"
            "</span></span><img class='mwe-math-fallback-image-inline' src=math.svg "
            "style='width:2ex'><img src=article.jpg></span><p>Neighbor</p>"));
        QCOMPARE(count(result.root, QStringLiteral("math")), 0);
        QVERIFY(!text(result.root).contains(QStringLiteral("x^2")));
        Node *mathImage = first(result.root, QStringLiteral("img"));
        QVERIFY(mathImage);
        QCOMPARE(attr(mathImage, "style"), QString());
        std::vector<Node *> images;
        descendants(result.root, QStringLiteral("img"), images);
        QCOMPARE(images.size(), size_t(2));
        QCOMPARE(attr(images.at(1), "style"), QStringLiteral("max-width: 100%; height: auto;"));
        QVERIFY(text(result.root).contains(QStringLiteral("Neighbor")));
    }

    void parserRepairsVoidEntitiesUnquotedAttrsAndOmittedClosers() {
        const auto result = processed(QStringLiteral(
            "<p>First&nbsp;line<p>Second &#x1F600;"
            "<figure><img alt=Coffee src=coffee.jpg width=100>"
            "<figcaption>Caf&eacute; &amp; tea"));
        QCOMPARE(text(result.root).contains(QStringLiteral("First lineSecond 😀Café & tea")), true);
        Node *table = first(result.root, QStringLiteral("table"));
        QVERIFY(table);
        QCOMPARE(attr(table, "width"), QStringLiteral("120"));
        QCOMPARE(text(table).trimmed(), QStringLiteral("Café & tea"));
    }

    void commentsAndTagLikeAttributeTextAreNotParsedAsMarkup() {
        const auto result = processed(QStringLiteral(
            "<!-- <style>comment</style><img src=bad> -->"
            "<p title='literal <style>text'>Survives</p>"));
        QVERIFY(text(result.root).contains(QStringLiteral("Survives")));
        QCOMPARE(count(result.root, QStringLiteral("img")), 0);
        QCOMPARE(count(result.root, QStringLiteral("style")), 0);
        QCOMPARE(attr(first(result.root, QStringLiteral("p")), "title"),
                 QStringLiteral("literal <style>text"));
    }

    void preservesInlineImagesAndNormalizesUrls() {
        const auto result = processed(QStringLiteral(
            "<p>Before<img src=//example.org/a.jpg>After</p>"));
        QCOMPARE(count(result.root, QStringLiteral("table")), 0);
        Node *image = first(result.root, QStringLiteral("img"));
        QCOMPARE(attr(image, "src"), QStringLiteral("https://example.org/a.jpg"));
        QCOMPARE(attr(image, "style"), QStringLiteral("max-width: 100%; height: auto;"));
        QVERIFY(text(result.root).contains(QStringLiteral("BeforeAfter")));
    }

    void figureContentOutsideCaptionAndImageSurvivesConversion() {
        const auto result = processed(QStringLiteral(
            "<figure>Introductory words <img src=figure.jpg> trailing note"
            "<figcaption>Image caption</figcaption></figure><p>Neighbor remains</p>"));
        QVERIFY(first(result.root, QStringLiteral("table")));
        QVERIFY(text(result.root).contains(QStringLiteral("Introductory words")));
        QVERIFY(text(result.root).contains(QStringLiteral("trailing note")));
        QVERIFY(text(result.root).contains(QStringLiteral("Neighbor remains")));
    }
};

QTEST_MAIN(HtmlProcessorTest)
#include "HtmlProcessorTest.moc"

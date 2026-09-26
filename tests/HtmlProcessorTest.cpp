// tests/HtmlProcessorTest.cpp
#include <QtTest/QtTest>
#include <QRegularExpression>
#include "html_processor.h"

class HtmlProcessorTest : public QObject {
    Q_OBJECT

    static QStringList imageTables(const QString &html) {
        static const QRegularExpression tableRegex(
            R"(<table\b[^>]*class="article-image-table"[^>]*>[\s\S]*?</table>)",
            QRegularExpression::CaseInsensitiveOption);
        QStringList tables;
        auto matches = tableRegex.globalMatch(html);
        while (matches.hasNext())
            tables.append(matches.next().captured());
        return tables;
    }

    static void verifyTwoRowImageTable(const QString &table, const QString &imageSource,
                                       const QString &description) {
        QVERIFY(table.contains("border=\"1\""));
        QCOMPARE(table.count(QRegularExpression(R"(<tr\b)")), 2);
        QCOMPARE(table.count(QRegularExpression(R"(<td\b)")), 2);
        QCOMPARE(table.count(QRegularExpression(R"(<img\b)")), 1);
        const int firstRowEnd = table.indexOf(QLatin1String("</tr>"));
        QVERIFY(firstRowEnd > 0);
        QVERIFY(table.left(firstRowEnd).contains(imageSource));
        QVERIFY(table.mid(firstRowEnd).contains(description));
    }

private slots:
    void testEmpty_data() {
        QTest::addColumn<QString>("input");
        QTest::newRow("empty") << "";
    }

    void testEmpty() {
        QFETCH(QString, input);
        QString result = HtmlProcessor::processHtml(input);
        // With no CSS resource, the function returns the raw processed HTML.
        // Empty input should produce empty or near-empty output.
        QVERIFY(result.trimmed().isEmpty() || result.contains("<style>"));
    }

    void testPlainText() {
        QString result = HtmlProcessor::processHtml("<p>Hello</p>");
        QVERIFY(result.contains("Hello"));
        QVERIFY(!result.contains("<style>body{"));
    }

    void testStyleNodeRemoved() {
        QString result = HtmlProcessor::processHtml("<style>body{}</style><p>Hi</p>");
        QVERIFY(result.contains("Hi"));
        QVERIFY(!result.contains("body{}"));
    }

    void testStyleAttributeRemoved() {
        QString result = HtmlProcessor::processHtml("<p style=\"color:red\">Hi</p>");
        QVERIFY(result.contains("Hi"));
        QVERIFY(!result.contains("color:red"));
    }

    void testImgNodePreservedInline() {
        QString result = HtmlProcessor::processHtml("<p>Before<img src=\"x.jpg\"/>After</p>");
        QVERIFY(result.contains("Before<img"));
        QVERIFY(result.contains("src=\"x.jpg\""));
        QVERIFY(result.contains("After"));
        QVERIFY(result.contains("max-width: 100%; height: auto;"));
    }

    void testFigureKeepsImageAndCaptionTogether() {
        const QString html = QStringLiteral(
            "<h2>Early life</h2><figure class=\"mw-default-size mw-halign-right\" "
            "typeof=\"mw:File/Thumb\"><a href=\"/wiki/File:Borden_house.jpg\" "
            "class=\"mw-file-description\"><img alt=\"House alt text\" "
            "src=\"//upload.wikimedia.org/Borden_house.jpg\" width=\"250\" height=\"180\"/>"
            "</a><figcaption>The Borden house at 92 Second Street in Fall River, "
            "Massachusetts</figcaption></figure><p>Lizzie Andrew Borden was born here.</p>");
        const QString result = HtmlProcessor::processHtml(html);
        const QStringList tables = imageTables(result);
        QCOMPARE(tables.size(), 1);
        verifyTwoRowImageTable(tables.first(), QStringLiteral("Borden_house.jpg"),
                               QStringLiteral("The Borden house at 92 Second Street"));
        QVERIFY(tables.first().contains("width=\"250\""));
        QVERIFY(tables.first().contains("<table class=\"article-image-table\" border=\"1\" width=\"270\""));
        QVERIFY(tables.first().contains("height=\"180\""));
        QVERIFY(tables.first().contains("href=\"/wiki/File:Borden_house.jpg\""));
        QVERIFY(tables.first().contains("src=\"https://upload.wikimedia.org/Borden_house.jpg\""));
        QVERIFY(!result.contains("<figure"));
        QVERIFY(!result.contains("<figcaption"));
        QVERIFY(result.indexOf("Early life") < result.indexOf("<table class=\"article-image-table\""));
        QVERIFY(result.indexOf("</table>") < result.indexOf("Lizzie Andrew Borden"));
    }

    void testCaptionlessFigureUsesAltText() {
        const QString result = HtmlProcessor::processHtml(
            "<figure><a href=\"/wiki/File:Portrait.jpg\"><img alt=\"A &amp; B\" "
            "src=\"portrait.jpg\"/></a><figcaption></figcaption></figure>");
        const QStringList tables = imageTables(result);
        QCOMPARE(tables.size(), 1);
        verifyTwoRowImageTable(tables.first(), QStringLiteral("portrait.jpg"),
                               QStringLiteral("A &amp; B"));
        QVERIFY(tables.first().contains("data-article-description=\"A%20%26%20B\""));
    }

    void testImageWithoutAltKeepsEmptyDescriptionRow() {
        const QString result = HtmlProcessor::processHtml(
            "<figure><img src=\"uncaptioned.jpg\"/><figcaption></figcaption></figure>");
        const QStringList tables = imageTables(result);
        QCOMPARE(tables.size(), 1);
        verifyTwoRowImageTable(tables.first(), QStringLiteral("uncaptioned.jpg"), QString());
        QVERIFY(QRegularExpression(R"(<tr>\s*<td\s*/>\s*</tr>|<tr>\s*<td>\s*</td>\s*</tr>)")
                    .match(tables.first())
                    .hasMatch());
    }

    void testLegacyThumbnailCaption() {
        const QString result = HtmlProcessor::processHtml(
            "<div class=\"thumb tright\"><div class=\"thumbinner\">"
            "<a href=\"/wiki/File:Old.jpg\"><img alt=\"Old alt\" src=\"old.jpg\"/></a>"
            "<div class=\"thumbcaption\"><div class=\"magnify\"><img src=\"zoom.svg\"/></div>"
            "An older thumbnail</div>"
            "</div></div>");
        const QStringList tables = imageTables(result);
        QCOMPARE(tables.size(), 1);
        verifyTwoRowImageTable(tables.first(), QStringLiteral("old.jpg"),
                               QStringLiteral("An older thumbnail"));
    }

    void testStandaloneDivImageUsesAltText() {
        const QString result = HtmlProcessor::processHtml(
            "<div class=\"center\"><div class=\"floatnone\">"
            "<a href=\"/wiki/File:Centered.jpg\"><img alt=\"Centered photo\" "
            "src=\"centered.jpg\"/></a></div></div>");
        const QStringList tables = imageTables(result);
        QCOMPARE(tables.size(), 1);
        verifyTwoRowImageTable(tables.first(), QStringLiteral("centered.jpg"),
                               QStringLiteral("Centered photo"));
    }

    void testMultipleImagesGetSeparateTables() {
        const QString result = HtmlProcessor::processHtml(
            "<figure><img alt=\"One\" src=\"one.jpg\"/><figcaption>First</figcaption></figure>"
            "<p>Between</p>"
            "<figure><img alt=\"Two\" src=\"two.jpg\"/><figcaption>Second</figcaption></figure>");
        const QStringList tables = imageTables(result);
        QCOMPARE(tables.size(), 2);
        verifyTwoRowImageTable(tables.at(0), QStringLiteral("one.jpg"), QStringLiteral("First"));
        verifyTwoRowImageTable(tables.at(1), QStringLiteral("two.jpg"), QStringLiteral("Second"));
        QVERIFY(result.indexOf("one.jpg") < result.indexOf("Between"));
        QVERIFY(result.indexOf("Between") < result.indexOf("two.jpg"));
    }

    void testHtmlFallbackConvertsFigureAndStandaloneImage() {
        const QString result = HtmlProcessor::processHtml(
            "<p>Intro&nbsp;text</p>"
            "<figure><a href=\"/wiki/File:One.jpg\"><img alt=\"Alt > text\" "
            "src=\"one.jpg\"></a><figcaption>A <b>caption</b></figcaption></figure>"
            "<p><a href=\"/wiki/File:Two.jpg\"><img alt=\"Second alt\" src=\"two.jpg\"></a></p>");
        const QStringList tables = imageTables(result);
        QCOMPARE(tables.size(), 2);
        verifyTwoRowImageTable(tables.at(0), QStringLiteral("one.jpg"),
                               QStringLiteral("A <b>caption</b>"));
        verifyTwoRowImageTable(tables.at(1), QStringLiteral("two.jpg"),
                               QStringLiteral("Second alt"));
        QVERIFY(result.contains("data-article-description=\"A%20caption\""));
        QVERIFY(result.contains("Intro&nbsp;text"));
    }

    void testExistingTableAndInlineImageRemainUnwrapped() {
        const QString result = HtmlProcessor::processHtml(
            "<table><tr><td><img src=\"infobox.jpg\"/></td></tr></table>"
            "<p>Text <img src=\"icon.jpg\"/> continues.</p>");
        QVERIFY(imageTables(result).isEmpty());
        QVERIFY(result.contains("infobox.jpg"));
        QVERIFY(result.contains("icon.jpg"));
    }

    void testProtocolRelativeImageUrlNormalized() {
        QString result = HtmlProcessor::processHtml(
            "<p>Image&nbsp;<img src=\"//upload.wikimedia.org/example.jpg\"/></p>");
        QVERIFY(result.contains("src=\"https://upload.wikimedia.org/example.jpg\""));
        QVERIFY(result.contains("height: auto;\"/>"));
    }

    void testFallbackKeepsGreaterThanInsideQuotedAttributes() {
        // &nbsp; is not an XML entity, so this input exercises the HTML fallback parser.
        QString result = HtmlProcessor::processHtml(
            "<p>Image&nbsp;<img alt=\"A > B\" title='C > D' src=\"x.jpg\" "
            "style=\"color:red\"></p>");

        QVERIFY(result.contains("alt=\"A > B\""));
        QVERIFY(result.contains("title='C > D'"));
        QVERIFY(result.contains("src=\"x.jpg\""));
        QVERIFY(result.contains("style=\"max-width: 100%; height: auto;\""));
        QVERIFY(!result.contains("color:red"));
        QVERIFY(result.contains("</p>"));
    }

    void testMathFallbackImagePreserved() {
        QString result = HtmlProcessor::processHtml(
            "<span class=\"mwe-math-element\" style=\"color: red\">"
            "<span class=\"mwe-math-mathml-inline mwe-math-mathml-a11y\"><math alttext=\"x^2\"><semantics>"
            "<annotation encoding=\"application/x-tex\">x^2</annotation></semantics></math></span>"
            "<img src=\"https://example.com/math.svg\" "
            "class=\"mwe-math-fallback-image-inline\" "
            "style=\"vertical-align: -0.3ex; width: 2ex; height: 1ex;\"></span>"
            "<img src=\"https://example.com/article-image.jpg\">");

        QVERIFY(result.contains("mwe-math-fallback-image-inline"));
        QVERIFY(result.contains("https://example.com/math.svg"));
        QVERIFY(!result.contains("<math"));
        QVERIFY(!result.contains("application/x-tex"));
        QVERIFY(!result.contains("x^2"));
        QVERIFY(!result.contains("color: red"));
        QVERIFY(!result.contains("vertical-align: -0.3ex"));
        QVERIFY(result.contains("article-image.jpg"));
    }

    void testNestedElements() {
        QString result = HtmlProcessor::processHtml(
            "<div><style>x{}</style><p>Hi</p></div>");
        QVERIFY(result.contains("Hi"));
        QVERIFY(!result.contains("x{}"));
    }

    void testMultipleRoots() {
        QString result = HtmlProcessor::processHtml("<p>A</p><p>B</p>");
        QVERIFY(result.contains("A"));
        QVERIFY(result.contains("B"));
    }
};

QTEST_MAIN(HtmlProcessorTest)
#include "HtmlProcessorTest.moc"

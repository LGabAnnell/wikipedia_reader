// tests/HtmlProcessorTest.cpp
#include <QtTest/QtTest>
#include "html_processor.h"

class HtmlProcessorTest : public QObject {
    Q_OBJECT

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

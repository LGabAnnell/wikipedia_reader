#ifndef HTML_PROCESSOR_H
#define HTML_PROCESSOR_H

#include <QString>

class HtmlProcessor {
  public:
    /**
     * @brief Cleans and prepares an HTML fragment for article display.
     * @param htmlContent HTML returned by the MediaWiki parse API.
     * @return A cleaned HTML fragment with the article stylesheet prepended.
     */
    static QString processHtml(const QString &htmlContent);
    /** @brief Sanitizes a short summary fragment to the supported inline markup. */
    static QString sanitizeSummaryHtml(const QString &htmlContent);
};

#endif // HTML_PROCESSOR_H

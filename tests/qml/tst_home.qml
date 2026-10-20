import QtQuick
import QtTest
import wikipedia_qt
import wikipedia_qt.Home

Item {
    id: root
    width: 960
    height: 720
    Component { id: modelComponent; HomeModel {} }
    Component { id: screenComponent; HomeScreen {} }
    Component { id: featuredComponent; FeaturedSection {} }
    TestCase {
        name: "HomeExperience"
        when: windowShown
        property var model: null
        property var screen: null
        function fixture(url, body, status) {
            const code = status === undefined ? 200 : status
            networkFixtures.addFixture("GET", url, body, "application/json", code)
            for (let offset = -1; offset <= 1; ++offset) {
                const date = new Date()
                date.setDate(date.getDate() + offset)
                const featuredUrl = url.replace(/\d{4}\/\d{2}\/\d{2}$/, Qt.formatDate(date, "yyyy/MM/dd"))
                const onThisDayUrl = url.replace(/\d{2}\/\d{2}$/, Qt.formatDate(date, "MM/dd"))
                networkFixtures.addFixture("GET", featuredUrl, body, "application/json", code)
                networkFixtures.addFixture("GET", onThisDayUrl, body, "application/json", code)
            }
        }
        function init() { networkFixtures.clearRequests(); model = modelComponent.createObject(root); }
        function cleanup() { if (screen) screen.destroy(); screen = null; if (model) model.destroy(); model = null; compare(testSupport.qmlWarningCount, 0, testSupport.qmlWarnings.join("\n")); }
        function installCompleteFixtures(featured, language) {
            const lang = language || "en"
            const date = Qt.formatDate(new Date(), "yyyy/MM/dd")
            fixture("https://api.wikimedia.org/feed/v1/wikipedia/" + lang + "/featured/" + date, featured)
            fixture("https://api.wikimedia.org/feed/v1/wikipedia/" + lang + "/onthisday/all/" + Qt.formatDate(new Date(), "MM/dd"), '{"selected":[]}')
            fixture("https://" + lang + ".wikipedia.org/api/rest_v1/page/random/title", '{"items":[]}')
        }
        function test_feed_completion_and_featured_thumbnail() {
            const date = Qt.formatDate(new Date(), "yyyy/MM/dd")
            const newsUrl = "https://api.wikimedia.org/feed/v1/wikipedia/en/featured/" + date
            const onThisDayUrl = "https://api.wikimedia.org/feed/v1/wikipedia/en/onthisday/all/" + Qt.formatDate(new Date(), "MM/dd").replace("/", "/")
            const randomUrl = "https://en.wikipedia.org/api/rest_v1/page/random/title"
            fixture(newsUrl, '{"tfa":{"title":"A & B/100%","extract":"Plain <text>","pageid":77,"thumbnail":{"source":"https://upload.wikimedia.org/featured.jpg"}},"mostread":{"articles":[{"title":"Popular","pageid":1,"extract":"Summary"}]}}')
            fixture(onThisDayUrl, '{"selected":[]}')
            fixture(randomUrl, '{"items":[]}')
            model.fetchHomeData()
            tryCompare(model, "featuredStatus", HomeModel.Ready)
            tryCompare(model, "newsStatus", HomeModel.Ready)
            tryCompare(model, "onThisDayStatus", HomeModel.Empty)
            tryCompare(model, "didYouKnowStatus", HomeModel.Empty)
            compare(model.featuredArticleTitle, "A & B/100%")
            compare(model.featuredArticleImageUrl, "https://upload.wikimedia.org/featured.jpg")
            compare(model.featuredArticleUrl, "https://en.wikipedia.org/wiki/A_%26_B%2F100%25")
            compare(model.newsItems.length, 1)
            compare(ArticleState.isLoading, false)
            compare(ArticleState.errorMessage, "")
            compare(HistoryState.history.length, 0)
            compare(testSupport.requestCount, 4)
        }
        function test_section_failure_isolated_and_retryable() {
            const date = Qt.formatDate(new Date(), "yyyy/MM/dd")
            const newsUrl = "https://api.wikimedia.org/feed/v1/wikipedia/en/featured/" + date
            const onThisDayUrl = "https://api.wikimedia.org/feed/v1/wikipedia/en/onthisday/all/" + Qt.formatDate(new Date(), "MM/dd")
            const randomUrl = "https://en.wikipedia.org/api/rest_v1/page/random/title"
            fixture(newsUrl, '{"mostread":{"articles":[{"title":"Popular","pageid":1}]}}')
            fixture(onThisDayUrl, "broken", 500)
            fixture(randomUrl, '{"items":[]}')
            model.fetchHomeData()
            tryCompare(model, "newsStatus", HomeModel.Ready)
            tryCompare(model, "onThisDayStatus", HomeModel.Error)
            compare(model.newsItems.length, 1)
            const before = testSupport.requestCount
            fixture(onThisDayUrl, '{"selected":[]}')
            model.retrySection(HomeModel.OnThisDay)
            tryCompare(model, "onThisDayStatus", HomeModel.Empty)
            compare(testSupport.requestCount, before + 1)
            compare(model.newsItems.length, 1)
        }
        function test_deferred_featured_ignores_duplicate_retry() {
            installCompleteFixtures('{"tfa":{"title":"Deferred","pageid":8},"mostread":{"articles":[{"title":"One","pageid":1}]}}')
            networkFixtures.deferNextReply()
            model.fetchHomeData()
            compare(model.featuredStatus, HomeModel.Loading)
            const count = testSupport.requestCount
            model.retrySection(HomeModel.Featured)
            compare(testSupport.requestCount, count)
            networkFixtures.completeNextReply()
            tryCompare(model, "featuredStatus", HomeModel.Ready)
        }
        function test_duplicate_retries_are_ignored_for_other_sections() {
            installCompleteFixtures('{"tfa":{"title":"Ready","pageid":8},"mostread":{"articles":[{"title":"One","pageid":1}]}}')
            model.fetchHomeData()
            tryCompare(model, "newsStatus", HomeModel.Ready)
            tryCompare(model, "onThisDayStatus", HomeModel.Empty)
            tryCompare(model, "didYouKnowStatus", HomeModel.Empty)
            const date = Qt.formatDate(new Date(), "yyyy/MM/dd")
            const dailyUrl = "https://api.wikimedia.org/feed/v1/wikipedia/en/featured/" + date
            networkFixtures.deferNextReply()
            model.retrySection(HomeModel.News)
            compare(model.newsStatus, HomeModel.Loading)
            let count = testSupport.requestCount
            model.retrySection(HomeModel.News)
            compare(testSupport.requestCount, count)
            networkFixtures.completeNextReply()
            tryCompare(model, "newsStatus", HomeModel.Ready)

            const onThisDayUrl = "https://api.wikimedia.org/feed/v1/wikipedia/en/onthisday/all/" + Qt.formatDate(new Date(), "MM/dd")
            networkFixtures.deferNextReply()
            model.retrySection(HomeModel.OnThisDay)
            count = testSupport.requestCount
            model.retrySection(HomeModel.OnThisDay)
            compare(testSupport.requestCount, count)
            networkFixtures.completeNextReply()
            tryCompare(model, "onThisDayStatus", HomeModel.Empty)

            networkFixtures.deferNextReply()
            model.retrySection(HomeModel.DidYouKnow)
            count = testSupport.requestCount
            model.retrySection(HomeModel.DidYouKnow)
            compare(testSupport.requestCount, count)
            networkFixtures.completeNextReply()
            tryCompare(model, "didYouKnowStatus", HomeModel.Empty)
            compare(dailyUrl.length > 0, true)
        }
        function test_refresh_rejects_delayed_featured_success() {
            installCompleteFixtures('{"tfa":{"title":"Old generation","pageid":18},"mostread":{"articles":[{"title":"One","pageid":1}]}}')
            networkFixtures.deferNextReply(true)
            model.fetchHomeData()
            compare(model.featuredStatus, HomeModel.Loading)
            compare(networkFixtures.pendingReplyCount, 1)
            installCompleteFixtures('{"tfa":{"title":"Current generation","pageid":19},"mostread":{"articles":[{"title":"One","pageid":1}]}}')
            model.fetchHomeData()
            tryCompare(model, "featuredStatus", HomeModel.Ready)
            compare(model.featuredArticleTitle, "Current generation")
            if (networkFixtures.pendingReplyCount > 0)
                networkFixtures.completeNextReply()
            wait(0)
            compare(model.featuredArticleTitle, "Current generation")

            const feedUrl = "https://api.wikimedia.org/feed/v1/wikipedia/en/featured/" + Qt.formatDate(new Date(), "yyyy/MM/dd")
            fixture(feedUrl, "unavailable", 503)
            networkFixtures.deferNextReply(true)
            model.retrySection(HomeModel.Featured)
            installCompleteFixtures('{"tfa":{"title":"After delayed error","pageid":20},"mostread":{"articles":[{"title":"One","pageid":1}]}}')
            model.fetchHomeData()
            tryCompare(model, "featuredStatus", HomeModel.Ready)
            compare(model.featuredArticleTitle, "After delayed error")
            if (networkFixtures.pendingReplyCount > 0)
                networkFixtures.completeNextReply()
            wait(0)
            compare(model.featuredStatus, HomeModel.Ready)
            compare(model.featuredError, "")
        }
        function test_language_change_cancels_delayed_summary_and_reloads_all_sections() {
            if (SettingsState.language !== "en")
                SettingsState.setLanguage("en")
            installCompleteFixtures('{"tfa":{"title":"English","pageid":30},"mostread":{"articles":[{"title":"Popular","pageid":1}]}}', "en")
            installCompleteFixtures('{"tfa":{"title":"Français","pageid":31},"mostread":{"articles":[{"title":"Populaire","pageid":2}]}}', "fr")
            fixture("https://en.wikipedia.org/api/rest_v1/page/random/title", '{"items":[{"title":"Old chain"}]}')
            fixture("https://en.wikipedia.org/api/rest_v1/page/summary/Old%20chain", JSON.stringify({pageid:40,extract:"Old result",content_urls:{desktop:{page:"https://en.wikipedia.org/wiki/Old_chain"}}}))
            fixture("https://fr.wikipedia.org/api/rest_v1/page/random/title", '{"items":[{"title":"Réponse actuelle"}]}')
            fixture("https://fr.wikipedia.org/api/rest_v1/page/summary/R%C3%A9ponse%20actuelle", JSON.stringify({pageid:41,extract:"Résultat français",content_urls:{desktop:{page:"https://fr.wikipedia.org/wiki/R%C3%A9ponse_actuelle"}}}))
            model.fetchHomeData()
            tryCompare(model, "didYouKnowStatus", HomeModel.Ready)
            compare(model.featuredArticleTitle, "English")

            fixture("https://en.wikipedia.org/api/rest_v1/page/summary/Old%20chain", "obsolete error", 503)
            networkFixtures.deferNextReply(true)
            model.retrySection(HomeModel.DidYouKnow)
            networkFixtures.deferNextReply(true)
            networkFixtures.completeNextReply()
            tryCompare(networkFixtures, "pendingReplyCount", 1)
            SettingsState.setLanguage("fr")
            compare(model.featuredArticleTitle, "")
            compare(model.didYouKnowItems.length, 0)
            compare(model.newsItems.length, 0)
            compare(model.onThisDayEvents.length, 0)
            tryCompare(model, "featuredStatus", HomeModel.Ready)
            tryCompare(model, "didYouKnowStatus", HomeModel.Ready)
            compare(model.featuredArticleTitle, "Français")
            compare(model.didYouKnowItems[0].text, "Résultat français")
            if (networkFixtures.pendingReplyCount > 0)
                networkFixtures.completeNextReply()
            wait(0)
            compare(model.didYouKnowItems[0].text, "Résultat français")
            SettingsState.setLanguage("en")
            tryCompare(model, "featuredStatus", HomeModel.Ready)
        }
        function test_did_you_know_random_and_summary_failures() {
            const date = Qt.formatDate(new Date(), "yyyy/MM/dd")
            fixture("https://api.wikimedia.org/feed/v1/wikipedia/en/featured/" + date, '{"tfa":{"title":"Featured","pageid":8},"mostread":{"articles":[{"title":"Popular","pageid":1}]}}')
            fixture("https://api.wikimedia.org/feed/v1/wikipedia/en/onthisday/all/" + Qt.formatDate(new Date(), "MM/dd"), '{"selected":[]}')
            fixture("https://en.wikipedia.org/api/rest_v1/page/random/title", "failure", 500)
            model.fetchHomeData()
            tryCompare(model, "didYouKnowStatus", HomeModel.Error)
            fixture("https://en.wikipedia.org/api/rest_v1/page/random/title", '{"items":[{"title":"Dyk Title"}]}')
            fixture("https://en.wikipedia.org/api/rest_v1/page/summary/Dyk Title", "failure", 500)
            model.retrySection(HomeModel.DidYouKnow)
            tryCompare(model, "didYouKnowStatus", HomeModel.Error)
            compare(model.featuredStatus, HomeModel.Ready)
        }
        function test_featured_titles_are_displayed_and_encoded_once() {
            installCompleteFixtures('{"tfa":{"title":"seed","pageid":9},"mostread":{"articles":[{"title":"Popular","pageid":1}]}}')
            model.fetchHomeData()
            tryCompare(model, "featuredStatus", HomeModel.Ready)
            const examples = [
                ["A title", "A_title"], ["Café 東京", "Caf%C3%A9_%E6%9D%B1%E4%BA%AC"],
                ["A&B", "A%26B"], ["A#B", "A%23B"], ["A/B", "A%2FB"],
                ["A?B", "A%3FB"], ["A%B", "A%25B"],
                ["The_Demi-Virgin", "The_Demi-Virgin", "The Demi-Virgin"]
            ]
            const feedUrl = "https://api.wikimedia.org/feed/v1/wikipedia/en/featured/" + Qt.formatDate(new Date(), "yyyy/MM/dd")
            for (let i = 0; i < examples.length; ++i) {
                fixture(feedUrl, JSON.stringify({tfa:{title:examples[i][0],extract:"<literal>",pageid:10+i},mostread:{articles:[{title:"Popular",pageid:1}]}}))
                model.retrySection(HomeModel.Featured)
                tryCompare(model, "featuredStatus", HomeModel.Ready)
                compare(model.featuredArticleTitle, examples[i][2] || examples[i][0])
                compare(model.featuredArticleUrl, "https://en.wikipedia.org/wiki/" + examples[i][1])
                compare(model.featuredArticleExtract, "<literal>")
            }
        }
        function test_missing_featured_is_empty_and_screen_feedback_is_section_specific() {
            const date = Qt.formatDate(new Date(), "yyyy/MM/dd")
            fixture("https://api.wikimedia.org/feed/v1/wikipedia/en/featured/" + date, '{"mostread":{"articles":[{"title":"Popular","pageid":1}]}}')
            fixture("https://api.wikimedia.org/feed/v1/wikipedia/en/onthisday/all/" + Qt.formatDate(new Date(), "MM/dd"), "failure", 503)
            fixture("https://en.wikipedia.org/api/rest_v1/page/random/title", "failure", 503)
            screen = screenComponent.createObject(root, {width: root.width, height: root.height})
            verify(screen !== null)
            const screenModel = findChild(screen, "homeModel")
            tryCompare(screenModel, "featuredStatus", HomeModel.Empty)
            tryCompare(screenModel, "onThisDayStatus", HomeModel.Error)
            tryCompare(screenModel, "didYouKnowStatus", HomeModel.Error)
            verify(findChild(screen, "featuredStatus") !== null)
            verify(findChild(screen, "onThisDayRetryButton") !== null)
            verify(findChild(screen, "didYouKnowRetryButton") !== null)
            fixture("https://api.wikimedia.org/feed/v1/wikipedia/en/onthisday/all/" + Qt.formatDate(new Date(), "MM/dd"), '{"selected":[]}')
            findChild(screen, "onThisDayRetryButton").clicked()
            tryCompare(screenModel, "onThisDayStatus", HomeModel.Empty)
            screen.destroy(); screen = null
        }
        function test_home_content_stays_within_normal_and_narrow_viewports() {
            const date = Qt.formatDate(new Date(), "yyyy/MM/dd")
            const longTitle = "A long featured title with Unicode 東京 and reserved characters & / ? # % " .repeat(4)
            const longExtract = "A long plain text extract that must wrap within the available home width. " .repeat(12)
            fixture("https://api.wikimedia.org/feed/v1/wikipedia/en/featured/" + date,
                    JSON.stringify({tfa:{title:longTitle,extract:longExtract,pageid:28},mostread:{articles:[{title:"Popular",pageid:1}]}}))
            fixture("https://api.wikimedia.org/feed/v1/wikipedia/en/onthisday/all/" + Qt.formatDate(new Date(), "MM/dd"), '{"selected":[]}')
            fixture("https://en.wikipedia.org/api/rest_v1/page/random/title", '{"items":[]}')
            screen = screenComponent.createObject(root, {width: 960, height: 700})
            const scroll = findChild(screen, "homeScrollView")
            tryCompare(findChild(screen, "homeModel"), "featuredStatus", HomeModel.Ready)
            verify(scroll.contentWidth <= 960)
            const featured = findChild(screen, "featuredSection")
            const title = findChild(screen, "featuredTitle")
            verify(featured.width <= 960)
            tryVerify(function() { return title.width <= featured.width })
            screen.width = 320
            wait(0)
            tryVerify(function() { return scroll.contentWidth <= 320 }, 5000, "Home content must wrap at narrow widths")
            tryVerify(function() { return featured.width <= 320 })
            tryVerify(function() { return title.width <= featured.width })
            screen.destroy(); screen = null
        }
        function test_featured_image_loading_never_hides_text_or_action() {
            const section = featuredComponent.createObject(root, {
                width: 960, title: "Plain title", extract: "Plain extract", pageId: 51,
                status: HomeModel.Ready, imageUrl: "qrc:/images/news_placeholder.jpg"
            })
            verify(section !== null)
            const featuredImage = findChild(section, "featuredArticleImage")
            tryCompare(featuredImage, "status", Image.Ready)
            compare(section.title, "Plain title")
            section.imageUrl = "file:///this-file-does-not-exist.png"
            tryCompare(featuredImage, "status", Image.Error)
            testSupport.clearQmlWarnings()
            compare(section.title, "Plain title")
            compare(section.extract, "Plain extract")
            compare(findChild(section, "featuredTitle").textFormat, Text.RichText)
            verify(findChild(section, "featuredTitle").text.indexOf('href="page://51"') >= 0)
            section.imageUrl = ""
            compare(section.status, HomeModel.Ready)
            section.destroy()
        }
    }
}

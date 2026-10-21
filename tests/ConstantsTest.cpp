#include "constants.h"

#include <QJSEngine>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QPointer>
#include <QtTest>

namespace {
constexpr auto testUri = "ConstantsLifetimeTest";

QObject *createConstants(QQmlEngine *engine, QJSEngine *jsEngine) {
    return Constants::create(engine, jsEngine);
}

bool registerTestType() {
    return qmlRegisterSingletonType<Constants>(testUri, 1, 0, "Constants", createConstants) >= 0;
}

QObject *loadRoot(QQmlEngine &engine) {
    QQmlComponent component(&engine);
    component.setData(R"(import QtQml
import ConstantsLifetimeTest 1.0
QtObject {
    property var singleton: Constants
})", QUrl());
    if (!component.isReady())
        return nullptr;
    return component.create();
}

Constants *singletonFrom(QObject *root) {
    return root ? qobject_cast<Constants *>(root->property("singleton").value<QObject *>()) : nullptr;
}
}

class ConstantsTest : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase() {
        QVERIFY(registerTestType());
    }

    void sameEngineSharesSingleton() {
        QQmlEngine engine;
        QScopedPointer<QObject> firstRoot(loadRoot(engine));
        QScopedPointer<QObject> secondRoot(loadRoot(engine));
        QVERIFY(firstRoot);
        QVERIFY(secondRoot);
        QVERIFY(singletonFrom(firstRoot.data()));
        QCOMPARE(singletonFrom(firstRoot.data()), singletonFrom(secondRoot.data()));
    }

    void factoryCreatesIndependentObjects() {
        Constants *first = Constants::create(nullptr, nullptr);
        Constants *second = Constants::create(nullptr, nullptr);
        const bool independent = first != second;
        if (independent) {
            delete first;
            delete second;
        } else {
            delete first;
        }
        QVERIFY(independent);
    }

    void separateEnginesOwnSeparateInstances() {
        QPointer<Constants> first;
        QPointer<Constants> second;
        {
            QQmlEngine firstEngine;
            QScopedPointer<QObject> firstRoot(loadRoot(firstEngine));
            QVERIFY(firstRoot);
            first = singletonFrom(firstRoot.data());
            QVERIFY(first);

            QQmlEngine secondEngine;
            QScopedPointer<QObject> secondRoot(loadRoot(secondEngine));
            QVERIFY(secondRoot);
            second = singletonFrom(secondRoot.data());
            QVERIFY(second);
            QVERIFY(first != second);
        }
        QVERIFY(first.isNull());
        QVERIFY(second.isNull());
    }

    void engineTeardownAndRecreationAreSafe() {
        QPointer<Constants> oldInstance;
        {
            QQmlEngine engine;
            QScopedPointer<QObject> root(loadRoot(engine));
            QVERIFY(root);
            oldInstance = singletonFrom(root.data());
            QVERIFY(oldInstance);
        }
        QVERIFY(oldInstance.isNull());

        QQmlEngine recreatedEngine;
        QScopedPointer<QObject> recreatedRoot(loadRoot(recreatedEngine));
        QVERIFY(recreatedRoot);
        Constants *recreated = singletonFrom(recreatedRoot.data());
        QVERIFY(recreated);
        QCOMPARE(recreated->property("historyView").toString(), QStringLiteral("history"));
    }

    void clearSingletonsAllowsRecreation() {
        QQmlEngine engine;
        QScopedPointer<QObject> firstRoot(loadRoot(engine));
        QVERIFY(firstRoot);
        QPointer<Constants> first = singletonFrom(firstRoot.data());
        QVERIFY(first);

        engine.clearSingletons();
        QVERIFY(first.isNull());

        QScopedPointer<QObject> secondRoot(loadRoot(engine));
        QVERIFY(secondRoot);
        Constants *second = singletonFrom(secondRoot.data());
        QVERIFY(second);
        QCOMPARE(second->property("homeView").toString(), QStringLiteral("home"));
    }

    void exposesExpectedViewNames() {
        QQmlEngine engine;
        QScopedPointer<QObject> root(loadRoot(engine));
        QVERIFY(root);
        QObject *constants = singletonFrom(root.data());
        QVERIFY(constants);
        const QList<QPair<const char *, QString>> properties = {
            {"historyView", QStringLiteral("history")},
            {"contentView", QStringLiteral("content")},
            {"searchView", QStringLiteral("search")},
            {"homeView", QStringLiteral("home")},
            {"imageGalleryView", QStringLiteral("imageGallery")},
            {"imageView", QStringLiteral("image")},
        };
        for (const auto &[name, expected] : properties)
            QCOMPARE(constants->property(name).toString(), expected);
    }
};

QTEST_MAIN(ConstantsTest)
#include "ConstantsTest.moc"

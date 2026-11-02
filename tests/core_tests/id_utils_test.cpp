#include <QtTest/QtTest>

#include "core/id_utils.h"

using fasthub::core::IdUtils;

class IdUtilsTest : public QObject {
    Q_OBJECT

private slots:
    void formatIsProviderColonExternal();
    void emptyInputsProduceValidFormat();
    void noCollisionForDifferentProviders();
};

void IdUtilsTest::formatIsProviderColonExternal() {
    QCOMPARE(IdUtils::makeInternalId("telegram", "1001"), std::string("telegram:1001"));
    QCOMPARE(IdUtils::makeInternalId("discord", "chan_1"), std::string("discord:chan_1"));
}

void IdUtilsTest::emptyInputsProduceValidFormat() {
    // Contract: even with an empty external id the output keeps `provider:` shape.
    QCOMPARE(IdUtils::makeInternalId("steam", ""), std::string("steam:"));
}

void IdUtilsTest::noCollisionForDifferentProviders() {
    const std::string idA = IdUtils::makeInternalId("telegram", "42");
    const std::string idB = IdUtils::makeInternalId("discord", "42");
    QVERIFY(idA != idB);
}

QTEST_MAIN(IdUtilsTest)
#include "id_utils_test.moc"

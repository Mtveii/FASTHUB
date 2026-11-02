#include <QtTest/QtTest>
#include <QJsonDocument>

#include "providers/steam/steam_presence.h"

namespace sp = fasthub::providers::steam;

class SteamPresenceTest : public QObject {
    Q_OBJECT

private slots:
    void parsePlayerSummaries();
    void parseFriendIds();
    void presenceLineMapping();
    void diffDetection();

private:
};

void SteamPresenceTest::parsePlayerSummaries() {
    const QByteArray json = R"({
        "response": {
            "players": [
                {"steamid": "76561190000000001", "personaname": "Gaben", "personastate": 1, "gameextrainfo": "Counter-Strike 2"},
                {"steamid": "76561190000000002", "personaname": "IdleFriend", "personastate": 0},
                {"steamid": "", "personaname": "broken-entry", "personastate": 1}
            ]
        }
    })";

    const auto players =
        sp::SteamPresenceCodec::parsePlayerSummaries(QJsonDocument::fromJson(json));
    QCOMPARE(players.size(), 2); // malformed entry skipped

    QCOMPARE(players[0].personaName, QString("Gaben"));
    QVERIFY(players[0].isInGame());
    QCOMPARE(players[0].gameName, QString("Counter-Strike 2"));

    QVERIFY(!players[1].isOnline());
    QVERIFY(!players[1].isInGame());
}

void SteamPresenceTest::parseFriendIds() {
    const QByteArray json = R"({
        "friendslist": {
            "friends": [
                {"steamid": "76561190000000001", "relationship": "friend"},
                {"steamid": "76561190000000002", "relationship": "friend"},
                {"steamid": "", "relationship": "friend"}
            ]
        }
    })";

    const auto ids = sp::SteamPresenceCodec::parseFriendIds(QJsonDocument::fromJson(json));
    QCOMPARE(ids.size(), 2);
    QCOMPARE(ids.front(), QString("76561190000000001"));
}

void SteamPresenceTest::presenceLineMapping() {
    sp::SteamPresence offline;
    offline.personaState = 0;
    QCOMPARE(offline.presenceLine(), QString("Offline"));

    sp::SteamPresence online;
    online.personaState = 1;
    QCOMPARE(online.presenceLine(), QString("Online"));

    sp::SteamPresence away;
    away.personaState = 3;
    QCOMPARE(away.presenceLine(), QString("Away"));

    sp::SteamPresence gaming;
    gaming.personaState = 1;
    gaming.gameName = "Dota 2";
    QCOMPARE(gaming.presenceLine(), QString("Playing: Dota 2"));
}

void SteamPresenceTest::diffDetection() {
    QMap<QString, sp::SteamPresence> a;
    a.insert("id1", {"id1", "Alice", 1, ""});

    QMap<QString, sp::SteamPresence> same = a;
    QVERIFY(!sp::SteamPresenceCodec::snapshotDiffers(a, same));

    QMap<QString, sp::SteamPresence> stateChanged = a;
    stateChanged["id1"].personaState = 0;
    QVERIFY(sp::SteamPresenceCodec::snapshotDiffers(a, stateChanged));

    QMap<QString, sp::SteamPresence> gameChanged = a;
    gameChanged["id1"].gameName = "Portal 2";
    QVERIFY(sp::SteamPresenceCodec::snapshotDiffers(a, gameChanged));

    QMap<QString, sp::SteamPresence> added = a;
    added.insert("id2", {"id2", "Bob", 0, ""});
    QVERIFY(sp::SteamPresenceCodec::snapshotDiffers(a, added));

    QMap<QString, sp::SteamPresence> removed;
    QVERIFY(sp::SteamPresenceCodec::snapshotDiffers(a, removed));
}

QTEST_MAIN(SteamPresenceTest)
#include "steam_presence_test.moc"

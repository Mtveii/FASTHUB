#include "steam_presence.h"

namespace fasthub::providers::steam {

namespace {

SteamPresence parsePlayer(const QJsonObject& o) {
    SteamPresence p;
    p.steamId = o.value(QStringLiteral("steamid")).toString();
    p.personaName = o.value(QStringLiteral("personaname")).toString();
    p.personaState = o.value(QStringLiteral("personastate")).toInt();
    p.gameName = o.value(QStringLiteral("gameextrainfo")).toString();
    return p;
}

} // namespace

namespace SteamPresenceCodec {

QVector<SteamPresence> parsePlayerSummaries(const QJsonDocument& doc) {
    QVector<SteamPresence> result;
    const QJsonArray players = doc.object().value(QStringLiteral("response"))
                                   .toObject().value(QStringLiteral("players")).toArray();
    result.reserve(players.size());
    for (const auto& v : players) {
        const SteamPresence p = parsePlayer(v.toObject());
        if (!p.steamId.isEmpty()) {
            result.push_back(p);
        }
    }
    return result;
}

QStringList parseFriendIds(const QJsonDocument& doc) {
    QStringList ids;
    const QJsonArray friends = doc.object().value(QStringLiteral("friendslist"))
                                   .toObject().value(QStringLiteral("friends")).toArray();
    for (const auto& v : friends) {
        const QString id = v.toObject().value(QStringLiteral("steamid")).toString();
        if (!id.isEmpty()) {
            ids.push_back(id);
        }
    }
    return ids;
}

bool snapshotDiffers(const QMap<QString, SteamPresence>& oldSnapshot,
                     const QMap<QString, SteamPresence>& newSnapshot) {
    if (oldSnapshot.size() != newSnapshot.size()) {
        return true;
    }
    for (auto it = newSnapshot.constBegin(); it != newSnapshot.constEnd(); ++it) {
        const auto oldIt = oldSnapshot.constFind(it.key());
        if (oldIt == oldSnapshot.constEnd()) {
            return true;
        }
        const SteamPresence& a = oldIt.value();
        const SteamPresence& b = it.value();
        if (a.personaName != b.personaName || a.personaState != b.personaState ||
            a.gameName != b.gameName) {
            return true;
        }
    }
    return false;
}

} // namespace SteamPresenceCodec

} // namespace fasthub::providers::steam

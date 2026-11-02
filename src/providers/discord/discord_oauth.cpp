#include "discord_oauth.h"

#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTcpSocket>
#include <QUrlQuery>
#include <QUrl>

#include "infrastructure/logger.h"
#include "infrastructure/secure_storage.h"

namespace fasthub::providers::discord {

namespace {
constexpr const char *kKeyClientId = "discord.client_id";
constexpr const char *kKeyClientSecret = "discord.client_secret";
constexpr const char *kKeyAccessToken = "discord.access_token";
constexpr const char *kKeyRefreshToken = "discord.refresh_token";

QString secret(const std::string &key) {
    infrastructure::SecureStorage storage;
    return QString::fromStdString(storage.getSecret(key).value_or(std::string()));
}
} // namespace

DiscordOauthService::DiscordOauthService(QObject *parent) : QObject(parent) {
    m_http = new QNetworkAccessManager(this);
}

bool DiscordOauthService::hasCredentials() const {
    return !secret(kKeyClientId).isEmpty() && !secret(kKeyClientSecret).isEmpty();
}

void DiscordOauthService::saveAppCredentials(const QString &clientId, const QString &clientSecret) {
    infrastructure::SecureStorage storage;
    storage.setSecret(kKeyClientId, clientId.toStdString());
    storage.setSecret(kKeyClientSecret, clientSecret.toStdString());
    infrastructure::Logger::instance().info("Discord",
                                            "OAuth app credentials saved (DPAPI-protected).");
}

void DiscordOauthService::beginLogin() {
    if (!hasCredentials()) {
        emit loginStateChanged(QStringLiteral("error"),
                               QStringLiteral("Missing application credentials. Register an app at "
                                              "discord.com/developers/applications first."));
        return;
    }

    if (!m_listener.listen(QHostAddress::LocalHost, kCallbackPort)) {
        emit loginStateChanged(QStringLiteral("error"),
                               QStringLiteral("Cannot listen on 127.0.0.1:%1: %2")
                                   .arg(kCallbackPort)
                                   .arg(m_listener.errorString()));
        return;
    }
    connect(&m_listener, &QTcpServer::newConnection, this,
            &DiscordOauthService::onCallbackConnection, Qt::UniqueConnection);

    QUrl url(QStringLiteral("https://discord.com/oauth2/authorize"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("client_id"), secret(kKeyClientId));
    query.addQueryItem(QStringLiteral("response_type"), QStringLiteral("code"));
    query.addQueryItem(QStringLiteral("redirect_uri"), QString::fromUtf8(kRedirectUri));
    query.addQueryItem(QStringLiteral("scope"), QString::fromUtf8(kScope));
    query.addQueryItem(QStringLiteral("prompt"), QStringLiteral("consent"));
    url.setQuery(query);

    emit loginStateChanged(QStringLiteral("opened"),
                           QStringLiteral("Browser opened for authorization."));
    QDesktopServices::openUrl(url);
    emit loginStateChanged(
        QStringLiteral("waiting"),
        QStringLiteral("Waiting for redirect on 127.0.0.1:%1...").arg(kCallbackPort));
}

void DiscordOauthService::onCallbackConnection() {
    QTcpSocket *socket = m_listener.nextPendingConnection();
    if (!socket) {
        return;
    }
    connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
        const QByteArray request = socket->readAll();
        const QString line = QString::fromUtf8(request.split('\r').value(0));

        // Example: GET /callback?code=abc&state=xyz HTTP/1.1
        const int codeIdx = line.indexOf(QStringLiteral("code="));
        QString code;
        if (codeIdx >= 0) {
            const QString tail = line.mid(codeIdx + 4);
            code = tail.section('&', 0, 0);
        }

        const QByteArray body =
            "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n\r\n"
            "<html><body style='font-family:sans-serif'><h3>Authorization received."
            "</h3>You can close this tab and return to FastHub.</body></html>";
        socket->write(body);
        socket->flush();
        socket->disconnectFromHost();

        finishListener();

        if (code.isEmpty()) {
            emit loginStateChanged(QStringLiteral("error"),
                                   QStringLiteral("Redirect without authorization code."));
            return;
        }

        emit loginStateChanged(QStringLiteral("code_captured"),
                               QStringLiteral("Exchanging token..."));
        exchangeCodeForToken(code);
    });
    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
}

void DiscordOauthService::finishListener() {
    disconnect(&m_listener, &QTcpServer::newConnection, this,
               &DiscordOauthService::onCallbackConnection);
    m_listener.close();
}

void DiscordOauthService::exchangeCodeForToken(const QString &code) {
    QNetworkRequest request{QUrl(QStringLiteral("https://discord.com/api/oauth2/token"))};
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/x-www-form-urlencoded"));

    QUrlQuery params;
    params.addQueryItem(QStringLiteral("client_id"), secret(kKeyClientId));
    params.addQueryItem(QStringLiteral("client_secret"), secret(kKeyClientSecret));
    params.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("authorization_code"));
    params.addQueryItem(QStringLiteral("code"), code);
    params.addQueryItem(QStringLiteral("redirect_uri"), QString::fromUtf8(kRedirectUri));

    auto *reply = m_http->post(request, params.query(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        const QByteArray payload = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            infrastructure::Logger::instance().warn(
                "Discord", QString("Token exchange failed: %1 (%2)")
                               .arg(reply->errorString(), QString::fromUtf8(payload.left(200))));
            emit loginStateChanged(
                QStringLiteral("error"),
                QStringLiteral("Token exchange failed: %1").arg(reply->errorString()));
            return;
        }

        const QJsonObject obj = QJsonDocument::fromJson(payload).object();
        const QString accessToken = obj.value("access_token").toString();
        const QString refreshToken = obj.value("refresh_token").toString();
        if (accessToken.isEmpty()) {
            emit loginStateChanged(QStringLiteral("error"),
                                   QStringLiteral("Response without access_token."));
            return;
        }

        infrastructure::SecureStorage storage;
        storage.setSecret(kKeyAccessToken, accessToken.toStdString());
        if (!refreshToken.isEmpty()) {
            storage.setSecret(kKeyRefreshToken, refreshToken.toStdString());
        }

        infrastructure::Logger::instance().info("Discord", "OAuth token stored securely.");
        emit loginStateChanged(QStringLiteral("authorized"), QStringLiteral("Authorized."));
    });
}

} // namespace fasthub::providers::discord

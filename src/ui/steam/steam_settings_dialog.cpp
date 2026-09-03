#include "steam_settings_dialog.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>

namespace fasthub::ui {

namespace {
constexpr int kVanityPort = 0; // unused; kept for clarity
}

SteamSettingsDialog::SteamSettingsDialog(std::shared_ptr<providers::steam::SteamProvider> provider, QWidget* parent)
    : QDialog(parent), m_provider(provider) {
    setupUi();
}

void SteamSettingsDialog::setupUi() {
    setWindowTitle("Steam - Web API Setup");
    resize(460, 260);

    auto* layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel(
        "1. Get a free key: steamcommunity.com/dev/apikey\n"
        "2. Your SteamID64 (from your profile URL) or vanity name.", this));

    m_apiKeyEdit = new QLineEdit(this);
    m_apiKeyEdit->setPlaceholderText("Steam Web API key");
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    // Prefill already-stored credentials so the user only fills the gaps.
    if (m_provider && !m_provider->apiKey().isEmpty()) {
        m_apiKeyEdit->setText(m_provider->apiKey());
    }
    layout->addWidget(m_apiKeyEdit);

    m_steamIdEdit = new QLineEdit(this);
    m_steamIdEdit->setPlaceholderText("SteamID64 (e.g. 76561198000000000) or vanity name");
    if (m_provider && !m_provider->steamId().isEmpty()) {
        m_steamIdEdit->setText(m_provider->steamId());
    }
    layout->addWidget(m_steamIdEdit);

    m_hintLabel = new QLabel(this);
    m_hintLabel->setWordWrap(true);
    m_hintLabel->setProperty("role", "muted");
    layout->addWidget(m_hintLabel);

    auto* save = new QPushButton("Save and start polling", this);
    save->setProperty("primary", true);
    connect(save, &QPushButton::clicked, this, &SteamSettingsDialog::onSave);
    layout->addWidget(save);

    auto* close = new QPushButton("Close", this);
    connect(close, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(close);
}

void SteamSettingsDialog::onSave() {
    const QString key = m_apiKeyEdit->text().trimmed();
    QString id = m_steamIdEdit->text().trimmed();
    if (key.isEmpty() || id.isEmpty()) {
        QMessageBox::warning(this, "Steam", "Both fields are required.");
        return;
    }

    // Vanity name -> resolve to SteamID64 via official endpoint.
    static const QRegularExpression digitsOnly(QStringLiteral("^\\d{17}$"));
    if (!digitsOnly.match(id).hasMatch()) {
        const QString vanity = id;
        m_hintLabel->setText(QString("Resolving vanity name '%1'...").arg(vanity));

        QUrl url(QStringLiteral("https://api.steampowered.com/ISteamUser/ResolveVanityURL/v1/"));
        QUrlQuery q;
        q.addQueryItem(QStringLiteral("key"), key);
        q.addQueryItem(QStringLiteral("vanityurl"), vanity);
        url.setQuery(q);

        auto* nam = new QNetworkAccessManager(this);
        auto* reply = nam->get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, this, [this, reply, key, nam]() {
            reply->deleteLater();
            nam->deleteLater();

            const QJsonObject response = QJsonDocument::fromJson(reply->readAll())
                                             .object().value(QStringLiteral("response")).toObject();
            const QString resolved = response.value(QStringLiteral("steamid")).toString();
            if (response.value(QStringLiteral("success")).toInt() != 1 || resolved.isEmpty()) {
                m_hintLabel->setText(QString("Could not resolve '%1'.").arg(m_steamIdEdit->text()));
                return;
            }
            m_steamIdEdit->setText(resolved);
            m_provider->saveCredentials(key, resolved);
            m_provider->startPolling();
            m_hintLabel->setText(QString("Resolved to %1. Saved.").arg(resolved));
            accept();
        });
        return;
    }

    m_provider->saveCredentials(key, id);
    m_provider->startPolling();
    accept();
}

} // namespace fasthub::ui

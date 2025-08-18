#include "OpenSkyAuthManager.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

OpenSkyAuthManager::OpenSkyAuthManager(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

void OpenSkyAuthManager::setCredentials(const QString& clientId, const QString& clientSecret)
{
    m_clientId = clientId;
    m_clientSecret = clientSecret;
}

void OpenSkyAuthManager::authenticate()
{
    if (m_clientId.isEmpty() || m_clientSecret.isEmpty()) {
        emit authenticationFailed("Missing credentials");
        return;
    }
    
    qDebug() << "Starting OpenSky Network authentication...";
    requestAccessToken();
}

void OpenSkyAuthManager::requestAccessToken()
{
    QUrl tokenUrl("https://auth.opensky-network.org/auth/realms/opensky-network/protocol/openid-connect/token");

    QNetworkRequest request(tokenUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery postData;
    postData.addQueryItem("grant_type", "client_credentials");
    postData.addQueryItem("client_id", m_clientId);
    postData.addQueryItem("client_secret", m_clientSecret);

    QNetworkReply *reply = m_networkManager->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, &OpenSkyAuthManager::onAuthenticationReply);
}

void OpenSkyAuthManager::onAuthenticationReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }

    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        QString error = QString("Authentication failed: %1").arg(reply->errorString());
        qDebug() << error;
        emit authenticationFailed(error);
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    if (obj.contains("access_token")) {
        m_accessToken = obj["access_token"].toString();
        qDebug() << "Authentication successful!";
        emit authenticationSuccess();
    } else {
        QString error = "No access token in response";
        if (obj.contains("error")) {
            error = obj["error"].toString();
            if (obj.contains("error_description")) {
                error += ": " + obj["error_description"].toString();
            }
        }
        qDebug() << "Authentication failed:" << error;
        emit authenticationFailed(error);
    }
}
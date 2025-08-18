#include "FlightDataService.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

FlightDataService::FlightDataService(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

void FlightDataService::setAccessToken(const QString& token)
{
    m_accessToken = token;
}

void FlightDataService::fetchFlightData()
{
    if (m_accessToken.isEmpty()) {
        emit dataFetchFailed("No access token available");
        return;
    }

    qDebug() << "Fetching flight data...";

    QUrl flightUrl("https://opensky-network.org/api/states/all");
    QNetworkRequest request(flightUrl);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_accessToken).toUtf8());

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &FlightDataService::onFlightDataReply);
}

void FlightDataService::fetchFlightTrack(const QString& icao24)
{
    if (m_accessToken.isEmpty() || icao24.isEmpty()) {
        emit dataFetchFailed("Missing access token or ICAO24");
        return;
    }

    qDebug() << "Fetching track for aircraft:" << icao24;

    qint64 timestamp = m_lastUpdateTime.toSecsSinceEpoch();
    QUrl trackUrl(QString("https://opensky-network.org/api/tracks/all?icao24=%1&time=%2")
                      .arg(icao24.toLower())
                      .arg(timestamp));

    QNetworkRequest request(trackUrl);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_accessToken).toUtf8());
    request.setRawHeader("X-ICAO24", icao24.toUtf8()); // Store ICAO24 for the reply

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &FlightDataService::onTrackDataReply);
}

void FlightDataService::onFlightDataReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }

    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit dataFetchFailed(QString("Flight data request failed: %1").arg(reply->errorString()));
        return;
    }

    QByteArray data = reply->readAll();
    m_lastUpdateTime = QDateTime::currentDateTime();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    QList<FlightData> flights;

    if (obj.contains("states")) {
        QJsonArray states = obj["states"].toArray();
        qDebug() << "Processing" << states.size() << "flights";

        for (const QJsonValue& value : states) {
            QJsonArray flightArray = value.toArray();
            FlightData flight(flightArray);
            
            if (flight.isValid()) {
                flights.append(flight);
            }
        }
    }

    emit flightDataReceived(flights);
}

void FlightDataService::onTrackDataReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }

    QString icao24 = reply->request().rawHeader("X-ICAO24");
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit dataFetchFailed(QString("Track data request failed: %1").arg(reply->errorString()));
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject trackObj = doc.object();

    emit trackDataReceived(icao24, trackObj);
}
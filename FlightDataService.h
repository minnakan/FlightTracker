#ifndef FLIGHTDATASERVICE_H
#define FLIGHTDATASERVICE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QDateTime>
#include "FlightData.h"

class FlightDataService : public QObject
{
    Q_OBJECT

public:
    explicit FlightDataService(QObject *parent = nullptr);

    void setAccessToken(const QString& token);
    void fetchFlightData();
    void fetchFlightTrack(const QString& icao24);

signals:
    void flightDataReceived(const QList<FlightData>& flights);
    void trackDataReceived(const QString& icao24, const QJsonObject& trackData);
    void dataFetchFailed(const QString& error);

private slots:
    void onFlightDataReply();
    void onTrackDataReply();

private:
    QNetworkAccessManager* m_networkManager;
    QString m_accessToken;
    QDateTime m_lastUpdateTime;
};

#endif // FLIGHTDATASERVICE_H
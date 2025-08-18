#ifndef FLIGHTDATA_H
#define FLIGHTDATA_H

#include <QString>
#include <QPointF>
#include <QJsonArray>

class FlightData
{
public:
    FlightData() = default;
    explicit FlightData(const QJsonArray& data);

    bool isValid() const;
    
    QString icao24() const { return m_icao24; }
    QString callsign() const { return m_callsign; }
    QString country() const { return m_country; }
    
    double longitude() const { return m_longitude; }
    double latitude() const { return m_latitude; }
    double altitude() const { return m_altitude; }
    double velocity() const { return m_velocity; }
    double heading() const { return m_heading; }
    double verticalRate() const { return m_verticalRate; }
    
    bool onGround() const { return m_onGround; }
    QString squawk() const { return m_squawk; }

private:
    QString m_icao24;
    QString m_callsign;
    QString m_country;
    double m_longitude = 0.0;
    double m_latitude = 0.0;
    double m_altitude = 0.0;
    double m_velocity = 0.0;
    double m_heading = 0.0;
    double m_verticalRate = 0.0;
    bool m_onGround = false;
    QString m_squawk;
    bool m_valid = false;
};

#endif // FLIGHTDATA_H
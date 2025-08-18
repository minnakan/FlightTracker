#include "FlightData.h"
#include <QJsonArray>
#include <cmath>

FlightData::FlightData(const QJsonArray& data)
{
    if (data.size() >= 17) {
        m_icao24 = data[0].toString();
        m_callsign = data[1].toString().trimmed();
        m_country = data[2].toString();
        m_longitude = data[5].toDouble();
        m_latitude = data[6].toDouble();
        m_altitude = data[7].toDouble();
        m_onGround = data[8].toBool();
        m_velocity = data[9].toDouble();
        m_heading = data[10].toDouble();
        m_verticalRate = data[11].toDouble();
        m_squawk = data[14].toString();

        // Valid if we have basic coordinate data
        m_valid = (m_longitude != 0.0 || m_latitude != 0.0) && !m_icao24.isEmpty();
    }
}

bool FlightData::isValid() const
{
    return m_valid;
}
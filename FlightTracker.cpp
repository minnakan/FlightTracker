// Copyright 2025 ESRI
//
// All rights reserved under the copyright laws of the United States
// and applicable international laws, treaties, and conventions.
//
// You may freely redistribute and use this sample code, with or
// without modification, provided you include the original copyright
// notice and use restrictions.
//
// See the Sample code usage restrictions document for further information.
//

#include "FlightTracker.h"

#include "Map.h"
#include "MapQuickView.h"
#include "MapTypes.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QDebug>

#include "GraphicsOverlay.h"
#include "Graphic.h"
#include "SymbolTypes.h"
#include "TextSymbol.h"
#include "Point.h"
#include "SpatialReference.h"
#include "GraphicListModel.h"
#include "GraphicsOverlayListModel.h"
#include "SimpleMarkerSymbol.h"
#include "SimpleLineSymbol.h"

#include "Popup.h"
#include "PopupDefinition.h"
#include "PopupField.h"
#include "PopupDefinition.h"
#include "TextPopupElement.h"

#include "Polyline.h"
#include "PolylineBuilder.h"

#include <QLineF>
#include <QDateTime>
#include "Geometry.h"

#include <QFile>


using namespace Esri::ArcGISRuntime;

FlightTracker::FlightTracker(QObject *parent /* = nullptr */)
    : QObject(parent)
    , m_map(new Map(BasemapStyle::ArcGISHumanGeographyDark, this))
    , m_networkManager(new QNetworkAccessManager(this))
{
    loadConfig();
    loadCountryMappings();
    authenticate();
    m_flightUpdateTimer = new QTimer(this);

    //Set to high value(~17 minutes) to prevent auto refresh for now; we can expose as a setting later
    m_flightUpdateTimer->setInterval(1000000);
    connect(m_flightUpdateTimer, &QTimer::timeout, this, &FlightTracker::fetchFlightData);

    m_displayUpdateTimer = new QTimer(this);
    m_displayUpdateTimer->setInterval(1000); // Update every second
    connect(m_displayUpdateTimer, &QTimer::timeout, this, &FlightTracker::updateDisplayTime);
    m_displayUpdateTimer->start();

    m_flightOverlay = new GraphicsOverlay(this);
    m_selectionOverlay = new GraphicsOverlay(this);
    m_trackOverlay = new GraphicsOverlay(this);
}


FlightTracker::~FlightTracker() = default;

void FlightTracker::loadConfig()
{
    QFile configFile(":/config/Config/config.json"); // adjust accordingly
    if (configFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(configFile.readAll());
        QJsonObject config = doc.object();

        // Load OpenSky API credentials
        QJsonObject opensky = config["opensky"].toObject();
        m_clientId = opensky["client_id"].toString();
        m_clientSecret = opensky["client_secret"].toString();

        qDebug() << "OpenSky credentials loaded from config.json";
    } else {
        qDebug() << "Could not open config.json, using hardcoded values";
        // Fallback to your current hardcoded values
    }
}

MapQuickView *FlightTracker::mapView() const
{
    return m_mapView;
}

// Set the view (created in QML)
void FlightTracker::setMapView(MapQuickView *mapView)
{
    if (!mapView || mapView == m_mapView) {
        return;
    }

    m_mapView = mapView;
    m_mapView->setMap(m_map);

    m_mapView->graphicsOverlays()->append(m_trackOverlay);
    m_mapView->graphicsOverlays()->append(m_flightOverlay);
    m_mapView->graphicsOverlays()->append(m_selectionOverlay);

    emit mapViewChanged();
}

void FlightTracker::authenticate()
{
    qDebug() << "Starting OpenSky Network authentication...";
    requestAccessToken();
}

void FlightTracker::requestAccessToken()
{
    // OpenSky Network OAuth2 token endpoint
    QUrl tokenUrl("https://auth.opensky-network.org/auth/realms/opensky-network/protocol/openid-connect/token");

    QNetworkRequest request(tokenUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    // Prepare POST data for client credentials flow
    QUrlQuery postData;
    postData.addQueryItem("grant_type", "client_credentials");
    postData.addQueryItem("client_id", m_clientId);
    postData.addQueryItem("client_secret", m_clientSecret);

    QNetworkReply *reply = m_networkManager->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());

    connect(reply, &QNetworkReply::finished, this, &FlightTracker::onAuthenticationReply);
}

void FlightTracker::onAuthenticationReply()
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

    // Parse JSON response
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    if (obj.contains("access_token")) {
        m_accessToken = obj["access_token"].toString();
        qDebug() << "Authentication successful! Token obtained.";
        emit authenticationSuccess();
        emit authenticationChanged();

        // Start fetching flight data
        fetchFlightData();
        m_flightUpdateTimer->start();

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

void FlightTracker::fetchFlightData()
{
    if (m_accessToken.isEmpty()) {
        qDebug() << "No access token available for flight data request";
        return;
    }


    if (m_selectedIcao24.isEmpty()) {
        clearFlightSelection();
    }
    m_selectedGraphic = nullptr;

    qDebug() << "Fetching all flight data...";


    QUrl flightUrl("https://opensky-network.org/api/states/all");
    QNetworkRequest request(flightUrl);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_accessToken).toUtf8());

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &FlightTracker::onFlightDataReply);
}

QColor FlightTracker::getAltitudeColor(double altitude)
{
    // Convert meters to feet if needed (OpenSky uses meters)
    double altitudeFeet = altitude * 3.28084;

    // Define altitude ranges and colors
    if (altitudeFeet <= 500) {
        return QColor(255, 69, 0);      // Red-orange (very low)
    } else if (altitudeFeet <= 1000) {
        return QColor(255, 140, 0);     // Orange
    } else if (altitudeFeet <= 2000) {
        return QColor(255, 215, 0);     // Gold
    } else if (altitudeFeet <= 4000) {
        return QColor(255, 255, 0);     // Yellow
    } else if (altitudeFeet <= 6000) {
        return QColor(173, 255, 47);    // Yellow-green
    } else if (altitudeFeet <= 8000) {
        return QColor(0, 255, 0);       // Green
    } else if (altitudeFeet <= 10000) {
        return QColor(0, 255, 127);     // Spring green
    } else if (altitudeFeet <= 20000) {
        return QColor(0, 191, 255);     // Deep sky blue
    } else if (altitudeFeet <= 30000) {
        return QColor(0, 100, 255);     // Blue
    } else if (altitudeFeet <= 40000) {
        return QColor(138, 43, 226);    // Blue violet
    } else {
        return QColor(128, 0, 128);     // Purple (very high)
    }
}

int FlightTracker::getCategoryFromCallsign(const QString& callsign)
{
    if (callsign.isEmpty()) {
        return 1;  // No information
    }

    QString trimmed = callsign.trimmed();
    int length = trimmed.length();

    // Count digits and letters
    int digitCount = 0;
    int letterCount = 0;
    for (QChar c : trimmed) {
        if (c.isDigit()) digitCount++;
        if (c.isLetter()) letterCount++;
    }

    // US N-number pattern
    if (trimmed.startsWith('N') && digitCount >= 2) {
        return 2;  // Light aircraft
    }

    // Cargo indicators
    if (trimmed.contains("FDX") || trimmed.contains("UPS") ||
        trimmed.contains("CARGO") || trimmed.contains("ABX")) {
        return 6;  // Heavy cargo
    }

    // Emergency/helicopter indicators
    if (trimmed.contains("MED") || trimmed.contains("RESCUE") ||
        trimmed.contains("LIFE") || trimmed.contains("HELI")) {
        return 8;  // Rotorcraft
    }

    // Statistical categorization based on length and composition
    if (length <= 5) {
        return 3;  // Short = likely private/light
    } else if (length == 6 || length == 7) {
        if (digitCount >= 2) {
            return 4;  // Commercial flight number pattern
        } else {
            return 4;  // Regional or small commercial
        }
    } else if (length >= 8) {
        return 3;  // Long codes often indicate private aircraft
    }

    // Default
    return 4;  // Small aircraft
}

TextSymbol* FlightTracker::getSymbolForCategory(int category, bool onGround, double altitude)
{
    QString aircraftChar;
    float fontSize = 24.0f;

    // Map category to basic Unicode symbols that work everywhere
    switch (category) {
    case 1:  aircraftChar = "✈"; fontSize = 24.0f; break;  // Unknown
    case 2:  aircraftChar = "✈"; fontSize = 12.0f; break;  // Light aircraft - U+2708
    case 3:  aircraftChar = "✈"; fontSize = 16.0f; break;  // Small aircraft - U+2708
    case 4:  aircraftChar = "✈"; fontSize = 20.0f; break;  // Large aircraft - U+2708
    case 5:  aircraftChar = "✈"; fontSize = 22.0f; break;  // High Vortex Large - U+2708
    case 6:  aircraftChar = "✈"; fontSize = 26.0f; break;  // Heavy aircraft - U+2708
    case 7:  aircraftChar = "▲"; fontSize = 22.0f; break;  // High Performance - triangle
    case 8:  aircraftChar = "●"; fontSize = 20.0f; break;  // Rotorcraft - circle
    case 9:  aircraftChar = "◇"; fontSize = 18.0f; break;  // Glider - diamond
    case 10: aircraftChar = "○"; fontSize = 26.0f; break;  // Lighter-than-air - circle
    case 11: aircraftChar = "·"; fontSize = 12.0f; break;  // Parachutist - dot
    case 12: aircraftChar = "△"; fontSize = 14.0f; break;  // Ultralight - triangle
    case 13: aircraftChar = "✈"; fontSize = 16.0f; break;  // Reserved
    case 14: aircraftChar = "◆"; fontSize = 12.0f; break;  // UAV - diamond
    case 15: aircraftChar = "▲"; fontSize = 26.0f; break;  // Space vehicle - triangle
    case 16: aircraftChar = "■"; fontSize = 18.0f; break;  // Emergency Vehicle - square
    case 17: aircraftChar = "▪"; fontSize = 16.0f; break;  // Service Vehicle - small square
    case 18: aircraftChar = "!"; fontSize = 10.0f; break;  // Point Obstacle
    case 19: aircraftChar = "⚠"; fontSize = 14.0f; break;  // Cluster Obstacle - warning
    case 20: aircraftChar = "⚠"; fontSize = 12.0f; break;  // Line Obstacle - warning
    default: aircraftChar = "✈"; fontSize = 24.0f; break;  // Default - airplane
    }

    // Special handling for ground vehicles
    if (onGround && (category == 16 || category == 17)) {
        aircraftChar = "■";    // Square for ground vehicle
        fontSize = 20.0f;
    }

    // Get altitude-based color
    QColor altitudeColor = getAltitudeColor(altitude);

    // Create text symbol
    TextSymbol* symbol = new TextSymbol(aircraftChar,
                                        altitudeColor,
                                        fontSize,
                                        HorizontalAlignment::Center,
                                        VerticalAlignment::Middle,
                                        this);

    symbol->setFontFamily("Arial Unicode MS");


    return symbol;
}

void FlightTracker::onFlightDataReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }

    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Flight data request failed:" << reply->errorString();
        return;
    }

    QByteArray data = reply->readAll();
    qDebug() << "Flight data received, size:" << data.size() << "bytes";

    m_lastUpdateDateTime = QDateTime::currentDateTime();
    updateDisplayTime();

    m_flightOverlay->graphics()->clear();
    m_flightDataCache.clear();

    QSet<QString> allCountries;

    // Parse JSON response
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();


    if (obj.contains("states")) {
        QJsonArray states = obj["states"].toArray();
        qDebug() << "Adding" << states.size() << "flights to map";

        //for persistant selection
        Graphic* graphicToSelect = nullptr;
        QJsonArray flightDataToSelect;

        for (const QJsonValue& value : std::as_const(states)) {
            QJsonArray flight = value.toArray();
            if (flight.size() >= 17) {

                QString country = extractCountryFromFlight(flight);
                if (!country.isEmpty()) {
                    allCountries.insert(country);
                }

                // Extract flight data
                QString callsign = flight[1].toString().trimmed();
                double longitude = flight[5].toDouble();
                double latitude = flight[6].toDouble();
                double altitude = flight[7].toDouble();        // Add altitude
                bool onGround = flight[8].toBool();
                double heading = flight[10].toDouble();

                // Skip flights with invalid coordinates
                if (longitude == 0.0 && latitude == 0.0) continue;

                QString cacheKey = QString::number(m_flightOverlay->graphics()->size());
                m_flightDataCache[cacheKey] = flight;

                // Skip if heading is null/invalid
                if (std::isnan(heading)) {
                    heading = 0.0;
                }

                //for persistant selection
                if (!m_selectedIcao24.isEmpty() && flight[0].toString() == m_selectedIcao24) {
                    flightDataToSelect = flight;
                }

                // Get category from callsign
                int category = getCategoryFromCallsign(callsign);

                // Get the configured symbol with altitude-based coloring
                TextSymbol* symbol = getSymbolForCategory(category, onGround, altitude);


                double adjustedHeading = heading - 45.0;

                // Ensure the angle stays within 0-360 range
                if (adjustedHeading < 0) {
                    adjustedHeading += 360.0;
                }

                // Set the rotation angle
                symbol->setAngle(adjustedHeading);

                Point flightPoint(longitude, latitude, SpatialReference::wgs84());
                Graphic* flightGraphic = new Graphic(flightPoint, symbol, this);

                //for persistant selection
                if (!flightDataToSelect.isEmpty() && flightDataToSelect == flight) {
                    graphicToSelect = flightGraphic;
                }

                m_flightOverlay->graphics()->append(flightGraphic);
            }
        }

        // organize into continents
        QVariantMap continentCountries = {
            {"Africa", QStringList()}, {"Asia", QStringList()}, {"Europe", QStringList()},
            {"North America", QStringList()}, {"Oceania", QStringList()},
            {"South America", QStringList()}, {"Other", QStringList()}
        };

        for (const QString& country : allCountries) {
            QString continent = getCountryContinent(country);
            QStringList currentList = continentCountries[continent].toStringList();
            currentList.append(country);
            continentCountries[continent] = currentList;
        }

        // Remove empty continents and sort
        QVariantMap finalData;
        for (auto it = continentCountries.begin(); it != continentCountries.end(); ++it) {
            QStringList countries = it.value().toStringList();
            if (!countries.isEmpty()) {
                countries.sort();
                finalData[it.key()] = countries;
            }
        }

        m_availableCountries = finalData;
        emit availableCountriesChanged();

        if (m_isInitialLoad) {
            QStringList allCountries;
            for (auto it = finalData.begin(); it != finalData.end(); ++it) {
                QStringList countries = it.value().toStringList();
                allCountries.append(countries);
            }
            setSelectedCountries(allCountries);
            m_isInitialLoad = false;
        } else {
            // On subsequent updates, apply the current filter to new data
            applyFilters();
        }

        //Restore selection
        if (graphicToSelect && !flightDataToSelect.isEmpty()) {
            m_selectedGraphic = graphicToSelect;
            createFlightPopup(m_selectedGraphic, flightDataToSelect);
            updateSelectionGraphic();
            if (m_showTrack) {
                fetchFlightTrack(m_selectedIcao24);
            }
        }
    }
}

void FlightTracker::selectFlightAtPoint(QPointF screenPoint)
{
    if (!m_mapView || !m_flightOverlay) {
        return;
    }

    // Store the currently found flight (if any)
    Graphic* foundGraphic = nullptr;
    QJsonArray foundFlightData;

    // Manual hit testing approach using coordinate recreation
    GraphicListModel* graphics = m_flightOverlay->graphics();
    constexpr double tolerancePixels = 15.0;

    for (int i = 0; i < graphics->size(); ++i) {
        Graphic* graphic = graphics->at(i);
        if (!graphic) continue;

        // Skip hidden flights - only allow selection of visible flights
        if (!graphic->isVisible()) {
            continue;
        }

        // Get flight data from cache to recreate the point
        QString cacheKey = QString::number(i);
        if (m_flightDataCache.contains(cacheKey)) {
            QJsonArray flightData = m_flightDataCache[cacheKey];

            // Extract coordinates from cached data
            double longitude = flightData[5].toDouble();
            double latitude = flightData[6].toDouble();

            // Skip invalid coordinates
            if (longitude == 0.0 && latitude == 0.0) continue;

            // Create a point and convert to screen coordinates
            Point flightPoint(longitude, latitude, SpatialReference::wgs84());
            QPointF flightScreen = m_mapView->locationToScreen(flightPoint);

            // Calculate screen distance
            double distance = QLineF(screenPoint, flightScreen).length();

            if (distance <= tolerancePixels) {
                foundGraphic = graphic;
                foundFlightData = flightData;
                qDebug() << "Flight found at index:" << i << "distance:" << distance;
                break; // Found one, stop searching
            }
        }
    }

    // Now handle the selection based on what we found
    if (foundGraphic) {
        // Only update if it's a different flight
        if (m_selectedGraphic != foundGraphic) {
            // Clear previous popup without affecting the selection state
            if (m_selectedFlightPopup) {
                Popup* oldPopup = m_selectedFlightPopup;
                m_selectedFlightPopup = nullptr;
                QTimer::singleShot(50, oldPopup, &QObject::deleteLater);
            }

            // Set new selection
            m_selectedGraphic = foundGraphic;
            m_selectedIcao24 = foundFlightData[0].toString();
            createFlightPopup(foundGraphic, foundFlightData);
            updateSelectionGraphic();

            if (m_showTrack) {
                fetchFlightTrack(m_selectedIcao24);
            }
        }
    } else {
        // do this to close popup on clicking elsewhere
        //clearFlightSelection();
    }
}

void FlightTracker::clearFlightSelection()
{
    m_selectedFlightTitle.clear();
    m_selectedFlightInfo.clear();
    m_selectedIcao24.clear();

    // Emit signal BEFORE clearing the popup - application closes if we emit after :)
    emit selectedFlightChanged();

    // Now clear the rest
    m_selectedGraphic = nullptr;

    if (m_selectedFlightPopup) {
        Popup* oldPopup = m_selectedFlightPopup;
        m_selectedFlightPopup = nullptr;
        QTimer::singleShot(50, oldPopup, &QObject::deleteLater);
    }
    m_selectionOverlay->graphics()->clear();
    clearTrackGraphics();
}

void FlightTracker::createFlightPopup(Esri::ArcGISRuntime::Graphic* flightGraphic, const QJsonArray& flightData)
{
    if (!flightGraphic || flightData.isEmpty())
        return;

    // Clear previous popup safely
    if (m_selectedFlightPopup) {
        Popup* oldPopup = m_selectedFlightPopup;
        m_selectedFlightPopup = nullptr;
        QTimer::singleShot(50, oldPopup, &QObject::deleteLater);
    }

    // Extract flight information
    QString icao24 = flightData[0].toString();
    QString callsign = flightData[1].toString().trimmed();
    QString country = flightData[2].toString();
    double longitude = flightData[5].toDouble();
    double latitude = flightData[6].toDouble();
    double altitude = flightData[7].toDouble();
    bool onGround = flightData[8].toBool();
    double velocity = flightData[9].toDouble();
    double heading = flightData[10].toDouble();
    double verticalRate = flightData[11].toDouble();
    QString squawk = flightData[14].toString();

    // Set title
    QString title = callsign.isEmpty() ? QString("Flight %1").arg(icao24.left(6)) : callsign;

    // Create popup definition
    PopupDefinition* popupDef = new PopupDefinition(this);
    popupDef->setTitle(title);

    // Build the HTML content with white/off-white text colors
    QString htmlContent = "<div style='font-family: Arial, sans-serif; color: #FFFFFF; background-color: transparent;'>";
    htmlContent += "<table style='border-collapse: collapse; width: 100%; color: #FFFFFF;'>";

    auto addRow = [&](const QString& label, const QString& value) {
        // Use white text and light gray borders for dark theme
        htmlContent += QString("<tr><td style='padding: 4px; border-bottom: 1px solid #4A4A4A; font-weight: bold; width: 40%; color: #F8F8F8;'>%1:</td>")
                           .arg(label);
        htmlContent += QString("<td style='padding: 4px; border-bottom: 1px solid #4A4A4A; color: #FFFFFF;'>%2</td></tr>")
                           .arg(value);
    };

    addRow("ICAO24", icao24);
    addRow("Callsign", callsign.isEmpty() ? "Unknown" : callsign);
    addRow("Country", country.isEmpty() ? "Unknown" : country);
    addRow("Status", onGround ? "On Ground" : "Airborne");
    addRow("Position", QString("%1°, %2°").arg(latitude, 0, 'f', 6).arg(longitude, 0, 'f', 6));

    if (altitude > 0) {
        addRow("Altitude", QString("%1 m (%2 ft)")
                   .arg(altitude, 0, 'f', 0)
                   .arg(altitude * 3.28084, 0, 'f', 0));
    } else {
        addRow("Altitude", "Unknown");
    }

    if (velocity > 0) {
        addRow("Speed", QString("%1 m/s (%2 knots)")
                   .arg(velocity, 0, 'f', 1)
                   .arg(velocity * 1.94384, 0, 'f', 1));
    } else {
        addRow("Speed", "Unknown");
    }

    if (!std::isnan(heading)) {
        addRow("Heading", QString("%1°").arg(heading, 0, 'f', 0));
    } else {
        addRow("Heading", "Unknown");
    }

    if (!std::isnan(verticalRate)) {
        QString verticalRateStr = verticalRate > 0 ? "Climbing" : (verticalRate < 0 ? "Descending" : "Level");
        addRow("Vertical Rate", QString("%1 m/s (%2)")
                                    .arg(verticalRate, 0, 'f', 1)
                                    .arg(verticalRateStr));
    } else {
        addRow("Vertical Rate", "Unknown");
    }

    if (!squawk.isEmpty()) {
        addRow("Squawk", squawk);
    }

    htmlContent += "</table></div>";

    // Create TextPopupElement with the content and parent
    TextPopupElement* textElement = new TextPopupElement(htmlContent, this);

    // Add the text element to the popup definition
    QList<PopupElement*> elements;
    elements.append(textElement);
    popupDef->setElements(elements);

    // Create popup from the graphic
    m_selectedFlightPopup = new Popup(flightGraphic, popupDef, this);

    // Set the string properties for QML compatibility
    m_selectedFlightTitle = title;
    m_selectedFlightInfo = QString("Flight information for %1").arg(title);

    emit selectedFlightChanged();
    qDebug() << "Popup created successfully for flight:" << callsign;
}

void FlightTracker::updateDisplayTime()
{
    if (!m_lastUpdateDateTime.isValid()) {
        m_lastUpdateTime = "Never";
    } else {
        qint64 secondsAgo = m_lastUpdateDateTime.secsTo(QDateTime::currentDateTime());

        if (secondsAgo < 60) {
            m_lastUpdateTime = secondsAgo <= 5 ? "Just now" : QString("%1s ago").arg(secondsAgo);
        } else if (secondsAgo < 3600) {
            int minutesAgo = secondsAgo / 60;
            m_lastUpdateTime = QString("%1m ago").arg(minutesAgo);
        } else {
            int hoursAgo = secondsAgo / 3600;
            m_lastUpdateTime = QString("%1h ago").arg(hoursAgo);
        }
    }

    emit lastUpdateTimeChanged();
}

void FlightTracker::loadCountryMappings()
{
    QFile file(":/resources/countries.json");  // Adjust path as needed
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Could not open countries.json file";
        return;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray countries = doc.array();

    for (const QJsonValue& value : countries) {
        QJsonObject country = value.toObject();
        QString countryName = country["country"].toString();

        // Handle the continent field (can be string or array for Russia)
        QJsonValue continentValue = country["continent"];
        QString continent;

        if (continentValue.isArray()) {
            // For countries like Russia that span multiple continents, use the first one
            QJsonArray continentArray = continentValue.toArray();
            if (!continentArray.isEmpty()) {
                continent = continentArray[0].toString();
            }
        } else {
            continent = continentValue.toString();
        }

        if (!countryName.isEmpty() && !continent.isEmpty()) {
            m_countryToContinentMap[countryName] = continent;
        }
    }

    qDebug() << "Loaded" << m_countryToContinentMap.size() << "country mappings";
}

QString FlightTracker::extractCountryFromFlight(const QJsonArray& flightData)
{
    if (flightData.size() >= 17) {
        QString country = flightData[2].toString().trimmed();
        return country.isEmpty() ? QString() : country;
    }
    return QString();
}

QString FlightTracker::getCountryContinent(const QString& country)
{
    // First try exact match
    QString exactMatch = m_countryToContinentMap.value(country, QString());
    if (!exactMatch.isEmpty()) {
        return exactMatch;
    }

    // Convert to lowercase for case-insensitive matching
    QString lowerCountry = country.toLower();

    if (lowerCountry == "republic of korea") {
        return m_countryToContinentMap.value("South Korea", "Asia");
    }

    // Try to find the core country name within the full official name
    for (auto it = m_countryToContinentMap.begin(); it != m_countryToContinentMap.end(); ++it) {
        QString jsonCountryName = it.key().toLower();
        QString continent = it.value();

        // Check both directions: does the OpenSky name contain the JSON name,
        // or does the JSON name contain the OpenSky name
        if (lowerCountry.contains(jsonCountryName) || jsonCountryName.contains(lowerCountry)) {
            qDebug() << "Matched:" << country << "with" << it.key() << "→" << continent;
            return continent;
        }
    }

    // Special handling for common word variations
    static QMap<QString, QString> wordReplacements = {
                                                      {"republic of", ""},
                                                      {"kingdom of", ""},
                                                      {"state of", ""},
                                                      {"federation", ""},
                                                      {"democratic", ""},
                                                      {"people's", ""},
                                                      {"islamic", ""},
                                                      {"arab", ""},
                                                      {"socialist", ""},
                                                      {"federal", ""},
                                                      {"united", ""},
                                                      {"the ", ""},
                                                      {" the", ""},
                                                      };

    // Create simplified versions by removing common official words
    QString simplifiedOpenSky = lowerCountry;
    for (auto it = wordReplacements.begin(); it != wordReplacements.end(); ++it) {
        simplifiedOpenSky = simplifiedOpenSky.replace(it.key(), it.value()).trimmed();
    }

    // Try matching with simplified names
    for (auto it = m_countryToContinentMap.begin(); it != m_countryToContinentMap.end(); ++it) {
        QString simplifiedJson = it.key().toLower();
        for (auto wordIt = wordReplacements.begin(); wordIt != wordReplacements.end(); ++wordIt) {
            simplifiedJson = simplifiedJson.replace(wordIt.key(), wordIt.value()).trimmed();
        }

        if (simplifiedOpenSky.contains(simplifiedJson) || simplifiedJson.contains(simplifiedOpenSky)) {
            qDebug() << "Simplified match:" << country << "(" << simplifiedOpenSky << ") with"
                     << it.key() << "(" << simplifiedJson << ") →" << it.value();
            return it.value();
        }
    }

    // Handle common name variations manually for problematic cases
    if (lowerCountry.contains("republic of korea")) return "Asia";
    if (lowerCountry.contains("democratic people's republic of korea")) return "Asia";
    if (lowerCountry.contains("netherlands")) return "Europe";
    if (lowerCountry.contains("korea")) return "Asia";
    if (lowerCountry.contains("moldova")) return "Europe";
    if (lowerCountry.contains("russia")) return "Europe";  // Could be Asia too
    if (lowerCountry.contains("vietnam") || lowerCountry.contains("viet nam")) return "Asia";
    if (lowerCountry.contains("libya")) return "Africa";
    if (lowerCountry.contains("iran")) return "Asia";
    if (lowerCountry.contains("syria")) return "Asia";
    if (lowerCountry.contains("macedonia")) return "Europe";
    if (lowerCountry.contains("bosnia")) return "Europe";
    if (lowerCountry.contains("congo")) return "Africa";

    // Log unknown countries for future reference
    qDebug() << "No continent mapping found for:" << country;
    return "Other";
}

void FlightTracker::setSelectedCountries(const QStringList& countries)
{
    if (m_selectedCountries != countries) {
        m_selectedCountries = countries;
        emit selectedCountriesChanged();

        // Apply filtering to existing flights immediately
        applyFilters();
    }
}

void FlightTracker::applyFilters()
{
    if (!m_flightOverlay) return;

    GraphicListModel* graphics = m_flightOverlay->graphics();

    for (int i = 0; i < graphics->size(); ++i) {
        Graphic* graphic = graphics->at(i);
        if (!graphic) continue;

        QString cacheKey = QString::number(i);
        if (m_flightDataCache.contains(cacheKey)) {
            QJsonArray flightData = m_flightDataCache[cacheKey];
            QString country = extractCountryFromFlight(flightData);
            bool onGround = flightData[8].toBool();
            double altitude = flightData[7].toDouble() * 3.28084; // Convert to feet
            double speed = flightData[9].toDouble() * 1.94384;   // Convert to knots

            // Check country filter
            bool countryMatch = m_selectedCountries.contains(country);

            // Check status filter
            bool statusMatch = true;
            if (m_selectedFlightStatus == "Airborne") {
                statusMatch = !onGround;
            } else if (m_selectedFlightStatus == "OnGround") {
                statusMatch = onGround;
            }

            bool altitudeMatch = true;
            double flightAltitude = 0.0;

            if (!std::isnan(altitude) && altitude > 0) {
                flightAltitude = altitude;  // Use actual altitude from API
            } else {
                flightAltitude = 0.0;  // Unknown/invalid altitude treated as ground level
            }

            // Apply altitude filter based on actual altitude
            if (m_maxAltitudeFilter >= 40000.0) {
                altitudeMatch = (flightAltitude >= m_minAltitudeFilter);
            } else {
                altitudeMatch = (flightAltitude >= m_minAltitudeFilter && flightAltitude <= m_maxAltitudeFilter);
            }

            bool speedMatch = true;
            double flightSpeed = 0.0;

            if (!std::isnan(speed) && speed > 0) {
                flightSpeed = speed;  // Use actual speed from API
            } else {
                // Set defaults for unknown speeds based on altitude
                if (!std::isnan(altitude) && altitude > 1000) {
                    flightSpeed = 150.0;  // Unknown airborne speed: assume ~150 knots (typical cruise)
                } else {
                    flightSpeed = 0.0;    // Unknown ground speed: assume stationary
                }
            }

            // Apply speed filter based on actual/estimated speed
            if (m_maxSpeedFilter >= 600.0) {
                speedMatch = (flightSpeed >= m_minSpeedFilter);
            } else {
                speedMatch = (flightSpeed >= m_minSpeedFilter && flightSpeed <= m_maxSpeedFilter);
            }

            bool verticalMatch = true;
            if (m_selectedVerticalStatus != "All") {
                double verticalRate = flightData[11].toDouble(); // vertical_rate from OpenSky API

                if (m_selectedVerticalStatus == "Climbing") {
                    verticalMatch = (!std::isnan(verticalRate) && verticalRate > 0.5); // > 0.5 m/s climbing
                } else if (m_selectedVerticalStatus == "Descending") {
                    verticalMatch = (!std::isnan(verticalRate) && verticalRate < -0.5); // < -0.5 m/s descending
                } else if (m_selectedVerticalStatus == "Level") {
                    verticalMatch = (std::isnan(verticalRate) || (verticalRate >= -0.5 && verticalRate <= 0.5)); // Level or unknown
                }
            }

            // Show only if all filters match
            bool shouldShow = countryMatch && statusMatch && altitudeMatch && speedMatch && verticalMatch;
            graphic->setVisible(shouldShow);

            if (!shouldShow && m_selectedGraphic && graphic == m_selectedGraphic) {
                clearFlightSelection();
            }
        }
    }
}

void FlightTracker::setSelectedFlightStatus(const QString& status)
{
    if (m_selectedFlightStatus != status) {
        m_selectedFlightStatus = status;
        emit selectedFlightStatusChanged();
        applyFilters();
    }
}

void FlightTracker::setMinAltitudeFilter(double minAlt)
{
    if (m_minAltitudeFilter != minAlt) {
        m_minAltitudeFilter = minAlt;
        emit altitudeFilterChanged();
        applyFilters();
    }
}

void FlightTracker::setMaxAltitudeFilter(double maxAlt)
{
    if (m_maxAltitudeFilter != maxAlt) {
        m_maxAltitudeFilter = maxAlt;
        emit altitudeFilterChanged();
        applyFilters();
    }
}

void FlightTracker::setMinSpeedFilter(double minSpeed)
{
    if (m_minSpeedFilter != minSpeed) {
        m_minSpeedFilter = minSpeed;
        emit speedFilterChanged();
        applyFilters();
    }
}

void FlightTracker::setMaxSpeedFilter(double maxSpeed)
{
    if (m_maxSpeedFilter != maxSpeed) {
        m_maxSpeedFilter = maxSpeed;
        emit speedFilterChanged();
        applyFilters();
    }
}

void FlightTracker::setSelectedVerticalStatus(const QString& status)
{
    if (m_selectedVerticalStatus != status) {
        m_selectedVerticalStatus = status;
        emit selectedVerticalStatusChanged();
        applyFilters();
    }
}

void FlightTracker::updateSelectionGraphic()
{
    m_selectionOverlay->graphics()->clear();

    if (!m_selectedGraphic || !m_flightOverlay)
        return;

    auto* originalSymbol = dynamic_cast<TextSymbol*>(m_selectedGraphic->symbol());
    if (!originalSymbol)
        return;

    GraphicListModel* graphics = m_flightOverlay->graphics();
    int selectedIndex = -1;
    for (int i = 0; i < graphics->size(); ++i) {
        if (graphics->at(i) == m_selectedGraphic) {
            selectedIndex = i;
            break;
        }
    }

    if (selectedIndex == -1)
        return;

    QString cacheKey = QString::number(selectedIndex);
    QJsonArray flightData = m_flightDataCache.value(cacheKey);
    if (flightData.isEmpty())
        return;

    double longitude = flightData[5].toDouble();
    double latitude = flightData[6].toDouble();
    Point flightPoint(longitude, latitude, SpatialReference::wgs84());

    QString callsign = flightData[1].toString().trimmed();
    QString icao = flightData[0].toString();
    QString labelText = !callsign.isEmpty() ? callsign : icao.left(6);

    float originalSize = originalSymbol->size();
    float ringSize = originalSize + 14.0f;

    SimpleLineSymbol* outline = new SimpleLineSymbol(
        SimpleLineSymbolStyle::Solid,
        QColor(255, 255, 255),
        2.5f,
        this
        );

    SimpleMarkerSymbol* ringSymbol = new SimpleMarkerSymbol(
        SimpleMarkerSymbolStyle::Circle,
        QColor(Qt::transparent),
        ringSize,
        this
        );
    ringSymbol->setOutline(outline);

    Graphic* ringGraphic = new Graphic(flightPoint, ringSymbol, this);
    m_selectionOverlay->graphics()->append(ringGraphic);

    TextSymbol* labelSymbol = new TextSymbol(labelText,
                                             QColor(Qt::white),
                                             14.0f,
                                             HorizontalAlignment::Center,
                                             VerticalAlignment::Top,
                                             this);
    labelSymbol->setFontFamily("Arial");
    labelSymbol->setAngle(0.0);
    labelSymbol->setHaloColor(Qt::gray);
    labelSymbol->setOffsetY(-16.0f);
    labelSymbol->setHaloWidth(1.0f);

    Point labelPoint(longitude, latitude - 0.0003, SpatialReference::wgs84());
    Graphic* labelGraphic = new Graphic(labelPoint, labelSymbol, this);
    m_selectionOverlay->graphics()->append(labelGraphic);
}

void FlightTracker::setShowTrack(bool show)
{
    if (m_showTrack != show) {
        m_showTrack = show;
        emit showTrackChanged();

        if (m_showTrack && m_selectedGraphic) {
            // Find the ICAO24 for selected flight
            GraphicListModel* graphics = m_flightOverlay->graphics();
            for (int i = 0; i < graphics->size(); ++i) {
                if (graphics->at(i) == m_selectedGraphic) {
                    QString cacheKey = QString::number(i);
                    if (m_flightDataCache.contains(cacheKey)) {
                        QJsonArray flightData = m_flightDataCache[cacheKey];
                        QString icao24 = flightData[0].toString();
                        fetchFlightTrack(icao24);
                    }
                    break;
                }
            }
        } else if (!m_showTrack) {
            clearTrackGraphics();
        }
    }
}

void FlightTracker::fetchFlightTrack(const QString& icao24)
{
    if (m_accessToken.isEmpty() || icao24.isEmpty()) {
        return;
    }

    qDebug() << "Fetching track for aircraft:" << icao24;

    // Use last update timestamp
    qint64 timestamp = m_lastUpdateDateTime.toSecsSinceEpoch();

    QUrl trackUrl(QString("https://opensky-network.org/api/tracks/all?icao24=%1&time=%2")
                      .arg(icao24.toLower())
                      .arg(timestamp));

    QNetworkRequest request(trackUrl);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_accessToken).toUtf8());

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &FlightTracker::onTrackDataReply);
}

void FlightTracker::clearTrackGraphics()
{
    if (m_trackOverlay) {
        m_trackOverlay->graphics()->clear();
    }
}

void FlightTracker::onTrackDataReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }

    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Track data request failed:" << reply->errorString();
        clearTrackGraphics();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject trackObj = doc.object();

    if (!trackObj.isEmpty()) {
        drawFlightTrack(trackObj);
    }
}

void FlightTracker::drawFlightTrack(const QJsonObject& trackData)
{
    clearTrackGraphics();

    if (!trackData.contains("path")) {
        qDebug() << "No path data in track response";
        return;
    }

    QJsonArray path = trackData["path"].toArray();
    if (path.isEmpty()) {
        qDebug() << "Empty path in track data";
        return;
    }

    // Collect all valid points
    QList<Point> trackPoints;
    QList<double> altitudes;

    for (const QJsonValue& value : path) {
        QJsonArray waypoint = value.toArray();
        if (waypoint.size() >= 6) {
            double lat = waypoint[1].toDouble();
            double lon = waypoint[2].toDouble();
            double altitude = waypoint[3].toDouble(); // meters
            bool onGround = waypoint[5].toBool();

            // Skip invalid points
            if (lat == 0.0 && lon == 0.0) continue;

            Point point(lon, lat, SpatialReference::wgs84());
            trackPoints.append(point);

            // Use 0 altitude if on ground or null
            if (onGround || std::isnan(altitude)) {
                altitude = 0.0;
            }
            altitudes.append(altitude);
        }
    }

    if (trackPoints.size() < 2) {
        qDebug() << "Not enough points for track";
        return;
    }

    // Draw line segments between consecutive points
    // Each segment gets colored based on the average altitude
    for (int i = 0; i < trackPoints.size() - 1; ++i) {
        PolylineBuilder polylineBuilder(SpatialReference::wgs84());
        polylineBuilder.addPoint(trackPoints[i]);
        polylineBuilder.addPoint(trackPoints[i + 1]);

        Polyline segment = polylineBuilder.toPolyline();

        // Use average altitude of the two points for color
        double avgAltitude = (altitudes[i] + altitudes[i + 1]) / 2.0;
        QColor lineColor = getAltitudeColor(avgAltitude);

        SimpleLineSymbol* lineSymbol = new SimpleLineSymbol(
            SimpleLineSymbolStyle::Solid,
            lineColor,
            3.0f,  // line width
            this
            );
        lineSymbol->setAntiAlias(true);

        Graphic* segmentGraphic = new Graphic(segment, lineSymbol, this);
        m_trackOverlay->graphics()->append(segmentGraphic);
    }

    // Add small dots at each waypoint
    for (int i = 0; i < trackPoints.size(); ++i) {
        SimpleMarkerSymbol* dotSymbol = new SimpleMarkerSymbol(
            SimpleMarkerSymbolStyle::Circle,
            Qt::white,
            3.0f,  // small dots
            this
            );
        dotSymbol->setOutline(new SimpleLineSymbol(
            SimpleLineSymbolStyle::Solid,
            Qt::darkGray,
            0.5f,
            this
            ));

        Graphic* dotGraphic = new Graphic(trackPoints[i], dotSymbol, this);
        m_trackOverlay->graphics()->append(dotGraphic);
    }
}



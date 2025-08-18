#include "FlightTracker.h"
#include "OpenSkyAuthManager.h"
#include "FlightDataService.h"
#include "FlightRenderer.h"
#include "Map.h"
#include "MapQuickView.h"
#include "MapTypes.h"
#include "GraphicsOverlay.h"
#include "Graphic.h"
#include "GraphicListModel.h"
#include "GraphicsOverlayListModel.h"
#include "Popup.h"
#include "PopupDefinition.h"
#include "TextPopupElement.h"
#include "Point.h"
#include "SpatialReference.h"
#include "GeoElement.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineF>
#include <QTimer>
#include <QDebug>

using namespace Esri::ArcGISRuntime;

FlightTracker::FlightTracker(QObject *parent)
    : QObject(parent)
    , m_map(new Map(BasemapStyle::ArcGISHumanGeographyDark, this))
    , m_authManager(new OpenSkyAuthManager(this))
    , m_dataService(new FlightDataService(this))
    , m_renderer(new FlightRenderer(this))
    , m_flightOverlay(new GraphicsOverlay(this))
    , m_selectionOverlay(new GraphicsOverlay(this))
    , m_trackOverlay(new GraphicsOverlay(this))
    , m_displayUpdateTimer(new QTimer(this))
    , m_flightUpdateTimer(new QTimer(this))
    , m_filterUpdateTimer(new QTimer(this))
{
    loadConfig();
    loadCountryMappings();
    
    // Connect authentication signals
    connect(m_authManager, &OpenSkyAuthManager::authenticationSuccess,
            this, &FlightTracker::onAuthenticationSuccess);
    connect(m_authManager, &OpenSkyAuthManager::authenticationFailed,
            this, &FlightTracker::onAuthenticationFailed);
    
    // Connect data service signals
    connect(m_dataService, &FlightDataService::flightDataReceived,
            this, &FlightTracker::onFlightDataReceived);
    connect(m_dataService, &FlightDataService::trackDataReceived,
            this, &FlightTracker::onTrackDataReceived);
    connect(m_dataService, &FlightDataService::dataFetchFailed,
            this, &FlightTracker::onDataFetchFailed);
    
    // Setup timers
    m_displayUpdateTimer->setInterval(1000);
    connect(m_displayUpdateTimer, &QTimer::timeout, this, &FlightTracker::updateDisplayTime);
    m_displayUpdateTimer->start();
    
    m_flightUpdateTimer->setInterval(60000); // 1 minute auto-refresh
    connect(m_flightUpdateTimer, &QTimer::timeout, this, &FlightTracker::fetchFlightData);
    
    // Setup filter debounce timer
    m_filterUpdateTimer->setSingleShot(true);
    m_filterUpdateTimer->setInterval(150); // 150ms debounce
    connect(m_filterUpdateTimer, &QTimer::timeout, this, &FlightTracker::applyFilters);
    
    // Start authentication
    m_authManager->authenticate();
}

FlightTracker::~FlightTracker() = default;

void FlightTracker::loadConfig()
{
    QFile configFile(":/config/Config/config.json");
    if (configFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(configFile.readAll());
        QJsonObject config = doc.object();
        QJsonObject opensky = config["opensky"].toObject();
        
        QString clientId = opensky["client_id"].toString();
        QString clientSecret = opensky["client_secret"].toString();
        
        m_authManager->setCredentials(clientId, clientSecret);
        qDebug() << "OpenSky credentials loaded from config.json";
    } else {
        qWarning() << "Could not open config.json - OpenSky credentials are required";
    }
}

MapQuickView *FlightTracker::mapView() const
{
    return m_mapView;
}

void FlightTracker::setMapView(MapQuickView *mapView)
{
    if (!mapView || mapView == m_mapView) {
        return;
    }

    m_mapView = mapView;
    m_mapView->setMap(m_map);

    // Add overlays in correct order
    m_mapView->graphicsOverlays()->append(m_trackOverlay);
    m_mapView->graphicsOverlays()->append(m_flightOverlay);
    m_mapView->graphicsOverlays()->append(m_selectionOverlay);

    emit mapViewChanged();
}

bool FlightTracker::isAuthenticated() const
{
    return m_authManager->isAuthenticated();
}

bool FlightTracker::hasSelectedFlight() const
{
    return m_selectedFlight.isValid();
}

void FlightTracker::setShowTrack(bool show)
{
    if (m_showTrack != show) {
        m_showTrack = show;
        emit showTrackChanged();
        
        if (m_showTrack && m_selectedFlight.isValid()) {
            m_dataService->fetchFlightTrack(m_selectedFlight.icao24());
        } else if (!m_showTrack) {
            m_trackOverlay->graphics()->clear();
        }
    }
}

void FlightTracker::fetchFlightData()
{
    if (!isAuthenticated()) {
        qDebug() << "Not authenticated, cannot fetch flight data";
        return;
    }
    
    clearFlightSelection();
    m_dataService->fetchFlightData();
}

void FlightTracker::selectFlightAtPoint(QPointF screenPoint)
{
    FlightData flight = findFlightAtPoint(screenPoint);
    
    if (flight.isValid() && !(flight.icao24() == m_selectedFlight.icao24())) {
        m_selectedFlight = flight;
        createFlightPopup(flight);
        m_renderer->createSelectionGraphic(m_selectionOverlay, flight);
        
        if (m_showTrack) {
            m_dataService->fetchFlightTrack(flight.icao24());
        }
        
        emit selectedFlightChanged();
    }
}

void FlightTracker::clearFlightSelection()
{
    try {
        // Clear the flight data first
        m_selectedFlight = FlightData();
        
        // CRITICAL: Emit signal BEFORE clearing popup
        emit selectedFlightChanged();
        
        // Now safely handle popup deletion
        if (m_selectedFlightPopup) {
            Popup* oldPopup = m_selectedFlightPopup;
            m_selectedFlightPopup = nullptr;
            QTimer::singleShot(50, oldPopup, &QObject::deleteLater);
        }
        
        // Clear overlays safely
        if (m_selectionOverlay && m_selectionOverlay->graphics()) {
            m_selectionOverlay->graphics()->clear();
        }
        
        if (m_trackOverlay && m_trackOverlay->graphics()) {
            m_trackOverlay->graphics()->clear();
        }
        
    } catch (...) {
        qDebug() << "Exception in clearFlightSelection";
    }
}

QVariantList FlightTracker::getSelectedFlightData()
{
    QVariantList result;
    if (m_selectedFlight.isValid()) {
        result << m_selectedFlight.icao24() << m_selectedFlight.callsign() 
               << m_selectedFlight.country() << "" << "" 
               << m_selectedFlight.longitude() << m_selectedFlight.latitude()
               << m_selectedFlight.altitude() << m_selectedFlight.onGround()
               << m_selectedFlight.velocity() << m_selectedFlight.heading()
               << m_selectedFlight.verticalRate() << "" << "" << m_selectedFlight.squawk();
    }
    return result;
}

Popup* FlightTracker::selectedFlightPopup() const
{
    // Return popup only if it's still valid and hasn't been marked for deletion
    return m_selectedFlightPopup;
}

void FlightTracker::setSelectedCountries(const QStringList& countries)
{
    if (m_selectedCountries != countries) {
        m_selectedCountries = countries;
        emit selectedCountriesChanged();
        scheduleFilterUpdate();
    }
}

void FlightTracker::setSelectedFlightStatus(const QString& status)
{
    if (m_selectedFlightStatus != status) {
        m_selectedFlightStatus = status;
        emit selectedFlightStatusChanged();
        scheduleFilterUpdate();
    }
}

void FlightTracker::setMinAltitudeFilter(double minAlt)
{
    if (m_minAltitudeFilter != minAlt) {
        m_minAltitudeFilter = minAlt;
        emit altitudeFilterChanged();
        scheduleFilterUpdate();
    }
}

void FlightTracker::setMaxAltitudeFilter(double maxAlt)
{
    if (m_maxAltitudeFilter != maxAlt) {
        m_maxAltitudeFilter = maxAlt;
        emit altitudeFilterChanged();
        scheduleFilterUpdate();
    }
}

void FlightTracker::setMinSpeedFilter(double minSpeed)
{
    if (m_minSpeedFilter != minSpeed) {
        m_minSpeedFilter = minSpeed;
        emit speedFilterChanged();
        scheduleFilterUpdate();
    }
}

void FlightTracker::setMaxSpeedFilter(double maxSpeed)
{
    if (m_maxSpeedFilter != maxSpeed) {
        m_maxSpeedFilter = maxSpeed;
        emit speedFilterChanged();
        scheduleFilterUpdate();
    }
}

void FlightTracker::setSelectedVerticalStatus(const QString& status)
{
    if (m_selectedVerticalStatus != status) {
        m_selectedVerticalStatus = status;
        emit selectedVerticalStatusChanged();
        scheduleFilterUpdate();
    }
}

void FlightTracker::onAuthenticationSuccess()
{
    m_dataService->setAccessToken(m_authManager->accessToken());
    emit authenticationSuccess();
    emit authenticationChanged();
    
    fetchFlightData();
    m_flightUpdateTimer->start();
}

void FlightTracker::onAuthenticationFailed(const QString& error)
{
    emit authenticationFailed(error);
}

void FlightTracker::onFlightDataReceived(const QList<FlightData>& flights)
{
    if (m_isUpdatingFlights) {
        return;
    }
    
    m_isUpdatingFlights = true;
    
    try {
        // Clear selection first to avoid dangling references
        if (m_selectedFlight.isValid()) {
            clearFlightSelection();
        }
        
        m_flights = flights;
        m_lastUpdateDateTime = QDateTime::currentDateTime();
        updateDisplayTime();
        
        if (!m_renderer || !m_flightOverlay) {
            m_isUpdatingFlights = false;
            return;
        }
        
        // Clear existing graphics first
        m_flightOverlay->graphics()->clear();
        
        // Update graphics after a brief delay to ensure UI is ready
        QTimer::singleShot(50, this, [this, flights]() {
            try {
                m_renderer->updateFlightGraphics(m_flightOverlay, flights);
                m_isUpdatingFlights = false;
            } catch (...) {
                qDebug() << "Exception updating flight graphics";
                m_isUpdatingFlights = false;
            }
        });
        
    } catch (...) {
        qDebug() << "Exception in onFlightDataReceived";
        m_isUpdatingFlights = false;
        return;
    }
    
    // Process countries and update available countries on initial load
    if (m_isInitialLoad) {
        QMap<QString, QStringList> continentsWithCountries;
        QSet<QString> uniqueCountries;
        
        for (const FlightData& flight : flights) {
            QString country = extractCountryFromFlight(flight);
            if (!country.isEmpty() && !uniqueCountries.contains(country)) {
                uniqueCountries.insert(country);
                QString continent = getCountryContinent(country);
                continentsWithCountries[continent].append(country);
            }
        }
        
        // Sort countries within each continent
        for (auto& countries : continentsWithCountries) {
            std::sort(countries.begin(), countries.end());
        }
        
        // Convert to QVariantMap for QML
        QVariantMap availableCountries;
        for (auto it = continentsWithCountries.begin(); it != continentsWithCountries.end(); ++it) {
            availableCountries[it.key()] = QVariant::fromValue(it.value());
        }
        
        m_availableCountries = availableCountries;
        
        // Initialize selected countries to all countries
        QStringList allCountries = uniqueCountries.values();
        std::sort(allCountries.begin(), allCountries.end());
        m_selectedCountries = allCountries;
        
        m_isInitialLoad = false;
        
        emit availableCountriesChanged();
        emit selectedCountriesChanged();
        
        qDebug() << "Populated" << uniqueCountries.size() << "countries";
    }
    
    // Restore selection if possible
    if (m_selectedFlight.isValid()) {
        for (const FlightData& flight : flights) {
            if (flight.icao24() == m_selectedFlight.icao24()) {
                m_selectedFlight = flight;
                createFlightPopup(flight);
                m_renderer->createSelectionGraphic(m_selectionOverlay, flight);
                break;
            }
        }
    }
    
    // Apply current filters using debounced approach
    scheduleFilterUpdate();
    
    qDebug() << "Updated" << flights.size() << "flights on map";
}

void FlightTracker::onTrackDataReceived(const QString& icao24, const QJsonObject& trackData)
{
    if (m_selectedFlight.isValid() && m_selectedFlight.icao24() == icao24) {
        m_renderer->drawFlightTrack(m_trackOverlay, trackData);
    }
}

void FlightTracker::onDataFetchFailed(const QString& error)
{
    qWarning() << "Data fetch failed:" << error;
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
            m_lastUpdateTime = QString("%1m ago").arg(secondsAgo / 60);
        } else {
            m_lastUpdateTime = QString("%1h ago").arg(secondsAgo / 3600);
        }
    }
    
    emit lastUpdateTimeChanged();
}

FlightData FlightTracker::findFlightAtPoint(QPointF screenPoint)
{
    if (!m_mapView || m_flights.isEmpty()) {
        return FlightData();
    }

    constexpr double tolerancePixels = 15.0;
    GraphicListModel* graphics = m_flightOverlay->graphics();
    
    for (int i = 0; i < graphics->size() && i < m_flights.size(); ++i) {
        Graphic* graphic = graphics->at(i);
        if (!graphic || !graphic->isVisible()) continue;
        
        const FlightData& flight = m_flights[i];
        Point flightPoint(flight.longitude(), flight.latitude(), SpatialReference::wgs84());
        QPointF flightScreen = m_mapView->locationToScreen(flightPoint);
        
        double distance = QLineF(screenPoint, flightScreen).length();
        if (distance <= tolerancePixels) {
            return flight;
        }
    }
    
    return FlightData();
}

void FlightTracker::createFlightPopup(const FlightData& flight)
{
    qDebug() << "Creating flight popup for" << flight.icao24();
    
    try {
        // Clear previous popup safely (matching old implementation)
        if (m_selectedFlightPopup) {
            qDebug() << "Cleaning up existing popup";
            Popup* oldPopup = m_selectedFlightPopup;
            m_selectedFlightPopup = nullptr;
            QTimer::singleShot(50, oldPopup, &QObject::deleteLater);
        }

    QString title = flight.callsign().isEmpty() ? 
                   QString("Flight %1").arg(flight.icao24().left(6)) : flight.callsign();

    PopupDefinition* popupDef = new PopupDefinition(this);
    popupDef->setTitle(title);

    QString htmlContent = "<div style='font-family: Arial, sans-serif; color: #FFFFFF;'>";
    htmlContent += "<table style='border-collapse: collapse; width: 100%; color: #FFFFFF;'>";

    auto addRow = [&](const QString& label, const QString& value) {
        htmlContent += QString("<tr><td style='padding: 4px; border-bottom: 1px solid #4A4A4A; font-weight: bold; color: #F8F8F8;'>%1:</td>").arg(label);
        htmlContent += QString("<td style='padding: 4px; border-bottom: 1px solid #4A4A4A; color: #FFFFFF;'>%2</td></tr>").arg(value);
    };

    addRow("ICAO24", flight.icao24());
    addRow("Callsign", flight.callsign().isEmpty() ? "Unknown" : flight.callsign());
    addRow("Country", flight.country().isEmpty() ? "Unknown" : flight.country());
    addRow("Status", flight.onGround() ? "On Ground" : "Airborne");
    addRow("Position", QString("%1°, %2°").arg(flight.latitude(), 0, 'f', 6).arg(flight.longitude(), 0, 'f', 6));
    
    if (flight.altitude() > 0) {
        addRow("Altitude", QString("%1 m (%2 ft)")
                   .arg(flight.altitude(), 0, 'f', 0)
                   .arg(flight.altitude() * 3.28084, 0, 'f', 0));
    }
    
    if (flight.velocity() > 0) {
        addRow("Speed", QString("%1 m/s (%2 knots)")
                   .arg(flight.velocity(), 0, 'f', 1)
                   .arg(flight.velocity() * 1.94384, 0, 'f', 1));
    }

    htmlContent += "</table></div>";

    TextPopupElement* textElement = new TextPopupElement(htmlContent, this);
    QList<PopupElement*> elements;
    elements.append(textElement);
    popupDef->setElements(elements);

        // Use the existing graphic from the overlay instead of creating a new one
        // This matches the old implementation pattern
        GraphicListModel* graphics = m_flightOverlay->graphics();
        Graphic* flightGraphic = nullptr;
        
        // Find the graphic for this flight
        for (int i = 0; i < graphics->size() && i < m_flights.size(); ++i) {
            const FlightData& flightData = m_flights[i];
            if (flightData.icao24() == flight.icao24()) {
                flightGraphic = graphics->at(i);
                break;
            }
        }
        
        if (!flightGraphic) {
            qDebug() << "Failed to find graphic for popup";
            return;
        }
        
        // Create popup from the existing graphic (like old implementation)
        m_selectedFlightPopup = new Popup(flightGraphic, popupDef, this);
        qDebug() << "Popup created successfully from existing graphic";
        
        emit selectedFlightChanged();
        
    } catch (const std::exception& e) {
        qDebug() << "Exception creating popup:" << e.what();
        m_selectedFlightPopup = nullptr;
    } catch (...) {
        qDebug() << "Unknown exception creating popup";
        m_selectedFlightPopup = nullptr;
    }
}

void FlightTracker::loadCountryMappings()
{
    QFile file(":/resources/countries.json");
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

        QJsonValue continentValue = country["continent"];
        QString continent;

        if (continentValue.isArray()) {
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

QString FlightTracker::extractCountryFromFlight(const FlightData& flight)
{
    QString country = flight.country().trimmed();
    return country.isEmpty() ? QString() : country;
}

QString FlightTracker::getCountryContinent(const QString& country)
{
    QString exactMatch = m_countryToContinentMap.value(country, QString());
    if (!exactMatch.isEmpty()) {
        return exactMatch;
    }

    QString lowerCountry = country.toLower();

    if (lowerCountry == "republic of korea") {
        return m_countryToContinentMap.value("South Korea", "Asia");
    }

    for (auto it = m_countryToContinentMap.begin(); it != m_countryToContinentMap.end(); ++it) {
        QString jsonCountryName = it.key().toLower();
        QString continent = it.value();

        if (lowerCountry.contains(jsonCountryName) || jsonCountryName.contains(lowerCountry)) {
            return continent;
        }
    }

    // Handle common name variations
    if (lowerCountry.contains("republic of korea")) return "Asia";
    if (lowerCountry.contains("democratic people's republic of korea")) return "Asia";
    if (lowerCountry.contains("netherlands")) return "Europe";
    if (lowerCountry.contains("korea")) return "Asia";
    if (lowerCountry.contains("moldova")) return "Europe";
    if (lowerCountry.contains("russia")) return "Europe";
    if (lowerCountry.contains("vietnam") || lowerCountry.contains("viet nam")) return "Asia";

    return "Other";
}

void FlightTracker::applyFilters()
{
    if (!m_flightOverlay) {
        qDebug() << "No flight overlay available for filtering";
        return;
    }

    GraphicListModel* graphics = m_flightOverlay->graphics();
    if (!graphics) {
        qDebug() << "No graphics model available";
        return;
    }

    int graphicsCount = graphics->size();
    int flightsCount = m_flights.size();
    
    qDebug() << "Applying filters to" << graphicsCount << "graphics and" << flightsCount << "flights";

    // Use the smaller of the two sizes to avoid out-of-bounds access
    int maxCount = qMin(graphicsCount, flightsCount);

    for (int i = 0; i < maxCount; ++i) {
        Graphic* graphic = graphics->at(i);
        if (!graphic) {
            qDebug() << "Null graphic at index" << i;
            continue;
        }

        if (i >= m_flights.size()) {
            qDebug() << "Flight index out of bounds:" << i;
            break;
        }

        const FlightData& flight = m_flights[i];
        if (!flight.isValid()) {
            qDebug() << "Invalid flight data at index" << i;
            continue;
        }
        
        bool shouldShow = true;

        // Check country filter
        if (m_selectedCountries.isEmpty()) {
            // No countries selected - hide all flights
            shouldShow = false;
        } else {
            // Get all available countries to check if all are selected
            QStringList allAvailableCountries;
            for (auto it = m_availableCountries.begin(); it != m_availableCountries.end(); ++it) {
                QStringList continentCountries = it.value().toStringList();
                allAvailableCountries.append(continentCountries);
            }
            
            // Only apply country filter if not all countries are selected
            if (m_selectedCountries.size() != allAvailableCountries.size()) {
                QString country = extractCountryFromFlight(flight);
                bool countryMatch = country.isEmpty() || m_selectedCountries.contains(country);
                shouldShow = shouldShow && countryMatch;
            }
            // If all countries are selected, don't filter by country (shouldShow remains unchanged)
        }

        // Check status filter
        if (m_selectedFlightStatus != "All") {
            bool statusMatch = true;
            if (m_selectedFlightStatus == "Airborne") {
                statusMatch = !flight.onGround();
            } else if (m_selectedFlightStatus == "OnGround") {
                statusMatch = flight.onGround();
            }
            shouldShow = shouldShow && statusMatch;
        }

        // Check altitude filter
        double altitudeFeet = flight.altitude() * 3.28084; // Convert to feet
        if (altitudeFeet >= 0) { // Only filter if altitude is valid
            bool altitudeMatch = (altitudeFeet >= m_minAltitudeFilter && altitudeFeet <= m_maxAltitudeFilter);
            shouldShow = shouldShow && altitudeMatch;
        }

        // Check speed filter
        double speedKnots = flight.velocity() * 1.94384; // Convert to knots
        if (speedKnots >= 0) { // Only filter if speed is valid
            bool speedMatch = (speedKnots >= m_minSpeedFilter && speedKnots <= m_maxSpeedFilter);
            shouldShow = shouldShow && speedMatch;
        }

        // Check vertical status filter
        if (m_selectedVerticalStatus != "All") {
            double verticalRate = flight.verticalRate();
            bool verticalMatch = true;
            
            if (m_selectedVerticalStatus == "Climbing") {
                verticalMatch = (verticalRate > 0.5);
            } else if (m_selectedVerticalStatus == "Descending") {
                verticalMatch = (verticalRate < -0.5);
            } else if (m_selectedVerticalStatus == "Level") {
                verticalMatch = (verticalRate >= -0.5 && verticalRate <= 0.5);
            }
            shouldShow = shouldShow && verticalMatch;
        }

        // Safely set visibility
        try {
            graphic->setVisible(shouldShow);
        } catch (...) {
            qDebug() << "Exception setting visibility for graphic at index" << i;
            continue;
        }

        // Clear selection if selected flight is filtered out
        if (!shouldShow && m_selectedFlight.isValid() && flight.icao24() == m_selectedFlight.icao24()) {
            clearFlightSelection();
        }
    }

    qDebug() << "Filter application completed";
}

void FlightTracker::scheduleFilterUpdate()
{
    if (m_flightOverlay && !m_flights.isEmpty() && m_filterUpdateTimer) {
        m_filterUpdateTimer->start(); // Restart the timer, will call applyFilters after delay
    }
}
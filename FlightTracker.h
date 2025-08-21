#ifndef FLIGHTTRACKER_H
#define FLIGHTTRACKER_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include "FlightData.h"

namespace Esri::ArcGISRuntime {
class Map;
class MapQuickView;
class GraphicsOverlay;
class Popup;
class Graphic;
class Basemap;
}

class OpenSkyAuthManager;
class FlightDataService;
class FlightRenderer;

Q_MOC_INCLUDE("MapQuickView.h")
Q_MOC_INCLUDE("Popup.h")

class FlightTracker : public QObject
{
    Q_OBJECT

    Q_PROPERTY(Esri::ArcGISRuntime::MapQuickView *mapView READ mapView WRITE setMapView NOTIFY mapViewChanged)
    Q_PROPERTY(bool isAuthenticated READ isAuthenticated NOTIFY authenticationChanged)
    Q_PROPERTY(bool hasSelectedFlight READ hasSelectedFlight NOTIFY selectedFlightChanged)
    Q_PROPERTY(Esri::ArcGISRuntime::Popup *selectedFlightPopup READ selectedFlightPopup NOTIFY selectedFlightChanged)
    Q_PROPERTY(QString lastUpdateTime READ lastUpdateTime NOTIFY lastUpdateTimeChanged)
    Q_PROPERTY(bool showTrack READ showTrack WRITE setShowTrack NOTIFY showTrackChanged)
    Q_PROPERTY(bool isDarkTheme READ isDarkTheme WRITE setIsDarkTheme NOTIFY isDarkThemeChanged)
    
    // Filter properties
    Q_PROPERTY(QVariantMap availableCountries READ availableCountries NOTIFY availableCountriesChanged)
    Q_PROPERTY(QStringList selectedCountries READ selectedCountries WRITE setSelectedCountries NOTIFY selectedCountriesChanged)
    Q_PROPERTY(QString selectedFlightStatus READ selectedFlightStatus WRITE setSelectedFlightStatus NOTIFY selectedFlightStatusChanged)
    Q_PROPERTY(double minAltitudeFilter READ minAltitudeFilter WRITE setMinAltitudeFilter NOTIFY altitudeFilterChanged)
    Q_PROPERTY(double maxAltitudeFilter READ maxAltitudeFilter WRITE setMaxAltitudeFilter NOTIFY altitudeFilterChanged)
    Q_PROPERTY(double minSpeedFilter READ minSpeedFilter WRITE setMinSpeedFilter NOTIFY speedFilterChanged)
    Q_PROPERTY(double maxSpeedFilter READ maxSpeedFilter WRITE setMaxSpeedFilter NOTIFY speedFilterChanged)
    Q_PROPERTY(QString selectedVerticalStatus READ selectedVerticalStatus WRITE setSelectedVerticalStatus NOTIFY selectedVerticalStatusChanged)

public:
    explicit FlightTracker(QObject *parent = nullptr);
    ~FlightTracker() override;

    // Property getters
    bool isAuthenticated() const;
    bool hasSelectedFlight() const;
    Esri::ArcGISRuntime::Popup *selectedFlightPopup() const;
    bool hasValidPopup() const { return m_selectedFlightPopup != nullptr; }
    QString lastUpdateTime() const { return m_lastUpdateTime; }
    bool showTrack() const { return m_showTrack; }
    void setShowTrack(bool show);
    bool isDarkTheme() const { return m_isDarkTheme; }
    void setIsDarkTheme(bool isDark);
    
    // Filter property getters/setters
    QVariantMap availableCountries() const { return m_availableCountries; }
    QStringList selectedCountries() const { return m_selectedCountries; }
    void setSelectedCountries(const QStringList& countries);
    QString selectedFlightStatus() const { return m_selectedFlightStatus; }
    void setSelectedFlightStatus(const QString& status);
    double minAltitudeFilter() const { return m_minAltitudeFilter; }
    void setMinAltitudeFilter(double minAlt);
    double maxAltitudeFilter() const { return m_maxAltitudeFilter; }
    void setMaxAltitudeFilter(double maxAlt);
    double minSpeedFilter() const { return m_minSpeedFilter; }
    void setMinSpeedFilter(double minSpeed);
    double maxSpeedFilter() const { return m_maxSpeedFilter; }
    void setMaxSpeedFilter(double maxSpeed);
    QString selectedVerticalStatus() const { return m_selectedVerticalStatus; }
    void setSelectedVerticalStatus(const QString& status);

public slots:
    Q_INVOKABLE void selectFlightAtPoint(QPointF screenPoint);
    Q_INVOKABLE void clearFlightSelection();
    Q_INVOKABLE void fetchFlightData();
    Q_INVOKABLE QVariantList getSelectedFlightData();

signals:
    void mapViewChanged();
    void authenticationChanged();
    void authenticationSuccess();
    void authenticationFailed(const QString &error);
    void selectedFlightChanged();
    void lastUpdateTimeChanged();
    void showTrackChanged();
    void isDarkThemeChanged();
    
    // Filter signals
    void availableCountriesChanged();
    void selectedCountriesChanged();
    void selectedFlightStatusChanged();
    void altitudeFilterChanged();
    void speedFilterChanged();
    void selectedVerticalStatusChanged();

private slots:
    void onAuthenticationSuccess();
    void onAuthenticationFailed(const QString& error);
    void onFlightDataReceived(const QList<FlightData>& flights);
    void onTrackDataReceived(const QString& icao24, const QJsonObject& trackData);
    void onDataFetchFailed(const QString& error);
    void updateDisplayTime();

private:
    Esri::ArcGISRuntime::MapQuickView *mapView() const;
    void setMapView(Esri::ArcGISRuntime::MapQuickView *mapView);
    
    void loadConfig();
    void createFlightPopup(const FlightData& flight);
    FlightData findFlightAtPoint(QPointF screenPoint);
    
    // Country and filtering helpers
    void loadCountryMappings();
    QString extractCountryFromFlight(const FlightData& flight);
    QString getCountryContinent(const QString& country);
    void applyFilters();
    void scheduleFilterUpdate();

    // Core components
    Esri::ArcGISRuntime::Map *m_map = nullptr;
    Esri::ArcGISRuntime::MapQuickView *m_mapView = nullptr;
    
    // Pre-created basemaps for fast switching
    Esri::ArcGISRuntime::Basemap *m_darkBasemap = nullptr;
    Esri::ArcGISRuntime::Basemap *m_lightBasemap = nullptr;
    
    // Services
    OpenSkyAuthManager* m_authManager;
    FlightDataService* m_dataService;
    FlightRenderer* m_renderer;
    
    // Graphics overlays
    Esri::ArcGISRuntime::GraphicsOverlay* m_flightOverlay;
    Esri::ArcGISRuntime::GraphicsOverlay* m_selectionOverlay;
    Esri::ArcGISRuntime::GraphicsOverlay* m_trackOverlay;
    
    // Selection state
    FlightData m_selectedFlight;
    Esri::ArcGISRuntime::Popup* m_selectedFlightPopup = nullptr;
    
    // Display state
    QList<FlightData> m_flights;
    QString m_lastUpdateTime = "Never";
    QDateTime m_lastUpdateDateTime;
    QTimer* m_displayUpdateTimer;
    QTimer* m_flightUpdateTimer;
    QTimer* m_filterUpdateTimer;
    bool m_showTrack = false;
    bool m_isDarkTheme = true;
    bool m_devMode = true;  // Set to false for production
    
    // Filter state
    QVariantMap m_availableCountries;
    QStringList m_selectedCountries;
    QString m_selectedFlightStatus = "All";
    double m_minAltitudeFilter = 0.0;
    double m_maxAltitudeFilter = 40000.0;
    double m_minSpeedFilter = 0.0;
    double m_maxSpeedFilter = 600.0;
    QString m_selectedVerticalStatus = "All";
    bool m_isInitialLoad = true;
    bool m_isUpdatingFlights = false;
    QMap<QString, QString> m_countryToContinentMap;
};

#endif // FLIGHTTRACKER_H
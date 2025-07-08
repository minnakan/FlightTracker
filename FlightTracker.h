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

#ifndef FLIGHTTRACKER_H
#define FLIGHTTRACKER_H

namespace Esri::ArcGISRuntime {
class Map;
class MapQuickView;
class GraphicsOverlay;
class GraphicListModel;
class GraphicsOverlayListModel;
class TextSymbol;
class Popup;
class PopupDefinition;
class FieldDescription;
class SelectionProperties;
class Graphic;
class TextPopupElement;
} // namespace Esri::ArcGISRuntime

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QMap>

class QJsonArray;

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

    bool isAuthenticated() const { return !m_accessToken.isEmpty(); }

    //Do not check selectedFlightPopup != nulltpr here beacuse we clear it after emiting signal to prevent app from closing
    bool hasSelectedFlight() const { return !m_selectedFlightInfo.isEmpty(); }

    Esri::ArcGISRuntime::Popup *selectedFlightPopup() const { return m_selectedFlightPopup; }

    QString lastUpdateTime() const { return m_lastUpdateTime; }
    QVariantMap availableCountries() const { return m_availableCountries; }

    QStringList selectedCountries() const { return m_selectedCountries; }
    void setSelectedCountries(const QStringList& countries);

    QString selectedFlightStatus() const { return m_selectedFlightStatus; }
    void setSelectedFlightStatus(const QString& status);

    double minAltitudeFilter() const { return m_minAltitudeFilter; }
    double maxAltitudeFilter() const { return m_maxAltitudeFilter; }
    void setMinAltitudeFilter(double minAlt);
    void setMaxAltitudeFilter(double maxAlt);

    double minSpeedFilter() const { return m_minSpeedFilter; }
    double maxSpeedFilter() const { return m_maxSpeedFilter; }
    void setMinSpeedFilter(double minSpeed);
    void setMaxSpeedFilter(double maxSpeed);

    QString selectedVerticalStatus() const { return m_selectedVerticalStatus; }
    void setSelectedVerticalStatus(const QString& status);

public slots:
    void authenticate();
    Q_INVOKABLE void selectFlightAtPoint(QPointF screenPoint);
    Q_INVOKABLE void clearFlightSelection();
    Q_INVOKABLE void fetchFlightData();

signals:
    void mapViewChanged();
    void authenticationChanged();
    void authenticationSuccess();
    void authenticationFailed(const QString &error);
    void selectedFlightChanged();
    void lastUpdateTimeChanged();
    void availableCountriesChanged();
    void selectedCountriesChanged();
    void selectedFlightStatusChanged();
    void altitudeFilterChanged();
    void speedFilterChanged();
    void selectedVerticalStatusChanged();

private:
    Esri::ArcGISRuntime::MapQuickView *mapView() const;
    void setMapView(Esri::ArcGISRuntime::MapQuickView *mapView);

    // Authentication
    void requestAccessToken();

    void updateDisplayTime();

    //Symbol helpers
    int getCategoryFromCallsign(const QString& callsign);
    Esri::ArcGISRuntime::TextSymbol* getSymbolForCategory(int category, bool onGround = false, double altitude = 0.0);
    QColor getAltitudeColor(double altitude);

    void createFlightPopup(Esri::ArcGISRuntime::Graphic* flightGraphic, const QJsonArray& flightData);

    //get by country helpers
    QString extractCountryFromFlight(const QJsonArray& flightData);
    QString getCountryContinent(const QString& country);
    void loadCountryMappings();

    void applyFilters();

    void loadConfig();

    void updateSelectionGraphic();

    Esri::ArcGISRuntime::Map *m_map = nullptr;
    Esri::ArcGISRuntime::MapQuickView *m_mapView = nullptr;

    //Loads from config in load config
    QString m_clientId = "";
    QString m_clientSecret = "";

    //Auto refresh
    QTimer *m_flightUpdateTimer;

    QNetworkAccessManager *m_networkManager;
    QString m_accessToken;

    Esri::ArcGISRuntime::GraphicsOverlay* m_flightOverlay;
    Esri::ArcGISRuntime::GraphicsOverlay* m_selectionOverlay;

    QString m_selectedFlightTitle;
    QString m_selectedFlightInfo;
    Esri::ArcGISRuntime::Popup* m_selectedFlightPopup = nullptr;
    Esri::ArcGISRuntime::Graphic* m_selectedGraphic = nullptr;


    //Might be better off using an array - TODO
    QMap<QString, QJsonArray> m_flightDataCache;

    QString m_lastUpdateTime = "Never";
    QDateTime m_lastUpdateDateTime;
    QTimer* m_displayUpdateTimer;

    QMap<QString, QString> m_countryToContinentMap;
    QVariantMap m_availableCountries;
    QStringList m_selectedCountries;
    bool m_isInitialLoad = true;

    QString m_selectedFlightStatus = "All";

    double m_minAltitudeFilter = 0.0;
    double m_maxAltitudeFilter = 40000.0;
    double m_minSpeedFilter = 0.0;
    double m_maxSpeedFilter = 600.0;

    QString m_selectedVerticalStatus = "All";

private slots:
    void onAuthenticationReply();
    void onFlightDataReply();
};

#endif // FLIGHTTRACKER_H

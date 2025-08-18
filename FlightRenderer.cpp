#include "FlightRenderer.h"
#include "TextSymbol.h"
#include "Graphic.h"
#include "Point.h"
#include "SpatialReference.h"
#include "SimpleLineSymbol.h"
#include "SimpleMarkerSymbol.h"
#include "GraphicsOverlay.h"
#include "GraphicListModel.h"
#include "Polyline.h"
#include "PolylineBuilder.h"
#include "SymbolTypes.h"
#include "MapTypes.h"
#include <QJsonObject>
#include <QJsonArray>
#include <cmath>

using namespace Esri::ArcGISRuntime;

FlightRenderer::FlightRenderer(QObject *parent)
    : QObject(parent)
{
}

QColor FlightRenderer::getAltitudeColor(double altitude)
{
    double altitudeFeet = altitude * 3.28084; // Convert meters to feet

    if (altitudeFeet <= 500) return QColor(255, 69, 0);       // Red-orange
    else if (altitudeFeet <= 1000) return QColor(255, 140, 0);     // Orange
    else if (altitudeFeet <= 2000) return QColor(255, 215, 0);     // Gold
    else if (altitudeFeet <= 4000) return QColor(255, 255, 0);     // Yellow
    else if (altitudeFeet <= 6000) return QColor(173, 255, 47);    // Yellow-green
    else if (altitudeFeet <= 8000) return QColor(0, 255, 0);       // Green
    else if (altitudeFeet <= 10000) return QColor(0, 255, 127);    // Spring green
    else if (altitudeFeet <= 20000) return QColor(0, 191, 255);    // Deep sky blue
    else if (altitudeFeet <= 30000) return QColor(0, 100, 255);    // Blue
    else if (altitudeFeet <= 40000) return QColor(138, 43, 226);   // Blue violet
    else return QColor(128, 0, 128);                               // Purple
}

int FlightRenderer::getCategoryFromCallsign(const QString& callsign)
{
    if (callsign.isEmpty()) return 1;

    QString trimmed = callsign.trimmed();
    int length = trimmed.length();
    int digitCount = 0;

    for (QChar c : trimmed) {
        if (c.isDigit()) digitCount++;
    }

    // US N-number pattern
    if (trimmed.startsWith('N') && digitCount >= 2) return 2;

    // Cargo indicators
    if (trimmed.contains("FDX") || trimmed.contains("UPS") ||
        trimmed.contains("CARGO") || trimmed.contains("ABX")) return 6;

    // Emergency/helicopter indicators
    if (trimmed.contains("MED") || trimmed.contains("RESCUE") ||
        trimmed.contains("LIFE") || trimmed.contains("HELI")) return 8;

    // Size-based categorization
    if (length <= 5) return 3;
    else if (length <= 7) return 4;
    else return 3;
}

TextSymbol* FlightRenderer::getSymbolForCategory(int category, bool onGround, double altitude)
{
    QString aircraftChar = "✈";
    float fontSize = 20.0f;

    switch (category) {
    case 1:  aircraftChar = "✈"; fontSize = 24.0f; break;  // Unknown
    case 2:  aircraftChar = "✈"; fontSize = 12.0f; break;  // Light aircraft
    case 3:  aircraftChar = "✈"; fontSize = 16.0f; break;  // Small aircraft
    case 4:  aircraftChar = "✈"; fontSize = 20.0f; break;  // Large aircraft
    case 5:  aircraftChar = "✈"; fontSize = 22.0f; break;  // High Vortex Large
    case 6:  aircraftChar = "✈"; fontSize = 26.0f; break;  // Heavy aircraft
    case 7:  aircraftChar = "▲"; fontSize = 22.0f; break;  // High Performance
    case 8:  aircraftChar = "●"; fontSize = 20.0f; break;  // Rotorcraft
    default: aircraftChar = "✈"; fontSize = 20.0f; break;
    }

    if (onGround) {
        aircraftChar = "■";
        fontSize = 16.0f;
    }

    QColor color = getAltitudeColor(altitude);

    TextSymbol* symbol = new TextSymbol(aircraftChar, color, fontSize,
                                       HorizontalAlignment::Center,
                                       VerticalAlignment::Middle, this);
    symbol->setFontFamily("Arial Unicode MS");
    return symbol;
}

TextSymbol* FlightRenderer::createFlightSymbol(const FlightData& flight)
{
    int category = getCategoryFromCallsign(flight.callsign());
    TextSymbol* symbol = getSymbolForCategory(category, flight.onGround(), flight.altitude());

    // Set rotation based on heading
    double heading = flight.heading();
    if (!std::isnan(heading)) {
        double adjustedHeading = heading - 45.0;
        if (adjustedHeading < 0) adjustedHeading += 360.0;
        symbol->setAngle(adjustedHeading);
    }

    return symbol;
}

Graphic* FlightRenderer::createFlightGraphic(const FlightData& flight)
{
    if (!flight.isValid()) {
        qDebug() << "FlightRenderer: Invalid flight data";
        return nullptr;
    }

    double lon = flight.longitude();
    double lat = flight.latitude();
    
    // Validate coordinates
    if (std::isnan(lon) || std::isnan(lat) || 
        lon < -180.0 || lon > 180.0 || 
        lat < -90.0 || lat > 90.0) {
        qDebug() << "FlightRenderer: Invalid coordinates:" << lat << "," << lon;
        return nullptr;
    }

    try {
        Point flightPoint(lon, lat, SpatialReference::wgs84());
        TextSymbol* symbol = createFlightSymbol(flight);
        
        if (!symbol) {
            qDebug() << "FlightRenderer: Failed to create symbol";
            return nullptr;
        }
        
        return new Graphic(flightPoint, symbol, this);
        
    } catch (const std::exception& e) {
        qDebug() << "FlightRenderer: Exception creating graphic:" << e.what();
        return nullptr;
    } catch (...) {
        qDebug() << "FlightRenderer: Unknown exception creating graphic";
        return nullptr;
    }
}

void FlightRenderer::updateFlightGraphics(GraphicsOverlay* overlay, const QList<FlightData>& flights)
{
    if (!overlay) {
        qDebug() << "FlightRenderer: overlay is null";
        return;
    }

    GraphicListModel* graphics = overlay->graphics();
    if (!graphics) {
        qDebug() << "FlightRenderer: graphics model is null";
        return;
    }

    qDebug() << "FlightRenderer: Starting with" << graphics->size() << "existing graphics";
    
    // Don't clear here - FlightTracker already cleared graphics safely
    // This avoids potential race conditions with selection graphics

    int validFlights = 0;
    for (int i = 0; i < flights.size(); ++i) {
        const FlightData& flight = flights[i];
        
        if (!flight.isValid()) {
            continue;
        }

        try {
            Graphic* graphic = createFlightGraphic(flight);
            if (graphic) {
                graphics->append(graphic);
                validFlights++;
            } else {
                qDebug() << "FlightRenderer: Failed to create graphic for flight" << i;
            }
        } catch (const std::exception& e) {
            qDebug() << "FlightRenderer: Exception creating graphic for flight" << i << ":" << e.what();
            continue;
        } catch (...) {
            qDebug() << "FlightRenderer: Unknown exception creating graphic for flight" << i;
            continue;
        }
    }

    qDebug() << "FlightRenderer: Created graphics for" << validFlights << "out of" << flights.size() << "flights";
}

void FlightRenderer::createSelectionGraphic(GraphicsOverlay* selectionOverlay, const FlightData& flight)
{
    if (!selectionOverlay) return;

    selectionOverlay->graphics()->clear();

    Point flightPoint(flight.longitude(), flight.latitude(), SpatialReference::wgs84());

    // Create selection ring
    SimpleLineSymbol* outline = new SimpleLineSymbol(SimpleLineSymbolStyle::Solid,
                                                    QColor(255, 255, 255), 2.5f, this);

    SimpleMarkerSymbol* ringSymbol = new SimpleMarkerSymbol(SimpleMarkerSymbolStyle::Circle,
                                                           QColor(Qt::transparent), 30.0f, this);
    ringSymbol->setOutline(outline);

    Graphic* ringGraphic = new Graphic(flightPoint, ringSymbol, this);
    selectionOverlay->graphics()->append(ringGraphic);

    // Create label
    QString labelText = !flight.callsign().isEmpty() ? flight.callsign() : flight.icao24().left(6);
    TextSymbol* labelSymbol = new TextSymbol(labelText, QColor(Qt::white), 14.0f,
                                           HorizontalAlignment::Center,
                                           VerticalAlignment::Top, this);
    labelSymbol->setHaloColor(Qt::gray);
    labelSymbol->setHaloWidth(1.0f);
    labelSymbol->setOffsetY(-16.0f);

    Point labelPoint(flight.longitude(), flight.latitude() - 0.0003, SpatialReference::wgs84());
    Graphic* labelGraphic = new Graphic(labelPoint, labelSymbol, this);
    selectionOverlay->graphics()->append(labelGraphic);
}

void FlightRenderer::drawFlightTrack(GraphicsOverlay* trackOverlay, const QJsonObject& trackData)
{
    if (!trackOverlay) return;

    trackOverlay->graphics()->clear();

    if (!trackData.contains("path")) return;

    QJsonArray path = trackData["path"].toArray();
    if (path.isEmpty()) return;

    QList<Point> trackPoints;
    QList<double> altitudes;

    for (const QJsonValue& value : path) {
        QJsonArray waypoint = value.toArray();
        if (waypoint.size() >= 6) {
            double lat = waypoint[1].toDouble();
            double lon = waypoint[2].toDouble();
            double altitude = waypoint[3].toDouble();
            bool onGround = waypoint[5].toBool();

            if (lat == 0.0 && lon == 0.0) continue;

            trackPoints.append(Point(lon, lat, SpatialReference::wgs84()));
            altitudes.append(onGround ? 0.0 : altitude);
        }
    }

    if (trackPoints.size() < 2) return;

    // Draw colored line segments
    for (int i = 0; i < trackPoints.size() - 1; ++i) {
        PolylineBuilder polylineBuilder(SpatialReference::wgs84());
        polylineBuilder.addPoint(trackPoints[i]);
        polylineBuilder.addPoint(trackPoints[i + 1]);

        double avgAltitude = (altitudes[i] + altitudes[i + 1]) / 2.0;
        QColor lineColor = getAltitudeColor(avgAltitude);

        SimpleLineSymbol* lineSymbol = new SimpleLineSymbol(SimpleLineSymbolStyle::Solid,
                                                           lineColor, 3.0f, this);
        lineSymbol->setAntiAlias(true);

        Graphic* segmentGraphic = new Graphic(polylineBuilder.toPolyline(), lineSymbol, this);
        trackOverlay->graphics()->append(segmentGraphic);
    }
}
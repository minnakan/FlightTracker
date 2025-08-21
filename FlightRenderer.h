#ifndef FLIGHTRENDERER_H
#define FLIGHTRENDERER_H

#include <QObject>
#include <QColor>
#include "FlightData.h"

namespace Esri::ArcGISRuntime {
class TextSymbol;
class GraphicsOverlay;
class Graphic;
class Point;
class Polyline;
class SimpleLineSymbol;
class SimpleMarkerSymbol;
}

class FlightRenderer : public QObject
{
    Q_OBJECT

public:
    explicit FlightRenderer(QObject *parent = nullptr);

    static QColor getAltitudeColor(double altitude);
    static int getCategoryFromCallsign(const QString& callsign);
    
    Esri::ArcGISRuntime::TextSymbol* createFlightSymbol(const FlightData& flight);
    Esri::ArcGISRuntime::Graphic* createFlightGraphic(const FlightData& flight);
    
    void updateFlightGraphics(Esri::ArcGISRuntime::GraphicsOverlay* overlay, 
                             const QList<FlightData>& flights);

    void createSelectionGraphic(Esri::ArcGISRuntime::GraphicsOverlay* selectionOverlay,
                               const FlightData& flight, bool isDarkTheme = true);

    void drawFlightTrack(Esri::ArcGISRuntime::GraphicsOverlay* trackOverlay,
                        const QJsonObject& trackData);

private:
    Esri::ArcGISRuntime::TextSymbol* getSymbolForCategory(int category, bool onGround, double altitude);
};

#endif // FLIGHTRENDERER_H
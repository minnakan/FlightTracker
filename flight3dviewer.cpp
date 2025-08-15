#include "Flight3DViewer.h"

#include "AttributeListModel.h"
#include <QFuture>
// Core ArcGIS includes
#include "Scene.h"
#include "SceneQuickView.h"
#include "Surface.h"
#include "SpatialReference.h"
#include "Point.h"

// Graphics includes
#include "GraphicsOverlay.h"
#include "GraphicsOverlayListModel.h"
#include "Graphic.h"
#include "GraphicListModel.h"

// Symbols and rendering
#include "SimpleMarkerSceneSymbol.h"
#include "SimpleRenderer.h"
#include "RendererSceneProperties.h"
#include "LayerSceneProperties.h"

// Camera and elevation
#include "OrbitGeoElementCameraController.h"
#include "ArcGISTiledElevationSource.h"
#include "ElevationSourceListModel.h"

// Enums - these are the key missing includes
#include "MapTypes.h"        // For BasemapStyle
#include "SceneViewTypes.h"  // For SurfacePlacement, SceneSymbolAnchorPosition
#include "SymbolTypes.h"     // For SimpleMarkerSceneSymbolStyle

#include <QJsonArray>
#include <QStandardPaths>
#include <QDebug>
#include <QtMath>

using namespace Esri::ArcGISRuntime;


Flight3DViewer::Flight3DViewer(QObject *parent)
    : QObject(parent)
{
}

Flight3DViewer::~Flight3DViewer() = default;

void Flight3DViewer::init()
{
    qmlRegisterType<SceneQuickView>("Esri.FlightTracker", 1, 0, "SceneView");
    qmlRegisterType<Flight3DViewer>("Esri.FlightTracker", 1, 0, "Flight3DViewer");
}

SceneQuickView* Flight3DViewer::sceneView() const
{
    return m_sceneView;
}

void Flight3DViewer::setSceneView(SceneQuickView* sceneView)
{
    if (!sceneView || sceneView == m_sceneView)
        return;

    m_sceneView = sceneView;
    setupScene();
    emit sceneViewChanged();
}

void Flight3DViewer::setupScene()
{
    if (!m_sceneView)
        return;

    // Create scene with imagery basemap
    m_scene = new Scene(BasemapStyle::ArcGISImageryStandard, this);

    // Add elevation source
    ArcGISTiledElevationSource* elevationSource = new ArcGISTiledElevationSource(
        QUrl("https://elevation3d.arcgis.com/arcgis/rest/services/WorldElevation3D/Terrain3D/ImageServer"), this);
    m_scene->baseSurface()->elevationSources()->append(elevationSource);

    // Create 3D graphics overlay
    m_flightOverlay = new GraphicsOverlay(this);
    m_flightOverlay->setSceneProperties(LayerSceneProperties(SurfacePlacement::Absolute));

    // Create renderer for 3D aircraft
    SimpleRenderer* renderer3D = new SimpleRenderer(this);
    RendererSceneProperties properties = renderer3D->sceneProperties();
    properties.setPitchExpression("[PITCH]");
    properties.setHeadingExpression("[HEADING]");
    renderer3D->setSceneProperties(properties);
    m_flightOverlay->setRenderer(renderer3D);

    m_sceneView->setArcGISScene(m_scene);
    m_sceneView->graphicsOverlays()->append(m_flightOverlay);
}

void Flight3DViewer::displayFlight(const QJsonArray& flightData)
{
    if (!m_sceneView || flightData.size() < 13)
        return;

    clearFlight();
    createFlightGraphic(flightData);
}

void Flight3DViewer::createFlightGraphic(const QJsonArray& flightData)
{
    // Safely extract flight data with bounds checking
    if (flightData.size() < 11) {
        qDebug() << "Insufficient flight data";
        return;
    }

    // Use .toDouble() and check for valid values
    QJsonValue lonValue = flightData[5];
    QJsonValue latValue = flightData[6];
    QJsonValue altValue = flightData[7];
    QJsonValue headingValue = flightData[10];

    if (lonValue.isNull() || latValue.isNull() || altValue.isNull()) {
        qDebug() << "Invalid flight position data";
        return;
    }

    double longitude = lonValue.toDouble();
    double latitude = latValue.toDouble();
    double altitude = altValue.toDouble();
    double heading = headingValue.isNull() ? 0.0 : headingValue.toDouble();

    if (qIsNaN(longitude) || qIsNaN(latitude) || qIsNaN(altitude)) {
        qDebug() << "NaN values in flight data";
        return;
    }

    // Convert altitude from meters to feet for display
    double altitudeFeet = altitude * 3.28084;

    // Create aircraft symbol - you can replace this with a 3D model
    SimpleMarkerSceneSymbol* aircraftSymbol = new SimpleMarkerSceneSymbol(
        SimpleMarkerSceneSymbolStyle::Cone,
        getAltitudeColor(altitudeFeet),
        100, 100, 100,
        SceneSymbolAnchorPosition::Center,
        this);

    // Create point for aircraft position
    Point aircraftPosition(longitude, latitude, altitude, SpatialReference::wgs84());

    // Create flight graphic
    m_flightGraphic = new Graphic(aircraftPosition, aircraftSymbol, this);
    m_flightGraphic->attributes()->insertAttribute("HEADING", heading);
    m_flightGraphic->attributes()->insertAttribute("PITCH", 0.0);

    m_flightOverlay->graphics()->append(m_flightGraphic);

    // Create orbit camera controller
    m_orbitCam = new OrbitGeoElementCameraController(m_flightGraphic, 2000, this);
    m_orbitCam->setMinCameraDistance(100);
    m_orbitCam->setMaxCameraDistance(10000);
    m_orbitCam->setCameraHeadingOffset(0);
    m_orbitCam->setCameraPitchOffset(45);

    // Connect camera signals
    connect(m_orbitCam, &OrbitGeoElementCameraController::cameraDistanceChanged,
            this, &Flight3DViewer::cameraDistanceChanged);
    connect(m_orbitCam, &OrbitGeoElementCameraController::cameraHeadingOffsetChanged,
            this, &Flight3DViewer::cameraHeadingChanged);
    connect(m_orbitCam, &OrbitGeoElementCameraController::cameraPitchOffsetChanged,
            this, &Flight3DViewer::cameraPitchChanged);

    m_sceneView->setCameraController(m_orbitCam);

    m_hasActiveFlight = true;
    emit activeFlightChanged();
}

QColor Flight3DViewer::getAltitudeColor(double altitude)
{
    // Same color scheme as your 2D map
    if (altitude < 500) return QColor("#FF4500");
    if (altitude < 1000) return QColor("#FF8C00");
    if (altitude < 2000) return QColor("#FFD700");
    if (altitude < 4000) return QColor("#FFFF00");
    if (altitude < 6000) return QColor("#ADFF2F");
    if (altitude < 8000) return QColor("#00FF00");
    if (altitude < 10000) return QColor("#00FF7F");
    if (altitude < 20000) return QColor("#00BFFF");
    if (altitude < 30000) return QColor("#0064FF");
    if (altitude < 40000) return QColor("#8A2BE2");
    return QColor("#800080");
}

void Flight3DViewer::clearFlight()
{
    if (m_flightOverlay)
        m_flightOverlay->graphics()->clear();

    m_flightGraphic = nullptr;
    m_orbitCam = nullptr;

    if (m_hasActiveFlight) {
        m_hasActiveFlight = false;
        emit activeFlightChanged();
    }
}

void Flight3DViewer::cockpitView()
{
    if (!m_orbitCam)
        return;

    m_orbitCam->setMinCameraDistance(0);

    // Handle the returned QFuture properly
    auto offsetFuture = m_orbitCam->setTargetOffsetsAsync(0, -10, 5, 1);
    Q_UNUSED(offsetFuture) // Suppress the warning

    auto moveFuture = m_orbitCam->moveCameraAsync(0, 0, 0, 1.0);
    Q_UNUSED(moveFuture) // Suppress the warning

    m_orbitCam->setAutoPitchEnabled(true);
}

void Flight3DViewer::followView()
{
    if (!m_orbitCam)
        return;

    m_orbitCam->setAutoPitchEnabled(false);
    m_orbitCam->setTargetOffsetX(0);
    m_orbitCam->setTargetOffsetY(0);
    m_orbitCam->setTargetOffsetZ(0);
    m_orbitCam->setCameraHeadingOffset(0);
    m_orbitCam->setCameraPitchOffset(45);
    m_orbitCam->setMinCameraDistance(100);
    m_orbitCam->setCameraDistance(2000);
}

// Property getters/setters
double Flight3DViewer::cameraDistance() const
{
    return m_orbitCam ? m_orbitCam->cameraDistance() : 2000.0;
}

void Flight3DViewer::setCameraDistance(double distance)
{
    if (m_orbitCam)
        m_orbitCam->setCameraDistance(distance);
}

double Flight3DViewer::cameraHeading() const
{
    return m_orbitCam ? m_orbitCam->cameraHeadingOffset() : 0.0;
}

void Flight3DViewer::setCameraHeading(double heading)
{
    if (m_orbitCam)
        m_orbitCam->setCameraHeadingOffset(heading);
}

double Flight3DViewer::cameraPitch() const
{
    return m_orbitCam ? m_orbitCam->cameraPitchOffset() : 45.0;
}

void Flight3DViewer::setCameraPitch(double pitch)
{
    if (m_orbitCam)
        m_orbitCam->setCameraPitchOffset(pitch);
}

bool Flight3DViewer::hasActiveFlight() const
{
    return m_hasActiveFlight;
}

void Flight3DViewer::displayFlight(const QVariantList& flightDataVariant)
{
    QJsonArray jsonArray;
    for (const QVariant& variant : flightDataVariant) {
        jsonArray.append(QJsonValue::fromVariant(variant));
    }
    displayFlight(jsonArray);  // Call the existing QJsonArray version
}

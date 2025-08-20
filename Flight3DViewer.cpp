#include "Flight3DViewer.h"

#include "AttributeListModel.h"
#include <QFuture>
// Core ArcGIS includes
#include "Scene.h"
#include "SceneQuickView.h"
#include "Map.h"
#include "MapQuickView.h"
#include "Surface.h"
#include "SpatialReference.h"
#include "Point.h"
#include "Viewpoint.h"

// Graphics includes
#include "GraphicsOverlay.h"
#include "GraphicsOverlayListModel.h"
#include "Graphic.h"
#include "GraphicListModel.h"

// Symbols and rendering
#include "SimpleMarkerSceneSymbol.h"
#include "SimpleMarkerSymbol.h"
#include "ModelSceneSymbol.h"
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
#include <QFileInfo>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>

using namespace Esri::ArcGISRuntime;


Flight3DViewer::Flight3DViewer(QObject *parent)
    : QObject(parent)
{
}

Flight3DViewer::~Flight3DViewer() = default;

void Flight3DViewer::init()
{
    qmlRegisterType<SceneQuickView>("Esri.FlightTracker", 1, 0, "SceneView");
    qmlRegisterType<MapQuickView>("Esri.FlightTracker", 1, 0, "MapView");
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

MapQuickView* Flight3DViewer::mapView() const
{
    return m_mapView;
}

void Flight3DViewer::setMapView(MapQuickView* mapView)
{
    if (!mapView || mapView == m_mapView)
        return;

    m_mapView = mapView;
    setupMap();
    emit mapViewChanged();
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

void Flight3DViewer::setupMap()
{
    if (!m_mapView)
        return;

    // Create map with imagery basemap (same as scene)
    m_map = new Map(BasemapStyle::ArcGISImageryStandard, this);

    // Create graphics overlay for 2D flight icon
    m_mapFlightOverlay = new GraphicsOverlay(this);
    
    m_mapView->setMap(m_map);
    m_mapView->graphicsOverlays()->append(m_mapFlightOverlay);
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
    
    const double minOffsetMeters = 40.0;
    if (altitude < minOffsetMeters) {
        altitude = minOffsetMeters;
        altitudeFeet = altitude * 3.28084;
    }


    static QString tempModelPath;
    if (tempModelPath.isEmpty()) {
        QTemporaryDir tempDir;
        tempDir.setAutoRemove(false);
        if (tempDir.isValid()) {
            QString basePath = tempDir.path();
            
            // Extract all model files
            QStringList filesToExtract = {
                "11803_Airplane_v1_l1.obj",
                "11803_Airplane_v1_l1.mtl",
                "11803_Airplane_body_diff.jpg",
                "11803_Airplane_tail_diff.jpg",
                "11803_Airplane_wing_big_L_diff.jpg",
                "11803_Airplane_wing_big_R_diff.jpg",
                "11803_Airplane_wing_details_L_diff.jpg",
                "11803_Airplane_wing_details_R_diff.jpg"
            };
            
            for (const QString& file : filesToExtract) {
                QString srcPath = ":/Resources/AirplaneModel/" + file;
                QString destPath = basePath + "/" + file;
                QFile::copy(srcPath, destPath);
            }
            
            tempModelPath = basePath + "/11803_Airplane_v1_l1.obj";
        }
    }
    
    // Create aircraft symbol using extracted model
    ModelSceneSymbol* aircraftSymbol = new ModelSceneSymbol(QUrl::fromLocalFile(tempModelPath), this);
    
    // Set aircraft scale (adjust these values to make it larger/smaller)
    aircraftSymbol->setWidth(2);   // Adjust size as needed
    aircraftSymbol->setHeight(2);
    aircraftSymbol->setDepth(0.5);
    
    aircraftSymbol->setAnchorPosition(SceneSymbolAnchorPosition::Center);
    
    // Monitor model loading status
    connect(aircraftSymbol, &ModelSceneSymbol::loadStatusChanged, [=]() {
        if (aircraftSymbol->loadStatus() == LoadStatus::FailedToLoad) {
            qDebug() << "Model failed to load!";
        }
    });

    // Create point for aircraft position
    Point aircraftPosition(longitude, latitude, altitude, SpatialReference::wgs84());

    // Create flight graphic
    m_flightGraphic = new Graphic(aircraftPosition, aircraftSymbol, this);
    
    // Adjust heading for model coordinate system (X-front to ArcGIS Y-north)
    double adjustedHeading = heading - 90;
    if (adjustedHeading < 0.0) adjustedHeading += 360.0;
    
    m_flightGraphic->attributes()->insertAttribute("HEADING", adjustedHeading);
    m_flightGraphic->attributes()->insertAttribute("PITCH", -90.0);

    m_flightOverlay->graphics()->append(m_flightGraphic);

    // Create 2D flight icon for minimap
    if (m_mapView && m_mapFlightOverlay) {
        // Create simple airplane icon for 2D map
        SimpleMarkerSymbol* airplaneIcon = new SimpleMarkerSymbol(
            SimpleMarkerSymbolStyle::Triangle,
            getAltitudeColor(altitudeFeet),
            20, this);  // Size 20 for visibility
            
        m_mapFlightGraphic = new Graphic(aircraftPosition, airplaneIcon, this);
        m_mapFlightGraphic->attributes()->insertAttribute("HEADING", adjustedHeading);
        
        // Set rotation for the triangle to match heading
        airplaneIcon->setAngle(adjustedHeading);
        
        m_mapFlightOverlay->graphics()->clear();
        m_mapFlightOverlay->graphics()->append(m_mapFlightGraphic);
        
        // Center minimap on aircraft position with appropriate scale
        // Rotate viewpoint so aircraft points north (rotate by negative heading)
        Viewpoint mapViewpoint(aircraftPosition, 1000000, 0);
        m_mapView->setViewpointAsync(mapViewpoint);
    }

    // Create orbit camera controller
    m_orbitCam = new OrbitGeoElementCameraController(m_flightGraphic, 5, this);
    m_orbitCam->setMinCameraDistance(1);
    m_orbitCam->setMaxCameraDistance(100000);
    
    // Disable auto pitch to prevent camera from following aircraft orientation
    m_orbitCam->setAutoPitchEnabled(false);
    m_orbitCam->setCameraHeadingOffset(90);
    m_orbitCam->setCameraPitchOffset(75);
    m_orbitCam->setTargetOffsetX(0);
    m_orbitCam->setTargetOffsetY(0);
    m_orbitCam->setTargetOffsetZ(0);

    // Connect camera signals
    connect(m_orbitCam, &OrbitGeoElementCameraController::cameraDistanceChanged,
            this, &Flight3DViewer::cameraDistanceChanged);
    connect(m_orbitCam, &OrbitGeoElementCameraController::cameraHeadingOffsetChanged,
            this, &Flight3DViewer::cameraHeadingChanged);
    connect(m_orbitCam, &OrbitGeoElementCameraController::cameraPitchOffsetChanged,
            this, &Flight3DViewer::cameraPitchChanged);

    m_sceneView->setCameraController(m_orbitCam);
    qDebug() << "Camera controller set to scene view";

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
    m_orbitCam->setCameraHeadingOffset(90);  // Behind the aircraft
    m_orbitCam->setCameraPitchOffset(75);     // Normal pitch
    m_orbitCam->setMinCameraDistance(1);
    m_orbitCam->setCameraDistance(5);
}

// Property getters/setters
double Flight3DViewer::cameraDistance() const
{
    return m_orbitCam ? m_orbitCam->cameraDistance() : 5.0;
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
    displayFlight(jsonArray);
}

#ifndef FLIGHT3DVIEWER_H
#define FLIGHT3DVIEWER_H

#include <QObject>
#include <QJsonArray>
#include <QColor>

// Add these forward declarations
namespace Esri::ArcGISRuntime {
class Scene;
class SceneQuickView;
class GraphicsOverlay;
class Graphic;
class OrbitGeoElementCameraController;
class SimpleMarkerSceneSymbol;
class SimpleRenderer;
class ArcGISTiledElevationSource;
}

Q_MOC_INCLUDE("SceneQuickView.h")

class Flight3DViewer : public QObject
{
    Q_OBJECT

    Q_PROPERTY(Esri::ArcGISRuntime::SceneQuickView* sceneView READ sceneView WRITE setSceneView NOTIFY sceneViewChanged)
    Q_PROPERTY(double cameraDistance READ cameraDistance WRITE setCameraDistance NOTIFY cameraDistanceChanged)
    Q_PROPERTY(double cameraHeading READ cameraHeading WRITE setCameraHeading NOTIFY cameraHeadingChanged)
    Q_PROPERTY(double cameraPitch READ cameraPitch WRITE setCameraPitch NOTIFY cameraPitchChanged)
    Q_PROPERTY(bool hasActiveFlight READ hasActiveFlight NOTIFY activeFlightChanged)

public:
    explicit Flight3DViewer(QObject *parent = nullptr);
    ~Flight3DViewer();

    static void init();

    Esri::ArcGISRuntime::SceneQuickView* sceneView() const;
    void setSceneView(Esri::ArcGISRuntime::SceneQuickView* sceneView);

    double cameraDistance() const;
    void setCameraDistance(double distance);

    double cameraHeading() const;
    void setCameraHeading(double heading);

    double cameraPitch() const;
    void setCameraPitch(double pitch);

    bool hasActiveFlight() const;

public slots:
    Q_INVOKABLE void displayFlight(const QJsonArray& flightData);
    Q_INVOKABLE void clearFlight();
    Q_INVOKABLE void cockpitView();
    Q_INVOKABLE void followView();
    Q_INVOKABLE void displayFlight(const QVariantList& flightDataVariant);

signals:
    void sceneViewChanged();
    void cameraDistanceChanged();
    void cameraHeadingChanged();
    void cameraPitchChanged();
    void activeFlightChanged();

private:
    void setupScene();
    void createFlightGraphic(const QJsonArray& flightData);
    QColor getAltitudeColor(double altitude);

    Esri::ArcGISRuntime::Scene* m_scene = nullptr;
    Esri::ArcGISRuntime::SceneQuickView* m_sceneView = nullptr;
    Esri::ArcGISRuntime::GraphicsOverlay* m_flightOverlay = nullptr;
    Esri::ArcGISRuntime::OrbitGeoElementCameraController* m_orbitCam = nullptr;
    Esri::ArcGISRuntime::Graphic* m_flightGraphic = nullptr;
    bool m_hasActiveFlight = false;
};

#endif

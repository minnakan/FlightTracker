import QtQuick
import QtQuick.Controls
import Esri.FlightTracker
import "qrc:/esri.com/imports/Calcite" 1.0 as Calcite

Item {
    // Properties passed from StackView.push()
    property var flightData: null
    property var flightTracker: null

    SceneView {
        id: sceneView
        anchors.fill: parent
        focus: true
    }

    Flight3DViewer {
        id: flight3DViewer
        sceneView: sceneView
        mapView: miniMapView

        Component.onCompleted: {
            // Check if we have valid flight data first
            if (flightData && Array.isArray(flightData) && flightData.length > 0) {
                console.log("Displaying flight in 3D view")
                displayFlight(flightData)
            } else if (flightTracker && typeof flightTracker.getSelectedFlightData === "function") {
                var data = flightTracker.getSelectedFlightData()
                if (data && data.length > 0) {
                    console.log("Displaying flight in 3D view from tracker")
                    displayFlight(data)
                } else {
                    console.log("No flight selected - select a flight first")
                }
            } else {
                console.log("No flight data available for 3D view")
            }
        }
    }

    // Back to 2D button
    Calcite.Button {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 16
        text: "Back to 2D"

        onClicked: {
            backTo2DRequested()
        }
    }

    // Camera controls
    Rectangle {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 16
        width: 220
        height: 200
        color: Calcite.Calcite.foreground1
        border.color: Calcite.Calcite.border1
        border.width: 1
        radius: 4
        visible: flight3DViewer.hasActiveFlight

        Column {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 12

            Text {
                text: "Camera Controls"
                color: Calcite.Calcite.text1
                font.pixelSize: 14
                font.weight: Font.Medium
            }

            Row {
                spacing: 8
                Calcite.Button {
                    text: "Follow"
                    onClicked: flight3DViewer.followView()
                }
                Calcite.Button {
                    text: "Cockpit"
                    onClicked: flight3DViewer.cockpitView()
                }
            }

            Text {
                text: "Distance: " + Math.round(flight3DViewer.cameraDistance) + "m"
                color: Calcite.Calcite.text2
                font.pixelSize: 10
            }

            Slider {
                id: distanceSlider
                width: parent.width
                from: 100
                to: 100000
                value: flight3DViewer.cameraDistance

                onMoved: {
                    flight3DViewer.cameraDistance = value
                }
            }

            Text {
                text: "Heading: " + Math.round(flight3DViewer.cameraHeading) + "°"
                color: Calcite.Calcite.text2
                font.pixelSize: 10
            }

            Slider {
                id: headingSlider
                width: parent.width
                from: -180
                to: 180
                value: flight3DViewer.cameraHeading

                onValueChanged: {
                    flight3DViewer.cameraHeading = value
                }
            }
        }
    }

    // Minimap in bottom-left corner
    Rectangle {
        id: minimapContainer
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 10
        width: 300
        height: 200
        color: "black"
        border.color: "white"
        border.width: 2
        radius: 5

        MapView {
            id: miniMapView
            anchors.fill: parent
            anchors.margins: 2
        }

        Text {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: 5
            text: "Map"
            color: "white"
            font.pixelSize: 12
            font.bold: true
        }
    }

    signal backTo2DRequested()
}

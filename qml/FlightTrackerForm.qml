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

import QtQuick
import QtQuick.Controls
import Esri.FlightTracker
import Esri.ArcGISRuntime.Toolkit
import QtQuick.Dialogs
import "qrc:/esri.com/imports/Calcite" 1.0 as Calcite


Item {

    MapView {
        id: view
        anchors.fill: parent
        focus: true
    }

    Component.onCompleted: {
            Calcite.Calcite.theme = Calcite.Calcite.Dark
        }

    Rectangle {
        anchors.fill: view
        color: "transparent"

        MouseArea {
            id: mapMouseArea
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton


            onPressed: function(mouse) {
                mouse.accepted = false
                model.selectFlightAtPoint(Qt.point(mouse.x, mouse.y))
            }

            propagateComposedEvents: true
            preventStealing: false
        }
    }

    Rectangle {
        id: altitudeLegend
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.rightMargin: 32
        anchors.bottomMargin: 32
        width: 400
        height: 45
        color: "#E0000000"  // Semi-transparent black
        border.color: "white"
        border.width: 1
        radius: 5
        z: 10
        visible: model.isAuthenticated

        Column {
            anchors.centerIn: parent
            spacing: 4

            Text {
                text: "ALTITUDE (ft)"
                color: "white"
                font.pixelSize: 10
                font.bold: true
                anchors.horizontalCenter: parent.horizontalCenter
            }

            // Gradient rectangle
            Rectangle {
                width: 360
                height: 12
                anchors.horizontalCenter: parent.horizontalCenter
                border.color: "white"
                border.width: 0.3

                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: "#FF4500" }   // 0-500 (Red-orange)
                    GradientStop { position: 0.09; color: "#FF8C00" }  // 500-1K (Orange)
                    GradientStop { position: 0.18; color: "#FFD700" }  // 1K-2K (Gold)
                    GradientStop { position: 0.30; color: "#FFFF00" }  // 2K-4K (Yellow)
                    GradientStop { position: 0.42; color: "#ADFF2F" }  // 4K-6K (Yellow-green)
                    GradientStop { position: 0.54; color: "#00FF00" }  // 6K-8K (Green)
                    GradientStop { position: 0.66; color: "#00FF7F" }  // 8K-10K (Spring green)
                    GradientStop { position: 0.78; color: "#00BFFF" }  // 10K-20K (Deep sky blue)
                    GradientStop { position: 0.87; color: "#0064FF" }  // 20K-30K (Blue)
                    GradientStop { position: 0.93; color: "#8A2BE2" }  // 30K-40K (Blue violet)
                    GradientStop { position: 1.0; color: "#800080" }   // 40K+ (Purple)
                }
            }

            // Labels positioned to match gradient positions exactly
            Item {
                width: 360
                height: 10
                anchors.horizontalCenter: parent.horizontalCenter

                // Position each text at the exact gradient stop position
                Text {
                    text: "0"
                    color: "white"
                    font.pixelSize: 7
                    x: 0 * parent.width - width/2  // position 0.0
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: "500"
                    color: "white"
                    font.pixelSize: 7
                    x: 0.09 * parent.width - width/2  // position 0.09
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: "1K"
                    color: "white"
                    font.pixelSize: 7
                    x: 0.18 * parent.width - width/2  // position 0.18
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: "2K"
                    color: "white"
                    font.pixelSize: 7
                    x: 0.30 * parent.width - width/2  // position 0.30
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: "4K"
                    color: "white"
                    font.pixelSize: 7
                    x: 0.42 * parent.width - width/2  // position 0.42
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: "6K"
                    color: "white"
                    font.pixelSize: 7
                    x: 0.54 * parent.width - width/2  // position 0.54
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: "8K"
                    color: "white"
                    font.pixelSize: 7
                    x: 0.66 * parent.width - width/2  // position 0.66
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: "10K"
                    color: "white"
                    font.pixelSize: 7
                    x: 0.78 * parent.width - width/2  // position 0.78
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: "20K"
                    color: "white"
                    font.pixelSize: 7
                    x: 0.87 * parent.width - width/2  // position 0.87
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: "30K"
                    color: "white"
                    font.pixelSize: 7
                    x: 0.93 * parent.width - width/2  // position 0.93
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: "40K+"
                    color: "white"
                    font.pixelSize: 7
                    x: 1.0 * parent.width - width/2  // position 1.0
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }

    Rectangle {
        id: authOverlay
        anchors.fill: parent
        color: "black"
        opacity: 0.8
        visible: !model.isAuthenticated

        Column {
            anchors.centerIn: parent
            spacing: 20

            Text {
                id: authStatus
                text: "Starting OpenSky Network authentication..."
                color: "white"
                font.pixelSize: 16
                anchors.horizontalCenter: parent.horizontalCenter
            }

            BusyIndicator {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: !model.isAuthenticated
            }
        }
    }

    Rectangle {
        id: popupContainer
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 400
        z: 20
        color: Calcite.Calcite.background
        border.color: Calcite.Calcite.border1
        border.width: 1

        // Initially positioned off-screen to the left
        x: model.hasSelectedFlight ? 0 : -width


        Behavior on x {
            SmoothedAnimation {
                duration: 300
                easing.type: Easing.OutQuart
            }
        }


        visible: true//model.hasSelectedFlight

        // Header bar with close button - Calcite themed
        Rectangle {
            id: headerBar
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 48
            color: Calcite.Calcite.foreground1  // Dynamic header background
            border.color: Calcite.Calcite.border2  // Dynamic border
            border.width: 1
            z: 1

            // Close button - Calcite themed
            Rectangle {
                id: closeButton
                anchors.right: parent.right
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                width: 32
                height: 32
                color: closeButtonMouseArea.pressed ?
                       (Calcite.Calcite.theme === Calcite.Calcite.Light ? "#e0e0e0" : "#404040") :
                       (closeButtonMouseArea.containsMouse ?
                        (Calcite.Calcite.theme === Calcite.Calcite.Light ? "#f0f0f0" : "#353535") :
                        "transparent")
                border.color: closeButtonMouseArea.containsMouse ? Calcite.Calcite.border1 : "transparent"
                border.width: 1
                radius: 2

                // Close icon (X) - dynamic color
                Text {
                    anchors.centerIn: parent
                    text: "✕"
                    font.pixelSize: 14
                    color: Calcite.Calcite.text2  // Dynamic text color
                    font.family: "Arial"
                }

                MouseArea {
                    id: closeButtonMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        model.clearFlightSelection()
                    }
                }
            }

            // Header title - Calcite themed
            Text {
                anchors.left: parent.left
                anchors.leftMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: closeButton.left
                anchors.rightMargin: 8
                text: "Flight Information"
                font.pixelSize: 16
                font.weight: Font.Medium
                color: Calcite.Calcite.text1  // Dynamic text color
                elide: Text.ElideRight
            }
        }

        // PopupView positioned below header, extending to bottom
        PopupView {
            id: flightPopupView
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: headerBar.bottom
            anchors.bottom: parent.bottom

            popup: model.selectedFlightPopup
            closeCallback: null
            openUrlsWithSystemDefaultApplication: true
            openAttachmentsWithSystemDefaultApplication: true
            openImagesInApp: true

            //Set title and text colors through palette
            palette {
                windowText: Calcite.Calcite.text1
                text: Calcite.Calcite.text1
                brightText: Calcite.Calcite.text1
                buttonText: Calcite.Calcite.text1
                base: Calcite.Calcite.background
                window: Calcite.Calcite.foreground1
            }

            background: Rectangle {
                color: Calcite.Calcite.background
                border.color: Calcite.Calcite.foreground1
                border.width: Calcite.Calcite.theme === Calcite.Calcite.Light ? 1 : 0
            }
        }
    }

    Rectangle {
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 16
        width: controlsRow.width + 20
        height: controlsRow.height + 12
        color: Calcite.Calcite.foreground1
        border.color: Calcite.Calcite.border1
        border.width: 1
        radius: 0
        z: 15
        visible: model.isAuthenticated

        Row {
            id: controlsRow
            anchors.centerIn: parent
            spacing: 12

            Calcite.Button {
                id: refreshButton
                text: "Update Flights"
                anchors.verticalCenter: parent.verticalCenter

                onClicked: {
                    model.fetchFlightData()
                }
            }

            // Separator line
            Rectangle {
                width: 1
                height: refreshButton.height - 8
                color: Calcite.Calcite.border2
                anchors.verticalCenter: parent.verticalCenter
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2

                Text {
                    text: "Last Update"
                    color: Calcite.Calcite.text2
                    font.pixelSize: 10
                    font.family: "Segoe UI"
                    horizontalAlignment: Text.AlignHCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Text {
                    id: lastUpdateText
                    text: model.lastUpdateTime
                    color: Calcite.Calcite.text1
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    font.family: "Segoe UI"
                    horizontalAlignment: Text.AlignHCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }
    }

    Rectangle {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 16
        anchors.rightMargin: 16
        width: filterControlsRow.width + 20
        height: filterControlsRow.height + 12
        color: Calcite.Calcite.foreground1
        border.color: Calcite.Calcite.border1
        border.width: 1
        radius: 0
        z: 25
        visible: model.isAuthenticated


        Row {
            id: filterControlsRow
            anchors.centerIn: parent
            spacing: 16

            // Track History Toggle
            Row {
                spacing: 8
                anchors.verticalCenter: parent.verticalCenter

                Text {
                    text: "Show Track"
                    color: Calcite.Calcite.text1
                    font.pixelSize: 12
                    font.family: "Segoe UI"
                    anchors.verticalCenter: parent.verticalCenter
                }

                Calcite.Switch {
                    id: trackSwitch
                    checked: false
                    anchors.verticalCenter: parent.verticalCenter

                    onCheckedChanged: {
                        model.showTrack = checked
                    }
                }
            }

            // Separator line
            Rectangle {
                width: 1
                height: parent.height - 16
                color: Calcite.Calcite.border2
                anchors.verticalCenter: parent.verticalCenter
            }

            // Filter Panel (embedded without its own container)
            FilterPanel {
                id: filterPanel
                anchors.verticalCenter: parent.verticalCenter
                flightModel: model
                height: parent.height
            }
        }
    }

    Calcite.Button {
        id: view3DButton
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 32
        text: "View in 3D"
        z: 15
        enabled: model.isAuthenticated && model.hasSelectedFlight

        onClicked: {
            console.log("3D View button clicked")
            console.log("Model:", model)
            switchTo3DRequested(model)
        }
    }
    signal switchTo3DRequested(var flightModel)

    // Declare the C++ instance which creates the map etc. and supply the view
    FlightTracker {
        id: model
        mapView: view

        onAuthenticationSuccess: {
            authStatus.text = "Authentication successful! Token obtained."
        }

        onAuthenticationFailed: function(error) {
            authStatus.text = "Authentication failed: " + error
        }
    }
}

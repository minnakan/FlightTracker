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

ApplicationWindow {
    id: window
    visible: true
    width: 800
    height: 600
    title: "Flight Tracker"

    visibility: Window.Maximized

    // Ensure proper window flags and modality
    flags: Qt.Window | Qt.WindowTitleHint | Qt.WindowSystemMenuHint | Qt.WindowMinMaxButtonsHint | Qt.WindowCloseButtonHint
    modality: Qt.NonModal

    // Minimum size to prevent invisible window
    minimumWidth: 400
    minimumHeight: 300

    StackView {
        id: stack
        anchors.fill: parent

        initialItem: MenuScreen {
            onStartClicked: {
                stack.push(flightTrackerComponent)
            }
        }
    }

    Component {
        id: flightTrackerComponent
        FlightTrackerForm {
            onSwitchTo3DRequested: function(flightModel) {
                // Check if the flightModel is valid and has the method
                if (flightModel && typeof flightModel.getSelectedFlightData === "function") {
                    var flightData = flightModel.getSelectedFlightData()
                    stack.push(flight3DComponent, {"flightData": flightData, "flightTracker": flightModel})
                } else {
                    console.error("Invalid flight model or missing getSelectedFlightData method")
                }
            }
        }
    }

    Component {
        id: flight3DComponent
        Flight3DView {
            onBackTo2DRequested: {
                stack.pop()
            }
        }
    }
}

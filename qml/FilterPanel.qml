// FilterPanel.qml - Using Calcite.Popup for proper theming

import QtQuick
import QtQuick.Controls
import "qrc:/esri.com/imports/Calcite" 1.0 as Calcite

Item {
    id: root

    // Public properties
    property bool isOpen: false

    property var flightModel: null

    width: Math.min(500, parent.width * 0.4)
    height: filterButton.height

    // Filter Button
    Calcite.Button {
        id: filterButton
        text: "Filter Flights"
        anchors.top: parent.top
        anchors.right: parent.right

        onClicked: {
            if (filterPopup.opened) {
                filterPopup.close()
            } else {
                filterPopup.open()
            }
        }
    }

    // Filter Popup using Calcite.Popup
    Calcite.Popup {
        id: filterPopup
        x: filterButton.x + filterButton.width - width
        y: filterButton.y + filterButton.height + 8
        width: Math.min(500, root.parent.width * 0.4)
        height: Math.min(600, root.parent.height * 0.8)

        modal: false
        closePolicy: Popup.NoAutoClose

        // Header with title - no close button
        Rectangle {
            id: header
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 48
            color: Calcite.Calcite.foreground1  // Same as flight info header
            border.color: Calcite.Calcite.border2
            border.width: 1

            // Header title - centered
            Text {
                anchors.centerIn: parent
                text: "Filters"
                font.pixelSize: 16
                font.weight: Font.Medium
                color: Calcite.Calcite.text1
            }
        }

        // Content area with scroll support
        ScrollView {
            id: contentFrame
            anchors.top: header.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: buttonRow.top
            anchors.bottomMargin: 8
            clip: true

            ScrollBar.vertical: Calcite.ScrollBar {
                    parent: contentFrame
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    policy: ScrollBar.AsNeeded
                }

            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            background: Rectangle {
                color: Calcite.Calcite.foreground1
                border.color: Calcite.Calcite.border2
                border.width: 1
            }

            Column {
                width: contentFrame.width
                spacing: 16
                padding: 16

                // Countries Filter Section
                Column {
                    width: parent.width - 32
                    spacing: 8

                    Text {
                        text: "Countries"
                        font.pixelSize: 14
                        font.weight: Font.Medium
                        color: Calcite.Calcite.text1
                    }

                    CountryFilterTree {
                        id: countryTree
                        width: parent.width
                        property var model: root.flightModel

                        onSelectionChanged: function(selectedCountries) {
                            if (root.flightModel) {
                                root.flightModel.selectedCountries = selectedCountries
                            }
                        }
                    }
                }

                // Separator
                Rectangle {
                    width: parent.width - 32
                    height: 1
                    color: Calcite.Calcite.border2
                }

                StatusFilterGroup {
                    id: statusFilter
                    width: parent.width - 32

                    // Configure the generic component for flight status
                    title: "Flight Status"
                    options: [
                        {text: "All Flights", value: "All"},
                        {text: "Airborne Only", value: "Airborne"},
                        {text: "On Ground Only", value: "OnGround"}
                    ]
                    selectedValue: "All"

                    // Connect the generic signal to your specific logic
                    onValueChanged: function(value) {
                        console.log("Flight status filter changed to:", value)
                        if (root.flightModel) {
                            root.flightModel.selectedFlightStatus = value
                        }
                    }
                }

                // Separator
                Rectangle {
                    width: parent.width - 32
                    height: 1
                    color: Calcite.Calcite.border2
                }

                // Altitude Filter Section
                RangeFilterGroup {
                    id: altitudeFilter
                     width: parent.width - 32

                     title: "Altitude (ft)"
                     unit: "ft"
                     minValue: 0
                     maxValue: 40000  // Changed from 45000
                     selectedMinValue: 0
                     selectedMaxValue: 40000  // Changed from 45000
                     stepSize: 1000

                     presets: [
                         {text: "All", min: 0, max: 40000},
                         {text: "Low", min: 0, max: 10000},
                         {text: "Mid", min: 10000, max: 30000},
                         {text: "High", min: 30000, max: 40000}
                     ]

                     onRangeChanged: function(minVal, maxVal) {
                         console.log("Altitude range changed:", minVal, "to", maxVal)
                                 if (root.flightModel) {
                                     root.flightModel.minAltitudeFilter = minVal
                                     root.flightModel.maxAltitudeFilter = maxVal
                                 }
                     }
                }

                //Sep
                Rectangle {
                    width: parent.width - 32
                    height: 1
                    color: Calcite.Calcite.border2
                }

                RangeFilterGroup {
                    id: speedFilter
                    width: parent.width - 32

                    title: "Speed (knots)"
                    unit: "knots"
                    minValue: 0
                    maxValue: 600
                    selectedMinValue: 0
                    selectedMaxValue: 600
                    stepSize: 10

                    presets: [
                        {text: "All", min: 0, max: 600},
                        {text: "Slow", min: 0, max: 200},
                        {text: "Fast", min: 200, max: 600}
                    ]

                    onRangeChanged: function(minVal, maxVal) {
                        console.log("Speed range changed:", minVal, "to", maxVal)
                        if (root.flightModel) {
                                    root.flightModel.minSpeedFilter = minVal
                                    root.flightModel.maxSpeedFilter = maxVal
                                }
                    }
                }

                Rectangle {
                    width: parent.width - 32
                    height: 1
                    color: Calcite.Calcite.border2
                }

                StatusFilterGroup {
                    id: verticalStatusFilter
                    width: parent.width - 32

                    title: "Vertical Rate"
                    options: [
                        {text: "All", value: "All"},
                        {text: "Climbing", value: "Climbing"},
                        {text: "Descending", value: "Descending"},
                        {text: "Level Flight", value: "Level"}
                    ]
                    selectedValue: "All"

                    onValueChanged: function(value) {
                            console.log("Vertical status filter changed to:", value)
                            if (root.flightModel) {
                                root.flightModel.selectedVerticalStatus = value  // Remove TODO comment and connect
                            }
                        }
                }
            }
        }

        // Bottom buttons row
        Row {
            id: buttonRow
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottomMargin: 4
            spacing: 12

            Calcite.Button {
                id: resetButton
                text: "Reset Filters"
                width: 100

                onClicked: {
                    console.log("Reset filters clicked")
                    if (root.flightModel) {
                        // Reset flight status filter
                        root.flightModel.selectedFlightStatus = "All"
                        statusFilter.selectedValue = "All"

                        // Reset vertical status filter
                        root.flightModel.selectedVerticalStatus = "All"
                        verticalStatusFilter.selectedValue = "All"

                        // Reset altitude filter
                        root.flightModel.minAltitudeFilter = 0
                        root.flightModel.maxAltitudeFilter = 40000
                        altitudeFilter.selectedMinValue = 0
                        altitudeFilter.selectedMaxValue = 40000
                        altitudeFilter.resetToDefaults()

                        // Reset speed filter
                        root.flightModel.minSpeedFilter = 0
                        root.flightModel.maxSpeedFilter = 600
                        speedFilter.selectedMinValue = 0
                        speedFilter.selectedMaxValue = 600
                        speedFilter.resetToDefaults()

                        // Reset country filter to all countries
                        if (root.flightModel.availableCountries) {
                            var allCountries = [];
                            var continents = root.flightModel.availableCountries;
                            for (var continent in continents) {
                                var countries = continents[continent];
                                allCountries = allCountries.concat(countries);
                            }
                            root.flightModel.selectedCountries = allCountries;
                            countryTree.resetToAllSelected();
                        }
                    }
                }
            }

            Calcite.Button {
                id: closeButton
                text: "Close"
                width: 100

                onClicked: {
                    filterPopup.close()
                }
            }
        }
    }
}

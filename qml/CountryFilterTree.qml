// CountryFilterTree.qml - Simplified tree structure for country filtering

import QtQuick
import QtQuick.Controls
import "qrc:/esri.com/imports/Calcite" 1.0 as Calcite

Column {
    id: root

    property var countryData: flightModel.availableCountries || {}

    // Simple object to track what's selected
    property var countrySelection: ({})

    // Signal for parent component
    signal selectionChanged(var selectedCountries)

    spacing: 4

    Component.onCompleted: {
        // Wait for data to be available
        if (Object.keys(countryData).length === 0) {
            return  // Data not ready yet
        }

        var newSelection = {}
        for (var continent in countryData) {
            var countries = countryData[continent]
            for (var i = 0; i < countries.length; i++) {
                newSelection[countries[i]] = true
            }
        }
        countrySelection = newSelection
        emitSelectionChange()
    }

    onCountryDataChanged: {
        if (Object.keys(countryData).length > 0) {
            var newSelection = {}
                // Check if we already have a selection from the backend
                if (flightModel && flightModel.selectedCountries && flightModel.selectedCountries.length > 0) {
                    // Use the backend's current selection instead of selecting all
                    var backendSelected = model.selectedCountries

                    for (var i = 0; i < backendSelected.length; i++) {
                        newSelection[backendSelected[i]] = true
                    }
                    countrySelection = newSelection
                    emitSelectionChange()
                } else {
                    // Only select all if we have no backend selection (first time)
                    for (var continent in countryData) {
                        var countries = countryData[continent]
                        for (var i = 0; i < countries.length; i++) {
                            newSelection[countries[i]] = true
                        }
                    }
                    countrySelection = newSelection
                    emitSelectionChange()
                }
            }
    }

    // Root "All Countries" checkbox with dropdown
    Column {
        id: rootSection
        width: parent.width
        spacing: 4
        property bool expanded: false  // Add expanded state for root section

        // Root header with expand/collapse
        Rectangle {
            width: parent.width
            height: 32
            color: "transparent"

            Row {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8

                // Expand/collapse arrow for All Countries
                Text {
                    text: rootSection.expanded ? "▼" : "▶"
                    font.pixelSize: 10
                    color: Calcite.Calcite.text2
                    anchors.verticalCenter: parent.verticalCenter
                }

                Calcite.CheckBox {
                    id: selectAllCheckbox
                    text: "All Countries (" + getSelectedCount() + "/" + getTotalCount() + ")"
                    font.weight: Font.Medium
                    checked: isAllSelected()
                    anchors.verticalCenter: parent.verticalCenter

                    onClicked: {
                        if (checked) {
                            selectAll()
                        } else {
                            selectNone()
                        }
                    }
                }
            }

            // Click anywhere on header to expand/collapse
            MouseArea {
                anchors.fill: parent
                anchors.leftMargin: 56  // Skip arrow + checkbox area (same as continents)
                onClicked: {
                    rootSection.expanded = !rootSection.expanded
                }
                cursorShape: Qt.PointingHandCursor
            }
        }

        // Selection summary - removed/hidden
        Rectangle {
            width: parent.width
            height: 24
            color: Calcite.Calcite.background
            border.color: Calcite.Calcite.border1
            border.width: 1
            radius: 2
            visible: false  // Hide this since we show count in the checkbox text now

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                text: getSelectedCount() + " of " + getTotalCount() + " countries selected"
                font.pixelSize: 11
                color: Calcite.Calcite.text2
            }
        }
    }

    // Continent sections container
    Column {
        width: parent.width
        spacing: 4
        visible: rootSection.expanded  // Show only when All Countries is expanded

        Repeater {
            model: Object.keys(root.countryData).sort()

            // Continent section
            Column {
                property string continentName: modelData
                property var countries: root.countryData[modelData] || []
                property bool expanded: false

                width: root.width
                spacing: 2

            // Continent header
            Rectangle {
                width: parent.width
                height: 32
                color: expanded ? Calcite.Calcite.foreground2 : "transparent"
                radius: 2

                Row {
                    anchors.left: parent.left
                    anchors.leftMargin: 16
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8

                    // Expand/collapse arrow
                    Text {
                        text: expanded ? "▼" : "▶"
                        font.pixelSize: 10
                        color: Calcite.Calcite.text2
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    // Continent checkbox
                    Calcite.CheckBox {
                        id: continentCheckbox
                        anchors.verticalCenter: parent.verticalCenter
                        checked: isContinentSelected(continentName)

                        onClicked: {
                            if (checked) {
                                selectContinent(continentName)
                            } else {
                                deselectContinent(continentName)
                            }
                        }
                    }

                    // Continent name with count
                    Text {
                        text: continentName + " (" + getContinentSelectedCount(continentName) + "/" + countries.length + ")"
                        font.pixelSize: 13
                        font.weight: Font.Medium
                        color: Calcite.Calcite.text1
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                // Click on text to expand/collapse, but not on checkbox or arrow
                MouseArea {
                    anchors.fill: parent
                    anchors.leftMargin: 56  // Skip arrow + checkbox area
                    onClicked: {
                        expanded = !expanded
                    }
                    cursorShape: Qt.PointingHandCursor
                }
            }

            // Countries list
            Column {
                width: parent.width
                visible: expanded
                spacing: 2

                Repeater {
                    model: countries.sort()

                    Rectangle {
                        width: parent.width
                        height: 28
                        color: countryMouseArea.containsMouse ? Calcite.Calcite.foreground2 : "transparent"
                        radius: 2

                        Row {
                            anchors.left: parent.left
                            anchors.leftMargin: 48
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 8

                            Calcite.CheckBox {
                                id: countryCheckbox
                                anchors.verticalCenter: parent.verticalCenter
                                checked: isCountrySelected(modelData)

                                onClicked: {
                                    toggleCountry(modelData, checked)
                                }
                            }

                            Text {
                                text: modelData
                                font.pixelSize: 12
                                color: Calcite.Calcite.text1
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }

                        MouseArea {
                            id: countryMouseArea
                            anchors.fill: parent
                            anchors.leftMargin: 16
                            hoverEnabled: true
                            onClicked: {
                                var isSelected = isCountrySelected(modelData)
                                toggleCountry(modelData, !isSelected)
                            }
                            cursorShape: Qt.PointingHandCursor
                        }
                    }
                }
            }
        }
    }
    }

    // Helper functions
    function isCountrySelected(country) {
        return countrySelection[country] === true
    }

    function isContinentSelected(continent) {
        var countries = countryData[continent] || []
        for (var i = 0; i < countries.length; i++) {
            if (!isCountrySelected(countries[i])) {
                return false
            }
        }
        return countries.length > 0
    }

    function isAllSelected() {
        for (var continent in countryData) {
            var countries = countryData[continent]
            for (var i = 0; i < countries.length; i++) {
                if (!isCountrySelected(countries[i])) {
                    return false
                }
            }
        }
        return true
    }

    function getSelectedCount() {
        var count = 0
        for (var country in countrySelection) {
            if (countrySelection[country]) {
                count++
            }
        }
        return count
    }

    function getTotalCount() {
        var total = 0
        for (var continent in countryData) {
            total += countryData[continent].length
        }
        return total
    }

    function getContinentSelectedCount(continent) {
        var count = 0
        var countries = countryData[continent] || []
        for (var i = 0; i < countries.length; i++) {
            if (isCountrySelected(countries[i])) {
                count++
            }
        }
        return count
    }

    function toggleCountry(country, selected) {
        var newSelection = {}
        // Copy existing selections
        for (var c in countrySelection) {
            newSelection[c] = countrySelection[c]
        }
        // Update this country
        newSelection[country] = selected
        countrySelection = newSelection
        emitSelectionChange()
    }

    function selectContinent(continent) {
        var newSelection = {}
        // Copy existing selections
        for (var c in countrySelection) {
            newSelection[c] = countrySelection[c]
        }
        // Select all countries in continent
        var countries = countryData[continent] || []
        for (var i = 0; i < countries.length; i++) {
            newSelection[countries[i]] = true
        }
        countrySelection = newSelection
        emitSelectionChange()
    }

    function deselectContinent(continent) {
        var newSelection = {}
        // Copy existing selections
        for (var c in countrySelection) {
            newSelection[c] = countrySelection[c]
        }
        // Deselect all countries in continent
        var countries = countryData[continent] || []
        for (var i = 0; i < countries.length; i++) {
            newSelection[countries[i]] = false
        }
        countrySelection = newSelection
        emitSelectionChange()
    }

    function selectAll() {
        var newSelection = {}
        for (var continent in countryData) {
            var countries = countryData[continent]
            for (var i = 0; i < countries.length; i++) {
                newSelection[countries[i]] = true
            }
        }
        countrySelection = newSelection
        emitSelectionChange()
    }

    function selectNone() {
        var newSelection = {}
        for (var continent in countryData) {
            var countries = countryData[continent]
            for (var i = 0; i < countries.length; i++) {
                newSelection[countries[i]] = false
            }
        }
        countrySelection = newSelection
        emitSelectionChange()
    }

    function emitSelectionChange() {
        var selectedList = []
        for (var country in countrySelection) {
            if (countrySelection[country]) {
                selectedList.push(country)
            }
        }
        selectionChanged(selectedList)
    }

    function resetToAllSelected() {
        if (Object.keys(countryData).length > 0) {
            var newSelection = {}
            for (var continent in countryData) {
                var countries = countryData[continent]
                for (var i = 0; i < countries.length; i++) {
                    newSelection[countries[i]] = true
                }
            }
            countrySelection = newSelection
            // Don't emit selection change since backend was already updated
        }
    }
}

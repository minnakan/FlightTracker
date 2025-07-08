// RangeFilterGroup.qml - Generic range filter component for altitude, speed, etc.
import QtQuick
import QtQuick.Controls
import "qrc:/esri.com/imports/Calcite" 1.0 as Calcite

Column {
    id: root

    // Generic properties
    property string title: "Range"
    property string unit: ""
    property real minValue: 0
    property real maxValue: 100
    property real selectedMinValue: 0
    property real selectedMaxValue: 100
    property real stepSize: 1

    // Preset buttons configuration
    property var presets: []  // Array of {text: "Label", min: 0, max: 100}

    signal rangeChanged(real minVal, real maxVal)

    spacing: 8

    Text {
        text: root.title
        font.pixelSize: 14
        font.weight: Font.Medium
        color: Calcite.Calcite.text1
    }

    Calcite.GroupBox {
        width: parent.width

        Column {
            width: parent.width
            spacing: 12

            // Range display
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 8

                Text {
                    text: formatValue(root.selectedMinValue) + " " + root.unit
                    font.pixelSize: 12
                    color: Calcite.Calcite.text1
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: "to"
                    font.pixelSize: 12
                    color: Calcite.Calcite.text2
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: formatMaxValue(root.selectedMaxValue) + " " + root.unit
                    font.pixelSize: 12
                    color: Calcite.Calcite.text1
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            // Range slider
            Calcite.RangeSlider {
                id: rangeSlider
                width: parent.width
                from: root.minValue
                to: root.maxValue
                first.value: root.selectedMinValue
                second.value: root.selectedMaxValue
                stepSize: root.stepSize

                first.onValueChanged: {
                    root.selectedMinValue = first.value
                    root.rangeChanged(first.value, second.value)
                }

                second.onValueChanged: {
                    root.selectedMaxValue = second.value
                    root.rangeChanged(first.value, second.value)
                }
            }

            // Dynamic preset buttons
            Row {
                width: parent.width * 0.9
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 4
                visible: root.presets.length > 0

                Repeater {
                    model: root.presets

                    Calcite.Button {
                        text: modelData.text
                        width: (parent.width - (root.presets.length - 1) * parent.spacing) / root.presets.length
                        height: 32
                        font.pixelSize: 10

                        onClicked: {
                            rangeSlider.first.value = modelData.min
                            rangeSlider.second.value = modelData.max
                        }
                    }
                }
            }

            Row {
               width: parent.width

               Text {
                   id: minText
                   text: formatValue(root.minValue) + " " + root.unit+" "
                   font.pixelSize: 10
                   color: Calcite.Calcite.text3
               }

               Item {
                   width: parent.width - minText.width - maxLabel.width
               }

               Text {
                   id: maxLabel
                   text: root.maxValue.toLocaleString() + "+ " + root.unit
                   font.pixelSize: 10
                   color: Calcite.Calcite.text3
               }
           }
        }
    }

    // Helper function to format values nicely
    function formatValue(value) {
        if (value >= 1000) {
            return Math.round(value).toLocaleString()
        } else if (value >= 1) {
            return Math.round(value * 10) / 10  // 1 decimal place
        } else {
            return Math.round(value * 100) / 100  // 2 decimal places
        }
    }

    // Helper function to format max values with + when at maximum
    function formatMaxValue(value) {
        if (value >= root.maxValue) {
            if (value >= 1000) {
                return Math.round(value).toLocaleString() + "+"
            } else {
                return formatValue(value) + "+"
            }
        } else {
            return formatValue(value)
        }
    }

    function resetToDefaults() {
        // Force slider to update by temporarily breaking and reconnecting bindings
        rangeSlider.first.value = root.minValue
        rangeSlider.second.value = root.maxValue

        // Update the properties
        root.selectedMinValue = root.minValue
        root.selectedMaxValue = root.maxValue

        // Emit the change
        root.rangeChanged(root.minValue, root.maxValue)
    }
}

// GenericStatusFilterGroup.qml - Generic status filter with configurable options
import QtQuick
import QtQuick.Controls
import "qrc:/esri.com/imports/Calcite" 1.0 as Calcite

Column {
    id: root

    // Generic properties
    property string title: "Status"
    property var options: [
        {text: "All", value: "All"},
        {text: "Option 2", value: "Value2"},
        {text: "Option 3", value: "Value3"}
    ]
    property string selectedValue: "All"

    signal valueChanged(string value)

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
            spacing: 8

            Repeater {
                model: root.options

                Calcite.RadioButton {
                    text: modelData.text
                    checked: root.selectedValue === modelData.value

                    onClicked: {
                        if (checked) {
                            root.selectedValue = modelData.value
                            root.valueChanged(modelData.value)
                        }
                    }
                }
            }
        }
    }
}

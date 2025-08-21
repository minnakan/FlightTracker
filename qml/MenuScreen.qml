import QtQuick
import QtQuick.Controls

Item {
    property bool isDarkTheme: true

    Rectangle {
        id: rectangle
        anchors.fill: parent
        anchors.leftMargin: 0
        anchors.rightMargin: 0
        anchors.topMargin: 0
        anchors.bottomMargin: 0
        color: isDarkTheme ? "#2b2b2b" : "#f5f5f5"

        Image {
            id: image
            width: 242
            height: 237
            anchors.top: parent.top
            anchors.topMargin: -49
            source: "qrc:/images/RadarLogo.png"
            anchors.verticalCenterOffset: -113
            anchors.horizontalCenterOffset: 0
            anchors.centerIn: parent
            layer.textureMirroring: ShaderEffectSource.NoMirroring
            layer.smooth: true
            layer.enabled: true
            fillMode: Image.PreserveAspectFit
        }

        Rectangle {
            id: customButton
            width: 228
            height: 50
            radius: 8
            color: mouseArea.pressed ? "#2d2d2d" : (mouseArea.containsMouse ? "#3a3a3a" : "#191819")
            border.color: "#ffffff"
            border.width: 1
            anchors.verticalCenterOffset: 40
            anchors.horizontalCenterOffset: 0
            anchors.centerIn: parent
            antialiasing: true

            Text {
                anchors.centerIn: parent
                text: "START"
                font.letterSpacing: 2
                color: "#ffffff"
                font.pixelSize: 20
                font.bold: true
                font.family: "Tahoma"
            }

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: startClicked()
            }

            Behavior on color {
                ColorAnimation { duration: 120 }
            }
        }

    }

    signal startClicked()
}

#-------------------------------------------------
#  Copyright 2025 ESRI
#
#  All rights reserved under the copyright laws of the United States
#  and applicable international laws, treaties, and conventions.
#
#  You may freely redistribute and use this sample code, with or
#  without modification, provided you include the original copyright
#  notice and use restrictions.
#
#  See the Sample code usage restrictions document for further information.
#-------------------------------------------------

TEMPLATE = app

CONFIG += c++17

# fixes dependency issues
QMAKE_PROJECT_DEPTH = 0

# additional modules are pulled in via arcgisruntime.pri
QT += qml quick
QT += quickcontrols2
QT += quick quickdialogs2

TARGET = FlightTracker

lessThan(QT_MAJOR_VERSION, 6) {
    error("$$TARGET requires Qt 6.8.2")
}

equals(QT_MAJOR_VERSION, 6) {
    lessThan(QT_MINOR_VERSION, 8) {
        error("$$TARGET requires Qt 6.8.2")
    }
	equals(QT_MINOR_VERSION, 8) : lessThan(QT_PATCH_VERSION, 2) {
		error("$$TARGET requires Qt 6.8.2")
	}
}

ARCGIS_RUNTIME_VERSION = 200.7.0
include($$PWD/arcgisruntime.pri)

#Arcgis sdk toolkit hardcoding values for now
# ArcGIS Toolkit - proper configuration
TOOLKIT_PATH = $$ARCGIS_TOOLKIT_PATH
isEmpty(TOOLKIT_PATH) {
    # Try common locations
    exists("C:/Program Files/ArcGIS SDKs/Qt/toolkit") {
        TOOLKIT_PATH = "C:/Program Files/ArcGIS SDKs/Qt/toolkit"
    } else:exists("D:/Qt/ArcGisSDK/arcgis-maps-sdk-toolkit-qt") {
        TOOLKIT_PATH = "D:/Qt/ArcGisSDK/arcgis-maps-sdk-toolkit-qt"
    }
}

!isEmpty(TOOLKIT_PATH) {
    exists($$TOOLKIT_PATH) {
        include($$TOOLKIT_PATH/uitools/toolkitcpp.pri)
        RESOURCES += $$TOOLKIT_PATH/calcite/Calcite/calcite.qrc
        DEFINES += TOOLKIT_AVAILABLE
    } else {
        warning("ArcGIS Toolkit not found at: $$TOOLKIT_PATH")
    }
} else {
    warning("ARCGIS_TOOLKIT_PATH not set")
}



HEADERS += \
    FlightTracker.h

SOURCES += \
    main.cpp \
    FlightTracker.cpp

RESOURCES += \
    qml/qml.qrc \
    Resources/Resources.qrc

#-------------------------------------------------------------------------------

win32 {
    include (Win/Win.pri)
}

macx {
    include (Mac/Mac.pri)
}

ios {
    include (iOS/iOS.pri)
}

android {
    include (Android/Android.pri)
}

DISTFILES += \
    qtquickcontrols2.conf

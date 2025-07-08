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

#include "FlightTracker.h"

#include "ArcGISRuntimeEnvironment.h"
#include "MapQuickView.h"

#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QFontDatabase>
#include <QDebug>
#include <QQuickStyle>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include "Esri/ArcGISRuntime/Toolkit/register.h"

//------------------------------------------------------------------------------

using namespace Esri::ArcGISRuntime;

QString loadArcGISApiKey()
{
    QFile configFile(":/config/Config/config.json"); // adjust accordingly
    if (configFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(configFile.readAll());
        QJsonObject config = doc.object();
        QJsonObject arcgis = config.value("arcgis").toObject();

        QString apiKey = arcgis.value("api_key").toString();

        if (!apiKey.isEmpty()) {
            qDebug() << "ArcGIS API key loaded from config.json";
            return apiKey;
        }
    }

    qWarning() << "Could not load ArcGIS API key from config.json";
    return QString("");
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);




    // Use of ArcGIS location services, such as basemap styles, geocoding, and routing services,
    // requires an access token. For more information see
    // https://links.esri.com/arcgis-runtime-security-auth.

    // The following methods grant an access token:

    // 1. User authentication: Grants a temporary access token associated with a user's ArcGIS account.
    // To generate a token, a user logs in to the app with an ArcGIS account that is part of an
    // organization in ArcGIS Online or ArcGIS Enterprise.

    // 2. API key authentication: Get a long-lived access token that gives your application access to
    // ArcGIS location services. Go to the tutorial at https://links.esri.com/create-an-api-key.
    // Copy the API Key access token.

    //QQuickStyle::addStylePath("qrc:///esri.com/imports/");


    const QString accessToken = loadArcGISApiKey();

    if (accessToken.isEmpty()) {
        qWarning()
            << "Use of ArcGIS location services, such as the basemap styles service, requires"
            << "you to authenticate with an ArcGIS account or set the API Key property.";
    } else {
        ArcGISRuntimeEnvironment::setApiKey(accessToken);
    }


    // Production deployment of applications built with ArcGIS Maps SDK requires you to
    // license ArcGIS Maps SDK functionality. For more information see
    // https://links.esri.com/arcgis-runtime-license-and-deploy.

    // ArcGISRuntimeEnvironment::setLicense("Place license string in here");

    // Register the map view for QML
    qmlRegisterType<MapQuickView>("Esri.FlightTracker", 1, 0, "MapView");

    // Register the FlightTracker (QQuickItem) for QML
    qmlRegisterType<FlightTracker>("Esri.FlightTracker", 1, 0, "FlightTracker");

    qmlRegisterModule("Calcite", 1, 0);

    // Initialize application view
    QQmlApplicationEngine engine;


    Esri::ArcGISRuntime::Toolkit::registerComponents(engine);

    engine.addImportPath("qrc:///esri.com/imports/");
    engine.addImportPath("qrc:///esri.com/imports");


    // Set the source
    engine.load(QUrl("qrc:/qml/main.qml"));

    return app.exec();
}

//------------------------------------------------------------------------------

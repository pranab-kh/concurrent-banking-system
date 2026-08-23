#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "BackendClient.h"
#include "QrCodeHelper.h"
#include "StressTestRunner.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    BackendClient backendClient;
    backendClient.connectToServer();

    QrCodeHelper qrCodeHelper;
    StressTestRunner stressTestRunner;

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty(
        "backend",
        &backendClient
    );

    engine.rootContext()->setContextProperty(
        "qrCodeHelper",
        &qrCodeHelper
    );

    engine.rootContext()->setContextProperty(
        "stressTestRunner",
        &stressTestRunner
    );

    engine.loadFromModule("bank", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
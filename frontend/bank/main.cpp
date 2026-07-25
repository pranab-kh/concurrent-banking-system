#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    // Make sure "main" is lowercase to match main.qml in CMakeLists.txt
    engine.loadFromModule("bank", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QApplication>
#include <QQmlContext>

#include "client.h"

int main(int argc, char *argv[])
{
    //QGuiApplication app(argc, argv);
    QApplication app(argc, argv);
    Client client;
    MyNetworkManager* myNetworkManager= MyNetworkManager::instance();
    //MyNetworkManager* login= Login::instance();
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("client",&client);
    engine.rootContext()->setContextProperty("mylogin",Login::instance());
    engine.rootContext()->setContextProperty("myFileManager",FileManager::instance());
    engine.rootContext()->setContextProperty("myNetworkManager",myNetworkManager);
    const QUrl url(u"qrc:/qml/main.qml"_qs);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);
    client.loadSaveCredentials();
    return app.exec();
}

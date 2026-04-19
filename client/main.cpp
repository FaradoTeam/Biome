#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/qml/pages/main.qml")));
    engine.load(QUrl(QStringLiteral("qrc:/qml/pages/AuthPage.qml")));
    engine.load(QUrl(QStringLiteral("qrc:/qml/theme/Theme.qml")));
    engine.load(QUrl(QStringLiteral("qrc:/qml/components/GearButton.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
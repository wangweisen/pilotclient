// #include "voiceclientui.h"

#include "blackcore/afv/model/atcstationmodel.h"
#include "blackcore/afv/model/afvmapreader.h"
#include "blackcore/afv/clients/afvclient.h"
#include "blackcore/application.h"
#include "blackmisc/network/user.h"
#include "blackmisc/obfuscation.h"
#include "blackconfig/buildconfig.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QPointer>
#include <QThread>
#include <QTimer>

using namespace BlackConfig;
using namespace BlackMisc;
using namespace BlackMisc::Network;
using namespace BlackCore;
using namespace BlackCore::Afv::Clients;
using namespace BlackCore::Afv::Model;

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication qa(argc, argv);

    CApplication a("sampleafvclient", CApplicationInfo::Sample);

    CAfvMapReader *afvMapReader = new CAfvMapReader(&a);
    afvMapReader->updateFromMap();

    CAfvClient voiceClient("https://voice1.vatsim.uk");

    // default user name
    QString defaultUserName("1234567");
    if (CBuildConfig::isLocalDeveloperDebugBuild())
    {
        CUser user("OBF:AwLZ7f9hUmpSZhm4=", "Joe Doe");
        defaultUserName = user.getId();
    }

    QQmlApplicationEngine engine;
    QQmlContext *ctxt = engine.rootContext();
    ctxt->setContextProperty("afvMapReader", afvMapReader);
    ctxt->setContextProperty("voiceClient", &voiceClient);
    ctxt->setContextProperty("userName", defaultUserName);

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    return a.exec();
}
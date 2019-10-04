/* Copyright (C) 2018
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \cond PRIVATE_TESTS

/*!
* \file
* \ingroup testblackfsd
*/

#include "blackconfig/buildconfig.h"
#include "blackcore/fsd/fsdclient.h"
#include "blackmisc/aviation/flightplan.h"
#include "blackmisc/network/clientprovider.h"
#include "blackmisc/network/rawfsdmessage.h"
#include "blackmisc/network/networkutils.h"
#include "blackmisc/simulation/ownaircraftproviderdummy.h"
#include "blackmisc/simulation/remoteaircraftproviderdummy.h"
#include "blackmisc/registermetadata.h"
#include "blackmisc/network/user.h"
#include "test.h"

#include <QObject>
#include <QSignalSpy>
#include <QTest>

using namespace BlackMisc;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::Geo;
using namespace BlackMisc::PhysicalQuantities;
using namespace BlackMisc::Network;
using namespace BlackMisc::Simulation;
using namespace BlackConfig;
using namespace BlackCore::Fsd;

namespace BlackFsdTest
{
    //! Testing FSD Client
    class CTestFSDClient : public QObject
    {
        Q_OBJECT

    public:
        //! Constructor
        explicit CTestFSDClient(QObject *parent = nullptr) : QObject(parent) {}

        //! Destructor
        virtual ~CTestFSDClient() {}

    private slots:
        void initTestCase();
        void init();
        void cleanup();
        void testConstructor();
        void testDeleteAtc();
        void testDeletePilot();
        void testTextMessage();
        void testRadioMessage();
        void testPilotDataUpdate();
        void testAtcDataUpdate();
        void testPong();
        void testClientResponseEmptyType();
        void testClientResponseRealName1();
        void testClientResponseRealName2();
        void testClientResponseRealName3();
        void testClientResponseCapabilities();
        void testPlaneInfoRequestFsinn();
        void testPlaneInformationFsinn();

        void testSendPilotLogin();
        void testSendAtcLogin();
        void testSendDeletePilot();
        void testSendDeleteAtc();
        void testSendPilotDataUpdate1();
        void testSendPilotDataUpdate2();
        void testSendAtcDataUpdate();
        void testSendPing();
        void testSendPong();
        void testSendClientResponse1();
        void testSendClientResponse2();
        void testSendClientQuery1();
        void testSendClientQuery2();
        void testSendClientQuery3();
        void testSendTextMessage1();
        void testSendTextMessage2();
        void testSendRadioMessage1();
        void testSendRadioMessage2();
        void testSendFlightPlan();
        void testSendPlaneInfoRequest();
        void testSendPlaneInformation1();
        void testSendPlaneInformation2();
        void testSendPlaneInformation3();
        void testSendPlaneInformation4();
        void testSendAircraftConfiguration();
        void testCom1FreqQueryResponse();
        void testPlaneInfoRequestResponse();
        void testAuth();
        void testConnection();

    private:
        CFSDClient *client = nullptr;
    };

    void CTestFSDClient::initTestCase()
    {
        BlackMisc::registerMetadata();
    }

    void CTestFSDClient::init()
    {
        const CServer server = CServer::swiftFsdTestServer(true);

        COwnAircraftProviderDummy::instance()->updateOwnCallsign("ABCD");
        CLivery livery;
        livery.setCombinedCode("BER");
        CAirlineIcaoCode airline("BER");
        livery.setAirlineIcaoCode(airline);
        CAircraftModel model("Cessna SP1 Paint2", CAircraftModel::TypeOwnSimulatorModel, CAircraftIcaoCode("B737"), livery);

        model.setSimulator(CSimulatorInfo::xplane());
        QString modelDescription("[CSL]");
        model.setDescription(modelDescription);
        COwnAircraftProviderDummy::instance()->updateOwnModel(model);
        // COwnAircraftProviderDummy::instance()->updateOwnIcaoCodes(CAircraftIcaoCode("B737"), CAirlineIcaoCode("BER"));

        CFrequency frequency(123.000, CFrequencyUnit::MHz());
        COwnAircraftProviderDummy::instance()->updateActiveComFrequency(frequency, CComSystem::Com1, {});

        client = new CFSDClient(CClientProviderDummy::instance(), COwnAircraftProviderDummy::instance(), CRemoteAircraftProviderDummy::instance(), this);
        client->setUnitTestMode(true);
        client->setCallsign("ABCD");
        client->setClientName("Test Client");
        client->setVersion(0, 8);
        client->setClientCapabilities(Capabilities::AtcInfo | Capabilities::AircraftInfo | Capabilities::AircraftConfig);
        client->setLoginMode(BlackMisc::Network::CLoginMode::Pilot);
        client->setServer(server);
        client->setPilotRating(PilotRating::Student);
        client->setSimType(SimType::XPLANE10);
        client->setPilotRating(PilotRating::Student);
        QString key("727d1efd5cb9f8d2c28372469d922bb4");
        client->setClientIdAndKey(0xb9ba, key.toLocal8Bit());
    }

    void CTestFSDClient::cleanup()
    {
        delete client;
    }

    void CTestFSDClient::testConstructor()
    {

    }

    void CTestFSDClient::testDeleteAtc()
    {
        QSignalSpy spy(client, &CFSDClient::deleteAtcReceived);
        client->sendFsdMessage("#DAEDDM_OBS:1234567\r\n");

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();

        QCOMPARE(arguments.at(0).toString(), "1234567");
    }

    void CTestFSDClient::testDeletePilot()
    {
        QSignalSpy spy(client, &CFSDClient::deletePilotReceived);
        client->sendFsdMessage("#DPOEHAB:1234567\r\n");

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();

        QCOMPARE(arguments.at(0).toString(), "1234567");
    }

    void CTestFSDClient::testTextMessage()
    {
        QSignalSpy spy(client, &CFSDClient::textMessagesReceived);
        client->sendFsdMessage("#TMEDMM_CTR:BER721:Hey how are you doing?\r\n");

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        CTextMessageList messages = arguments.at(0).value<CTextMessageList>();
        QCOMPARE(messages.size(), 1);
        CTextMessage message = messages.front();
        QCOMPARE(message.getMessage(), "Hey how are you doing?");
        QCOMPARE(message.getRecipientCallsign(), "BER721");
        QCOMPARE(message.getSenderCallsign(), "EDMM_CTR");
    }

    void CTestFSDClient::testRadioMessage()
    {
        // reference
        const CFrequency frequency(124050, CFrequencyUnit::kHz());
        const QString text("BER721, Descend F140 when ready");

        COwnAircraftProviderDummy::instance()->updateActiveComFrequency(CFrequency(124050, CFrequencyUnit::kHz()), CComSystem::Com1, CIdentifier::anonymous());

        QSignalSpy spy(client, &CFSDClient::textMessagesReceived);
        client->sendFsdMessage("#TMEDMM_CTR:@24050:BER721, Descend F140 when ready\r\n");

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        CTextMessageList receivedMessages = arguments.at(0).value<CTextMessageList>();
        QCOMPARE(receivedMessages.size(), 1);
        CTextMessage message = receivedMessages.front();
        QCOMPARE(message.getMessage(), text);
        QCOMPARE(message.getFrequency(), frequency);
        QCOMPARE(message.getSenderCallsign(), "EDMM_CTR");

        QSignalSpy spy2(client, &CFSDClient::textMessagesReceived);
        client->sendFsdMessage("#TMEDMM_CTR:@24050&@27000:BER721, Descend F140 when ready\r\n");

        QCOMPARE(spy2.count(), 1);
        arguments = spy.takeFirst();
        receivedMessages = arguments.at(0).value<CTextMessageList>();
        QCOMPARE(receivedMessages.size(), 1);
        message = receivedMessages.front();
        QCOMPARE(message.getMessage(), text);
        QCOMPARE(message.getFrequency(), frequency);
        QCOMPARE(message.getSenderCallsign(), "EDMM_CTR");
    }

    void CTestFSDClient::testPilotDataUpdate()
    {
        QSignalSpy spy(client, &CFSDClient::pilotDataUpdateReceived);
        client->sendFsdMessage("@N:ABCD:1200:1:48.353855:11.786155:110:0:4290769188:1\r\n");

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 2);
        const CAircraftSituation situation = arguments.at(0).value<CAircraftSituation>();
        const CTransponder transponder = arguments.at(1).value<CTransponder>();

        QCOMPARE(situation.getCallsign().asString(), "ABCD");
        QCOMPARE(transponder.getTransponderMode(), CTransponder::ModeC);
        QCOMPARE(transponder.getTransponderCode(), 1200);
        QCOMPARE(situation.getPosition().latitude(), CLatitude(48.353855, CAngleUnit::deg()));
        QCOMPARE(situation.getPosition().longitude(), CLongitude(11.786155, CAngleUnit::deg()));
        QCOMPARE(situation.getAltitude(), CAltitude(110, CLengthUnit::ft()));
        QCOMPARE(situation.getPressureAltitude(), CAltitude(111, CAltitude::MeanSeaLevel, CAltitude::PressureAltitude, CLengthUnit::ft()));
        QCOMPARE(situation.getGroundSpeed(), CSpeed(0.0, CSpeedUnit::kts()));
        // TODO
//        QVERIFY(qAbs(arguments.at(9).toDouble()) < 1.0);
//        QVERIFY(qAbs(arguments.at(10).toDouble()) < 1.0);
//        QVERIFY(qAbs(arguments.at(11).toDouble() - 25) < 1.0);
//        QCOMPARE(arguments.at(12).toBool(), false);
    }

    void CTestFSDClient::testAtcDataUpdate()
    {
        QSignalSpy spy(client, &CFSDClient::atcDataUpdateReceived);
        client->sendFsdMessage("%ZZZZ_APP:28200:5:150:5:48.11028:16.56972:0\r\n");
        const CCallsign callsign("ZZZZ_APP");
        const CFrequency freq(128200, CFrequencyUnit::kHz());
        const CCoordinateGeodetic pos(48.11028, 16.56972, 0.0);
        const CLength range(150.0, CLengthUnit::NM());

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 4);
        QCOMPARE(arguments.at(0).value<CCallsign>(), callsign);
        QCOMPARE(arguments.at(1).value<CFrequency>(), freq);
        QCOMPARE(arguments.at(2).value<CCoordinateGeodetic>(), pos);
        QCOMPARE(arguments.at(3).value<CLength>(), range);
    }

    void CTestFSDClient::testPong()
    {
        QSignalSpy spy(client, &CFSDClient::pongReceived);
        client->sendFsdMessage("$POSERVER:BER368:90835991\r\n");

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 2);
        QCOMPARE(arguments.at(0).toString(), "SERVER");
        bool ok;
        arguments.at(1).toDouble(&ok);
        QVERIFY(ok);
    }

    void CTestFSDClient::testClientResponseEmptyType()
    {
        client->sendFsdMessage("$CRDLH123:BER721::Jon Doe\r\n");
    }

    void CTestFSDClient::testClientResponseRealName1()
    {
        QSignalSpy spy(client, &CFSDClient::realNameResponseReceived);
        client->sendFsdMessage("$CRDLH123:BER721:RN:Jon Doe\r\n");

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 2);
        QCOMPARE(arguments.at(0).toString(), "DLH123");
        QCOMPARE(arguments.at(1).toString(), "Jon Doe");
    }

    void CTestFSDClient::testClientResponseRealName2()
    {
        QSignalSpy spy(client, &CFSDClient::realNameResponseReceived);
        client->sendFsdMessage("$CRDLH123:BER721:RN:Jon Doe:\r\n");

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 2);
        QCOMPARE(arguments.at(0).toString(), "DLH123");
        QCOMPARE(arguments.at(1).toString(), "Jon Doe");
    }

    void CTestFSDClient::testClientResponseRealName3()
    {
        QSignalSpy spy(client, &CFSDClient::realNameResponseReceived);
        client->sendFsdMessage("$CRDLH123:BER721:RN:Jon Doe::1\r\n");

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 2);
        QCOMPARE(arguments.at(0).toString(), "DLH123");
        QCOMPARE(arguments.at(1).toString(), "Jon Doe");
    }

    void CTestFSDClient::testClientResponseCapabilities()
    {
        QSignalSpy spy(client, &CFSDClient::capabilityResponseReceived);
        client->sendFsdMessage("$CRAUA64MN:DECHK:CAPS:VERSION=1:ATCINFO=1:MODELDESC=1:ACCONFIG=1\r\n");

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 2);
        CClient::Capabilities caps = arguments.at(1).value<BlackMisc::Network::CClient::Capabilities>();
        QVERIFY(caps.testFlag(CClient::FsdWithAircraftConfig));
    }

    void CTestFSDClient::testPlaneInfoRequestFsinn()
    {
        QSignalSpy spy(client, &CFSDClient::planeInformationFsinnReceived);
        client->sendFsdMessage("#SBLHA449:DLH53M:FSIPIR:1::A320:10.05523:0.49785:1320.00000:2.AB13B127.5611C1A2::A320-200 Airbus Leipzig Air CFM\r\n");

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 5);
        /*QCOMPARE(arguments.at(0).toString(), "DLH123");
        QCOMPARE(arguments.at(1).toString(), "Jon Doe");*/
    }

    void CTestFSDClient::testPlaneInformationFsinn()
    {
        QSignalSpy spy(client, &CFSDClient::planeInformationFsinnReceived);
        client->sendFsdMessage("#SBLHA449:AUA89SY:FSIPI:1::A320:10.05523:0.49785:1320.00000:2.AB13B127.5611C1A2::A320-200 Airbus Leipzig Air CFM\r\n");

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 5);
//        QCOMPARE(arguments.at(0).toString(), "DLH123");
//        QCOMPARE(arguments.at(1).toString(), "Jon Doe");
    }

    void CTestFSDClient::testSendPilotLogin()
    {
        QSignalSpy spy(client, &CFSDClient::rawFsdMessage);

        client->sendLogin();

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 1);
        CRawFsdMessage fsdMessage = arguments.at(0).value<CRawFsdMessage>();
        QCOMPARE(fsdMessage.getRawMessage(), "FSD Sent=>#APABCD:SERVER:1234567:123456:1:100:14:Test User");
    }

    void CTestFSDClient::testSendAtcLogin()
    {
        QSignalSpy spy(client, &CFSDClient::rawFsdMessage);
        client->setLoginMode(CLoginMode::Observer);
        client->setAtcRating(AtcRating::Controller1);
        client->sendLogin();

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 1);
        CRawFsdMessage fsdMessage = arguments.at(0).value<CRawFsdMessage>();
        QCOMPARE(fsdMessage.getRawMessage(), "FSD Sent=>#AAABCD:SERVER:Test User:1234567:123456:5:100");
    }

    void CTestFSDClient::testSendDeletePilot()
    {
        QSignalSpy spy(client, &CFSDClient::rawFsdMessage);

        client->sendDeletePilot();

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 1);
        CRawFsdMessage fsdMessage = arguments.at(0).value<CRawFsdMessage>();
        QCOMPARE(fsdMessage.getRawMessage(), "FSD Sent=>#DPABCD:1234567");
    }

    void CTestFSDClient::testSendDeleteAtc()
    {
        QSignalSpy spy(client, &CFSDClient::rawFsdMessage);

        client->sendDeleteAtc();

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 1);
        CRawFsdMessage fsdMessage = arguments.at(0).value<CRawFsdMessage>();
        QCOMPARE(fsdMessage.getRawMessage(), "FSD Sent=>#DAABCD:1234567");
    }

    void CTestFSDClient::testSendPilotDataUpdate1()
    {
        QSignalSpy spy(client, &CFSDClient::rawFsdMessage);

        const Geo::CCoordinateGeodetic ownPosition(48.353855, 11.786155);
        const CHeading heading(25.0, CAngleUnit::deg());
        const CAngle pitch(0.0, CAngleUnit::deg());
        const CAngle bank(0.0, CAngleUnit::deg());
        const CSpeed gs(0.0, CSpeedUnit::kts());
        CAircraftSituation situation;
        situation.setPosition(ownPosition);
        situation.setAltitude(CAltitude(110, CAltitude::MeanSeaLevel, CLengthUnit::ft()));
        situation.setPressureAltitude(CAltitude(111, CAltitude::MeanSeaLevel, CAltitude::PressureAltitude, CLengthUnit::ft()));
        situation.setGroundSpeed(gs);
        situation.setPitch(pitch);
        situation.setBank(bank);
        situation.setHeading(heading);
        COwnAircraftProviderDummy::instance()->updateOwnSituation(situation);
        COwnAircraftProviderDummy::instance()->updateCockpit({}, {}, CTransponder(1200, CTransponder::ModeC), {});

        client->sendPilotDataUpdate();

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 1);
        CRawFsdMessage fsdMessage = arguments.at(0).value<CRawFsdMessage>();
        QCOMPARE(fsdMessage.getRawMessage(), "FSD Sent=>@N:ABCD:1200:0:48.35386:11.78616:110:0:4294963484:1");
    }

    void CTestFSDClient::testSendPilotDataUpdate2()
    {
        QSignalSpy spy(client, &CFSDClient::rawFsdMessage);

        const Geo::CCoordinateGeodetic ownPosition(48.353855, 11.786155);
        const CHeading heading(25.0, CAngleUnit::deg());
        const CAngle pitch(0.0, CAngleUnit::deg());
        const CAngle bank(0.0, CAngleUnit::deg());
        const CSpeed gs(0.0, CSpeedUnit::kts());
        CAircraftSituation situation;
        situation.setPosition(ownPosition);
        situation.setAltitude(CAltitude(110, CAltitude::MeanSeaLevel, CLengthUnit::ft()));
        situation.setPressureAltitude(CAltitude(111, CAltitude::MeanSeaLevel, CAltitude::PressureAltitude, CLengthUnit::ft()));
        situation.setGroundSpeed(gs);
        situation.setPitch(pitch);
        situation.setBank(bank);
        situation.setHeading(heading);
        COwnAircraftProviderDummy::instance()->updateOwnSituation(situation);
        COwnAircraftProviderDummy::instance()->updateCockpit({}, {}, CTransponder(1200, CTransponder::ModeC), {});

        client->sendPilotDataUpdate();

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 1);
        CRawFsdMessage fsdMessage = arguments.at(0).value<CRawFsdMessage>();
        QCOMPARE(fsdMessage.getRawMessage(), "FSD Sent=>@N:ABCD:1200:0:48.35386:11.78616:110:0:4294963484:1");
    }

    void CTestFSDClient::testSendAtcDataUpdate()
    {
        QSignalSpy spy(client, &CFSDClient::rawFsdMessage);
        client->sendAtcDataUpdate(48.11028, 8.56972);

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 1);
        CRawFsdMessage fsdMessage = arguments.at(0).value<CRawFsdMessage>();
        QCOMPARE(fsdMessage.getRawMessage(), "FSD Sent=>%ABCD:99998:0:10:1:48.11028:8.56972:0");
    }

    void CTestFSDClient::testSendPing()
    {
        QSignalSpy spy(client, &CFSDClient::rawFsdMessage);
        client->sendPing("SERVER");

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 1);
        CRawFsdMessage fsdMessage = arguments.at(0).value<CRawFsdMessage>();
        QVERIFY(fsdMessage.getRawMessage().contains("FSD Sent=>$PIABCD:SERVER:"));
    }

    void CTestFSDClient::testSendPong()
    {
        QSignalSpy spy(client, &CFSDClient::rawFsdMessage);
        client->sendPong("SERVER", "123456789");

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 1);
        CRawFsdMessage fsdMessage = arguments.at(0).value<CRawFsdMessage>();
        QCOMPARE(fsdMessage.getRawMessage(), "FSD Sent=>$POABCD:SERVER:123456789");
    }

    void CTestFSDClient::testSendClientResponse1()
    {
        QSignalSpy spy(client, &CFSDClient::rawFsdMessage);
        client->sendClientResponse(ClientQueryType::RealName, "ZZZZ_TWR");

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 1);
        CRawFsdMessage fsdMessage = arguments.at(0).value<CRawFsdMessage>();
        QCOMPARE(fsdMessage.getRawMessage(), "FSD Sent=>$CRABCD:ZZZZ_TWR:RN:Test User::1");
    }

    void CTestFSDClient::testSendClientResponse2()
    {
        QSignalSpy spy(client, &CFSDClient::rawFsdMessage);
        client->sendClientResponse(ClientQueryType::Capabilities, "ZZZZ_TWR");

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 1);
        CRawFsdMessage fsdMessage = arguments.at(0).value<CRawFsdMessage>();
        QCOMPARE(fsdMessage.getRawMessage(), "FSD Sent=>$CRABCD:ZZZZ_TWR:CAPS:ATCINFO=1:MODELDESC=1:ACCONFIG=1");
    }

    void CTestFSDClient::testSendClientQuery1()
    {
        QSignalSpy spy(client, &CFSDClient::rawFsdMessage);
        client->sendClientQuery(ClientQueryType::RealName, "ZZZZ_TWR");

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 1);
        CRawFsdMessage fsdMessage = arguments.at(0).value<CRawFsdMessage>();
        QCOMPARE(fsdMessage.getRawMessage(), "FSD Sent=>$CQABCD:ZZZZ_TWR:RN");
    }

    void CTestFSDClient::testSendClientQuery2()
    {
        QSignalSpy spy(client, &CFSDClient::rawFsdMessage);
        client->sendClientQueryIsValidAtc("EDDM_TWR");

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 1);
        CRawFsdMessage fsdMessage = arguments.at(0).value<CRawFsdMessage>();
        QCOMPARE(fsdMessage.getRawMessage(), "FSD Sent=>$CQABCD:SERVER:ATC:EDDM_TWR");
    }

    void CTestFSDClient::testSendClientQuery3()
    {
        QSignalSpy spy(client, &CFSDClient::rawFsdMessage);
        client->sendClientQueryAircraftConfig("DLH123");

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 1);
        CRawFsdMessage fsdMessage = arguments.at(0).value<CRawFsdMessage>();
        QCOMPARE(fsdMessage.getRawMessage(), "FSD Sent=>$CQABCD:DLH123:ACC:{\"request\":\"full\"}");
    }

    void CTestFSDClient::testSendTextMessage1()
    {
        QSignalSpy spy(client, &CFSDClient::rawFsdMessage);
        client->sendTextMessage("ZZZZ_TWR", "hey dude!");

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 1);
        CRawFsdMessage fsdMessage = arguments.at(0).value<CRawFsdMessage>();
        QCOMPARE(fsdMessage.getRawMessage(), "FSD Sent=>#TMABCD:ZZZZ_TWR:hey dude!");
    }

    void CTestFSDClient::testSendTextMessage2()
    {
        QSignalSpy spy(client, &CFSDClient::rawFsdMessage);
        client->sendTextMessage(TextMessageGroups::AllSups, "Please help!!!");

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 1);
        CRawFsdMessage fsdMessage = arguments.at(0).value<CRawFsdMessage>();
        QCOMPARE(fsdMessage.getRawMessage(), "FSD Sent=>#TMABCD:*S:Please help!!!");
    }

    void CTestFSDClient::testSendRadioMessage1()
    {
        QSignalSpy spy(client, &CFSDClient::rawFsdMessage);
        QVector<int> frequencies { 124050 };
        client->sendRadioMessage(frequencies, "hey dude!");

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 1);
        CRawFsdMessage fsdMessage = arguments.at(0).value<CRawFsdMessage>();
        QCOMPARE(fsdMessage.getRawMessage(), "FSD Sent=>#TMABCD:@24050:hey dude!");
    }

    void CTestFSDClient::testSendRadioMessage2()
    {
        QSignalSpy spy(client, &CFSDClient::rawFsdMessage);
        QVector<int> frequencies { 124050, 135725 };
        client->sendRadioMessage(frequencies, "hey dude!");

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 1);
        CRawFsdMessage fsdMessage = arguments.at(0).value<CRawFsdMessage>();
        QCOMPARE(fsdMessage.getRawMessage(), "FSD Sent=>#TMABCD:@24050&@35725:hey dude!");
    }

    void CTestFSDClient::testSendFlightPlan()
    {
        QSignalSpy spy(client, &CFSDClient::rawFsdMessage);
        QDateTime takeoffTimePlanned = QDateTime::fromString("1530", "hhmm");
        takeoffTimePlanned.setTimeSpec(Qt::UTC);
        QDateTime takeoffTimeActual = QDateTime::fromString("1535", "hhmm");
        takeoffTimeActual.setTimeSpec(Qt::UTC);
        CAltitude flightLevel(35000, CAltitude::FlightLevel, CLengthUnit::ft());
        CFlightPlan fp({}, "B744", "EGLL", "KORD", "NONE",
                       takeoffTimePlanned, takeoffTimeActual,
                       CTime(8.25, CTimeUnit::h()), CTime(9.5, CTimeUnit::h()),
                       flightLevel, CSpeed(420, CSpeedUnit::kts()), CFlightPlan::VFR,
                       "EGLL.KORD", "Unit Test");
        client->sendFlightPlan(fp);

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 1);
        CRawFsdMessage fsdMessage = arguments.at(0).value<CRawFsdMessage>();
        QCOMPARE(fsdMessage.getRawMessage(), "FSD Sent=>$FPABCD:SERVER:V:B744:420:EGLL:1530:1535:FL350:KORD:8:15:9:30:NONE:UNIT TEST:EGLL.KORD");
    }

    void CTestFSDClient::testSendPlaneInfoRequest()
    {
        QSignalSpy spy(client, &CFSDClient::rawFsdMessage);
        client->sendPlaneInfoRequest("XYZ");

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 1);
        CRawFsdMessage fsdMessage = arguments.at(0).value<CRawFsdMessage>();
        QCOMPARE(fsdMessage.getRawMessage(), "FSD Sent=>#SBABCD:XYZ:PIR");
    }

    void CTestFSDClient::testSendPlaneInformation1()
    {
        QSignalSpy spy(client, &CFSDClient::rawFsdMessage);
        client->sendPlaneInformation("XYZ", "B744", "BAW");

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 1);
        CRawFsdMessage fsdMessage = arguments.at(0).value<CRawFsdMessage>();
        QCOMPARE(fsdMessage.getRawMessage(), "FSD Sent=>#SBABCD:XYZ:PI:GEN:EQUIPMENT=B744:AIRLINE=BAW");
    }

    void CTestFSDClient::testSendPlaneInformation2()
    {
        QSignalSpy spy(client, &CFSDClient::rawFsdMessage);
        client->sendPlaneInformation("XYZ", "B744", "BAW", "UNION");

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 1);
        CRawFsdMessage fsdMessage = arguments.at(0).value<CRawFsdMessage>();
        QCOMPARE(fsdMessage.getRawMessage(), "FSD Sent=>#SBABCD:XYZ:PI:GEN:EQUIPMENT=B744:AIRLINE=BAW:LIVERY=UNION");
    }

    void CTestFSDClient::testSendPlaneInformation3()
    {
        QSignalSpy spy(client, &CFSDClient::rawFsdMessage);
        client->sendPlaneInformation("XYZ", "B744");

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 1);
        CRawFsdMessage fsdMessage = arguments.at(0).value<CRawFsdMessage>();
        QCOMPARE(fsdMessage.getRawMessage(), "FSD Sent=>#SBABCD:XYZ:PI:GEN:EQUIPMENT=B744");
    }

    void CTestFSDClient::testSendPlaneInformation4()
    {
        QSignalSpy spy(client, &CFSDClient::rawFsdMessage);
        client->sendPlaneInformation("XYZ", "", "", "UNION");

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 1);
        CRawFsdMessage fsdMessage = arguments.at(0).value<CRawFsdMessage>();
        QCOMPARE(fsdMessage.getRawMessage(), "FSD Sent=>#SBABCD:XYZ:PI:GEN:LIVERY=UNION");
    }

    void CTestFSDClient::testSendAircraftConfiguration()
    {
        QSignalSpy spy(client, &CFSDClient::rawFsdMessage);
        client->sendAircraftConfiguration("XYZ", "{\"request\":\"full\"}");

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 1);
        CRawFsdMessage fsdMessage = arguments.at(0).value<CRawFsdMessage>();
        QCOMPARE(fsdMessage.getRawMessage(), "FSD Sent=>$CQABCD:XYZ:ACC:{\"request\":\"full\"}");
    }

    void CTestFSDClient::testCom1FreqQueryResponse()
    {
        QSignalSpy spy(client, &CFSDClient::rawFsdMessage);
        client->sendFsdMessage("$CQEDMM_CTR:ABCD:C?\r\n");

        QCOMPARE(spy.count(), 2);
        QList<QVariant> arguments = spy.takeAt(1);
        QCOMPARE(arguments.size(), 1);
        CRawFsdMessage fsdMessage = arguments.at(0).value<CRawFsdMessage>();
        QCOMPARE(fsdMessage.getRawMessage(), "FSD Sent=>$CRABCD:EDMM_CTR:C?:123.000");
    }

    void CTestFSDClient::testPlaneInfoRequestResponse()
    {
        QSignalSpy spy(client, &CFSDClient::rawFsdMessage);
        client->sendFsdMessage("#SBEDMM_CTR:ABCD:PIR\r\n");

        QCOMPARE(spy.count(), 2);
        QList<QVariant> arguments = spy.takeAt(1);
        QCOMPARE(arguments.size(), 1);
        CRawFsdMessage fsdMessage = arguments.at(0).value<CRawFsdMessage>();
        QCOMPARE(fsdMessage.getRawMessage(), "FSD Sent=>#SBABCD:EDMM_CTR:PI:GEN:EQUIPMENT=B737:AIRLINE=BER");
    }

    void CTestFSDClient::testAuth()
    {
        quint16 m_clientId = 0xb9ba;
        QString m_privateKey("727d1efd5cb9f8d2c28372469d922bb4");

        // TODO fix with the test key
//        QString initialChallenge("a054064f45cb6d6a6f1345");

//        vatsim_auth *auth = vatsim_auth_create(m_clientId, qPrintable(m_privateKey));
//        vatsim_auth_set_initial_challenge(auth, qPrintable(initialChallenge));

//        QString challenge("0b8244efa2bd0f6da0");
//        char buffer[33];
//        vatsim_auth_generate_response(auth, qPrintable(challenge), buffer);
//        QString response(buffer);
//        QCOMPARE(response, "df00748db5ec02ea416ab79b441a88f7");

//        challenge = "6d1beed4fa142b9b5567c0";
//        vatsim_auth_generate_response(auth, qPrintable(challenge), buffer);
//        response = QString(buffer);
//        QCOMPARE(response, "5d7e48df0ff0f52b268d4e23d32483c2");

//        vatsim_auth_generate_challenge(auth, buffer);
//        QVERIFY(QString(buffer).length() > 0);

//        char sysuid[50];
//        vatsim_get_system_unique_id(sysuid);
//        qDebug() << sysuid;
    }

    bool pingServer(const CServer &server)
    {
        QString m;
        const CUrl url(server.getAddress(), server.getPort());
        if (!CNetworkUtils::canConnect(url, m, 2500))
        {
            qWarning() << "Skipping unit test as" << url.getFullUrl() << "cannot be connected";
            return false;
        }
        return true;
    }

    void CTestFSDClient::testConnection()
    {
        const CServer fsdServer = CServer::swiftFsdTestServer(true);
        if (!pingServer(fsdServer)) { QSKIP("Server not reachable."); }

        QSignalSpy spy(client, &CFSDClient::connectionStatusChanged);
        client->setUnitTestMode(false);
        client->connectToServer();

        if (spy.isEmpty()) { QVERIFY(spy.wait()); }
        QList<QVariant> arguments = spy.takeAt(0);
        QCOMPARE(arguments.size(), 2);
        QCOMPARE(CConnectionStatus::Disconnected, arguments.at(0).value<CConnectionStatus>().getConnectionStatus());
        QCOMPARE(CConnectionStatus::Connecting, arguments.at(1).value<CConnectionStatus>().getConnectionStatus());

        if (spy.isEmpty()) { QVERIFY(spy.wait()); }
        arguments = arguments = spy.takeAt(0);
        QCOMPARE(arguments.size(), 2);
        QCOMPARE(CConnectionStatus::Connecting, arguments.at(0).value<CConnectionStatus>().getConnectionStatus());
        QCOMPARE(CConnectionStatus::Connected, arguments.at(1).value<CConnectionStatus>().getConnectionStatus());

        QSignalSpy pongSpy(client, &CFSDClient::pongReceived);
        connect(client, &CFSDClient::pongReceived, [](const QString &sender, double elapsedTimeM)
        {
            qDebug() << "Received pong from" << sender << "in" << elapsedTimeM << "ms";
        });
        client->sendPing("SERVER");
        QVERIFY(pongSpy.wait());

        client->disconnectFromServer();
        if (spy.isEmpty()) { QVERIFY(spy.wait()); }
        arguments = spy.takeAt(0);
        QCOMPARE(arguments.size(), 2);
        QCOMPARE(CConnectionStatus::Connected, arguments.at(0).value<CConnectionStatus>().getConnectionStatus());
        QCOMPARE(CConnectionStatus::Disconnecting, arguments.at(1).value<CConnectionStatus>().getConnectionStatus());

        if (spy.isEmpty()) { QVERIFY(spy.wait()); }
        arguments = spy.takeAt(0);
        QCOMPARE(arguments.size(), 2);
        QCOMPARE(CConnectionStatus::Disconnecting, arguments.at(0).value<CConnectionStatus>().getConnectionStatus());
        QCOMPARE(CConnectionStatus::Disconnected, arguments.at(1).value<CConnectionStatus>().getConnectionStatus());
    }
}

//! main
BLACKTEST_MAIN(BlackFsdTest::CTestFSDClient);

#include "testfsdclient.moc"

//! \endcond
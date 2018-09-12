/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackcore/context/contextsimulatorproxy.h"
#include "blackmisc/dbus.h"
#include "blackmisc/dbusserver.h"
#include "blackmisc/genericdbusinterface.h"
#include "blackmisc/simulation/simulatedaircraft.h"

#include <QDBusConnection>
#include <QLatin1String>
#include <QObject>
#include <QtGlobal>

using namespace BlackMisc;
using namespace BlackMisc::PhysicalQuantities;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::Network;
using namespace BlackMisc::Geo;
using namespace BlackMisc::Simulation;

namespace BlackCore
{
    namespace Context
    {
        CContextSimulatorProxy::CContextSimulatorProxy(const QString &serviceName, QDBusConnection &connection, CCoreFacadeConfig::ContextMode mode, CCoreFacade *runtime) : IContextSimulator(mode, runtime), m_dBusInterface(nullptr)
        {
            this->m_dBusInterface = new BlackMisc::CGenericDBusInterface(
                serviceName, IContextSimulator::ObjectPath(), IContextSimulator::InterfaceName(),
                connection, this);
            this->relaySignals(serviceName, connection);
        }

        void CContextSimulatorProxy::unitTestRelaySignals()
        {
            // connect signals, asserts when failures
            QDBusConnection con = QDBusConnection::sessionBus();
            CContextSimulatorProxy c(CDBusServer::coreServiceName(), con, CCoreFacadeConfig::Remote, nullptr);
            Q_UNUSED(c);
        }

        void CContextSimulatorProxy::relaySignals(const QString &serviceName, QDBusConnection &connection)
        {
            bool s = connection.connect(serviceName, IContextSimulator::ObjectPath(), IContextSimulator::InterfaceName(),
                                        "simulatorStatusChanged", this, SIGNAL(simulatorStatusChanged(int)));
            Q_ASSERT(s);
            s = connection.connect(serviceName, IContextSimulator::ObjectPath(), IContextSimulator::InterfaceName(),
                                   "modelSetChanged", this, SIGNAL(modelSetChanged(BlackMisc::Simulation::CSimulatorInfo)));
            Q_ASSERT(s);
            s = connection.connect(serviceName, IContextSimulator::ObjectPath(), IContextSimulator::InterfaceName(),
                                   "ownAircraftModelChanged", this, SIGNAL(ownAircraftModelChanged(BlackMisc::Simulation::CAircraftModel)));
            Q_ASSERT(s);
            s = connection.connect(serviceName, IContextSimulator::ObjectPath(), IContextSimulator::InterfaceName(),
                                   "modelMatchingCompleted", this, SIGNAL(modelMatchingCompleted(BlackMisc::Simulation::CSimulatedAircraft)));
            Q_ASSERT(s);
            s = connection.connect(serviceName, IContextSimulator::ObjectPath(), IContextSimulator::InterfaceName(),
                                   "renderRestrictionsChanged", this, SIGNAL(renderRestrictionsChanged(bool, bool, int, BlackMisc::PhysicalQuantities::CLength)));
            Q_ASSERT(s);
            s = connection.connect(serviceName, IContextSimulator::ObjectPath(), IContextSimulator::InterfaceName(),
                                   "interpolationAndRenderingSetupChanged", this, SIGNAL(interpolationAndRenderingSetupChanged()));
            Q_ASSERT(s);
            s = connection.connect(serviceName, IContextSimulator::ObjectPath(), IContextSimulator::InterfaceName(),
                                   "matchingSetupChanged", this, SIGNAL(matchingSetupChanged()));
            Q_ASSERT(s);
            s = connection.connect(serviceName, IContextSimulator::ObjectPath(), IContextSimulator::InterfaceName(),
                                   "simulatorPluginChanged", this, SIGNAL(simulatorPluginChanged(BlackMisc::Simulation::CSimulatorPluginInfo)));
            Q_ASSERT(s);
            s = connection.connect(serviceName, IContextSimulator::ObjectPath(), IContextSimulator::InterfaceName(),
                                   "airspaceSnapshotHandled", this, SIGNAL(airspaceSnapshotHandled()));
            Q_ASSERT(s);
            s = connection.connect(serviceName, IContextSimulator::ObjectPath(), IContextSimulator::InterfaceName(),
                                   "weatherGridReceived", this, SIGNAL(weatherGridReceived(BlackMisc::Weather::CWeatherGrid, BlackMisc::CIdentifier)));
            Q_ASSERT(s);
            s = connection.connect(serviceName, IContextSimulator::ObjectPath(), IContextSimulator::InterfaceName(),
                                   "addingRemoteModelFailed", this, SIGNAL(addingRemoteModelFailed(BlackMisc::Simulation::CSimulatedAircraft, bool, BlackMisc::CStatusMessage)));
            Q_ASSERT(s);
            s = connection.connect(serviceName, IContextSimulator::ObjectPath(), IContextSimulator::InterfaceName(),
                                   "driverMessages", this, SIGNAL(driverMessages(BlackMisc::CStatusMessageList)));

            Q_ASSERT(s);
            s = connection.connect(serviceName, IContextSimulator::ObjectPath(), IContextSimulator::InterfaceName(),
                                   "requestUiConsoleMessage", this, SIGNAL(requestUiConsoleMessage(QString, bool)));
            Q_ASSERT(s);
            Q_UNUSED(s);
            this->relayBaseClassSignals(serviceName, connection, IContextSimulator::ObjectPath(), IContextSimulator::InterfaceName());
        }

        CSimulatorPluginInfoList CContextSimulatorProxy::getAvailableSimulatorPlugins() const
        {
            return m_dBusInterface->callDBusRet<CSimulatorPluginInfoList>(QLatin1String("getAvailableSimulatorPlugins"));
        }

        int CContextSimulatorProxy::getSimulatorStatus() const
        {
            return m_dBusInterface->callDBusRet<int>(QLatin1String("getSimulatorStatus"));
        }

        CAirportList CContextSimulatorProxy::getAirportsInRange(bool recalculatePosition) const
        {
            return m_dBusInterface->callDBusRet<BlackMisc::Aviation::CAirportList>(QLatin1String("getAirportsInRange"), recalculatePosition);
        }

        CAircraftModelList CContextSimulatorProxy::getModelSet() const
        {
            return m_dBusInterface->callDBusRet<BlackMisc::Simulation::CAircraftModelList>(QLatin1String("getModelSet"));
        }

        CSimulatorInfo CContextSimulatorProxy::simulatorsWithInitializedModelSet() const
        {
            return m_dBusInterface->callDBusRet<BlackMisc::Simulation::CSimulatorInfo>(QLatin1String("simulatorsWithInitializedModelSet"));
        }

        CStatusMessageList CContextSimulatorProxy::verifyPrerequisites() const
        {
            return m_dBusInterface->callDBusRet<BlackMisc::CStatusMessageList>(QLatin1String("verifyPrerequisites"));
        }

        CSimulatorInfo CContextSimulatorProxy::getModelSetLoaderSimulator() const
        {
            return m_dBusInterface->callDBusRet<BlackMisc::Simulation::CSimulatorInfo>(QLatin1String("getModelSetLoaderSimulator"));
        }

        void CContextSimulatorProxy::setModelSetLoaderSimulator(const CSimulatorInfo &simulator)
        {
            m_dBusInterface->callDBus(QLatin1String("setModelSetLoaderSimulator"), simulator);
        }

        QStringList CContextSimulatorProxy::getModelSetStrings() const
        {
            return m_dBusInterface->callDBusRet<QStringList>(QLatin1String("getModelSetStrings"));
        }

        QStringList CContextSimulatorProxy::getModelSetCompleterStrings(bool sorted) const
        {
            return m_dBusInterface->callDBusRet<QStringList>(QLatin1String("getModelSetCompleterStrings"), sorted);
        }

        bool CContextSimulatorProxy::isKnownModel(const QString &modelString) const
        {
            return m_dBusInterface->callDBusRet<bool>(QLatin1String("isKnownModel"), modelString);
        }

        CAircraftModelList CContextSimulatorProxy::getModelSetModelsStartingWith(const QString &modelString) const
        {
            return m_dBusInterface->callDBusRet<BlackMisc::Simulation::CAircraftModelList>(QLatin1String("getModelSetModelsStartingWith"), modelString);
        }

        int CContextSimulatorProxy::getModelSetCount() const
        {
            return m_dBusInterface->callDBusRet<int>(QLatin1String("getModelSetCount"));
        }

        BlackMisc::Simulation::CSimulatorPluginInfo CContextSimulatorProxy::getSimulatorPluginInfo() const
        {
            return m_dBusInterface->callDBusRet<BlackMisc::Simulation::CSimulatorPluginInfo>(QLatin1String("getSimulatorPluginInfo"));
        }

        CSimulatorInternals CContextSimulatorProxy::getSimulatorInternals() const
        {
            return m_dBusInterface->callDBusRet<BlackMisc::Simulation::CSimulatorInternals>(QLatin1String("getSimulatorInternals"));
        }

        bool CContextSimulatorProxy::setTimeSynchronization(bool enable, const CTime &offset)
        {
            return m_dBusInterface->callDBusRet<bool>(QLatin1String("setTimeSynchronization"), enable, offset);
        }

        bool CContextSimulatorProxy::isTimeSynchronized() const
        {
            return m_dBusInterface->callDBusRet<bool>(QLatin1String("isTimeSynchronized"));
        }

        CInterpolationAndRenderingSetupGlobal CContextSimulatorProxy::getInterpolationAndRenderingSetupGlobal() const
        {
            return m_dBusInterface->callDBusRet<CInterpolationAndRenderingSetupGlobal>(QLatin1String("getInterpolationAndRenderingSetupGlobal"));
        }

        CInterpolationSetupList CContextSimulatorProxy::getInterpolationAndRenderingSetupsPerCallsign() const
        {
            return m_dBusInterface->callDBusRet<CInterpolationSetupList>(QLatin1String("getInterpolationAndRenderingSetupsPerCallsign"));
        }

        CInterpolationAndRenderingSetupPerCallsign CContextSimulatorProxy::getInterpolationAndRenderingSetupPerCallsignOrDefault(const CCallsign &callsign) const
        {
            return m_dBusInterface->callDBusRet<CInterpolationAndRenderingSetupPerCallsign>(QLatin1String("getInterpolationAndRenderingSetupsPerCallsign"), callsign);
        }

        bool CContextSimulatorProxy::setInterpolationAndRenderingSetupsPerCallsign(const CInterpolationSetupList &setups, bool ignoreSameAsGlobal)
        {
            return m_dBusInterface->callDBusRet<bool>(QLatin1String("setInterpolationAndRenderingSetupsPerCallsign"), setups, ignoreSameAsGlobal);
        }

        void CContextSimulatorProxy::setInterpolationAndRenderingSetupGlobal(const CInterpolationAndRenderingSetupGlobal &setup)
        {
            m_dBusInterface->callDBus(QLatin1String("setInterpolationAndRenderingSetupGlobal"), setup);
        }

        CTime CContextSimulatorProxy::getTimeSynchronizationOffset() const
        {
            return m_dBusInterface->callDBusRet<BlackMisc::PhysicalQuantities::CTime>(QLatin1String("getTimeSynchronizationOffset"));
        }

        bool CContextSimulatorProxy::startSimulatorPlugin(const BlackMisc::Simulation::CSimulatorPluginInfo &simulatorInfo)
        {
            return m_dBusInterface->callDBusRet<bool>(QLatin1String("startSimulatorPlugin"), simulatorInfo);
        }

        void CContextSimulatorProxy::stopSimulatorPlugin(const BlackMisc::Simulation::CSimulatorPluginInfo &simulatorInfo)
        {
            m_dBusInterface->callDBus(QLatin1String("stopSimulatorPlugin"), simulatorInfo);
        }

        int CContextSimulatorProxy::checkListeners()
        {
            return m_dBusInterface->callDBusRet<int>(QLatin1String("checkListeners"));
        }

        CPixmap CContextSimulatorProxy::iconForModel(const QString &modelString) const
        {
            return m_dBusInterface->callDBusRet<CPixmap>(QLatin1String("iconForModel"), modelString);
        }

        void CContextSimulatorProxy::highlightAircraft(const CSimulatedAircraft &aircraftToHighlight, bool enableHighlight, const CTime &displayTime)
        {
            m_dBusInterface->callDBus(QLatin1String("highlightAircraft"), aircraftToHighlight, enableHighlight, displayTime);
        }

        bool CContextSimulatorProxy::followAircraft(const CCallsign &callsign)
        {
            return m_dBusInterface->callDBusRet<bool>(QLatin1String("followAircraft"), callsign);
        }

        bool CContextSimulatorProxy::resetToModelMatchingAircraft(const CCallsign &callsign)
        {
            return m_dBusInterface->callDBusRet<bool>(QLatin1String("resetToModelMatchingAircraft"), callsign, callsign);
        }

        void CContextSimulatorProxy::setWeatherActivated(bool activated)
        {
            m_dBusInterface->callDBus(QLatin1String("setWeatherActivated"), activated);
        }

        void CContextSimulatorProxy::requestWeatherGrid(const Weather::CWeatherGrid &weatherGrid, const CIdentifier &identifier)
        {
            m_dBusInterface->callDBus(QLatin1String("requestWeatherGrid"), weatherGrid, identifier);
        }

        CStatusMessageList CContextSimulatorProxy::getMatchingMessages(const BlackMisc::Aviation::CCallsign &callsign) const
        {
            return m_dBusInterface->callDBusRet<BlackMisc::CStatusMessageList>(QLatin1String("getMatchingMessages"), callsign);
        }

        bool CContextSimulatorProxy::isMatchingMessagesEnabled() const
        {
            return m_dBusInterface->callDBusRet<bool>(QLatin1String("isMatchingMessagesEnabled"));
        }

        void CContextSimulatorProxy::enableMatchingMessages(bool enabled)
        {
            m_dBusInterface->callDBus(QLatin1String("enableMatchingMessages"), enabled);
        }

        bool CContextSimulatorProxy::parseCommandLine(const QString &commandLine, const CIdentifier &originator)
        {
            return m_dBusInterface->callDBusRet<bool>(QLatin1String("parseCommandLine"), commandLine, originator);
        }

        int CContextSimulatorProxy::doMatchingsAgain()
        {
            return m_dBusInterface->callDBusRet<int>(QLatin1String("doMatchingsAgain"));
        }

        bool CContextSimulatorProxy::doMatchingAgain(const CCallsign &callsign)
        {
            return m_dBusInterface->callDBusRet<bool>(QLatin1String("doMatchingAgain"), callsign);
        }

        CMatchingStatistics CContextSimulatorProxy::getCurrentMatchingStatistics(bool missingOnly) const
        {
            return m_dBusInterface->callDBusRet<CMatchingStatistics>(QLatin1String("getCurrentMatchingStatistics"), missingOnly);
        }

        void CContextSimulatorProxy::setMatchingSetup(const CAircraftMatcherSetup &setup)
        {
            m_dBusInterface->callDBus(QLatin1String("setMatchingSetup"), setup);
        }

        CAircraftMatcherSetup CContextSimulatorProxy::getMatchingSetup() const
        {
            return m_dBusInterface->callDBusRet<CAircraftMatcherSetup>(QLatin1String("getMatchingSetup"));
        }
    } // namespace
} // namespace

/* Copyright (C) 2017
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKSIMPLUGIN_EMULATED_SIMULATOREMULATED_H
#define BLACKSIMPLUGIN_EMULATED_SIMULATOREMULATED_H

#include "blackcore/simulatorcommon.h"
#include "blackmisc/simulation/simulatorplugininfo.h"
#include "blackmisc/simulation/settings/swiftpluginsettings.h"
#include "blackmisc/pq/time.h"
#include "blackmisc/connectionguard.h"
#include "simulatoremulatedmonitordialog.h"

#include <QScopedPointer>

namespace BlackSimPlugin
{
    namespace Emulated
    {
        //! swift simulator implementation
        class CSimulatorEmulated : public BlackCore::CSimulatorCommon
        {
            Q_OBJECT
            friend class CSimulatorEmulatedMonitorDialog; //!< the monitor widget represents the simulator and needs access to internals (i.e. private/protected)

        public:
            //! Constructor, parameters as in \sa BlackCore::ISimulatorFactory::create
            CSimulatorEmulated(
                const BlackMisc::Simulation::CSimulatorPluginInfo &info,
                BlackMisc::Simulation::IOwnAircraftProvider *ownAircraftProvider,
                BlackMisc::Simulation::IRemoteAircraftProvider *remoteAircraftProvider,
                BlackMisc::Weather::IWeatherGridProvider *weatherGridProvider,
                QObject *parent = nullptr);

            //! \copydoc BlackCore::CSimulatorCommon::getSimulatorInfo
            virtual BlackMisc::Simulation::CSimulatorInfo getSimulatorInfo() const override;

            // functions implemented
            virtual bool isTimeSynchronized() const override;
            virtual bool connectTo() override;
            virtual bool disconnectFrom() override;
            virtual bool changeRemoteAircraftModel(const BlackMisc::Simulation::CSimulatedAircraft &aircraft) override;
            virtual bool changeRemoteAircraftEnabled(const BlackMisc::Simulation::CSimulatedAircraft &aircraft) override;
            virtual bool updateOwnSimulatorCockpit(const BlackMisc::Simulation::CSimulatedAircraft &aircraft, const BlackMisc::CIdentifier &originator) override;
            virtual bool updateOwnSimulatorSelcal(const BlackMisc::Aviation::CSelcal &selcal, const BlackMisc::CIdentifier &originator) override;
            virtual void displayStatusMessage(const BlackMisc::CStatusMessage &message) const override;
            virtual void displayTextMessage(const BlackMisc::Network::CTextMessage &message) const override;
            virtual bool setTimeSynchronization(bool enable, const BlackMisc::PhysicalQuantities::CTime &offset) override;
            virtual BlackMisc::PhysicalQuantities::CTime getTimeSynchronizationOffset() const override;
            virtual bool isPhysicallyRenderedAircraft(const BlackMisc::Aviation::CCallsign &callsign) const override;
            virtual BlackMisc::Aviation::CCallsignSet physicallyRenderedAircraft() const override;
            virtual bool setInterpolatorMode(BlackMisc::Simulation::CInterpolatorMulti::Mode mode, const BlackMisc::Aviation::CCallsign &callsign) override;

            // functions just logged
            virtual void highlightAircraft(const BlackMisc::Simulation::CSimulatedAircraft &aircraftToHighlight, bool enableHighlight, const BlackMisc::PhysicalQuantities::CTime &displayTime) override;
            virtual bool logicallyAddRemoteAircraft(const BlackMisc::Simulation::CSimulatedAircraft &remoteAircraft) override;
            virtual bool logicallyRemoveRemoteAircraft(const BlackMisc::Aviation::CCallsign &callsign) override;
            virtual int physicallyRemoveMultipleRemoteAircraft(const BlackMisc::Aviation::CCallsignSet &callsigns) override;

            // functions logged and used
            //! \addtogroup swiftdotcommands
            //! @{
            //! <pre>
            //! .drv show   show swift driver window     BlackSimPlugin::Swift::CSimulatorEmulated
            //! .drv hide   hide swift driver window     BlackSimPlugin::Swift::CSimulatorEmulated
            //! </pre>
            //! @}
            //! \copydoc BlackCore::ISimulator::parseCommandLine
            virtual bool parseCommandLine(const QString &commandLine, const BlackMisc::CIdentifier &originator) override;

            //! Register help
            static void registerHelp();

            //! UI setter
            void setCombinedStatus(bool connected, bool simulating, bool paused);

            //! Internal own aircraft
            //! \remark normally used by corresponding BlackSimPlugin::Emulated::CSimulatorEmulatedMonitorDialog
            const BlackMisc::Simulation::CSimulatedAircraft &getInternalOwnAircraft() const { return m_myAircraft; }

            //! Simulator internal change of COM values
            //! \remark normally used by corresponding BlackSimPlugin::Emulated::CSimulatorEmulatedMonitorDialog
            bool changeInternalCom(const BlackMisc::Simulation::CSimulatedAircraft &aircraft);

            //! Simulator internal change of SELCAL
            //! \remark normally used by corresponding BlackSimPlugin::Emulated::CSimulatorEmulatedMonitorDialog
            bool changeInternalSelcal(const BlackMisc::Aviation::CSelcal &selcal);

            //! Simulator internal change of situation
            //! \remark normally used by corresponding BlackSimPlugin::Emulated::CSimulatorEmulatedMonitorDialog
            bool changeInternalSituation(const BlackMisc::Aviation::CAircraftSituation &situation);

            //! Simulator internal change of parts
            //! \remark normally used by corresponding BlackSimPlugin::Emulated::CSimulatorEmulatedMonitorDialog
            bool changeInternalParts(const BlackMisc::Aviation::CAircraftParts &parts);

        signals:
            //! Internal aircraft changed
            void internalAircraftChanged();

        protected:
            virtual bool isConnected() const override;
            virtual bool isPaused() const override;
            virtual bool isSimulating() const override;
            virtual bool physicallyAddRemoteAircraft(const BlackMisc::Simulation::CSimulatedAircraft &remoteAircraft) override;
            virtual bool physicallyRemoveRemoteAircraft(const BlackMisc::Aviation::CCallsign &callsign) override;

            // just logged
            virtual int physicallyRemoveAllRemoteAircraft() override;

            //! \copydoc BlackCore::CSimulatorCommon::parseDetails
            virtual bool parseDetails(const BlackMisc::CSimpleCommandParser &parser) override;

        private:
            //! Can append log messages?
            bool canLog() const;

            //! Close window
            void closeMonitor();

            //! Settings changed
            void onSettingsChanged();

            //! Values from UI
            void onSimulatorStatusChanged();

            //! Connect own signals for monitoring
            void connectOwnSignals();

            bool m_log = false; //!< from settings
            bool m_paused = false;
            bool m_connected = true;
            bool m_simulating = true;
            bool m_timeSyncronized = false;
            BlackMisc::PhysicalQuantities::CTime m_offsetTime;
            BlackMisc::Simulation::CSimulatedAircraft m_myAircraft; //!< represents own aircraft of simulator
            BlackMisc::Simulation::CSimulatedAircraftList m_renderedAircraft; //!< represents other aircraft of simulator

            QScopedPointer<CSimulatorEmulatedMonitorDialog> m_monitorWidget; //!< parent will be main window, so we need to destroy widget when destroyed
            BlackMisc::CSettingReadOnly<BlackMisc::Simulation::Settings::TSwiftPlugin> m_settings { this, &CSimulatorEmulated::onSettingsChanged };
            BlackMisc::CConnectionGuard m_connectionGuard;
        };

        //! Listener for swift
        class CSimulatorEmulatedListener : public BlackCore::ISimulatorListener
        {
            Q_OBJECT

        public:
            //! Constructor
            CSimulatorEmulatedListener(const BlackMisc::Simulation::CSimulatorPluginInfo &info);

        protected:
            //! \copydoc BlackCore::ISimulatorListener::startImpl
            virtual void startImpl() override;

            //! \copydoc BlackCore::ISimulatorListener::stopImpl
            virtual void stopImpl() override;
        };
    } // ns
} // ns

#endif // guard

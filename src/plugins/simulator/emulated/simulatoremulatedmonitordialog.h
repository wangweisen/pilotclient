/* Copyright (C) 2017
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKSIMPLUGIN_EMULATED_SIMULATOREMULATEDMONITORDIALOG_H
#define BLACKSIMPLUGIN_EMULATED_SIMULATOREMULATEDMONITORDIALOG_H

#include <QDialog>
#include <QScopedPointer>
#include "blackmisc/simulation/simulatedaircraft.h"
#include "blackmisc/statusmessagelist.h"
#include "blackmisc/logcategorylist.h"

namespace Ui { class CSimulatorEmulatedMonitorDialog; }
namespace BlackSimPlugin
{
    namespace Emulated
    {
        class CSimulatorEmulated;

        /**
         * Monitor widget for the pseudo driver
         */
        class CSimulatorEmulatedMonitorDialog : public QDialog
        {
            Q_OBJECT

        public:
            //! Log categories
            static const BlackMisc::CLogCategoryList &getLogCategories();

            //! Ctor
            explicit CSimulatorEmulatedMonitorDialog(CSimulatorEmulated *simulator, QWidget *parent = nullptr);

            //! Dtor
            virtual ~CSimulatorEmulatedMonitorDialog();

            //! \copydoc BlackGui::Components::CLogComponent::appendStatusMessageToList
            void appendStatusMessageToList(const BlackMisc::CStatusMessage &statusMessage);

            //! \copydoc BlackGui::Components::CLogComponent::appendStatusMessagesToList
            void appendStatusMessagesToList(const BlackMisc::CStatusMessageList &statusMessages);

            //! Receiving call to be written in log widget
            void appendReceivingCall(const QString &function, const QString &p1 = {}, const QString &p2 = {}, const QString &p3 = {});

            //! Sending call to be written in log widget
            void appendSendingCall(const QString &function, const QString &p1 = {}, const QString &p2 = {}, const QString &p3 = {});

            //! Display status message
            void displayStatusMessage(const BlackMisc::CStatusMessage &message);

            //! Display text message
            void displayTextMessage(const BlackMisc::Network::CTextMessage &message);

        private:
            static int constexpr MaxLogMessages = 500; //!< desired log message number

            //! Append a function call as status message
            void appendFunctionCall(const QString &function, const QString &p1 = {}, const QString &p2 = {}, const QString &p3 = {});

            //! UI values changed
            void onSimulatorValuesChanged();

            //! Cockpit COM values changed
            void changeComFromUi(const BlackMisc::Simulation::CSimulatedAircraft &aircraft);

            //! SELCAL values changed
            void changeSelcalFromUi(const BlackMisc::Aviation::CSelcal &selcal);

            //! Update aircraft situation
            void changeSituationFromUi();

            //! Change the aircraft parts
            void changePartsFromUi();

            //! UI values
            void setSimulatorUiValues();

            //! Set values from internal aircraft
            void setInteralAircraftUiValues();

            QScopedPointer<Ui::CSimulatorEmulatedMonitorDialog> ui;
            CSimulatorEmulated *m_simulator = nullptr;
        };
    } // ns
} // ns

#endif // guard

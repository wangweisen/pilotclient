/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKSIMPLUGIN_SIMULATOR_FSXFACTORY_H
#define BLACKSIMPLUGIN_SIMULATOR_FSXFACTORY_H

#include "blackcore/simulator.h"
#include "blackmisc/simulation/simulatorplugininfo.h"

#include <simconnect/SimConnect.h>
#include <QObject>
#include <QtPlugin>

namespace BlackSimPlugin
{
    namespace Fsx
    {
        //! Factory implementation to create CSimulatorFsx instances
        class CSimulatorFsxFactory :
            public QObject,
            public BlackCore::ISimulatorFactory
        {
            Q_OBJECT
            Q_PLUGIN_METADATA(IID "org.swift-project.blackcore.simulatorinterface" FILE "simulator_fsx.json")
            Q_INTERFACES(BlackCore::ISimulatorFactory)

        public:
            //! \copydoc BlackCore::ISimulatorFactory::create
            virtual BlackCore::ISimulator *create(const BlackMisc::Simulation::CSimulatorPluginInfo &info,
                                                  BlackMisc::Simulation::IOwnAircraftProvider *ownAircraftProvider,
                                                  BlackMisc::Simulation::IRemoteAircraftProvider *renderedAircraftProvider,
                                                  BlackMisc::IPluginStorageProvider *pluginStorageProvider) override;

            //! \copydoc BlackCore::ISimulatorFactory::getListener
            virtual BlackCore::ISimulatorListener *createListener(const BlackMisc::Simulation::CSimulatorPluginInfo &info, QObject *parent = nullptr) override;
        };

    } // namespace
} // namespace

#endif // guard

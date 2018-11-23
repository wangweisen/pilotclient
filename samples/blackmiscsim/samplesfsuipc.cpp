/* Copyright (C) 2018
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file
//! \ingroup sampleblackmiscsim

#include "plugins/simulator/fscommon/fsuipc.h"
#include "samplesfsuipc.h"
#include "blackmisc/simulation/simulatedaircraft.h"
#include "blackmisc/registermetadata.h"

#include <QTextStream>
#include <QScopedPointer>

using namespace BlackMisc;
using namespace BlackMisc::Simulation;

namespace BlackSample
{
#ifdef Q_OS_WIN
    void CSamplesFsuipc::samplesFsuipc(QTextStream &streamOut)
    {
        using namespace BlackSimPlugin::FsCommon;

        BlackMisc::registerMetadata();
        QScopedPointer<CFsuipc> fsuipc(new CFsuipc());
        streamOut << "FSUIPC initialized" << endl;

        if (fsuipc->open())
        {
            streamOut << "FSUIPC connected" << endl;
        }
        else
        {
            streamOut << "FSUIPC NOT(!) connected" << endl;
            streamOut << "Need FS WideClient?" << endl;
            return;
        }

        CSimulatedAircraft aircraft;
        if (fsuipc->read(aircraft, true, true, true))
        {
            streamOut << "Aircraft read: " << aircraft.toQString(true) << endl;
        }

        fsuipc->close();
    }

#else
    void CSamplesFsuipc::samplesFsuipc(QTextStream &streamOut)
    {
        Q_UNUSED(streamOut);
    }
#endif

} // namespace
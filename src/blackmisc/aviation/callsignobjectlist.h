/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution. No part of swift project, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_AVIATION_CALLSIGNOBJECTLIST_H
#define BLACKMISC_AVIATION_CALLSIGNOBJECTLIST_H

#include "blackmisc/aviation/callsignset.h"
#include "blackmisc/propertyindexvariantmap.h"

#include <QHash>
#include <QMap>

namespace BlackMisc
{
    namespace Aviation
    {
        class CAircraftSituation;
        class CAircraftSituationList;
        class CAtcStation;
        class CAtcStationList;
        class CFlightPlan;
        class CFlightPlanList;
    }

    namespace Simulation
    {
        class CSimulatedAircraft;
        class CSimulatedAircraftList;
        class CAircraftModel;
        class CAircraftModelList;
        class CInterpolationAndRenderingSetupPerCallsign;
        class CInterpolationSetupList;
    }

    namespace Network
    {
        class CClient;
        class CClientList;
        class CUser;
        class CUserList;
    }

    namespace Aviation
    {
        //! List of objects with callsign.
        template<class OBJ, class CONTAINER>
        class ICallsignObjectList
        {
        public:
            //! Contains callsign?
            bool containsCallsign(const CCallsign &callsign) const;

            //! Apply for given callsign
            int applyIfCallsign(const CCallsign &callsign, const CPropertyIndexVariantMap &variantMap, bool skipEqualValues = true);

            //! All callsigns
            BlackMisc::Aviation::CCallsignSet getCallsigns() const;

            //! Get callsign string list
            QStringList getCallsignStrings(bool sorted = false) const;

            //! Get callsigns as strings
            QString getCallsignsAsString(const QString &separator, bool sorted = false) const;

            //! Find 0..n stations by callsign
            CONTAINER findByCallsign(const CCallsign &callsign) const;

            //! Find 0..n aircraft matching any of a set of callsigns
            CONTAINER findByCallsigns(const CCallsignSet &callsigns) const;

            //! Find the first aircraft by callsign, if none return given one
            OBJ findFirstByCallsign(const CCallsign &callsign, const OBJ &ifNotFound = {}) const;

            //! Find the back object by callsign, if none return given one
            OBJ findLastByCallsign(const CCallsign &callsign, const OBJ &ifNotFound = {}) const;

            //! All with given suffix, empty suffixes ignored
            CONTAINER findBySuffix(const QString &suffix) const;

            //! First found index of callsign, otherwise -1
            int firstIndexOfCallsign(const CCallsign &callsign);

            //! Remove all objects with callsign
            int removeByCallsign(const CCallsign &callsign);

            //! Remove all objects with callsigns
            int removeByCallsigns(const CCallsignSet &callsigns);

            //! All suffixes with their respective count
            //! \remark since using QMap sorted by suffix
            QMap<QString, int> getSuffixesAndCount() const;

            //! All suffixes, in the order of the list
            //! \remark first found suffixes first
            QStringList getSuffixes() const;

            //! Split into 0..n containers as per callsign
            QHash<CCallsign, CONTAINER> splitPerCallsign() const;

            //! Replace or add objects by callsign
            int replaceOrAddObjectByCallsign(const OBJ &otherObject);

            //! Replace or add objects by callsign
            int replaceOrAddObjectsByCallsign(const CONTAINER &others);

            //! Incremental update or add object
            int incrementalUpdateOrAdd(const OBJ &objectBeforeChanged, const CPropertyIndexVariantMap &changedValues);

            //! Sort by callsign
            void sortByCallsign();

            //! Turn into callsign map
            QMap<CCallsign, OBJ> asCallsignMap() const;

            //! Turn into callsign hash
            QHash<CCallsign, OBJ> asCallsignHash() const;

            //! Copy of list sorted by callsign
            CONTAINER sortedByCallsign() const;

        protected:
            //! Constructor
            ICallsignObjectList();

            //! Container
            const CONTAINER &container() const;

            //! Container
            CONTAINER &container();
        };

        //! \cond PRIVATE
        extern template class BLACKMISC_EXPORT_DECLARE_TEMPLATE ICallsignObjectList<BlackMisc::Aviation::CAtcStation, BlackMisc::Aviation::CAtcStationList>;
        extern template class BLACKMISC_EXPORT_DECLARE_TEMPLATE ICallsignObjectList<BlackMisc::Aviation::CAircraftSituation, BlackMisc::Aviation::CAircraftSituationList>;
        extern template class BLACKMISC_EXPORT_DECLARE_TEMPLATE ICallsignObjectList<BlackMisc::Aviation::CFlightPlan, BlackMisc::Aviation::CFlightPlanList>;
        extern template class BLACKMISC_EXPORT_DECLARE_TEMPLATE ICallsignObjectList<BlackMisc::Simulation::CAircraftModel, BlackMisc::Simulation::CAircraftModelList>;
        extern template class BLACKMISC_EXPORT_DECLARE_TEMPLATE ICallsignObjectList<BlackMisc::Simulation::CSimulatedAircraft, BlackMisc::Simulation::CSimulatedAircraftList>;
        extern template class BLACKMISC_EXPORT_DECLARE_TEMPLATE ICallsignObjectList<BlackMisc::Simulation::CInterpolationAndRenderingSetupPerCallsign, BlackMisc::Simulation::CInterpolationSetupList>;
        extern template class BLACKMISC_EXPORT_DECLARE_TEMPLATE ICallsignObjectList<BlackMisc::Network::CClient, BlackMisc::Network::CClientList>;
        extern template class BLACKMISC_EXPORT_DECLARE_TEMPLATE ICallsignObjectList<BlackMisc::Network::CUser, BlackMisc::Network::CUserList>;
        //! \endcond

    } //namespace
} // namespace

#endif //guard

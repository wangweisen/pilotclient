/* Copyright (C) 2013 VATSIM Community / authors
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/*!
    \file
*/

#ifndef BLACKMISC_ATCSTATIONLIST_H
#define BLACKMISC_ATCSTATIONLIST_H

#include "avatcstation.h"
#include "collection.h"
#include "sequence.h"
#include <QObject>
#include <QString>
#include <QList>

namespace BlackMisc
{
    namespace Aviation
    {
        /*!
         * Value object for a list of ATC stations.
         */
        class CAtcStationList : public CSequence<CAtcStation>
        {
        public:
            //! Default constructor.
            CAtcStationList();

            //! Construct from a base class object.
            CAtcStationList(const CSequence<CAtcStation> &other);

            //! \copydoc CValueObject::toQVariant()
            virtual QVariant toQVariant() const
            {
                return QVariant::fromValue(*this);
            }

            //! Find 0..n stations by callsign
            CAtcStationList findByCallsign(const CCallsign &callsign) const;

            //! Find 0..n stations within range of given coordinate
            CAtcStationList findWithinRange(const BlackMisc::Geo::ICoordinateGeodetic &coordinate, const BlackMisc::PhysicalQuantities::CLength &range) const;

            //! Find 0..n stations tune in frequency of COM unit (with 25kHt channel spacing
            CAtcStationList findIfComUnitTunedIn25KHz(const BlackMisc::Aviation::CComSystem &comUnit) const;

            //! All controllers (with valid data)
            //! Update distances to coordinate, usually own aircraft's position
            void calculateDistancesToPlane(const BlackMisc::Geo::CCoordinateGeodetic &position);

            //! Register metadata
            static void registerMetadata();

            //! Merge with ATC station representing booking information
            //! \remarks Can be used if the stored data in this list are online ATC stations
            int mergeWithBooking(CAtcStation &bookedAtcStation);

            //! Merge with the data from the VATSIM data file
            //! \remarks Can be used if the stored data in this list are VATSIM data file stations
            int updateFromVatsimDataFileStation(CAtcStation &stationToBeUpdated) const;
        };

    } //namespace
} // namespace

Q_DECLARE_METATYPE(BlackMisc::Aviation::CAtcStationList)
Q_DECLARE_METATYPE(BlackMisc::CCollection<BlackMisc::Aviation::CAtcStation>)
Q_DECLARE_METATYPE(BlackMisc::CSequence<BlackMisc::Aviation::CAtcStation>)

#endif //guard

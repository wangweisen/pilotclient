/* Copyright (C) 2015
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackmisc/timestampobjectlist.h"
#include "blackmisc/predicates.h"
#include "blackmisc/avaircraftsituationlist.h"
#include "blackmisc/aviation/aircraftpartslist.h"
#include <algorithm>
#include <iterator>

namespace BlackMisc
{
    template <class OBJ, class CONTAINER>
    ITimestampObjectList<OBJ, CONTAINER>::ITimestampObjectList()
    { }

    template <class OBJ, class CONTAINER>
    CONTAINER ITimestampObjectList<OBJ, CONTAINER>::findBefore(qint64 msSinceEpoch) const
    {
        return this->container().findBy([&](const OBJ & obj)
        {
            return obj.isOlderThan(msSinceEpoch);
        });
    }

    template <class OBJ, class CONTAINER>
    CONTAINER ITimestampObjectList<OBJ, CONTAINER>::findBeforeNowMinusOffset(qint64 msOffset) const
    {
        return this->findBefore(QDateTime::currentMSecsSinceEpoch() - msOffset);
    }

    template <class OBJ, class CONTAINER>
    CONTAINER ITimestampObjectList<OBJ, CONTAINER>::findBefore(const QDateTime &dateTime) const
    {
        return this->findBefore(dateTime.toMSecsSinceEpoch());
    }

    template <class OBJ, class CONTAINER>
    CONTAINER ITimestampObjectList<OBJ, CONTAINER>::findAfter(qint64 msSinceEpoc) const
    {
        return this->container().findBy([&](const OBJ & obj)
        {
            return obj.isNewerThan(msSinceEpoc);
        });
    }

    template <class OBJ, class CONTAINER>
    QList<CONTAINER> ITimestampObjectList<OBJ, CONTAINER>::splitByTime(qint64 msSinceEpoch) const
    {
        CONTAINER newer(this->container());
        newer.sortLatestFirst();
        CONTAINER older;
        for (auto it = newer.begin(); it != newer.end(); ++it)
        {
            if (it->isOlderThan(msSinceEpoch))
            {
                // better "move", ?? std::make_move_iterator
                older.insert(CRange<CONTAINER::iterator>(it, newer.end()));
                newer.erase(it, newer.end());
                break;
            }
        }

        // before / after
        return QList<CONTAINER>({newer, older});
    }

    template <class OBJ, class CONTAINER>
    OBJ ITimestampObjectList<OBJ, CONTAINER>::latestValue() const
    {
        if (this->container().isEmpty()) { return OBJ(); }
        CONTAINER container(container()); // copy
        container.sortLatestFirst();
        return container.front();
    }

    template <class OBJ, class CONTAINER>
    OBJ ITimestampObjectList<OBJ, CONTAINER>::oldestValue() const
    {
        if (this->container().isEmpty()) { return OBJ(); }
        CONTAINER container(container()); // copy
        container.sortLatestFirst();
        return container.back();
    }

    template <class OBJ, class CONTAINER>
    CONTAINER ITimestampObjectList<OBJ, CONTAINER>::findAfter(const QDateTime &dateTime) const
    {
        return this->findAfter(dateTime.toMSecsSinceEpoch());
    }

    template <class OBJ, class CONTAINER>
    void ITimestampObjectList<OBJ, CONTAINER>::removeBefore(const QDateTime &dateTime)
    {
        this->removeBefore(dateTime.toMSecsSinceEpoch());
    }

    template <class OBJ, class CONTAINER>
    void ITimestampObjectList<OBJ, CONTAINER>::removeBefore(qint64 msSinceEpoc)
    {
        this->container().removeIf([&](const OBJ & obj)
        {
            return obj.isOlderThan(msSinceEpoc);
        });
    }

    template <class OBJ, class CONTAINER>
    void ITimestampObjectList<OBJ, CONTAINER>::removeOlderThanNowMinusOffset(int offsetMs)
    {
        const qint64 epoch = QDateTime::currentMSecsSinceEpoch() - offsetMs;
        this->container().removeIf([&](const OBJ & obj)
        {
            return obj.isOlderThan(epoch);
        });
    }

    template <class OBJ, class CONTAINER>
    void ITimestampObjectList<OBJ, CONTAINER>::sortLatestFirst()
    {
        this->container().sortOldestFirst();
        std::reverse(this->container().begin(), this->container().end());
    }

    template <class OBJ, class CONTAINER>
    void ITimestampObjectList<OBJ, CONTAINER>::sortOldestFirst()
    {
        this->container().sort(BlackMisc::Predicates::MemberLess(&OBJ::getMSecsSinceEpoch));
    }

    template <class OBJ, class CONTAINER>
    void ITimestampObjectList<OBJ, CONTAINER>::insertTimestampObject(const OBJ &object, int maxElements)
    {
        Q_ASSERT(maxElements > 1);
        if (this->container().size() >= (maxElements - 1))
        {
            this->container().truncate(maxElements - 1);
        }
        this->container().insert_front(object);
    }

    // see here for the reason of thess forward instantiations
    // http://www.parashift.com/c++-faq/separate-template-class-defn-from-decl.html
    template class ITimestampObjectList<BlackMisc::Aviation::CAircraftSituation, BlackMisc::Aviation::CAircraftSituationList>;
    template class ITimestampObjectList<BlackMisc::Aviation::CAircraftParts, BlackMisc::Aviation::CAircraftPartsList>;

} // namespace

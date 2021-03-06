/* Copyright (C) 2019
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution. No part of swift project, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 */

#include "simbriefdata.h"
#include "blackmisc/logcategorylist.h"
#include <QStringBuilder>

using namespace BlackMisc::Network;

namespace BlackMisc
{
    namespace Aviation
    {
        const CLogCategoryList &CSimBriefData::getLogCategories()
        {
            static const CLogCategoryList cats { CLogCategory::flightPlan() };
            return cats;
        }

        CSimBriefData::CSimBriefData() : m_url("https://www.simbrief.com/api/xml.fetcher.php")
        { }

        CSimBriefData::CSimBriefData(const QString &url, const QString &username) :
            m_url(url), m_username(username)
        { }

        CUrl CSimBriefData::getUrlAndUsername() const
        {
            CUrl url(this->getUrl());
            if (!m_username.isEmpty()) { url.setQuery("username=" % m_username); }
            return url;
        }

        CVariant CSimBriefData::propertyByIndex(const CPropertyIndex &index) const
        {
            if (index.isMyself()) { return CVariant::from(*this); }
            const ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexUrl:      return CVariant::from(m_url);
            case IndexUsername: return CVariant::from(m_username);
            default: return CValueObject::propertyByIndex(index);
            }
        }

        void CSimBriefData::setPropertyByIndex(const CPropertyIndex &index, const CVariant &variant)
        {
            if (index.isMyself()) { (*this) = variant.to<CSimBriefData>(); return; }
            const ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexUrl:      m_url = variant.toQString(); break;
            case IndexUsername: m_username = variant.toQString(); break;
            default: CValueObject::setPropertyByIndex(index, variant); break;
            }
        }

        QString CSimBriefData::convertToQString(bool i18n) const
        {
            Q_UNUSED(i18n);
            return m_username % " " % m_url;
        }
    } // namespace
} // namespace

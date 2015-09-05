/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "settingscache.h"
#include <QStandardPaths>

namespace BlackCore
{

    CSettingsCache::CSettingsCache() :
        CValueCache(CValueCache::Distributed)
    {}

    CSettingsCache *CSettingsCache::instance()
    {
        static CSettingsCache cache;
        return &cache;
    }

    const QString &CSettingsCache::persistentStore()
    {
        static const QString dir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/org.swift-project/settings/core";
        return dir;
    }

    BlackMisc::CStatusMessage CSettingsCache::saveToStore(const QString &keyPrefix) const
    {
        return saveToFiles(persistentStore(), keyPrefix);
    }

    BlackMisc::CStatusMessage CSettingsCache::loadFromStore()
    {
        return loadFromFiles(persistentStore());
    }

}

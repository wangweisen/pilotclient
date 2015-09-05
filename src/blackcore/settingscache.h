/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKCORE_SETTINGSCACHE_H
#define BLACKCORE_SETTINGSCACHE_H

#include "blackcore/blackcoreexport.h"
#include "blackmisc/valuecache.h"

namespace BlackCore
{

    /*!
     * Singleton derived class of CValueCache, for core settings.
     */
    class BLACKCORE_EXPORT CSettingsCache : public BlackMisc::CValueCache
    {
    public:
        //! Return the singleton instance.
        static CSettingsCache *instance();

        //! The directory where core settings are stored.
        static const QString &persistentStore();

        //! Save core settings to disk.
        BlackMisc::CStatusMessage saveToStore(const QString &keyPrefix = {}) const;

        //! Load core settings from disk.
        BlackMisc::CStatusMessage loadFromStore();

    private:
        CSettingsCache();
    };

    /*!
     * Class template for accessing a specific value in the CSettingsCache.
     * \tparam Trait A subclass of BlackCore::CSettingTrait that identifies the value's key and other metadata.
     */
    template <typename Trait>
    class CSetting : public BlackMisc::CCached<typename Trait::type>
    {
    public:
        //! \copydoc BlackMisc::CCached::NotifySlot
        template <typename T>
        using NotifySlot = typename BlackMisc::CCached<typename Trait::type>::template NotifySlot<T>;

        //! Constructor.
        //! \param owner Will be the parent of the internal QObject used to access the value.
        //! \param slot Slot to call when the value is modified by another object.
        //!             Must be a void, non-const member function of the owner.
        template <typename T>
        CSetting(T *owner, NotifySlot<T> slot = nullptr) :
            CSetting::CCached(CSettingsCache::instance(), Trait::key(), Trait::isValid, Trait::defaultValue(), owner, slot)
        {}

        //! Reset the setting to its default value.
        void setDefault() { this->set(Trait::defaultValue()); }
    };

    /*!
     * Base class for traits to be used as template argument to BlackCore::CSetting.
     */
    template <typename T>
    struct CSettingTrait
    {
        //! Data type of the value.
        using type = T;

        //! Key string of the value. Reimplemented in derived class.
        static const char *key() { qFatal("Not implemented"); return ""; }

        //! Validator function. Return true if the argument is valid, false otherwise. Default
        //! implementation just returns true. Reimplemented in derived class to support validation of the value.
        static bool isValid(const T &) { return true; }

        //! Return the value to use in case the supplied value does not satisfy the validator.
        //! Default implementation returns a default-constructed value.
        static const T &defaultValue() { static const T def {}; return def; }

        //! Deleted default constructor.
        CSettingTrait() = delete;

        //! Deleted copy constructor.
        CSettingTrait(const CSettingTrait &) = delete;

        //! Deleted copy assignment operator.
        CSettingTrait &operator =(const CSettingTrait &) = delete;
    };

}

#endif

/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution. No part of swift project, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 */

#include "registermetadataaudio.h"
#include "audio.h"
#include "ptt.h"
#include "blackmisc/variant.h"
#include <QDBusMetaType>

namespace BlackMisc
{
    namespace Audio
    {
        //! Register metadata for audio / voice
        void registerMetadata()
        {
            CAudioDeviceInfo::registerMetadata();
            CAudioDeviceInfoList::registerMetadata();
            CSettings::registerMetadata();
            CVoiceSetup::registerMetadata();

            // ENUMs
            qDBusRegisterMetaType<PTTCOM>();
            qDBusRegisterMetaType<CNotificationSounds::PlayMode>();
            qDBusRegisterMetaType<CNotificationSounds::NotificationFlag>();
            qDBusRegisterMetaType<CAudioDeviceInfo::DeviceType>();
            qRegisterMetaTypeStreamOperators<CNotificationSounds::PlayMode>();
            qRegisterMetaTypeStreamOperators<CNotificationSounds::NotificationFlag>();
        }
    } // ns
} // ns

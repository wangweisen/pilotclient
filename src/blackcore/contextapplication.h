/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKCORE_CONTEXTAPPLICATION_H
#define BLACKCORE_CONTEXTAPPLICATION_H

#include "blackcore/blackcoreexport.h"
#include "blackcore/context.h"
#include "blackcore/corefacadeconfig.h"
#include "blackmisc/audio/voiceroomlist.h"
#include "blackmisc/compare.h"
#include "blackmisc/dictionary.h"
#include "blackmisc/identifier.h"
#include "blackmisc/identifierlist.h"
#include "blackmisc/logpattern.h"
#include "blackmisc/statusmessage.h"
#include "blackmisc/valuecache.h"

#include <QDBusArgument>
#include <QHash>
#include <QList>
#include <QMetaType>
#include <QObject>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QtGlobal>

class QDBusConnection;

namespace BlackMisc { class CDBusServer; }

//! \addtogroup dbus
//! @{

//! DBus interface for context
#define BLACKCORE_CONTEXTAPPLICATION_INTERFACENAME "org.swift_project.blackcore.contextapplication"

//! DBus object path for context
#define BLACKCORE_CONTEXTAPPLICATION_OBJECTPATH "/application"

//! @}

namespace BlackCore
{
    class CCoreFacade;
    class CInputManager;

    //! Used by application context to track which processes are subscribed to which patterns of log message
    using CLogSubscriptionHash = QHash<BlackMisc::CIdentifier, QList<BlackMisc::CLogPattern>>;

    //! Used when marshalling CLogSubscriptionHash, as a QHash with CIdentifier keys can't be marshalled
    using CLogSubscriptionPair = QPair<BlackMisc::CIdentifier, QList<BlackMisc::CLogPattern>>;

    //! Application context interface
    class BLACKCORE_EXPORT IContextApplication : public CContext
    {
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", BLACKCORE_CONTEXTAPPLICATION_INTERFACENAME)

    protected:
        //! Constructor
        IContextApplication(CCoreFacadeConfig::ContextMode mode, CCoreFacade *runtime);

    public:
        //! Service name
        static const QString &InterfaceName()
        {
            static QString s(BLACKCORE_CONTEXTAPPLICATION_INTERFACENAME);
            return s;
        }

        //! Service path
        static const QString &ObjectPath()
        {
            static QString s(BLACKCORE_CONTEXTAPPLICATION_OBJECTPATH);
            return s;
        }

        //! \copydoc CContext::getPathAndContextId()
        virtual QString getPathAndContextId() const { return this->buildPathAndContextId(ObjectPath()); }

        //! Factory method
        static IContextApplication *create(CCoreFacade *parent, CCoreFacadeConfig::ContextMode mode, BlackMisc::CDBusServer *server, QDBusConnection &conn);

        //! Destructor
        virtual ~IContextApplication() {}

    signals:
        //! A component changes
        void registrationChanged();

        //! A log message was logged
        //! \note Used with CLogMessage, do not use directly
        void messageLogged(const BlackMisc::CStatusMessage &message, const BlackMisc::CIdentifier &origin);

        //! A process subscribed to a particular pattern of log messages
        //! \note Used with CLogMessage, do not use directly
        void logSubscriptionAdded(const BlackMisc::CIdentifier &subscriber, const BlackMisc::CLogPattern &pattern);

        //! A process unsubscribed from a particular pattern of log messages
        //! \note Used with CLogMessage, do not use directly
        void logSubscriptionRemoved(const BlackMisc::CIdentifier &subscriber, const BlackMisc::CLogPattern &pattern);

        //! One or more settings were changed
        //! \note Used for cache relay, do not use directly
        void settingsChanged(const BlackMisc::CValueCachePacket &settings, const BlackMisc::CIdentifier &origin);

        //! New action was registered
        //! \note Used to register hotkey action, do not use directly
        void hotkeyActionsRegistered(const QStringList &actions, const BlackMisc::CIdentifier &origin);

        //! Call a hotkey action on a remote process
        //! \note Used for hotkey action, do not use directly
        void remoteHotkeyAction(const QString &action, bool argument, const BlackMisc::CIdentifier &origin);

        //! Work around for audio context, #382
        void fakedSetComVoiceRoom(const BlackMisc::Audio::CVoiceRoomList &requestedRooms);

    protected:
        //! Compute which process' subscriptions match a given log message.
        BlackMisc::CIdentifierList subscribersOf(const BlackMisc::CStatusMessage &message) const;

        //! Tracks which processes are subscribed to which patterns of log messages.
        CLogSubscriptionHash m_logSubscriptions;

    public slots:
        //! Log a log message
        //! \note Not pure because it can be called from the base class constructor.
        //! \note this is the function which relays CLogMessage via DBus
        virtual void logMessage(const BlackMisc::CStatusMessage &message, const BlackMisc::CIdentifier &origin) { Q_UNUSED(message); Q_UNUSED(origin); }

        //! Subscribe a process to a particular pattern of log messages
        //! \note This is the function which relays subscription changes via DBus
        virtual void addLogSubscription(const BlackMisc::CIdentifier &subscriber, const BlackMisc::CLogPattern &pattern) = 0;

        //! Unsubscribe a process from a particular pattern of log messages
        //! \note This is the function which relays subscription changes via DBus
        virtual void removeLogSubscription(const BlackMisc::CIdentifier &subscriber, const BlackMisc::CLogPattern &pattern) = 0;

        //! Returns hash identifying which processes are subscribed to which patterns of log message
        virtual BlackCore::CLogSubscriptionHash getAllLogSubscriptions() const = 0;

        //! Update log subscriptions hash from core
        virtual void synchronizeLogSubscriptions() = 0;

        //! Ratify some settings changed by another process
        //! \note Not pure because it can be called from the base class constructor.
        //! \note This is the function which relays cache changes via DBus.
        virtual void changeSettings(const BlackMisc::CValueCachePacket &settings, const BlackMisc::CIdentifier &origin);

        //! Get all settings currently in core settings cache
        virtual BlackMisc::CValueCachePacket getAllSettings() const = 0;

        //! Get keys of all unsaved settings currently in core settings cache
        virtual QStringList getUnsavedSettingsKeys() const = 0;

        //! Update local settings with settings from core
        virtual void synchronizeLocalSettings() = 0;

        //! Save core settings to disk
        virtual BlackMisc::CStatusMessage saveSettings(const QString &keyPrefix = {}) = 0;

        //! Save core settings to disk
        virtual BlackMisc::CStatusMessage saveSettingsByKey(const QStringList &keys) = 0;

        //! Load core settings from disk
        virtual BlackMisc::CStatusMessage loadSettings() = 0;

        //! Register hotkey action implemented by another process
        //! \note Not pure because it can be called from the base class constructor.
        //! \note This is the function which relays action registrations via DBus
        virtual void registerHotkeyActions(const QStringList &actions, const BlackMisc::CIdentifier &origin);

        //! Call a hotkey action on a remote process
        //! \note Not pure because it can be called from the base class constructor.
        //! \note This is the function which relays action calls via DBus
        virtual void callHotkeyAction(const QString &action, bool argument, const BlackMisc::CIdentifier &origin);

        //! Register application, can also be used for ping
        virtual BlackMisc::CIdentifier registerApplication(const BlackMisc::CIdentifier &application) = 0;

        //! Unregister application
        virtual void unregisterApplication(const BlackMisc::CIdentifier &application) = 0;

        //! All registered applications
        virtual BlackMisc::CIdentifierList getRegisteredApplications() const = 0;

        //! Remote enabled version of writing a text file
        virtual bool writeToFile(const QString &fileName, const QString &content) = 0;

        //!  Remote enabled version of reading a text file
        virtual QString readFromFile(const QString &fileName) const = 0;

        //!  Remote enabled version of deleting a file
        virtual bool removeFile(const QString &fileName) = 0;

        //!  Remote enabled version of file exists
        virtual bool existsFile(const QString &fileName) const = 0;

        //! Change settings
        //! \todo Remove with old settings
        void changeSettings(uint typeValue);

    };
}

//! DBus marshalling for CLogSubscriptionHash, needed because QtDBus can't marshal a QHash with CIdentifier keys.
QDBusArgument &operator <<(QDBusArgument &arg, const BlackCore::CLogSubscriptionHash &);

//! DBus unmarshalling for CLogSubscriptionHash, needed because QtDBus can't marshal a QHash with CIdentifier keys.
const QDBusArgument &operator >>(const QDBusArgument &arg, BlackCore::CLogSubscriptionHash &);

Q_DECLARE_METATYPE(BlackCore::CLogSubscriptionHash)
Q_DECLARE_METATYPE(BlackCore::CLogSubscriptionPair)

#endif // guard

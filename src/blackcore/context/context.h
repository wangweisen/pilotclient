/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKCORE_CONTEXT_CONTEXT_H
#define BLACKCORE_CONTEXT_CONTEXT_H

#include "blackcore/blackcoreexport.h"
#include "blackcore/corefacade.h"
#include "blackcore/corefacadeconfig.h"
#include "blackmisc/logcategory.h"
#include "blackmisc/logmessage.h"
#include "blackmisc/statusmessage.h"

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QtGlobal>

namespace BlackMisc { class CLogCategoryList; }
namespace BlackCore
{
    namespace Context
    {
        class IContextApplication;
        class IContextAudio;
        class IContextNetwork;
        class IContextOwnAircraft;
        class IContextSimulator;

        //! Base for all context classes
        class BLACKCORE_EXPORT CContext : public QObject
        {
        public:
            //! Destructor
            virtual ~CContext() {}

            //! Log categories
            static const BlackMisc::CLogCategoryList &getLogCategories();

            //! Using local implementing object?
            bool isUsingImplementingObject() const
            {
                return m_mode == CCoreFacadeConfig::Local || m_mode == CCoreFacadeConfig::LocalInDbusServer;
            }

            //! Local or remote object?
            bool isLocalObject() const
            {
                return isUsingImplementingObject() || m_mode == CCoreFacadeConfig::NotUsed;
            }

            //! Empty object?
            bool isEmptyObject() const
            {
                return m_mode == CCoreFacadeConfig::NotUsed;
            }

            //! Runtime
            CCoreFacade *getRuntime()
            {
                Q_ASSERT(this->parent());
                return static_cast<CCoreFacade *>(this->parent());
            }

            //! Const runtime
            const CCoreFacade *getRuntime() const
            {
                Q_ASSERT(this->parent());
                return static_cast<CCoreFacade *>(this->parent());
            }

            //! Mode
            CCoreFacadeConfig::ContextMode getMode() const { return this->m_mode; }

            //! Unique id
            qint64 getUniqueId() const { return this->m_contextId; }

            //
            // cross context access
            //

            //! Context for application
            const IContextApplication *getIContextApplication() const;

            //! Application
            IContextApplication *getIContextApplication();

            //! Context for network
            IContextAudio *getIContextAudio();

            //! Context for network
            const IContextAudio *getIContextAudio() const;

            //! Context for network
            IContextNetwork *getIContextNetwork();

            //! Context for network
            const IContextNetwork *getIContextNetwork() const;

            //! Context for own aircraft
            IContextOwnAircraft *getIContextOwnAircraft();

            //! Context for own aircraft
            const IContextOwnAircraft *getIContextOwnAircraft() const;

            //! Context for simulator
            const IContextSimulator *getIContextSimulator() const;

            //! Simulator
            IContextSimulator *getIContextSimulator();

            //! Set debug flag
            void setDebugEnabled(bool debug);

            //! Debug enabled?
            bool isDebugEnabled() const;

            //! Id and path name for round trip protection
            virtual QString getPathAndContextId() const = 0;

        protected:
            CCoreFacadeConfig::ContextMode m_mode; //!< How context is used
            qint64 m_contextId;                    //!< unique identifer, avoid redirection rountrips
            bool m_debugEnabled = false;           //!< debug messages enabled

            //! Constructor
            CContext(CCoreFacadeConfig::ContextMode mode, QObject *parent) :
                QObject(parent), m_mode(mode), m_contextId(QDateTime::currentMSecsSinceEpoch())
            {}

            //! Path and context id
            QString buildPathAndContextId(const QString &path) const
            {
                return QString(path) + ":" + QString::number(this->getUniqueId());
            }

            //! Empty context called
            void logEmptyContextWarning(const QString &functionName) const
            {
                BlackMisc::CLogMessage(this, BlackMisc::CLogCategory::contextSlot()).warning("Empty context called, details: %1") << functionName;
            }

            //! Standard message when status message is returned in empty context
            static const BlackMisc::CStatusMessage &statusMessageEmptyContext();
        };
    } // ns
} // ns
#endif // guard

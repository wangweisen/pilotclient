/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution. No part of swift project, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 */

#include "blackconfig/buildconfig.h"
#include "blackmisc/simulation/simulatorplugininfo.h"
#include <QJsonValue>
#include <QtGlobal>

using namespace BlackConfig;
using namespace BlackMisc;

namespace BlackMisc
{
    namespace Simulation
    {
        CSimulatorPluginInfo::CSimulatorPluginInfo(const QString &identifier, const QString &name, const QString &simulator, const QString &description, bool valid) :
            m_identifier(identifier), m_name(name), m_simulator(simulator), m_description(description), m_info(simulator), m_valid(valid)
        {
            Q_ASSERT_X(m_info.isSingleSimulator(), Q_FUNC_INFO, "need single simulator");
        }

        void CSimulatorPluginInfo::convertFromJson(const QJsonObject &json)
        {
            if (json.contains("IID")) // comes from the plugin
            {
                if (! json.contains("MetaData")) { throw CJsonException("Missing 'MetaData'"); }

                // json data is already validated by CPluginManagerSimulator
                CJsonScope scope("MetaData");
                Q_UNUSED(scope)
                CValueObject::convertFromJson(json["MetaData"].toObject());
                m_valid = true;
            }
            else
            {
                CValueObject::convertFromJson(json);
            }

            // set info if it wasn't set already
            if (m_info.isNoSimulator() && !m_simulator.isEmpty())
            {
                if (!this->isEmulatedPlugin())
                {
                    m_info = CSimulatorInfo(m_simulator);
                }
            }
        }

        bool CSimulatorPluginInfo::isUnspecified() const
        {
            return m_identifier.isEmpty();
        }

        bool CSimulatorPluginInfo::isEmulatedPlugin() const
        {
            return this->getIdentifier() == emulatedPluginIdentifier();
        }

        QString CSimulatorPluginInfo::convertToQString(bool i18n) const
        {
            Q_UNUSED(i18n)
            return QStringLiteral("%1 (%2)").arg(m_name, m_identifier);
        }

        const QString &CSimulatorPluginInfo::fsxPluginIdentifier()
        {
            static const QString s("org.swift-project.plugins.simulator.fsx");
            return s;
        }

        const QString &CSimulatorPluginInfo::p3dPluginIdentifier()
        {
            static const QString s("org.swift-project.plugins.simulator.p3d");
            return s;
        }

        const QString &CSimulatorPluginInfo::fs9PluginIdentifier()
        {
            static const QString s("org.swift-project.plugins.simulator.fs9");
            return s;
        }

        const QString &CSimulatorPluginInfo::xplanePluginIdentifier()
        {
            static const QString s("org.swift-project.plugins.simulator.xplane");
            return s;
        }

        const QString &CSimulatorPluginInfo::fgPluginIdentifier()
        {
            static const QString s("org.swift-project.plugins.simulator.flightgear");
            return s;
        }

        const QString &CSimulatorPluginInfo::fs2020PluginIdentifier()
        {
            static const QString s("org.swift-project.plugins.simulator.fs2020");
            return s;
        }

        const QString &CSimulatorPluginInfo::emulatedPluginIdentifier()
        {
            static const QString s("org.swift-project.plugins.simulator.emulated");
            return s;
        }

        const QStringList &CSimulatorPluginInfo::allIdentifiers()
        {
            static const QStringList identifiers(
            {
                fsxPluginIdentifier(),
                p3dPluginIdentifier(),
                xplanePluginIdentifier(),
                fs9PluginIdentifier(),
                emulatedPluginIdentifier(),
                fgPluginIdentifier(),
                fs2020PluginIdentifier()
            });
            return identifiers;
        }

        QStringList CSimulatorPluginInfo::guessDefaultPlugins()
        {
            if (CBuildConfig::isRunningOnUnixPlatform())
            {
                // On UNIX we likely run XP
                return QStringList { xplanePluginIdentifier(), fgPluginIdentifier() };
            }

            return QStringList
            {
                fsxPluginIdentifier(),
                fs2020PluginIdentifier(),
                p3dPluginIdentifier(),
                xplanePluginIdentifier(),
                fgPluginIdentifier()
            };
        }
    } // ns
} // ns

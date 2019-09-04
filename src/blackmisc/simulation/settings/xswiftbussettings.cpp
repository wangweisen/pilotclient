/* Copyright (C) 2019
 * swift project community / contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution. No part of swift project, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 */

#include "xswiftbussettings.h"

namespace BlackMisc
{
    namespace Simulation
    {
        namespace Settings
        {
            CXSwiftBusSettings::CXSwiftBusSettings() { }

            CXSwiftBusSettings::CXSwiftBusSettings(const QString &json)
            {
                this->parseXSwiftBusStringQt(json);
            }

            CVariant CXSwiftBusSettings::propertyByIndex(const CPropertyIndex &index) const
            {
                if (index.isMyself()) { return CVariant::from(*this); }
                if (ITimestampBased::canHandleIndex(index)) { return ITimestampBased::propertyByIndex(index); }
                const ColumnIndex i = index.frontCasted<ColumnIndex>();
                switch (i)
                {
                case IndexMaxPlanes:               return CVariant::fromValue(m_maxPlanes);
                case IndexDBusServerAddress:       return CVariant::fromValue(QString::fromStdString(m_dBusServerAddress));
                case IndexNightTextureMode:        return CVariant::fromValue(QString::fromStdString(m_nightTextureMode));
                case IndexMessageBoxMargins:       return CVariant::fromValue(QString::fromStdString(m_msgBox));
                case IndexDrawingLabels:           return CVariant::fromValue(m_drawingLabels);
                case IndexBundleTaxiLandingLights: return CVariant::fromValue(m_bundleTaxiLandingLights);
                case IndexMaxDrawingDistance:      return CVariant::fromValue(m_maxDrawDistanceNM);
                case IndexFollowAircraftDistance:  return CVariant::fromValue(m_followAircraftDistanceM);
                default: break;
                }
                return CValueObject::propertyByIndex(index);
            }

            void CXSwiftBusSettings::setPropertyByIndex(const CPropertyIndex &index, const CVariant &variant)
            {
                if (index.isMyself()) { (*this) = variant.to<CXSwiftBusSettings>(); return; }
                if (ITimestampBased::canHandleIndex(index)) { ITimestampBased::setPropertyByIndex(index, variant); }

                const ColumnIndex i = index.frontCasted<ColumnIndex>();
                switch (i)
                {
                case IndexMaxPlanes:               m_maxPlanes  = variant.toInt(); break;
                case IndexDBusServerAddress:       m_dBusServerAddress = variant.toStdString(); break;
                case IndexMessageBoxMargins:       m_msgBox   = variant.toStdString(); break;
                case IndexNightTextureMode:        m_nightTextureMode  = variant.toStdString(); break;
                case IndexDrawingLabels:           m_drawingLabels = variant.toBool(); break;
                case IndexBundleTaxiLandingLights: m_bundleTaxiLandingLights = variant.toBool(); break;
                case IndexMaxDrawingDistance:      m_maxDrawDistanceNM = variant.toDouble(); break;
                case IndexFollowAircraftDistance:  m_followAircraftDistanceM = variant.toInt(); break;
                default:
                    CValueObject::setPropertyByIndex(index, variant);
                    break;
                }
            }

            QString CXSwiftBusSettings::convertToQString(bool i18n) const
            {
                Q_UNUSED(i18n);
                return QString::fromStdString(CXSwiftBusSettingsQtFree::convertToString());
            }

            void CXSwiftBusSettings::setCurrentUtcTime()
            {
                ITimestampBased::setCurrentUtcTime();
                m_msSinceEpochQtFree = m_timestampMSecsSinceEpoch;
            }

            CStatusMessageList CXSwiftBusSettings::validate() const
            {
                CStatusMessageList msgs;
                const  QString dbus = QString::fromStdString(m_dBusServerAddress);
                if (!CDBusServer::isSessionOrSystemAddress(dbus) && !CDBusServer::isQtDBusAddress(dbus))
                {
                    msgs.addValidationMessage(QStringLiteral("Invalid DBus address '%1'").arg(dbus), CStatusMessage::SeverityError);
                }
                return msgs;
            }

            const CXSwiftBusSettings &CXSwiftBusSettings::defaultValue()
            {
                static const CXSwiftBusSettings s;
                return s;
            }

            void CXSwiftBusSettings::objectUpdated()
            {
                m_timestampMSecsSinceEpoch = m_msSinceEpochQtFree;
            }
        } // ns
    } // ns
} // ns
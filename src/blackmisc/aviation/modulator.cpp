/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution. No part of swift project, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 */

#include "blackmisc/aviation/modulator.h"
#include "blackmisc/aviation/comsystem.h"
#include "blackmisc/aviation/navsystem.h"
#include "blackmisc/aviation/adfsystem.h"
#include "blackmisc/math/mathutils.h"
#include "blackmisc/pq/units.h"
#include "blackmisc/propertyindex.h"
#include "blackmisc/variant.h"
#include "blackmisc/comparefunctions.h"
#include <QtGlobal>
#include <type_traits>

using BlackMisc::PhysicalQuantities::CFrequency;
using BlackMisc::PhysicalQuantities::CFrequencyUnit;

namespace BlackMisc
{
    namespace Aviation
    {
        template <class AVIO>
        void CModulator<AVIO>::toggleActiveStandby()
        {
            const CFrequency a = m_frequencyActive;
            m_frequencyActive  = m_frequencyStandby;
            m_frequencyStandby = a;
        }

        template <class AVIO>
        BlackMisc::PhysicalQuantities::CFrequency CModulator<AVIO>::getFrequencyActive() const
        {
            return m_frequencyActive;
        }

        template <class AVIO>
        BlackMisc::PhysicalQuantities::CFrequency CModulator<AVIO>::getFrequencyStandby() const
        {
            return m_frequencyStandby;
        }

        template <class AVIO>
        void CModulator<AVIO>::setFrequencyActive(const BlackMisc::PhysicalQuantities::CFrequency &frequency)
        {
            m_frequencyActive = frequency;
        }

        template <class AVIO>
        void CModulator<AVIO>::setFrequencyStandby(const BlackMisc::PhysicalQuantities::CFrequency &frequency)
        {
            m_frequencyStandby = frequency;
        }

        template <class AVIO>
        int CModulator<AVIO>::getVolumeOutput() const
        {
            return m_volumeOutput;
        }

        template <class AVIO>
        int CModulator<AVIO>::getVolumeInput() const
        {
            return m_volumeInput;
        }

        template <class AVIO>
        void CModulator<AVIO>::setVolumeOutput(int volume)
        {
            m_volumeOutput = volume;
        }

        template <class AVIO>
        void CModulator<AVIO>::setVolumeInput(int volume)
        {
            m_volumeInput = volume;
        }

        template <class AVIO>
        bool CModulator<AVIO>::isSendEnabled() const
        {
            return m_sendEnabled;
        }

        template<class AVIO>
        bool CModulator<AVIO>::isReceiveEnabled() const
        {
            return m_receiveEnabled;
        }

        template <class AVIO>
        void CModulator<AVIO>::setSendEnabled(bool enable)
        {
            m_sendEnabled = enable;
        }

        template<class AVIO>
        void CModulator<AVIO>::setReceiveEnabled(bool enable)
        {
            m_receiveEnabled = enable;
        }

        template <class AVIO>
        QString CModulator<AVIO>::getName() const
        {
            return m_name;
        }

        template <class AVIO>
        CVariant CModulator<AVIO>::propertyByIndex(const CPropertyIndex &index) const
        {
            if (index.isMyself()) { return CVariant::from(*derived()); }
            const ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexActiveFrequency:  return this->getFrequencyActive().propertyByIndex(index.copyFrontRemoved());
            case IndexStandbyFrequency: return this->getFrequencyStandby().propertyByIndex(index.copyFrontRemoved());
            case IndexEnabledSend:      return CVariant::from(this->isSendEnabled());
            case IndexEnabledReceive:   return CVariant::from(this->isReceiveEnabled());
            case IndexInputVolume:      return CVariant::from(this->getVolumeInput());
            case IndexOutputVolume:     return CVariant::from(this->getVolumeOutput());
            default:
                return CValueObject<CModulator<AVIO>>::propertyByIndex(index);
            }
        }

        template <class AVIO>
        void CModulator<AVIO>::setPropertyByIndex(const CPropertyIndex &index, const CVariant &variant)
        {
            if (index.isMyself()) { Q_ASSERT_X(false, Q_FUNC_INFO, "Wrong index to base template"); return; }
            const ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexActiveFrequency:  m_frequencyActive.setPropertyByIndex(index.copyFrontRemoved(), variant); break;
            case IndexStandbyFrequency: m_frequencyStandby.setPropertyByIndex(index.copyFrontRemoved(), variant); break;
            case IndexEnabledSend:      this->setSendEnabled(variant.toBool()); break;
            case IndexEnabledReceive:   this->setReceiveEnabled(variant.toBool()); break;
            case IndexInputVolume:      this->setVolumeInput(variant.toInt());  break;
            case IndexOutputVolume:     this->setVolumeOutput(variant.toInt()); break;
            default:
                CValueObject<CModulator<AVIO>>::setPropertyByIndex(index, variant);
                break;
            }
        }

        template <class AVIO>
        int CModulator<AVIO>::comparePropertyByIndex(const CPropertyIndex &index, const AVIO &compareValue) const
        {
            if (index.isMyself()) { return m_frequencyActive.comparePropertyByIndex(index.copyFrontRemoved(), compareValue.m_frequencyActive); }
            const ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexActiveFrequency:  return m_frequencyActive.comparePropertyByIndex(index.copyFrontRemoved(), compareValue.m_frequencyActive);
            case IndexStandbyFrequency: return m_frequencyStandby.comparePropertyByIndex(index.copyFrontRemoved(), compareValue.m_frequencyStandby);
            case IndexEnabledSend:      return Compare::compare(this->isSendEnabled(), compareValue.isSendEnabled());
            case IndexEnabledReceive:   return Compare::compare(this->isReceiveEnabled(), compareValue.isReceiveEnabled());
            case IndexInputVolume:      return Compare::compare(this->getVolumeInput(), compareValue.getVolumeInput());
            case IndexOutputVolume:     return Compare::compare(this->getVolumeOutput(), compareValue.getVolumeOutput());
            default: break;
            }
            Q_ASSERT_X(false, Q_FUNC_INFO, "Compare failed");
            return 0;
        }

        template <class AVIO>
        CModulator<AVIO>::CModulator() : m_name("default")
        {
            static_assert(!std::is_polymorphic<AVIO>::value, "Must not use virtual functions for value classes");
        }

        template <class AVIO>
        CModulator<AVIO>::CModulator(const QString &name, const BlackMisc::PhysicalQuantities::CFrequency &activeFrequency, const BlackMisc::PhysicalQuantities::CFrequency &standbyFrequency) :
            m_name(name), m_frequencyActive(activeFrequency), m_frequencyStandby(standbyFrequency)
        {
            static_assert(!std::is_polymorphic<AVIO>::value, "Must not use virtual functions for value classes");
        }

        template <class AVIO>
        QString CModulator<AVIO>::convertToQString(bool i18n) const
        {
            QString s(this->getName());
            s.append(" Active: ").append(m_frequencyActive.valueRoundedWithUnit(3, i18n));
            s.append(" Standby: ").append(m_frequencyStandby.valueRoundedWithUnit(3, i18n));
            return s;
        }

        template <class AVIO>
        void CModulator<AVIO>::setFrequencyActiveKHz(double frequencyKHz)
        {
            m_frequencyActive = BlackMisc::PhysicalQuantities::CFrequency(frequencyKHz, BlackMisc::PhysicalQuantities::CFrequencyUnit::kHz());
        }

        template <class AVIO>
        void CModulator<AVIO>::setFrequencyStandbyKHz(double frequencyKHz)
        {
            m_frequencyStandby = BlackMisc::PhysicalQuantities::CFrequency(frequencyKHz, BlackMisc::PhysicalQuantities::CFrequencyUnit::kHz());
        }

        template <class AVIO>
        const QString &CModulator<AVIO>::NameCom1()
        {
            static QString n("COM1");
            return n;
        }

        template <class AVIO>
        const QString &CModulator<AVIO>::NameCom2()
        {
            static QString n("COM2");
            return n;
        }

        template <class AVIO>
        const QString &CModulator<AVIO>::NameCom3()
        {
            static QString n("COM3");
            return n;
        }

        template <class AVIO>
        const QString &CModulator<AVIO>::NameNav1()
        {
            static QString n("NAV1");
            return n;
        }

        template <class AVIO>
        const QString &CModulator<AVIO>::NameNav2()
        {
            static QString n("NAV2");
            return n;
        }

        template <class AVIO>
        const QString &CModulator<AVIO>::NameNav3()
        {
            static QString n("NAV3");
            return n;
        }

        template <class AVIO>
        const QString &CModulator<AVIO>::NameAdf1()
        {
            static QString n("ADF1");
            return n;
        }

        template <class AVIO>
        const QString &CModulator<AVIO>::NameAdf2()
        {
            static QString n("ADF2");
            return n;
        }

        // see here for the reason of the forward instantiations
        // https://isocpp.org/wiki/faq/templates#separate-template-fn-defn-from-decl
        //! \cond PRIVATE
        template class BLACKMISC_EXPORT_DEFINE_TEMPLATE CModulator<CComSystem>;
        template class BLACKMISC_EXPORT_DEFINE_TEMPLATE CModulator<CNavSystem>;
        template class BLACKMISC_EXPORT_DEFINE_TEMPLATE CModulator<CAdfSystem>;
        //! \endcond

    } // namespace
} // namespace

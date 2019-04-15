/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution. No part of swift project, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_AVIATION_MODULATORUNIT_H
#define BLACKMISC_AVIATION_MODULATORUNIT_H

#include "blackmisc/propertyindex.h"
#include "blackmisc/pq/frequency.h"

namespace BlackMisc
{
    namespace Aviation
    {
        //! Base class for COM, NAV, Squawk units.
        template <class AVIO> class CModulator : public CValueObject<CModulator<AVIO>>
        {
        public:
            //! Column indexes
            enum ColumnIndex
            {
                IndexActiveFrequency = BlackMisc::CPropertyIndex::GlobalIndexCModulator,
                IndexStandbyFrequency,
                IndexOutputVolume,
                IndexInputVolume,
                IndexEnabledSend,
                IndexEnabledReceive
            };

            //! Toggle active and standby frequencies
            void toggleActiveStandby();

            //! Active frequency
            BlackMisc::PhysicalQuantities::CFrequency getFrequencyActive() const;

            //! Standby frequency
            BlackMisc::PhysicalQuantities::CFrequency getFrequencyStandby() const;

            //! Output volume 0..100
            int getVolumeOutput() const;

            //! Input volume 0..100
            int getVolumeInput() const;

            //! Output volume 0.100
            void setVolumeOutput(int volume);

            //! Input volume 0..100
            void setVolumeInput(int volume);

            //! Name
            QString getName() const;

            //! Enabled?
            bool isSendEnabled() const;

            //! Enabled?
            bool isReceiveEnabled() const;

            //! Enabled?
            void setSendEnabled(bool enable);

            //! Enabled?
            void setReceiveEnabled(bool enable);

            //! \copydoc BlackMisc::Mixin::Index::propertyByIndex
            CVariant propertyByIndex(const BlackMisc::CPropertyIndex &index) const;

            //! \copydoc BlackMisc::Mixin::Index::setPropertyByIndex
            void setPropertyByIndex(const BlackMisc::CPropertyIndex &index, const CVariant &variant);

            //! Compare by property index
            int comparePropertyByIndex(const BlackMisc::CPropertyIndex &index, const AVIO &compareValue) const;

            //! \copydoc BlackMisc::Mixin::String::toQString
            QString convertToQString(bool i18n = false) const;

        protected:
            //! Default constructor
            CModulator();

            //! Constructor
            CModulator(const QString &name, const BlackMisc::PhysicalQuantities::CFrequency &activeFrequency, const BlackMisc::PhysicalQuantities::CFrequency &standbyFrequency);

            //! Set active frequency
            void setFrequencyActive(const BlackMisc::PhysicalQuantities::CFrequency &frequency);

            //! Set standby frequency
            void setFrequencyStandby(const BlackMisc::PhysicalQuantities::CFrequency &frequency);

            //! Set active frequency
            void setFrequencyActiveKHz(double frequencyKHz);

            //! Set standby frequency
            void setFrequencyStandbyKHz(double frequencyKHz);

            //! COM1
            static const QString &NameCom1();

            //! COM2
            static const QString &NameCom2();

            //! COM3
            static const QString &NameCom3();

            //! NAV1
            static const QString &NameNav1();

            //! NAV2
            static const QString &NameNav2();

            //! NAV3
            static const QString &NameNav3();

            //! ADF1
            static const QString &NameAdf1();

            //! ADF2
            static const QString &NameAdf2();

        private:
            QString m_name; //!< name of the unit
            BlackMisc::PhysicalQuantities::CFrequency m_frequencyActive;  //!< active frequency
            BlackMisc::PhysicalQuantities::CFrequency m_frequencyStandby; //!< standby frequency
            int m_volumeInput     = 0;     //!< volume input
            int m_volumeOutput    = 0;     //!< volume output
            bool m_sendEnabled    = true;  //!< is enabled, used e.g. for mute etc.
            bool m_receiveEnabled = true;  //!< is enabled, used e.g. for mute etc.

            //! Easy access to derived class (CRTP template parameter)
            AVIO const *derived() const
            {
                return static_cast<AVIO const *>(this);
            }

            //! Easy access to derived class (CRTP template parameter)
            AVIO *derived()
            {
                return static_cast<AVIO *>(this);
            }

            BLACK_METACLASS(
                CModulator,
                BLACK_METAMEMBER(name),
                BLACK_METAMEMBER(frequencyActive),
                BLACK_METAMEMBER(frequencyStandby),
                BLACK_METAMEMBER(volumeInput),
                BLACK_METAMEMBER(volumeOutput),
                BLACK_METAMEMBER(sendEnabled),
                BLACK_METAMEMBER(receiveEnabled)
            );
        };

        //! \cond PRIVATE
        class CComSystem;
        class CNavSystem;
        class CAdfSystem;

        extern template class BLACKMISC_EXPORT_DECLARE_TEMPLATE CModulator<CComSystem>;
        extern template class BLACKMISC_EXPORT_DECLARE_TEMPLATE CModulator<CNavSystem>;
        extern template class BLACKMISC_EXPORT_DECLARE_TEMPLATE CModulator<CAdfSystem>;
        //! \endcond

    } // namespace
} // namespace

#endif // guard

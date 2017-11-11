/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_NETWORK_TEXTMESSAGE_H
#define BLACKMISC_NETWORK_TEXTMESSAGE_H

#include "blackmisc/aviation/callsign.h"
#include "blackmisc/blackmiscexport.h"
#include "blackmisc/icon.h"
#include "blackmisc/metaclass.h"
#include "blackmisc/pq/frequency.h"
#include "blackmisc/pq/units.h"
#include "blackmisc/propertyindex.h"
#include "blackmisc/statusmessage.h"
#include "blackmisc/timestampbased.h"
#include "blackmisc/valueobject.h"
#include "blackmisc/variant.h"

#include <QMetaType>
#include <QPixmap>
#include <QString>

namespace BlackMisc
{
    namespace Network
    {
        /*!
         * Value object encapsulating information of a text message
         */
        class BLACKMISC_EXPORT CTextMessage :
            public CValueObject<CTextMessage>,
            public ITimestampBased
        {
        public:
            //! Properties by index
            enum ColumnIndex
            {
                IndexSenderCallsign = CPropertyIndex::GlobalIndexCTextMessage,
                IndexRecipientCallsign,
                IndexRecipientCallsignOrFrequency,
                IndexMessage
            };

            //! Default constructor.
            CTextMessage() {}

            //! Constructor, radio message
            CTextMessage(const QString &message, const PhysicalQuantities::CFrequency &frequency, const Aviation::CCallsign &senderCallsign = {})
                : m_message(message), m_senderCallsign(senderCallsign), m_frequency(frequency)
            {
                m_frequency.switchUnit(PhysicalQuantities::CFrequencyUnit::MHz());
            }

            //! Constructor, private message
            CTextMessage(const QString &message, const Aviation::CCallsign &senderCallsign, const Aviation::CCallsign &recipientCallsign = {})
                : m_message(message), m_senderCallsign(senderCallsign), m_recipientCallsign(recipientCallsign), m_frequency(0, nullptr) {}

            //! Get callsign (from)
            const Aviation::CCallsign &getSenderCallsign() const { return m_senderCallsign; }

            //! Set callsign (from)
            void setSenderCallsign(const Aviation::CCallsign &callsign) { m_senderCallsign = callsign;}

            //! Get callsign (to)
            const Aviation::CCallsign &getRecipientCallsign() const { return m_recipientCallsign; }

            //! Set callsign (recipient)
            void setRecipientCallsign(const Aviation::CCallsign &callsign) { m_recipientCallsign = callsign; }

            //! Get recipient or frequency
            QString getRecipientCallsignOrFrequency() const;

            //! Send to particular frequency?
            bool isSendToFrequency(const PhysicalQuantities::CFrequency &frequency) const;

            //! Send to UNICOM?
            bool isSendToUnicom() const;

            //! Valid receviver?
            bool hasValidRecipient() const;

            //! Get message
            const QString &getMessage() const { return m_message; }

            //! Empty message
            bool isEmpty() const { return m_message.isEmpty(); }

            //! Set message
            void setMessage(const QString &message) { m_message = message.trimmed(); }

            //! Get frequency
            const PhysicalQuantities::CFrequency &getFrequency() const { return m_frequency; }

            //! Set frequency
            void setFrequency(const PhysicalQuantities::CFrequency &frequency) {  m_frequency = frequency; }

            //! Is private message?
            bool isPrivateMessage() const;

            //! Is radio message?
            bool isRadioMessage() const;

            //! Initial message of server?
            bool isServerMessage() const;

            /*!
             * Whole message as formatted string.
             * Used to display message in a console window.
             * \param withSender        include sender information in string?
             * \param withRecipient     include recipient information in string?
             * \param separator         values separated by given value
             * \return
             */
            QString asString(bool withSender, bool withRecipient, const QString &separator = ", ") const;

            /*!
             * Whole message as BlackMisc::CStatusMessage.
             * Used to display message in logs, or any other situation where a CStatusMessage is required.
             * \param withSender        include sender information in string?
             * \param withRecipient     include recipient information in string?
             * \param separator         values separated by given value
             * \return
             */
            CStatusMessage asStatusMessage(bool withSender, bool withRecipient, const QString &separator = ", ") const;

            //! Toggle sender receiver, can be used to ping my own message
            void toggleSenderRecipient();

            /*!
             * Is this a text message encapsulating a SELCAL
             * \see http://forums.vatsim.net/viewtopic.php?f=8&t=63467
             */
            bool isSelcalMessage() const;

            //! Is this a text message encapsulating a SELCAL for given code?
            bool isSelcalMessageFor(const QString &selcal) const;

            //! Supervisor message?
            bool isSupervisorMessage() const;

            //! Was sent?
            bool wasSent() const;

            //! Mark as sent
            //! \remark also sets current timestamp if there is no valid timestamp
            void markAsSent();

            //! Can another message be appended
            bool canBeAppended(const CTextMessage &textMessage) const;

            //! Append if possible
            bool appendIfPossible(const CTextMessage &textMessage);

            //! Get SELCAL code (if applicable, e.g. ABCD), otherwise ""
            QString getSelcalCode() const;

            //! As icon, not implement by all classes
            CIcon toIcon() const;

            //! As pixmap, required for most GUI views
            QPixmap toPixmap() const;

            //! \copydoc BlackMisc::Mixin::Index::propertyByIndex
            CVariant propertyByIndex(const BlackMisc::CPropertyIndex &index) const;

            //! \copydoc BlackMisc::Mixin::Index::setPropertyByIndex
            void setPropertyByIndex(const BlackMisc::CPropertyIndex &index, const CVariant &variant);

            //! \copydoc BlackMisc::Mixin::String::toQString
            QString convertToQString(bool i18n = false) const;

        private:
            QString m_message;
            Aviation::CCallsign m_senderCallsign;
            Aviation::CCallsign m_recipientCallsign;
            PhysicalQuantities::CFrequency m_frequency { 0, nullptr };
            bool m_wasSent = false;

            BLACK_METACLASS(
                CTextMessage,
                BLACK_METAMEMBER(message),
                BLACK_METAMEMBER(timestampMSecsSinceEpoch),
                BLACK_METAMEMBER(senderCallsign),
                BLACK_METAMEMBER(recipientCallsign),
                BLACK_METAMEMBER(frequency)
            );
        };
    } // namespace
} // namespace

Q_DECLARE_METATYPE(BlackMisc::Network::CTextMessage)

#endif // guard

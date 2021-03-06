/* Copyright (C) 2016
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution. No part of swift project, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 */

#include "blackmisc/network/rawfsdmessage.h"
#include "blackmisc/logcategory.h"
#include "blackmisc/logcategorylist.h"
#include "blackmisc/propertyindex.h"
#include "blackmisc/statusmessage.h"
#include "blackmisc/stringutils.h"
#include "blackmisc/variant.h"

#include <Qt>
#include <QtGlobal>

using namespace BlackMisc;

namespace BlackMisc
{
    namespace Network
    {
        CRawFsdMessage::CRawFsdMessage(const QString &rawMessage)
            : m_rawMessage(rawMessage) {}

        QString CRawFsdMessage::convertToQString(bool i18n) const
        {
            Q_UNUSED(i18n)
            return QStringLiteral("%1 %2").arg(this->getFormattedUtcTimestampHmsz(), m_rawMessage);
        }

        bool CRawFsdMessage::isPacketType(const QString &type) const
        {
            return m_rawMessage.startsWith("FSD Sent=>" + type) || m_rawMessage.startsWith("FSD Recv=>" + type);
        }

        bool CRawFsdMessage::containsString(const QString &str) const
        {
            return m_rawMessage.contains(str);
        }

        const QStringList &CRawFsdMessage::getAllPacketTypes()
        {
            static const QStringList allPacketTypes = { "@", "%", "#AA", "#DA", "#AP", "#DP", "#TM", "#WX", "#DL", "#TD", "#WD"
                                                        "#CD", "#PC", "#SB", "$FP", "$AM", "$PI", "$PO", "$HO", "$HA", "$AX", "$AR",
                                                        "$CQ", "$CR", "$ER", "$!!"
                                                      };
            return allPacketTypes;
        }

        CVariant CRawFsdMessage::propertyByIndex(const CPropertyIndex &index) const
        {
            if (index.isMyself()) { return CVariant::from(*this); }
            if (ITimestampBased::canHandleIndex(index)) { return ITimestampBased::propertyByIndex(index); }

            const ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexRawMessage: return CVariant::fromValue(m_rawMessage);
            default: return CValueObject::propertyByIndex(index);
            }
        }

        void CRawFsdMessage::setPropertyByIndex(const CPropertyIndex &index, const CVariant &variant)
        {
            if (index.isMyself()) { (*this) = variant.to<CRawFsdMessage>(); return; }
            if (ITimestampBased::canHandleIndex(index)) { ITimestampBased::setPropertyByIndex(index, variant); return; }

            const ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexRawMessage:
                this->setRawMessage(variant.value<QString>());
                break;
            default:
                CValueObject::setPropertyByIndex(index, variant);
                break;
            }
        }
    } // namespace
} // namespace

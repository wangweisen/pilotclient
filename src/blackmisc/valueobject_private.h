/* Copyright (C) 2014
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_VALUEOBJECT_PRIVATE_H
#define BLACKMISC_VALUEOBJECT_PRIVATE_H

#include <QMetaType>
#include <QDBusArgument>
#include <QJsonObject>

namespace BlackMisc
{
    class CValueObject;

    template <class T> typename std::enable_if<std::is_base_of<CValueObject, T>::value, QDBusArgument>::type const &
    operator>>(const QDBusArgument &argument, T &valueObject);

    namespace Private
    {
        //! \private Abstract base class representing the set of operations supported by a particular value type.
        struct IValueObjectMetaInfo
        {
            virtual ~IValueObjectMetaInfo() = default;
            virtual QString toQString(const void *object, bool i18n) const = 0;
            virtual QJsonObject toJson(const void *object) const = 0;
            virtual void convertFromJson(const QJsonObject &json, void *object) const = 0;
            virtual void unmarshall(const QDBusArgument &arg, void *object) const = 0;
            virtual uint getValueHash(const void *object) const = 0;
            virtual int getMetaTypeId() const = 0;
            virtual const void *upCastTo(const void *object, int metaTypeId) const = 0;
            virtual int compare(const void *lhs, const void *rhs) const = 0;
        };

        //! \private Implementation of IValueObjectMetaInfo representing the set of operations supported by T.
        template <typename T>
        struct CValueObjectMetaInfo : public IValueObjectMetaInfo
        {
            virtual QString toQString(const void *object, bool i18n) const override
            {
                return cast(object).toQString(i18n);
            }
            virtual QJsonObject toJson(const void *object) const override
            {
                return cast(object).toJson();
            }
            virtual void convertFromJson(const QJsonObject &json, void *object) const override
            {
                cast(object).convertFromJson(json);
            }
            virtual void unmarshall(const QDBusArgument &arg, void *object) const override
            {
                // FIXME Using the usual >> operator syntax here attempts to call the Container<T> overload in QtDBus/qdbusargument.h
                BlackMisc::operator >>(arg, cast(object));
            }
            virtual uint getValueHash(const void *object) const override
            {
                return cast(object).getValueHash();
            }
            virtual int getMetaTypeId() const override
            {
                return maybeGetMetaTypeId(std::integral_constant<bool, QMetaTypeId<T>::Defined>{});
            }
            virtual const void *upCastTo(const void *object, int metaTypeId) const override
            {
                const auto base = static_cast<const void *>(static_cast<const typename T::base_type *>(&cast(object)));
                return metaTypeId == getMetaTypeId() ? object : CValueObjectMetaInfo<typename T::base_type>{}.upCastTo(base, metaTypeId);
            }
            virtual int compare(const void *lhs, const void *rhs) const override; // FIXME defined out-of-line in valueobject.h because it uses CValueObject

            static const T &cast(const void *object) { return *static_cast<const T *>(object); }
            static T &cast(void *object) { return *static_cast<T *>(object); }

            static int maybeGetMetaTypeId(std::true_type) { return qMetaTypeId<T>(); }
            static int maybeGetMetaTypeId(std::false_type) { return QMetaType::UnknownType; }
        };

        //! \private Explicit specialization for the terminating case of the recursive CValueObjectMetaInfo::upCastTo.
        template <>
        inline const void *CValueObjectMetaInfo<CValueObject>::upCastTo(const void *, int) const
        {
            return nullptr;
        }

        //! \private Getter to obtain the IValueObjectMetaInfo which was stored by BlackMisc::registerMetaValueType.
        IValueObjectMetaInfo *getValueObjectMetaInfo(int typeId);

        //! \private Getter to obtain the IValueObjectMetaInfo which was stored by BlackMisc::registerMetaValueType.
        IValueObjectMetaInfo *getValueObjectMetaInfo(const QVariant &);

        //! \private Getter to obtain the IValueObjectMetaInfo which was stored by BlackMisc::registerMetaValueType.
        template <typename T>
        IValueObjectMetaInfo *getValueObjectMetaInfo() { return getValueObjectMetaInfo(qMetaTypeId<T>()); }
    }
}

Q_DECLARE_METATYPE(BlackMisc::Private::IValueObjectMetaInfo *)

#endif

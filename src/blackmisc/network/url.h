/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_NETWORK_NETWORKLOCATION_H
#define BLACKMISC_NETWORK_NETWORKLOCATION_H

#include "blackmisc/blackmiscexport.h"
#include "blackmisc/valueobject.h"
#include <QUrl>

namespace BlackMisc
{
    namespace Network
    {
        //! Value object encapsulating information of a location,
        //! kind of simplied CValueObject compliant version of QUrl
        class BLACKMISC_EXPORT CUrl : public CValueObject<CUrl>
        {
        public:
            //! Properties by index
            enum ColumnIndex
            {
                IndexScheme = BlackMisc::CPropertyIndex::GlobalIndexCUrl,
                IndexHost,
                IndexPort,
                IndexPath,
                IndexQuery
            };

            //! Default constructor.
            CUrl(const QString &fullUrl = QString());

            //! By QUrl.
            CUrl(const QUrl &url);

            //! Constructor.
            CUrl(const QString &address, int port);

            //! Constructor.
            CUrl(const QString &scheme, const QString &address, int port, const QString &path);

            //! Get host.
            const QString &getHost() const { return m_host; }

            //! Set address (e.g. myserver.foo.com)
            void setHost(const QString &address) { m_host = address.trimmed(); }

            //! Get protocl
            const QString &getScheme() const { return m_scheme; }

            //! Set protocol
            void setScheme(const QString &protocol);

            //! Protocol
            bool hasScheme() const { return !m_scheme.isEmpty(); }

            //! Get path
            const QString &getPath() const { return m_path; }

            //! Set path
            void setPath(const QString &path);

            //! Append path
            QString appendPath(const QString &pathToAppend);

            //! Has path?
            bool hasPath() const { return !m_path.isEmpty(); }

            //! Get port
            int getPort() const { return m_port; }

            //! Set port
            void setPort(int port) { m_port = port; }

            //! Port?
            bool hasPort() const;

            //! Default port
            bool hasDefaultPort() const;

            //! Get query part
            QString getQuery() const { return m_query; }

            //! Set query
            void setQuery(const QString &query);

            //! Query string?
            bool hasQuery() const;

            //! Append query
            QString appendQuery(const QString &queryToAppend);

            //! Empty
            bool isEmpty() const;

            //! Qualified name
            QString getFullUrl() const;

            //! Set full URL
            void setFullUrl(const QString &fullUrl);

            //! To QUrl
            QUrl toQUrl() const;

            //! To QUrl
            void setQUrl(const QUrl &url);

            //! Append path
            CUrl withAppendedPath(const QString &path) const;

            //! Append path
            CUrl withAppendedQuery(const QString &query) const;

            //! Switch protocol
            CUrl withSwitchedScheme(const QString &protocol, int port) const;

            //! \copydoc CValueObject::propertyByIndex
            CVariant propertyByIndex(const BlackMisc::CPropertyIndex &index) const;

            //! \copydoc CValueObject::setPropertyByIndex
            void setPropertyByIndex(const CVariant &variant, const BlackMisc::CPropertyIndex &index);

            //! \copydoc CValueObject::convertToQString()
            QString convertToQString(bool i18n = false) const;

            //! \copydoc BlackMisc::Mixin::JsonByTuple::toJson
            QJsonObject toJson() const;

            //! \copydoc BlackMisc::Mixin::JsonByTuple::convertFromJson
            void convertFromJson(const QJsonObject &json);

            //! Protocol to default port
            static int protocolToDefaultPort(const QString &protocol);

            //! Default port for given protocol
            static bool isDefaultPort(const QString &protocol, int port);

            //! Convert to QUrl
            operator QUrl() const { return this->toQUrl(); }

        private:
            BLACK_ENABLE_TUPLE_CONVERSION(CUrl)
            QString m_scheme;
            QString m_host;
            int     m_port = -1;
            QString m_path;
            QString m_query;

            static QString stripQueryString(const QString query);
        };
    } // namespace
} // namespace

BLACK_DECLARE_TUPLE_CONVERSION(BlackMisc::Network::CUrl, (o.m_scheme, o.m_host, o.m_port, o.m_path, o.m_query))
Q_DECLARE_METATYPE(BlackMisc::Network::CUrl)

#endif // guard

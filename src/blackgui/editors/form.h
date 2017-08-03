/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_EDITORS_FORM_H
#define BLACKGUI_EDITORS_FORM_H

#include "blackgui/overlaymessagesframe.h"
#include "blackgui/blackguiexport.h"
#include "blackcore/data/authenticateduser.h"
#include "blackmisc/datacache.h"
#include <QFrame>
#include <QObject>

class QWidget;

namespace BlackMisc { namespace Network { class CAuthenticatedUser; } }
namespace BlackGui
{
    namespace Editors
    {
        /*!
         * Form base class
         */
        class BLACKGUI_EXPORT CForm : public COverlayMessagesFrame
        {
            Q_OBJECT

        public:
            //! Constructor
            explicit CForm(QWidget *parent = nullptr);

            //! Destructor
            virtual ~CForm();

            //! Set editable
            virtual void setReadOnly(bool readOnly) = 0;

            //! Read only, but entity can be selected (normally used in mapping).
            //! Use setReadOnly to reset this very state
            virtual void setSelectOnly();

            //! Validate, empty list means OK
            virtual BlackMisc::CStatusMessageList validate(bool withNestedObjects = true) const;

            //! Is read only?
            bool isReadOnly() const { return m_readOnly; }

            //! Authenticated DB user
            BlackMisc::Network::CAuthenticatedUser getSwiftDbUser() const;

        protected:
            //! JSON string has been pasted
            //! \remark The JSON string has been pre-checked
            virtual void jsonPasted(const QString &json);

            bool m_readOnly = false; //!< read only
            BlackMisc::CDataReadOnly<BlackCore::Data::TAuthenticatedDbUser> m_swiftDbUser {this, &CForm::ps_userChanged}; //!< authenticated user

        protected slots:
            //! User has been changed
            virtual void ps_userChanged();

            //! Pasted from clipboard
            void ps_pasted();
        };
    } // ns
} // ns

#endif // guard

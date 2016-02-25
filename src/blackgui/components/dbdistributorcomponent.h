/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_COMPONENTS_DBDISTRIBUTORCOMPONENT_H
#define BLACKGUI_COMPONENTS_DBDISTRIBUTORCOMPONENT_H

#include "blackgui/blackguiexport.h"
#include "blackgui/enableforviewbasedindicator.h"
#include "blackgui/components/enablefordockwidgetinfoarea.h"
#include "blackmisc/network/entityflags.h"
#include <QFrame>
#include <QScopedPointer>

namespace Ui { class CDbDistributorComponent; }

namespace BlackGui
{
    namespace Components
    {
        /**
         * Distributors
         */
        class BLACKGUI_EXPORT CDbDistributorComponent :
            public QFrame,
            public CEnableForDockWidgetInfoArea,
            public BlackGui::CEnableForViewBasedIndicator
        {
            Q_OBJECT

        public:
            //! Constructor
            explicit CDbDistributorComponent(QWidget *parent = nullptr);

            //! Destructor
            ~CDbDistributorComponent();

        private slots:
            //! Distributors have been read
            void ps_distributorsRead(BlackMisc::Network::CEntityFlags::Entity entity, BlackMisc::Network::CEntityFlags::ReadState readState, int count);

            //! Reload models
            void ps_reload();

        private:
            QScopedPointer<Ui::CDbDistributorComponent> ui;
        };

    } // ns
} // ns

#endif // guard

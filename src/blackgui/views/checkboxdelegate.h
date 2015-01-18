/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_CCOLUMNCHECKBOXDELEGATE_H
#define BLACKGUI_CCOLUMNCHECKBOXDELEGATE_H

#include <QItemDelegate>
#include <QCheckBox>

namespace BlackGui
{
    namespace Views
    {

        //! CheckBox for single column
        class CCheckBoxDelegate : public QItemDelegate
        {
            Q_OBJECT

        public:
            //! Constructor
            CCheckBoxDelegate(QObject *parent = nullptr);

            //! Constructor
            CCheckBoxDelegate(const QString &iconCheckedUrl, const QString &iconUncheckedUrl, QObject *parent = nullptr);

            //! Destructor
            ~CCheckBoxDelegate();

            //! \copydoc QItemDelegate::createEditor
            virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

            //! \copydoc QItemDelegate::setEditorData
            virtual void setEditorData(QWidget *editor, const QModelIndex &index) const override;

            //! \copydoc QItemDelegate::setModelData
            virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

            //! \copydoc QItemDelegate::updateEditorGeometry
            virtual void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;

        private:
            QString m_iconCheckedUrl;
            QString m_iconUncheckedUrl;
        };

    } // namespace
} // namespace

#endif // guard

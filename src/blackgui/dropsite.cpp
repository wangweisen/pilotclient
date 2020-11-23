/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution. No part of swift project, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 */

#include "blackgui/dropsite.h"
#include "blackgui/guiapplication.h"
#include "blackgui/stylesheetutility.h"

#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFrame>
#include <QPalette>
#include <QStyle>
#include <Qt>
#include <QtGlobal>

using namespace BlackMisc;

namespace BlackGui
{
    CDropSite::CDropSite(QWidget *parent) : QLabel(parent)
    {
        this->setFrameStyle(static_cast<int>(QFrame::Sunken) | QFrame::StyledPanel);
        this->setAlignment(Qt::AlignCenter);
        this->setAcceptDrops(true);
        this->setTextFormat(Qt::RichText);
        this->setInfoText("drop data here");
        this->onStyleSheetsChanged();
        connect(sGui, &CGuiApplication::styleSheetsChanged, this, &CDropSite::onStyleSheetsChanged, Qt::QueuedConnection);
    }

    void CDropSite::setInfoText(const QString &dropSiteText)
    {
        m_infoText = dropSiteText;
        this->resetText();
    }

    void CDropSite::allowDrop(bool allowed)
    {
        CDropBase::allowDrop(allowed);
        this->setEnabled(allowed);
        this->setVisible(allowed);
    }

    void CDropSite::resetText()
    {
        const QString html = "<img src=':/own/icons/own/drophere16.png'>&nbsp;&nbsp;" + m_infoText.toHtmlEscaped();
        setText(html);
    }

    void CDropSite::dragEnterEvent(QDragEnterEvent *event)
    {
        if (!event || !acceptDrop(event->mimeData())) { return; }
        setBackgroundRole(QPalette::Highlight);
        event->acceptProposedAction();
    }

    void CDropSite::dragMoveEvent(QDragMoveEvent *event)
    {
        if (!event || !acceptDrop(event->mimeData())) { return; }
        setBackgroundRole(QPalette::Highlight);
        event->acceptProposedAction();
    }

    void CDropSite::dragLeaveEvent(QDragLeaveEvent *event)
    {
        if (!event || !m_allowDrop) { return; }
        resetText();
        event->accept();
    }

    void CDropSite::dropEvent(QDropEvent *event)
    {
        if (!event || !acceptDrop(event->mimeData())) { return; }
        CVariant valueVariant(toCVariant(event->mimeData()));
        if (valueVariant.isValid())
        {
            emit droppedValueObject(valueVariant);
        }
        this->resetText();
    }

    void CDropSite::onStyleSheetsChanged()
    {
        // style sheet changes go here
    }
} // ns

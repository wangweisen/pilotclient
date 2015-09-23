/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackmisc/rgbcolor.h"
#include "colorselector.h"
#include "guiutility.h"
#include "ui_colorselector.h"
#include <QColorDialog>
#include <QColor>
#include <QMimeData>
#include <QDrag>
#include <QDropEvent>
#include <QCompleter>

using namespace BlackMisc;

namespace BlackGui
{
    CColorSelector::CColorSelector(QWidget *parent) :
        QFrame(parent),
        ui(new Ui::CColorSelector)
    {
        ui->setupUi(this);
        ui->tb_ColorDialog->setIcon(CIcons::color16());
        this->setAcceptDrops(true);
        connect(this->ui->tb_ColorDialog, &QToolButton::clicked, this, &CColorSelector::ps_colorDialog);
        connect(this->ui->le_Color, &QLineEdit::editingFinished, this, &CColorSelector::ps_returnPressed);
        connect(this->ui->le_Color, &QLineEdit::returnPressed, this, &CColorSelector::ps_returnPressed);

        QCompleter *completer = new QCompleter(QColor::colorNames(), this);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        completer->setMaxVisibleItems(10);
        completer->setCompletionMode(QCompleter::PopupCompletion);
        this->ui->le_Color->setCompleter(completer);
        this->connect(completer, static_cast<void (QCompleter::*)(const QString &)>(&QCompleter::activated), this, &CColorSelector::ps_colorName);
    }

    CColorSelector::~CColorSelector() { }

    void CColorSelector::setColor(const BlackMisc::CRgbColor &color)
    {
        if (color == m_lastColor) { return; }
        if (!color.isValid())
        {
            this->clear();
            m_lastColor = CRgbColor();
        }
        else
        {
            this->ui->le_Color->setText(color.hex());
            this->ui->lbl_ColorIcon->setPixmap(color.toPixmap());
            m_lastColor = color;
        }
        emit colorChanged(color);
    }

    void CColorSelector::setColor(const QColor &color)
    {
        this->setColor(CRgbColor(color));
    }

    const BlackMisc::CRgbColor CColorSelector::getColor() const
    {
        CRgbColor q(this->ui->le_Color->text());
        return q;
    }

    void CColorSelector::setReadOnly(bool readOnly)
    {
        this->ui->le_Color->setReadOnly(readOnly);
        this->ui->tb_ColorDialog->setVisible(!readOnly);
        this->setAcceptDrops(!readOnly);
    }

    void CColorSelector::clear()
    {
        this->ui->le_Color->clear();
        this->ui->lbl_ColorIcon->setPixmap(QPixmap());
    }

    void CColorSelector::dragEnterEvent(QDragEnterEvent *event)
    {
        if (!event) { return; }
        setBackgroundRole(QPalette::Highlight);
        event->acceptProposedAction();
    }

    void CColorSelector::dragMoveEvent(QDragMoveEvent *event)
    {
        if (!event) { return; }
        event->acceptProposedAction();
    }

    void CColorSelector::dragLeaveEvent(QDragLeaveEvent *event)
    {
        event->accept();
    }

    void CColorSelector::dropEvent(QDropEvent *event)
    {
        if (!event) { return; }
        const QMimeData *mime = event->mimeData();
        if (!mime) { return; }

        if (mime->hasColor())
        {
            QColor color = qvariant_cast<QColor>(event->mimeData()->colorData());
            if (!color.isValid()) { return; }
            this->setColor(color);
        }
        else if (CGuiUtility::hasSwiftVariantMimeType(mime))
        {
            CVariant valueVariant(CGuiUtility::fromSwiftDragAndDropData(mime));
            if (valueVariant.isValid())
            {
                if (valueVariant.canConvert<CRgbColor>())
                {
                    CRgbColor rgb(valueVariant.value<CRgbColor>());
                    if (!rgb.isValid()) { return; }
                    this->setColor(rgb);
                }
                else if (valueVariant.canConvert<QColor>())
                {
                    QColor qColor(valueVariant.value<QColor>());
                    if (!qColor.isValid()) { return; }
                    this->setColor(qColor);
                }
            }
        }
        else if (mime->hasText())
        {
            QString t = mime->text();
            if (t.isEmpty()) { return; }
            if (!t.contains("{"))
            {
                // we assume a color string, not an object (because there are no {})
                CRgbColor c(t);
                if (!c.isValid()) { return; }
                this->setColor(c);
            }
        }
    }

    void CColorSelector::mousePressEvent(QMouseEvent *event)
    {
        if (!event || event->button() != Qt::LeftButton) { QFrame::mousePressEvent(event); }
        CRgbColor c(this->getColor());
        if (!c.isValid()) { return; }

        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;

        mimeData->setColorData(QVariant::fromValue(c.toQColor()));
        drag->setMimeData(mimeData);
        drag->setPixmap(c.toPixmap());

        Qt::DropAction dropAction = drag->exec(Qt::CopyAction);
        Q_UNUSED(dropAction);
    }

    void CColorSelector::ps_colorDialog()
    {
        QColor q = this->getColor().toQColor();
        if (!q.isValid()) { q = m_lastColor.toQColor(); }
        QColor newColor  = QColorDialog::getColor(q, this, "Color picker");
        if (!newColor.isValid()) { return; }
        this->setColor(newColor);
    }

    void CColorSelector::ps_returnPressed()
    {
        CRgbColor color = this->getColor();
        if (color.isValid())
        {
            this->setColor(color);
        }
    }

    void CColorSelector::ps_colorName(const QString &colorName)
    {
        if (colorName.isEmpty()) { return; }
        CRgbColor c(colorName, true);
        if (c.isValid()) { this->setColor(c); }
    }

    void CColorSelector::resetToLastValidColor()
    {
        if (!m_lastColor.isValid()) { return; }
        this->setColor(this->m_lastColor);
    }
} // ns

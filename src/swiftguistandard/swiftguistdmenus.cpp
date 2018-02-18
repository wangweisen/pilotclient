/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackgui/components/maininfoareacomponent.h"
#include "blackgui/components/settingscomponent.h"
#include "blackgui/guiactionbind.h"
#include "blackgui/guiapplication.h"
#include "blackgui/foreignwindows.h"

#include "blackmisc/aviation/altitude.h"
#include "blackmisc/network/urllist.h"
#include "blackmisc/pq/units.h"
#include "blackmisc/logmessage.h"
#include "swiftguistd.h"
#include "ui_swiftguistd.h"

#include <QAction>
#include <QMenu>
#include <QObject>
#include <QScopedPointer>
#include <QStackedWidget>
#include <QtGlobal>
#include <QDesktopServices>

using namespace BlackGui;
using namespace BlackCore;
using namespace BlackMisc;
using namespace BlackMisc::PhysicalQuantities;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::Network;

void SwiftGuiStd::onMenuClicked()
{
    QObject *sender = QObject::sender();
    if (sender == ui->menu_TestLocationsEDRY)
    {
        this->setTestPosition("N 049° 18' 17", "E 008° 27' 05",
                              CAltitude(312, CAltitude::MeanSeaLevel, CLengthUnit::ft()),
                              CAltitude(312, CAltitude::MeanSeaLevel, CAltitude::PressureAltitude, CLengthUnit::ft()));
    }
    else if (sender == ui->menu_TestLocationsEDNX)
    {
        this->setTestPosition("N 048° 14′ 22", "E 011° 33′ 41",
                              CAltitude(486, CAltitude::MeanSeaLevel, CLengthUnit::m()),
                              CAltitude(486, CAltitude::MeanSeaLevel, CAltitude::PressureAltitude, CLengthUnit::m()));
    }
    else if (sender == ui->menu_TestLocationsEDDM)
    {
        this->setTestPosition("N 048° 21′ 14", "E 011° 47′ 10",
                              CAltitude(448, CAltitude::MeanSeaLevel, CLengthUnit::m()),
                              CAltitude(448, CAltitude::MeanSeaLevel, CAltitude::PressureAltitude, CLengthUnit::m()));
    }
    else if (sender == ui->menu_TestLocationsEDDF)
    {
        this->setTestPosition("N 50° 2′ 0", "E 8° 34′ 14",
                              CAltitude(100, CAltitude::MeanSeaLevel, CLengthUnit::m()),
                              CAltitude(100, CAltitude::MeanSeaLevel, CAltitude::PressureAltitude, CLengthUnit::m()));
    }
    else if (sender == ui->menu_TestLocationsLOWW)
    {
        this->setTestPosition("N 48° 7′ 6.3588", "E 16° 33′ 39.924",
                              CAltitude(100, CAltitude::MeanSeaLevel, CLengthUnit::m()),
                              CAltitude(100, CAltitude::MeanSeaLevel, CAltitude::PressureAltitude, CLengthUnit::m()));
    }
    else if (sender == ui->menu_WindowFont)
    {
        this->setMainPageToInfoArea();
        ui->comp_MainInfoArea->selectSettingsTab(BlackGui::Components::CSettingsComponent::SettingTabGui);
    }
    else if (sender == ui->menu_InternalsPage)
    {
        ui->sw_MainMiddle->setCurrentIndex(MainPageInternals);
    }
    else if (sender == ui->menu_MovingMap && sGui && !sGui->getGlobalSetup().getSwiftMapUrls().isEmpty())
    {
        const CUrlList urls = sGui->getGlobalSetup().getSwiftMapUrls();
        const CUrl url = urls.getRandomUrl();
        QDesktopServices::openUrl(url);
    }
}

void SwiftGuiStd::attachSimulatorWindow()
{
    this->activateWindow(); // attaching requires active window
    QWindow *w = CForeignWindows::getFirstFoundSimulatorWindow();
    if (!w)
    {
        CLogMessage(this).warning("No simulator window found");
        return;
    }
    const bool a = CForeignWindows::setSimulatorAsParent(w, this);
    if (a)
    {
        CLogMessage(this).info("Attached to simulator");
    }
    else
    {
        CLogMessage(this).warning("No simulator window found");
    }
}

void SwiftGuiStd::detachSimulatorWindow()
{
    if (CForeignWindows::unsetSimulatorAsParent(this))
    {
        CLogMessage(this).info("Detached simulator window");
    }
    else
    {
        CLogMessage(this).info("No simulator window to detach");
    }
}

void SwiftGuiStd::initMenus()
{
    Q_ASSERT_X(ui->menu_InfoAreas, Q_FUNC_INFO, "No menu");
    Q_ASSERT_X(ui->menu_Window, Q_FUNC_INFO, "No menu");
    Q_ASSERT_X(ui->comp_MainInfoArea, Q_FUNC_INFO, "no main area");
    sGui->addMenuFile(*ui->menu_File);
    sGui->addMenuInternals(*ui->menu_Internals);
    sGui->addMenuWindow(*ui->menu_Window);
    ui->menu_Window->addSeparator();
    QAction *a = ui->menu_Window->addAction("Attach simulator window");
    bool c = connect(a, &QAction::triggered, this, &SwiftGuiStd::attachSimulatorWindow);
    Q_ASSERT_X(c, Q_FUNC_INFO, "connect failed");
    a = ui->menu_Window->addAction("Detach simulator window");
    c = connect(a, &QAction::triggered, this, &SwiftGuiStd::detachSimulatorWindow);
    Q_ASSERT_X(c, Q_FUNC_INFO, "connect failed");

    sGui->addMenuHelp(*ui->menu_Help);
    ui->menu_InfoAreas->addActions(ui->comp_MainInfoArea->getInfoAreaSelectActions(true, ui->menu_InfoAreas));
    ui->menu_MovingMap->setIcon(CIcons::swiftMap16());

    // for hotkeys
    const QString swift(CGuiActionBindHandler::pathSwiftPilotClient());
    static const CActionBind swiftRoot(swift, CIcons::swift16()); // inserts action for root folder
    Q_UNUSED(swiftRoot);
    m_menuHotkeyHandlers.append(CGuiActionBindHandler::bindMenu(ui->menu_InfoAreas, swift + "Info areas"));
    m_menuHotkeyHandlers.append(CGuiActionBindHandler::bindMenu(ui->menu_File, swift + "File"));
    m_menuHotkeyHandlers.append(CGuiActionBindHandler::bindMenu(ui->menu_Window, swift + "Window"));
}

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "blackgui/atcstationlistmodel.h"
#include "blackcore/dbus_server.h"
#include "blackcore/context_network.h"
#include "blackcore/coreruntime.h"
#include <QSortFilterProxyModel>
#include <QSizeGrip>
#include <QHBoxLayout>

using namespace BlackCore;
using namespace BlackMisc;
using namespace BlackGui;


/*
 * Init data
 */
void MainWindow::init(GuiModes::CoreMode coreMode)
{
    if (this->m_init) return;
    this->m_coreMode = coreMode;

    // with frameless window, we shift menu and statusbar into central widget
    // http://stackoverflow.com/questions/18316710/frameless-and-transparent-window-qt5
    if (this->m_windowMode == GuiModes::WindowFrameless)
    {

        this->ui->wi_CentralWidgetOutside->setStyleSheet("#wi_CentralWidgetOutside {border: 2px solid green; border-radius: 20px; }");
        this->ui->vl_CentralWidgetOutside->setContentsMargins(8, 8, 8, 8);

        QHBoxLayout *menuBarLayout = new QHBoxLayout();
        QPushButton *closeIcon = new QPushButton(this);
        closeIcon->setStyleSheet("margin: 0; padding: 0; background: transparent;");
        closeIcon->setIcon(QIcon(":/blackgui/icons/close.png"));
        QObject::connect(closeIcon, SIGNAL(clicked()), this, SLOT(close()));
        menuBarLayout->addWidget(this->ui->mb_MainMenuBar, 0, Qt::AlignTop | Qt::AlignLeft);
        menuBarLayout->addWidget(closeIcon, 0, Qt::AlignTop | Qt::AlignRight);
        this->ui->vl_CentralWidgetOutside->insertLayout(0, menuBarLayout, 0);

        QSizeGrip *grip = new QSizeGrip(this);
        grip->setStyleSheet("margin-right: 25px; background-color: transparent;");
        this->ui->sb_MainStatusBar->setParent(this->ui->wi_CentralWidgetOutside);
        this->ui->vl_CentralWidgetOutside->addWidget(this->ui->sb_MainStatusBar, 0);
        this->ui->sb_MainStatusBar->addPermanentWidget(grip);
    }

    // init models, the delete allows to re-init
    if (this->m_atcListBooked != nullptr) this->m_atcListBooked->deleteLater();
    this->m_atcListBooked = new CAtcListModel(this);

    if (this->m_atcListOnline != nullptr) this->m_atcListOnline->deleteLater();
    this->m_atcListOnline = new CAtcListModel(this);

    if (this->m_trafficServerList != nullptr) this->m_trafficServerList->deleteLater();
    this->m_trafficServerList = new CServerListModel(this);

    if (this->m_aircraftsInRange != nullptr) this->m_aircraftsInRange->deleteLater();
    this->m_aircraftsInRange = new CAircraftListModel(this);

    if (this->m_allUsers != nullptr) this->m_allUsers->deleteLater();
    this->m_allUsers = new CUserListModel(this);

    if (this->m_usersVoiceCom1 != nullptr) this->m_usersVoiceCom1->deleteLater();
    this->m_usersVoiceCom1 = new CUserListModel(this);

    if (this->m_usersVoiceCom2 != nullptr) this->m_usersVoiceCom2->deleteLater();
    this->m_usersVoiceCom2 = new CUserListModel(this);

    // set sort order and models
    // enable first, otherwise order in the model will be reset
    this->ui->tv_SettingsTnServers->setModel(this->m_trafficServerList);


    this->ui->tv_AtcStationsOnline->setSortingEnabled(true);
    this->ui->tv_AtcStationsOnline->setModel(this->m_atcListOnline);
    this->m_atcListBooked->setSortColumnByPropertyIndex(BlackMisc::Aviation::CAtcStation::IndexDistance);
    if (this->m_atcListOnline->hasValidSortColumn())
        this->ui->tv_AtcStationsOnline->horizontalHeader()->setSortIndicator(this->m_atcListOnline->getSortColumn(), this->m_atcListOnline->getSortOrder());

    this->ui->tv_AtcStationsBooked->setSortingEnabled(true);
    this->ui->tv_AtcStationsBooked->setModel(this->m_atcListBooked);
    this->m_atcListBooked->setSortColumnByPropertyIndex(BlackMisc::Aviation::CAtcStation::IndexBookedFrom);
    if (this->m_atcListBooked->hasValidSortColumn())
        this->ui->tv_AtcStationsBooked->horizontalHeader()->setSortIndicator(this->m_atcListBooked->getSortColumn(), this->m_atcListBooked->getSortOrder());

    this->ui->tv_AircraftsInRange->setSortingEnabled(true);
    this->ui->tv_AircraftsInRange->setModel(this->m_aircraftsInRange);
    this->m_atcListBooked->setSortColumnByPropertyIndex(BlackMisc::Aviation::CAircraft::IndexDistance);
    if (this->m_aircraftsInRange->hasValidSortColumn())
        this->ui->tv_AircraftsInRange->horizontalHeader()->setSortIndicator(this->m_aircraftsInRange->getSortColumn(), this->m_aircraftsInRange->getSortOrder());
    this->ui->tv_AircraftsInRange->resizeColumnsToContents();
    this->ui->tv_AircraftsInRange->resizeRowsToContents();

    this->ui->tv_AllUsers->setSortingEnabled(true);
    this->ui->tv_AllUsers->setModel(this->m_allUsers);
    this->m_allUsers->setSortColumnByPropertyIndex(BlackMisc::Network::CUser::IndexRealName);
    if (this->m_allUsers->hasValidSortColumn())
        this->ui->tv_AllUsers->horizontalHeader()->setSortIndicator(this->m_allUsers->getSortColumn(), this->m_allUsers->getSortOrder());
    this->ui->tv_AllUsers->resizeColumnsToContents();
    this->ui->tv_AllUsers->resizeRowsToContents();
    this->ui->tv_AllUsers->horizontalHeader()->setStretchLastSection(true);

    this->ui->tv_CockpitVoiceRoom1->setSortingEnabled(true);
    this->ui->tv_CockpitVoiceRoom1->setModel(this->m_usersVoiceCom1);
    this->m_usersVoiceCom1->setSortColumnByPropertyIndex(BlackMisc::Network::CUser::IndexRealName);
    if (this->m_usersVoiceCom1->hasValidSortColumn())
        this->ui->tv_CockpitVoiceRoom1->horizontalHeader()->setSortIndicator(this->m_usersVoiceCom1->getSortColumn(), this->m_usersVoiceCom1->getSortOrder());
    this->ui->tv_CockpitVoiceRoom1->resizeColumnsToContents();
    this->ui->tv_CockpitVoiceRoom1->resizeRowsToContents();
    this->ui->tv_CockpitVoiceRoom1->horizontalHeader()->setStretchLastSection(true);

    this->ui->tv_CockpitVoiceRoom2->setSortingEnabled(true);
    this->ui->tv_CockpitVoiceRoom2->setModel(this->m_usersVoiceCom2);
    this->m_usersVoiceCom2->setSortColumnByPropertyIndex(BlackMisc::Network::CUser::IndexRealName);
    if (this->m_usersVoiceCom1->hasValidSortColumn())
        this->ui->tv_CockpitVoiceRoom2->horizontalHeader()->setSortIndicator(this->m_usersVoiceCom2->getSortColumn(), this->m_usersVoiceCom2->getSortOrder());
    this->ui->tv_CockpitVoiceRoom2->resizeColumnsToContents();
    this->ui->tv_CockpitVoiceRoom2->resizeRowsToContents();
    this->ui->tv_CockpitVoiceRoom2->horizontalHeader()->setStretchLastSection(true);

    // timer
    if (this->m_timerUpdateAircraftsInRange == nullptr) this->m_timerUpdateAircraftsInRange = new QTimer(this);
    if (this->m_timerUpdateAtcStationsOnline == nullptr) this->m_timerUpdateAtcStationsOnline = new QTimer(this);
    if (this->m_timerUpdateUsers == nullptr) this->m_timerUpdateUsers = new QTimer(this);
    if (this->m_timerContextWatchdog == nullptr) this->m_timerContextWatchdog = new QTimer(this);
    if (this->m_timerCollectedCockpitUpdates == nullptr) this->m_timerCollectedCockpitUpdates = new QTimer(this);

    // context
    if (this->m_coreMode != GuiModes::CoreInGuiProcess)
    {
        this->m_dBusConnection = QDBusConnection::sessionBus();
        this->m_contextNetwork = new BlackCore::IContextNetwork(BlackCore::CDBusServer::ServiceName, this->m_dBusConnection, this);
        this->m_contextVoice = new BlackCore::IContextVoice(BlackCore::CDBusServer::ServiceName, this->m_dBusConnection, this);
        this->m_contextSettings = new BlackCore::IContextSettings(BlackCore::CDBusServer::ServiceName, this->m_dBusConnection, this);
        this->m_contextApplication = new BlackCore::IContextApplication(BlackCore::CDBusServer::ServiceName, this->m_dBusConnection, this);
    }
    else
    {
        this->m_coreRuntime.reset(new CCoreRuntime(false, this));
        this->m_contextNetwork = this->m_coreRuntime->getIContextNetwork();
        this->m_contextVoice = this->m_coreRuntime->getIContextVoice();
        this->m_contextSettings = this->m_coreRuntime->getIContextSettings();
        this->m_contextApplication = this->m_coreRuntime->getIContextApplication();
    }

    // wire GUI signals
    this->initGuiSignals();

    // images
    this->m_resPixmapConnectionConnected = QPixmap(":/blackgui/icons/logingreen.png");
    this->m_resPixmapConnectionDisconnected = QPixmap(":/blackgui/icons/loginred.png");
    this->m_resPixmapConnectionConnecting = QPixmap(":/blackgui/icons/loginyellow.png");
    this->m_resPixmapVoiceLow = QPixmap(":/blackgui/icons/audiovolumelow.png");
    this->m_resPixmapVoiceHigh = QPixmap(":/blackgui/icons/audiovolumehigh.png");
    this->m_resPixmapVoiceMuted = QPixmap(":/blackgui/icons/audiovolumemuted.png");

    // signal / slots
    bool connect;
    this->connect(this->m_contextNetwork, &IContextNetwork::statusMessage, this, &MainWindow::displayStatusMessage);
    this->connect(this->m_contextNetwork, &IContextNetwork::statusMessages, this, &MainWindow::displayStatusMessages);
    this->connect(this->m_contextNetwork, &IContextNetwork::connectionTerminated, this, &MainWindow::connectionTerminated);
    this->connect(this->m_contextNetwork, &IContextNetwork::connectionStatusChanged, this, &MainWindow::connectionStatusChanged);
    this->connect(this->m_contextSettings, &IContextSettings::changedNetworkSettings, this, &MainWindow::changedNetworkSettings);
    connect = this->connect(this->m_contextNetwork, SIGNAL(textMessagesReceived(BlackMisc::Network::CTextMessageList)), this, SLOT(appendTextMessagesToGui(BlackMisc::Network::CTextMessageList)));
    Q_ASSERT(connect);
    this->connect(this->m_timerUpdateAircraftsInRange, &QTimer::timeout, this, &MainWindow::timerBasedUpdates);
    this->connect(this->m_timerUpdateAtcStationsOnline, &QTimer::timeout, this, &MainWindow::timerBasedUpdates);
    this->connect(this->m_timerUpdateUsers, &QTimer::timeout, this, &MainWindow::timerBasedUpdates);
    this->connect(this->m_timerContextWatchdog, &QTimer::timeout, this, &MainWindow::timerBasedUpdates);
    this->connect(this->m_timerCollectedCockpitUpdates, &QTimer::timeout, this, &MainWindow::sendCockpitUpdates);

    // start timers, update timers will be started when network is connected
    this->m_timerContextWatchdog->start(2 * 1000);

    // init availability
    this->setContextAvailability();

    // voice panel
    this->setAudioDeviceLists();

    // data
    this->initialDataReads();

    // start screen
    this->setMainPage(true);

    // init context menus
    this->initContextMenus();

    // do this as last statement, so it can be used as flag
    // whether init has been completed
    this->m_init = true;
}

//
// GUI
//
void MainWindow::initGuiSignals()
{
    bool connected;

    // Remark: With new style, only methods of same signature can be connected
    // This is why we still have some "old" SIGNAL/SLOT connections here

    // MAIN buttons
    this->connect(this->ui->sw_MainMiddle, &QStackedWidget::currentChanged, this, &MainWindow::middlePanelChanged);
    connected = this->connect(this->ui->pb_MainAircrafts, SIGNAL(released()), this, SLOT(setMainPage()));
    Q_ASSERT(connected);
    connected = this->connect(this->ui->pb_MainAtc, SIGNAL(released()), this, SLOT(setMainPage()));
    Q_ASSERT(connected);
    connected = this->connect(this->ui->pb_MainCockpit, SIGNAL(released()), this, SLOT(setMainPage()));
    Q_ASSERT(connected);
    connected = this->connect(this->ui->pb_MainConnect, SIGNAL(released()), this, SLOT(setMainPage()));
    Q_ASSERT(connected);
    connected = this->connect(this->ui->pb_MainConnect, SIGNAL(released()), this, SLOT(toggleNetworkConnection()));
    Q_ASSERT(connected);
    connected = this->connect(this->ui->pb_MainFlightplan, SIGNAL(released()), this, SLOT(setMainPage()));
    Q_ASSERT(connected);
    connected = this->connect(this->ui->pb_MainSettings, SIGNAL(released()), this, SLOT(setMainPage()));
    Q_ASSERT(connected);
    connected = this->connect(this->ui->pb_MainStatus, SIGNAL(released()), this, SLOT(setMainPage()));
    Q_ASSERT(connected);
    connected = this->connect(this->ui->pb_MainUsers, SIGNAL(released()), this, SLOT(setMainPage()));
    Q_ASSERT(connected);
    connected = this->connect(this->ui->pb_MainTextMessages, SIGNAL(released()), this, SLOT(setMainPage()));
    Q_ASSERT(connected);
    connected = this->connect(this->ui->pb_MainWeather, SIGNAL(released()), this, SLOT(setMainPage()));
    Q_ASSERT(connected);
    connected = this->connect(this->ui->pb_MainKeypadOpacity050, SIGNAL(clicked()), this, SLOT(changeWindowOpacity()));
    Q_ASSERT(connected);
    connected = this->connect(this->ui->pb_MainKeypadOpacity100, SIGNAL(clicked()), this, SLOT(changeWindowOpacity()));
    Q_ASSERT(connected);

    // Sound buttons
    this->connect(this->ui->pb_SoundMute, &QPushButton::clicked, this, &MainWindow::audioVolumes);
    this->connect(this->ui->pb_SoundMaxVolume, &QPushButton::clicked, this, &MainWindow::audioVolumes);
    connected = this->connect(this->ui->di_CockpitCom1Volume, &QDial::valueChanged, this, &MainWindow::audioVolumes);
    connected = this->connect(this->ui->di_CockpitCom2Volume, &QDial::valueChanged, this, &MainWindow::audioVolumes);

    // menu
    this->connect(this->ui->menu_ReloadSettings, &QAction::triggered, this, &MainWindow::menuClicked);
    this->connect(this->ui->menu_TestLocationsEDDF, &QAction::triggered, this, &MainWindow::menuClicked);
    this->connect(this->ui->menu_TestLocationsEDDM, &QAction::triggered, this, &MainWindow::menuClicked);
    this->connect(this->ui->menu_TestLocationsEDNX, &QAction::triggered, this, &MainWindow::menuClicked);
    this->connect(this->ui->menu_TestLocationsEDRY, &QAction::triggered, this, &MainWindow::menuClicked);
    this->connect(this->ui->menu_FileClose, &QAction::triggered, this, &MainWindow::menuClicked);

    // command line
    this->connect(this->ui->le_CommandLineInput, &QLineEdit::returnPressed, this, &MainWindow::commandEntered);

    // cockpit
    connected = this->connect(this->ui->cb_CockpitTransponderMode, SIGNAL(currentIndexChanged(QString)), this, SLOT(cockpitValuesChanged()));
    Q_ASSERT(connected);
    this->connect(this->ui->ds_CockpitCom1Active, &QDoubleSpinBox::editingFinished, this, &MainWindow::cockpitValuesChanged);
    this->connect(this->ui->ds_CockpitCom2Active, &QDoubleSpinBox::editingFinished, this, &MainWindow::cockpitValuesChanged);
    this->connect(this->ui->ds_CockpitCom1Standby, &QDoubleSpinBox::editingFinished, this, &MainWindow::cockpitValuesChanged);
    this->connect(this->ui->ds_CockpitCom2Standby, &QDoubleSpinBox::editingFinished, this, &MainWindow::cockpitValuesChanged);
    this->connect(this->ui->ds_CockpitTransponder, &QDoubleSpinBox::editingFinished, this, &MainWindow::cockpitValuesChanged);

    this->connect(this->ui->cb_CockpitVoiceRoom1Override, &QCheckBox::clicked, this, &MainWindow::setAudioVoiceRooms);
    this->connect(this->ui->cb_CockpitVoiceRoom2Override, &QCheckBox::clicked, this, &MainWindow::setAudioVoiceRooms);
    this->connect(this->ui->le_CockpitVoiceRoomCom1, &QLineEdit::returnPressed, this, &MainWindow::setAudioVoiceRooms);
    this->connect(this->ui->le_CockpitVoiceRoomCom2, &QLineEdit::returnPressed, this, &MainWindow::setAudioVoiceRooms);
    this->connect(this->ui->pb_CockpitToggleCom1, &QPushButton::clicked, this, &MainWindow::cockpitValuesChanged);
    this->connect(this->ui->pb_CockpitToggleCom2, &QPushButton::clicked, this, &MainWindow::cockpitValuesChanged);
    this->connect(this->ui->pb_CockpitIdent, &QPushButton::clicked, this, &MainWindow::cockpitValuesChanged);

    // voice
    connected = this->connect(this->ui->cb_VoiceInputDevice, SIGNAL(currentIndexChanged(int)), this, SLOT(audioDeviceSelected(int)));
    Q_ASSERT(connected);
    connected = this->connect(this->ui->cb_VoiceOutputDevice, SIGNAL(currentIndexChanged(int)), this, SLOT(audioDeviceSelected(int)));
    Q_ASSERT(connected);

    // ATC
    connected = this->connect(this->ui->le_AtcStationsOnlineMetar, SIGNAL(returnPressed()), this, SLOT(getMetar()));
    Q_ASSERT(connected);
    connected = this->connect(this->ui->pb_AtcStationsLoadMetar, SIGNAL(clicked()), this, SLOT(getMetar()));
    Q_ASSERT(connected);
    this->connect(this->ui->tw_AtcStations, &QTabWidget::currentChanged, this, &MainWindow::atcStationTabChanged);
    this->connect(this->ui->pb_ReloadAtcStationsBooked, &QPushButton::clicked, this, &MainWindow::reloadAtcStationsBooked);
    this->connect(this->ui->tv_AtcStationsOnline, &QTableView::clicked, this, &MainWindow::onlineAtcStationSelected);

    // Settings server
    this->connect(this->ui->pb_SettingsTnCurrentServer, &QPushButton::released, this, &MainWindow::alterTrafficServer);
    this->connect(this->ui->pb_SettingsTnRemoveServer, &QPushButton::released, this, &MainWindow::alterTrafficServer);
    this->connect(this->ui->pb_SettingsTnSaveServer, &QPushButton::released, this, &MainWindow::alterTrafficServer);
    this->connect(this->ui->tv_SettingsTnServers, &QTableView::clicked, this, &MainWindow::networkServerSelected);

    // Settings
    this->connect(this->ui->hs_SettingsGuiOpacity, &QSlider::valueChanged, this, &MainWindow::changeWindowOpacity);

    // no warnings in release build
    Q_UNUSED(connected);
}

/*
 * Init data when started
 */
void MainWindow::initialDataReads()
{
    qint64 t = QDateTime::currentMSecsSinceEpoch();
    this->m_coreAvailable = (this->m_contextNetwork->usingLocalObjects() || (this->m_contextApplication->ping(t) == t));
    if (!this->m_coreAvailable)
    {
        this->displayStatusMessage(CStatusMessage(CStatusMessage::TypeCore, CStatusMessage::SeverityError,
                                   "No initial data read as network context is not available"));
        return;
    }

    this->reloadSettings(); // init read
    this->reloadAtcStationsBooked(); // init read, to do this no traffic network required
    this->reloadOwnAircraft(); // init read, independent of traffic network

    if (this->m_contextNetwork->isConnected())
    {
        // connection is already established
        this->reloadAircraftsInRange();
        this->reloadAllUsers();
        this->reloadAtcStationsOnline();
        this->updateGuiStatusInformation();
    }
}

/*
 * Start update timers
 */
void MainWindow::startUpdateTimers()
{
    this->m_timerUpdateAircraftsInRange->start(this->ui->hs_SettingsGuiAircraftRefreshTime->value() * 1000);
    this->m_timerUpdateAtcStationsOnline->start(this->ui->hs_SettingsGuiAtcRefreshTime->value() * 1000);
    this->m_timerUpdateUsers->start(this->ui->hs_SettingsGuiUserRefreshTime->value() * 1000);
}

/*
 * Stop udate timers
 */
void MainWindow::stopUpdateTimers(bool disconnect)
{
    this->m_timerUpdateAircraftsInRange->stop();
    this->m_timerUpdateAtcStationsOnline->stop();
    this->m_timerUpdateUsers->stop();
    if (!disconnect) return;
    this->disconnect(this->m_timerUpdateAircraftsInRange);
    this->disconnect(this->m_timerUpdateAtcStationsOnline);
    this->disconnect(this->m_timerUpdateUsers);
}


/* Copyright (C) 2016
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackconfig/buildconfig.h"
#include "blackcore/application.h"
#include "blackcore/db/networkwatchdog.h"
#include "blackcore/context/contextnetwork.h"
#include "blackcore/context/contextsimulatorimpl.h"
#include "blackcore/context/contextapplication.h"
#include "blackcore/cookiemanager.h"
#include "blackcore/corefacade.h"
#include "blackcore/vatsim/networkvatlib.h"
#include "blackcore/registermetadata.h"
#include "blackcore/setupreader.h"
#include "blackcore/webdataservices.h"
#include "blackmisc/atomicfile.h"
#include "blackmisc/applicationinfo.h"
#include "blackmisc/datacache.h"
#include "blackmisc/dbusserver.h"
#include "blackmisc/directoryutils.h"
#include "blackmisc/eventloop.h"
#include "blackmisc/filelogger.h"
#include "blackmisc/logcategory.h"
#include "blackmisc/logcategorylist.h"
#include "blackmisc/loghandler.h"
#include "blackmisc/logmessage.h"
#include "blackmisc/logpattern.h"
#include "blackmisc/network/networkutils.h"
#include "blackmisc/registermetadata.h"
#include "blackmisc/settingscache.h"
#include "blackmisc/slot.h"
#include "blackmisc/stringutils.h"
#include "blackmisc/threadutils.h"
#include "blackmisc/verify.h"
#include "application.h"

#include <stdbool.h>
#include <stdio.h>
#include <QCoreApplication>
#include <QDateTime>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QRegularExpression>
#include <QSslSocket>
#include <QStandardPaths>
#include <QStringBuilder>
#include <QStringList>
#include <QTemporaryDir>
#include <QThread>
#include <QTime>
#include <QTimer>
#include <QTranslator>
#include <QWriteLocker>
#include <Qt>
#include <QtGlobal>
#include <QSysInfo>
#include <cstdlib>

#ifdef BLACK_USE_CRASHPAD
#include "crashpad/client/crashpad_client.h"
#include "crashpad/client/crash_report_database.h"
#include "crashpad/client/settings.h"
#include "crashpad/client/simulate_crash.h"
#endif

using namespace BlackConfig;
using namespace BlackMisc;
using namespace BlackMisc::Db;
using namespace BlackMisc::Network;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::Simulation;
using namespace BlackMisc::Weather;
using namespace BlackCore;
using namespace BlackCore::Context;
using namespace BlackCore::Vatsim;
using namespace BlackCore::Data;
using namespace BlackCore::Db;
using namespace crashpad;

BlackCore::CApplication *sApp = nullptr; // set by constructor

//! \private
static const QString &swiftDataRoot()
{
    static const QString path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/org.swift-project/";
    return path;
}

namespace BlackCore
{
    CApplication::CApplication(CApplicationInfo::Application application, bool init) :
        CApplication(executable(), application, init)
    { }

    CApplication::CApplication(const QString &applicationName, CApplicationInfo::Application application, bool init) :
        m_accessManager(new QNetworkAccessManager(this)),
        m_applicationInfo(application),
        m_cookieManager({}, this), m_applicationName(applicationName), m_coreFacadeConfig(CCoreFacadeConfig::allEmpty())
    {
        Q_ASSERT_X(!sApp, Q_FUNC_INFO, "already initialized");
        Q_ASSERT_X(QCoreApplication::instance(), Q_FUNC_INFO, "no application object");

        QCoreApplication::setApplicationName(m_applicationName);
        QCoreApplication::setApplicationVersion(CBuildConfig::getVersionString());
        this->setObjectName(m_applicationName);

        // init skipped when called from CGuiApplication
        if (init)
        {
            this->init(true);
        }
    }

    void CApplication::init(bool withMetadata)
    {
        if (!sApp)
        {
            // notify when app goes down
            connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &CApplication::gracefulShutdown);

            // metadata
            if (withMetadata) { CApplication::registerMetadata(); }

            // unit test
            if (this->getApplicationInfo().application() == CApplicationInfo::UnitTest)
            {
                const QString tempPath(this->getTemporaryDirectory());
                BlackMisc::setMockCacheRootDirectory(tempPath);
            }
            m_alreadyRunning = CApplication::getRunningApplications().containsApplication(CApplication::getSwiftApplication());
            this->initParser();
            this->initLogging();
            this->tagApplicationDataDirectory();

            //
            // cmd line arguments not yet parsed here
            //

            // Translations
            QTranslator translator;
            if (translator.load("blackmisc_i18n_de", ":blackmisc/translations/")) { CLogMessage(this).debug() << "Translator loaded"; }
            QCoreApplication::instance()->installTranslator(&translator);

            // Init network
            sApp = this;
            Q_ASSERT_X(m_accessManager, Q_FUNC_INFO, "Need QAM");
            m_networkWatchDog.reset(new CNetworkWatchdog(this)); // not yet started
            m_cookieManager.setParent(m_accessManager);
            m_accessManager->setCookieJar(&m_cookieManager);
            connect(m_accessManager, &QNetworkAccessManager::networkAccessibleChanged, this, &CApplication::changedInternetAccessibility, Qt::QueuedConnection);
            connect(m_accessManager, &QNetworkAccessManager::networkAccessibleChanged, this, &CApplication::onChangedNetworkAccessibility, Qt::QueuedConnection);
            connect(m_accessManager, &QNetworkAccessManager::networkAccessibleChanged, m_networkWatchDog.data(), &CNetworkWatchdog::onChangedNetworkAccessibility, Qt::QueuedConnection);
            connect(m_networkWatchDog.data(), &CNetworkWatchdog::changedInternetAccessibility, this, &CApplication::onChangedInternetAccessibility, Qt::QueuedConnection);
            connect(m_networkWatchDog.data(), &CNetworkWatchdog::changedSwiftDbAccessibility, this, &CApplication::onChangedSwiftDbAccessibility, Qt::QueuedConnection);
            connect(m_networkWatchDog.data(), &CNetworkWatchdog::changedInternetAccessibility, this, &CApplication::changedInternetAccessibility, Qt::QueuedConnection);
            connect(m_networkWatchDog.data(), &CNetworkWatchdog::changedSwiftDbAccessibility, this, &CApplication::changedSwiftDbAccessibility, Qt::QueuedConnection);

            CLogMessage::preformatted(CNetworkUtils::createNetworkReport(m_accessManager));
            m_networkWatchDog->start(QThread::LowestPriority);
            m_networkWatchDog->startUpdating(10);

            // global setup
            m_setupReader.reset(new CSetupReader(this));
            connect(m_setupReader.data(), &CSetupReader::setupHandlingCompleted, this, &CApplication::setupHandlingIsCompleted, Qt::QueuedConnection);
            connect(m_setupReader.data(), &CSetupReader::updateInfoAvailable, this, &CApplication::updateInfoAvailable, Qt::QueuedConnection);
            connect(m_setupReader.data(), &CSetupReader::successfullyReadSharedUrl, m_networkWatchDog.data(), &CNetworkWatchdog::setWorkingSharedUrl, Qt::QueuedConnection);

            this->addParserOptions(m_setupReader->getCmdLineOptions()); // add options from reader

            // startup done
            connect(this, &CApplication::startUpCompleted, this, &CApplication::onStartUpCompleted, Qt::QueuedConnection);
            connect(this, &CApplication::coreFacadeStarted, this, &CApplication::onCoreFacadeStarted, Qt::QueuedConnection);
        }
    }

    bool CApplication::registerAsRunning()
    {
        //! \fixme KB 2017-11 maybe this code can be encapsulated somewhere
        CApplicationInfoList apps = CApplication::getRunningApplications();
        const CApplicationInfo myself = CApplication::instance()->getApplicationInfo();
        if (!apps.contains(myself)) { apps.insert(myself); }
        const bool ok = CFileUtils::writeStringToLockedFile(apps.toJsonString(), CFileUtils::appendFilePaths(swiftDataRoot(), "apps.json"));
        if (!ok) { CLogMessage(static_cast<CApplication *>(nullptr)).error("Failed to write to application list file"); }
        return ok;
    }

    bool CApplication::unregisterAsRunning()
    {
        //! \fixme KB 2017-11 maybe this code can be encapsulated somewhere
        CApplicationInfoList apps = CApplication::getRunningApplications();
        const CApplicationInfo myself = CApplication::instance()->getApplicationInfo();
        if (!apps.contains(myself)) { return true; }
        apps.remove(myself);
        const bool ok = CFileUtils::writeStringToLockedFile(apps.toJsonString(), CFileUtils::appendFilePaths(swiftDataRoot(), "apps.json"));
        if (!ok) { CLogMessage(static_cast<CApplication *>(nullptr)).error("Failed to write to application list file"); }
        return ok;
    }

    int CApplication::exec()
    {
        Q_ASSERT_X(instance(), Q_FUNC_INFO, "missing application");
        CApplication::registerAsRunning();
        return QCoreApplication::exec();
    }

    void CApplication::restartApplication(const QStringList &newArguments, const QStringList &removeArguments)
    {
        CApplication::unregisterAsRunning();
        const QString prg = QCoreApplication::applicationFilePath();
        const QStringList args = this->argumentsJoined(newArguments, removeArguments);
        this->gracefulShutdown();
        QProcess::startDetached(prg, args);
        this->exit(0);
    }

    CApplication::~CApplication()
    {
        this->gracefulShutdown();
    }

    const CApplicationInfo &CApplication::getApplicationInfo() const
    {
        static const CApplicationInfo a(CApplication::getSwiftApplication());
        return a;
    }

    CApplicationInfoList CApplication::getRunningApplications()
    {
        CApplicationInfoList apps;
        apps.convertFromJsonNoThrow(CFileUtils::readLockedFileToString(swiftDataRoot() + "apps.json"), {}, {});
        apps.removeIf([](const CApplicationInfo & info) { return !info.getProcessInfo().exists(); });
        return apps;
    }

    bool CApplication::isApplicationRunning(CApplicationInfo::Application application)
    {
        const CApplicationInfoList running = CApplication::getRunningApplications();
        return running.containsApplication(application);
    }

    bool CApplication::isAlreadyRunning() const
    {
        return getRunningApplications().containsBy([this](const CApplicationInfo & info) { return info.application() == getSwiftApplication(); });
    }

    bool CApplication::isShuttingDown() const
    {
        return m_shutdown;
    }

    const QString &CApplication::getApplicationNameAndVersion() const
    {
        static const QString s(m_applicationName + " " + CBuildConfig::getVersionString());
        return s;
    }

    const QString &CApplication::getApplicationNameVersionDetailed() const
    {
        static const QString s(m_applicationName + " " + this->versionStringDetailed());
        return s;
    }

    void CApplication::setSingleApplication(bool singleApplication)
    {
        m_singleApplication = singleApplication;
    }

    QString CApplication::getExecutableForApplication(CApplicationInfo::Application application) const
    {
        QString search;
        switch (application)
        {
        case CApplicationInfo::PilotClientCore: search = "core"; break;
        case CApplicationInfo::Laucher: search = "launcher"; break;
        case CApplicationInfo::MappingTool: search = "data"; break;
        case CApplicationInfo::PilotClientGui: search = "gui"; break;
        default: break;
        }
        if (search.isEmpty()) { return ""; }
        for (const QString &executable : CFileUtils::getSwiftExecutables())
        {
            if (!executable.contains("swift", Qt::CaseInsensitive)) { continue; }
            if (executable.contains(search, Qt::CaseInsensitive)) { return executable; }
        }
        return "";
    }

    bool CApplication::startLauncher()
    {
        static const QString launcher = CApplication::getExecutableForApplication(CApplicationInfo::Application::Laucher);
        if (launcher.isEmpty() || CApplication::isApplicationRunning(CApplicationInfo::Laucher)) { return false; }

        const QStringList args = this->argumentsJoined({}, { "--dbus" });
        return QProcess::startDetached(launcher, args);
    }

    bool CApplication::startLauncherAndQuit()
    {
        const bool started = CApplication::startLauncher();
        if (!started) { return false; }
        this->gracefulShutdown();
        CApplication::exit();
        return true;
    }

    CGlobalSetup CApplication::getGlobalSetup() const
    {
        if (m_shutdown) { return CGlobalSetup(); }
        const CSetupReader *r = m_setupReader.data();
        if (!r) { return CGlobalSetup(); }
        return r->getSetup();
    }

    CUpdateInfo CApplication::getUpdateInfo() const
    {
        if (m_shutdown) { return CUpdateInfo(); }
        const CSetupReader *r = m_setupReader.data();
        if (!r) { return CUpdateInfo(); }
        return r->getUpdateInfo();
    }

    CDistribution CApplication::getOwnDistribution() const
    {
        if (CBuildConfig::isLocalDeveloperDebugBuild()) { return CDistribution::localDeveloperBuild(); }
        const CUpdateInfo u = this->getUpdateInfo();
        return u.anticipateOwnDistribution();
    }

    bool CApplication::start()
    {
        m_started = false; // reset

        // parse if needed, parsing contains its own error handling
        if (!m_parsed)
        {
            const bool s = this->parseAndStartupCheck();
            if (!s) { return false; }
        }

        // parsing itself is done
        CStatusMessageList msgs;
        do
        {
            // clear cache?
            if (this->isSet(m_cmdClearCache))
            {
                const QStringList files(CApplication::clearCaches());
                msgs.push_back(
                    CLogMessage(this).debug() << "Cleared cache, " << files.size() << " files"
                );
            }

            // crashpad dump
            if (this->isSet(m_cmdTestCrashpad))
            {
                QTimer::singleShot(10 * 1000, [ = ]
                {
#ifdef BLACK_USE_CRASHPAD
                    CRASHPAD_SIMULATE_CRASH();
#else
                    CLogMessage(this).warning("This compiler or platform does not support crashpad. Cannot simulate crash dump!");
#endif
                });
            }

            //! \fixme KB 9/17 waiting for setup reader here is supposed to be replaced by explicitly waiting for reader
            if (!m_setupReader->isSetupAvailable())
            {
                msgs = this->requestReloadOfSetupAndVersion();
                if (msgs.isFailure()) { break; }
                if (msgs.isSuccess()) { msgs.push_back(this->waitForSetup()); }
            }

            // start hookin
            msgs.push_back(this->startHookIn());
            if (msgs.isFailure()) { break; }

            // trigger loading and saving of settings in appropriate scenarios
            if (m_coreFacadeConfig.getModeApplication() != CCoreFacadeConfig::Remote)
            {
                // facade running here locally
                msgs.push_back(CSettingsCache::instance()->loadFromStore());
                if (msgs.isFailure()) { break; }

                // Settings are distributed via DBus. So only one application is responsible for saving. `enableLocalSave()` means
                // "this is the application responsible for saving". If swiftgui requests a setting to be saved, it is sent to swiftcore and saved by swiftcore.
                CSettingsCache::instance()->enableLocalSave();

                // From this moment on, we have settings, so enable crash handler.
                msgs.push_back(this->initCrashHandler());
            }
        }
        while (false);

        // terminate with failures, otherwise log messages
        if (msgs.isFailure())
        {
            this->cmdLineErrorMessage(msgs);
            return false;
        }
        else if (!msgs.isEmpty())
        {
            CLogMessage::preformatted(msgs);
        }

        m_started = true;
        return m_started;
    }

    CStatusMessageList CApplication::waitForSetup(int timeoutMs)
    {
        if (!m_setupReader) { return CStatusMessage(this).error("No setup reader"); }
        CEventLoop::processEventsUntil(this, &CApplication::setupHandlingCompleted, timeoutMs, [this]
        {
            return m_setupReader->isSetupAvailable();
        });

        // setup handling completed with success or failure, or we run into time out
        if (m_setupReader->isSetupAvailable()) { return CStatusMessage(this).info("Setup available"); }
        CStatusMessageList msgs(CStatusMessage(this).error("Setup not available, setup reading failed or timed out."));
        if (m_setupReader->getLastSetupReadErrorMessages().hasErrorMessages())
        {
            msgs.push_back(m_setupReader->getLastSetupReadErrorMessages());
        }
        if (m_setupReader->hasCmdLineBootstrapUrl())
        {
            msgs.push_back(CStatusMessage(this).info("Bootstrap URL cmd line argument '%1'") << m_setupReader->getCmdLineBootstrapUrl());
        }
        return msgs;
    }

    bool CApplication::isSetupAvailable() const
    {
        if (m_shutdown || !m_setupReader) { return false; }
        return m_setupReader->isSetupAvailable();
    }

    CStatusMessageList CApplication::requestReloadOfSetupAndVersion()
    {
        if (m_shutdown) { return CStatusMessage(this).warning("Shutting down, not reading"); }
        if (!m_setupReader) { return CStatusMessage(this).error("No reader for setup/version"); }
        Q_ASSERT_X(m_parsed, Q_FUNC_INFO, "Not yet parsed");
        return m_setupReader->asyncLoad();
    }

    bool CApplication::hasWebDataServices() const
    {
        if (this->isShuttingDown()) { return false; } // service will not survive for long
        return m_webDataServices;
    }

    CWebDataServices *CApplication::getWebDataServices() const
    {
        // use hasWebDataServices() to test if services are available
        // getting the assert means web services are accessed before the are initialized

        Q_ASSERT_X(m_webDataServices, Q_FUNC_INFO, "Missing web data services, use hasWebDataServices to test if existing");
        return m_webDataServices.data();
    }

    bool CApplication::isApplicationThread() const
    {
        return CThreadUtils::isCurrentThreadApplicationThread();
    }

    const QString &CApplication::versionStringDetailed() const
    {
        if (this->isDeveloperFlagSet() && CBuildConfig::isLocalDeveloperDebugBuild())
        {
            static const QString s(CBuildConfig::getVersionString() + " [dev,DEVDBG]");
            return s;
        }
        if (isDeveloperFlagSet())
        {
            static const QString s(CBuildConfig::getVersionString() + " [dev]");
            return s;
        }
        if (CBuildConfig::isLocalDeveloperDebugBuild())
        {
            static const QString s(CBuildConfig::getVersionString() + " [DEVDBG]");
            return s;
        }
        return CBuildConfig::getVersionString();
    }

    const QString &CApplication::swiftVersionString() const
    {
        static const QString s(QString("swift %1").arg(versionStringDetailed()));
        return s;
    }

    const char *CApplication::swiftVersionChar()
    {
        static const QByteArray a(swiftVersionString().toUtf8());
        return a.constData();
    }

    bool CApplication::initIsRunningInDeveloperEnvironment() const
    {
        //
        // assumption: restricted distributions are development versions
        //

        if (this->getApplicationInfo().isSampleOrUnitTest()) { return true; }
        if (CBuildConfig::isLocalDeveloperDebugBuild()) { return true; }

        const CDistribution d(this->getOwnDistribution());
        if (d.isRestricted() && this->isSet(m_cmdDevelopment)) { return true; }

        // we can globally set a dev.flag
        if (this->isSetupAvailable())
        {
            // assume value from setup
            return this->getGlobalSetup().isDevelopment();
        }
        return false;
    }

    bool CApplication::hasUnsavedSettings() const
    {
        return !this->getUnsavedSettingsKeys().isEmpty();
    }

    void CApplication::saveSettingsOnShutdown(bool saveSettings)
    {
        m_saveSettingsOnShutdown = saveSettings;
    }

    QStringList CApplication::getUnsavedSettingsKeys() const
    {
        return this->supportsContexts() ?
               this->getIContextApplication()->getUnsavedSettingsKeys() :
               CSettingsCache::instance()->getAllUnsavedKeys();
    }

    CStatusMessage CApplication::saveSettingsByKey(const QStringList &keys)
    {
        if (keys.isEmpty()) { return CStatusMessage(); }
        return this->supportsContexts() ?
               this->getIContextApplication()->saveSettingsByKey(keys) :
               CSettingsCache::instance()->saveToStore(keys);
    }

    QString CApplication::getTemporaryDirectory() const
    {
        static const QTemporaryDir tempDir;
        if (tempDir.isValid()) { return tempDir.path(); }
        return QDir::tempPath();
    }

    QString CApplication::getInfoString(const QString &separator) const
    {
        QString str =
            CBuildConfig::getVersionString() %
            QStringLiteral(" ") % (CBuildConfig::isReleaseBuild() ? QStringLiteral("Release build") : QStringLiteral("Debug build")) %
            separator %
            QStringLiteral("Local dev.dbg.: ") %
            boolToYesNo(CBuildConfig::isLocalDeveloperDebugBuild()) %
            separator %
            QStringLiteral("dev.env.: ") %
            boolToYesNo(this->isDeveloperFlagSet()) %
            separator %
            QStringLiteral("distribution: ") %
            this->getOwnDistribution().toQString(true) %
            separator %
            QStringLiteral("Windows NT: ") %
            boolToYesNo(CBuildConfig::isRunningOnWindowsNtPlatform()) %
            QStringLiteral(" Windows 10: ") %
            boolToYesNo(CBuildConfig::isRunningOnWindows10()) %
            separator %
            QStringLiteral("Linux: ") %
            boolToYesNo(CBuildConfig::isRunningOnLinuxPlatform()) %
            QStringLiteral(" Unix: ") %
            boolToYesNo(CBuildConfig::isRunningOnUnixPlatform()) %
            separator %
            QStringLiteral("MacOS: ") %
            boolToYesNo(CBuildConfig::isRunningOnMacOSPlatform()) %
            separator %
            QStringLiteral("Build Abi: ") %
            QSysInfo::buildAbi() %
            separator %
            QStringLiteral("Build CPU: ") %
            QSysInfo::buildCpuArchitecture() %
            separator %
            CBuildConfig::compiledWithInfo(false);

        if (this->supportsContexts())
        {
            str += (separator % QStringLiteral("Supporting contexts"));
            if (this->getIContextNetwork())
            {
                str += (separator % this->getIContextNetwork()->getLibraryInfo(true));
            }
        }

        return str;
    }

    QNetworkReply *CApplication::getFromNetwork(const CUrl &url, const CSlot<void(QNetworkReply *)> &callback, int maxRedirects)
    {
        return getFromNetwork(url.toNetworkRequest(), NoLogRequestId, callback, maxRedirects);
    }

    QNetworkReply *CApplication::getFromNetwork(const CUrl &url, int logId, const CSlot<void(QNetworkReply *)> &callback, int maxRedirects)
    {
        return getFromNetwork(url.toNetworkRequest(), logId, callback, maxRedirects);
    }

    QNetworkReply *CApplication::getFromNetwork(const QNetworkRequest &request, const CSlot<void(QNetworkReply *)> &callback, int maxRedirects)
    {
        return getFromNetwork(request, NoLogRequestId, callback, maxRedirects);
    }

    QNetworkReply *CApplication::getFromNetwork(const QNetworkRequest &request, int logId, const CSlot<void (QNetworkReply *)> &callback, int maxRedirects)
    {
        return httpRequestImpl(request, logId, callback, maxRedirects, [](QNetworkAccessManager & qam, const QNetworkRequest & request)
        {
            QNetworkReply *nr = qam.get(request);
            return nr;
        });
    }

    QNetworkReply *CApplication::postToNetwork(const QNetworkRequest &request, int logId, const QByteArray &data, const CSlot<void(QNetworkReply *)> &callback)
    {
        return httpRequestImpl(request, logId, callback, NoRedirects, [ data ](QNetworkAccessManager & qam, const QNetworkRequest & request)
        {
            QNetworkReply *nr = qam.post(request, data);
            return nr;
        });
    }

    QNetworkReply *CApplication::postToNetwork(const QNetworkRequest &request, int logId, QHttpMultiPart *multiPart, const CSlot<void(QNetworkReply *)> &callback)
    {
        if (!this->isNetworkAccessible()) { return nullptr; }
        if (QThread::currentThread() != m_accessManager->thread())
        {
            multiPart->moveToThread(m_accessManager->thread());
        }

        return httpRequestImpl(request, logId, callback, NoRedirects, [ this, multiPart ](QNetworkAccessManager & qam, const QNetworkRequest & request)
        {
            QNetworkReply *nr = qam.post(request, multiPart);
            multiPart->setParent(nr);
            return nr;
        });
    }

    QNetworkReply *CApplication::headerFromNetwork(const CUrl &url, const CSlot<void (QNetworkReply *)> &callback, int maxRedirects)
    {
        return headerFromNetwork(url.toNetworkRequest(), callback, maxRedirects);
    }

    QNetworkReply *CApplication::headerFromNetwork(const QNetworkRequest &request, const CSlot<void (QNetworkReply *)> &callback, int maxRedirects)
    {
        return httpRequestImpl(request, NoLogRequestId, callback, maxRedirects, [ ](QNetworkAccessManager & qam, const QNetworkRequest & request) { return qam.head(request); });
    }

    QNetworkReply *CApplication::downloadFromNetwork(const CUrl &url, const QString &saveAsFileName, const BlackMisc::CSlot<void (const CStatusMessage &)> &callback, int maxRedirects)
    {
        // upfront checks
        if (url.isEmpty()) { return nullptr; }
        if (saveAsFileName.isEmpty()) { return nullptr; }
        const QFileInfo fi(saveAsFileName);
        if (!fi.dir().exists()) { return nullptr; }

        CSlot<void (QNetworkReply *)> slot([ = ](QNetworkReply * reply)
        {
            QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> nwReply(reply);
            CStatusMessage msg;
            if (reply->error() != QNetworkReply::NoError)
            {
                msg = CStatusMessage(this, CStatusMessage::SeverityError, "Download for '%1' failed: '%2'") <<  url.getFullUrl() << nwReply->errorString();
            }
            else
            {
                const bool ok = CFileUtils::writeByteArrayToFile(reply->readAll(), saveAsFileName);
                msg = ok ?
                      CStatusMessage(this, CStatusMessage::SeverityInfo, "Saved file '%1' downloaded from '%2'") << saveAsFileName << url.getFullUrl() :
                      CStatusMessage(this, CStatusMessage::SeverityError, "Saving file '%1' downloaded from '%2' failed") << saveAsFileName << url.getFullUrl();
            }
            nwReply->close();
            QTimer::singleShot(0, callback.object(), [ = ] { callback(msg); });
        });
        slot.setObject(this); // object for thread
        QNetworkReply *reply = this->getFromNetwork(url, slot, maxRedirects);
        return reply;
    }

    void CApplication::deleteAllCookies()
    {
        m_cookieManager.deleteAllCookies();
    }

    CNetworkWatchdog *CApplication::getNetworkWatchdog() const
    {
        return m_networkWatchDog.data();
    }

    void CApplication::setSwiftDbAccessibility(bool accessible)
    {
        if (!m_networkWatchDog) { return; }
        m_networkWatchDog->setDbAccessibility(accessible);
    }

    int CApplication::triggerNetworkChecks()
    {
        if (!m_networkWatchDog) { return -1; }
        return m_networkWatchDog->triggerCheck();
    }

    bool CApplication::isNetworkAccessible() const
    {
        if (!m_accessManager) { return false; }
        const QNetworkAccessManager::NetworkAccessibility a = m_accessManager->networkAccessible();
        if (a == QNetworkAccessManager::Accessible) { return true; }

        // currently I also accept unknown because of that issue with Network Manager
        return a == QNetworkAccessManager::UnknownAccessibility;
    }

    bool CApplication::isInternetAccessible() const
    {
        if (!this->isNetworkAccessible()) { return false; }
        return m_networkWatchDog && m_networkWatchDog->isInternetAccessible();
    }

    bool CApplication::isSwiftDbAccessible() const
    {
        if (!this->isNetworkAccessible()) { return false; }
        return m_networkWatchDog && m_networkWatchDog->isSwiftDbAccessible();
    }

    bool CApplication::hasWorkingSharedUrl() const
    {
        if (!this->isNetworkAccessible()) { return false; }
        return m_networkWatchDog && m_networkWatchDog->hasWorkingSharedUrl();
    }

    CUrl CApplication::getWorkingSharedUrl() const
    {
        if (!m_networkWatchDog || !this->isNetworkAccessible()) { return CUrl(); }
        return m_networkWatchDog->getWorkingSharedUrl();
    }

    void CApplication::exit(int retcode)
    {
        if (sApp) { instance()->gracefulShutdown(); }

        // when the event loop is not running, this does nothing
        QCoreApplication::exit(retcode);
    }

    QStringList CApplication::arguments()
    {
        return QCoreApplication::arguments();
    }

    int CApplication::indexOfCommandLineOption(const QCommandLineOption &option, const QStringList &args)
    {
        const QStringList names = option.names();
        if (names.isEmpty() || args.isEmpty()) { return -1; }
        int i = -1;
        for (const QString &arg : args)
        {
            i++;
            QString a;
            if (arg.startsWith("--")) { a = arg.mid(2); }
            else if (arg.startsWith("-")) { a = arg.mid(1); }
            else { continue; }

            if (names.contains(a, Qt::CaseInsensitive)) { return i; }
        }
        return -1;
    }

    void CApplication::argumentsWithoutOption(const QCommandLineOption &option, QStringList &args)
    {
        const int index = indexOfCommandLineOption(option, args);
        if (index < 0) { return; }

        // remove argument and its value
        args.removeAt(index);
        if (!option.valueName().isEmpty() && args.size() > index) { args.removeAt(index); }
    }

    void CApplication::processEventsFor(int milliseconds)
    {
        // sApp check allows to use it in test cases without sApp
        if (sApp && sApp->isShuttingDown()) { return; }
        QEventLoop eventLoop;
        QTimer::singleShot(milliseconds, &eventLoop, &QEventLoop::quit);
        connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, &eventLoop, &QEventLoop::quit);
        eventLoop.exec();
    }

    CStatusMessageList CApplication::useContexts(const CCoreFacadeConfig &coreConfig)
    {
        Q_ASSERT_X(m_parsed, Q_FUNC_INFO, "Call this function after parsing");

        m_useContexts = true;
        m_coreFacadeConfig = coreConfig;

        // if not yet initialized, init web data services
        if (!m_useWebData)
        {
            const CStatusMessageList msgs = this->useWebDataServices(CWebReaderFlags::AllReaders, CDatabaseReaderConfigList::forPilotClient());
            if (msgs.hasErrorMessages()) { return msgs; }
        }
        return this->startCoreFacadeAndWebDataServices(); // will do nothing if setup is not yet loaded
    }

    CStatusMessageList CApplication::useWebDataServices(const CWebReaderFlags::WebReader webReaders, const CDatabaseReaderConfigList &dbReaderConfig)
    {
        Q_ASSERT_X(m_webDataServices.isNull(), Q_FUNC_INFO, "Services already started");
        BLACK_VERIFY_X(QSslSocket::supportsSsl(), Q_FUNC_INFO, "No SSL");
        if (!QSslSocket::supportsSsl())
        {
            return CStatusMessage(this).error("No SSL supported, can`t be used");
        }

        m_webReadersUsed = webReaders;
        m_dbReaderConfig = dbReaderConfig;
        m_useWebData = true;
        return this->startWebDataServices();
    }

    CStatusMessageList CApplication::startCoreFacadeAndWebDataServices()
    {
        Q_ASSERT_X(m_parsed, Q_FUNC_INFO, "Call this function after parsing");

        if (!m_useContexts) { return CStatusMessage(this).error("No need to start core facade"); } // we do not use context, so no need to startup
        if (!m_setupReader || !m_setupReader->isSetupAvailable()) { return CStatusMessage(this).error("No setup reader or setup available"); }

        Q_ASSERT_X(m_coreFacade.isNull(), Q_FUNC_INFO, "Cannot alter facade");
        Q_ASSERT_X(m_setupReader, Q_FUNC_INFO, "No facade without setup possible");
        Q_ASSERT_X(m_useWebData, Q_FUNC_INFO, "Need web data services");

        this->startWebDataServices();

        const CStatusMessageList msgs(CStatusMessage(this).info("Will start core facade now"));
        m_coreFacade.reset(new CCoreFacade(m_coreFacadeConfig));
        emit this->coreFacadeStarted();
        return msgs;
    }

    CStatusMessageList CApplication::startWebDataServices()
    {
        Q_ASSERT_X(m_parsed, Q_FUNC_INFO, "Call this function after parsing");

        if (!m_useWebData) { return CStatusMessage(this).warning("No need to start web data services"); }
        if (!m_setupReader || !m_setupReader->isSetupAvailable()) { return CStatusMessage(this).error("No setup reader or setup available"); }

        Q_ASSERT_X(m_setupReader, Q_FUNC_INFO, "No web data services without setup possible");
        CStatusMessageList msgs;
        if (!m_webDataServices)
        {
            msgs.push_back(CStatusMessage(this).info("Will start web data services now"));
            m_webDataServices.reset(
                new CWebDataServices(m_webReadersUsed, m_dbReaderConfig, {}, this)
            );
            Q_ASSERT_X(m_webDataServices, Q_FUNC_INFO, "Missing web services");

            // caches from local files (i.e. the files delivered)
            if (this->isInstallerOptionSet())
            {
                const QDateTime ts = m_webDataServices->getLatestDbEntityCacheTimestamp();
                if (!ts.isValid() || ts < QDateTime::currentDateTimeUtc().addYears(-2))
                {
                    // we only init, if there are:
                    // a) no cache timestamps b) or it was not updated for some years
                    msgs.push_back(m_webDataServices->initDbCachesFromLocalResourceFiles(false));
                }
            }

            // watchdog
            if (m_networkWatchDog)
            {
                connect(m_webDataServices.data(), &CWebDataServices::swiftDbDataRead, m_networkWatchDog.data(), &CNetworkWatchdog::setDbAccessibility);
            }

            emit this->webDataServicesStarted(true);
        }
        else
        {
            msgs.push_back(CStatusMessage(this).info("Web data services already running"));
        }

        return msgs;
    }

    void CApplication::initLogging()
    {
        CLogHandler::instance()->install(); // make sure we have a log handler!

        // File logger
        m_fileLogger.reset(new CFileLogger(executable(), CDirectoryUtils::logDirectory()));
        m_fileLogger->changeLogPattern(CLogPattern().withSeverityAtOrAbove(CStatusMessage::SeverityDebug));
    }

    void CApplication::initParser()
    {
        m_parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
        m_parser.setApplicationDescription(m_applicationName);
        m_cmdHelp = m_parser.addHelpOption();
        m_cmdVersion = m_parser.addVersionOption();
        m_allOptions.append(m_cmdHelp);
        m_allOptions.append(m_cmdVersion);

        // dev. system
        m_cmdDevelopment = QCommandLineOption({ "dev", "development" },
                                              QCoreApplication::translate("application", "Dev. system features?"));
        this->addParserOption(m_cmdDevelopment);

        // can read a local bootstrap file
        m_cmdSharedDir = QCommandLineOption({ "shared", "shareddir" },
                                            QCoreApplication::translate("application", "Local shared directory."),
                                            "shared");
        this->addParserOption(m_cmdSharedDir);

        // reset caches upfront
        m_cmdClearCache = QCommandLineOption({ "ccache", "clearcache" },
                                             QCoreApplication::translate("application", "Clear (reset) the caches."));
        this->addParserOption(m_cmdClearCache);

        // test crashpad upload
        m_cmdTestCrashpad = QCommandLineOption({ "testcp", "testcrashpad" },
                                               QCoreApplication::translate("application", "Simulate crashpad situation."));
        this->addParserOption(m_cmdTestCrashpad);
    }

    bool CApplication::isSet(const QCommandLineOption &option) const
    {
        return (m_parser.isSet(option));
    }

    void CApplication::registerMetadata()
    {
        BlackMisc::registerMetadata();
        BlackCore::registerMetadata();
    }

    QStringList CApplication::clearCaches()
    {
        const QStringList files(CDataCache::instance()->enumerateStore());
        CDataCache::instance()->clearAllValues();
        return files;
    }

    void CApplication::gracefulShutdown()
    {
        if (m_shutdown) { return; }
        if (m_shutdownInProgress) { return; }
        m_shutdownInProgress = true;

        // info that we will shutdown
        emit this->aboutToShutdown();

        // before marked as shutdown, otherwise URL
        if (m_networkWatchDog)
        {
            m_networkWatchDog->gracefulShutdown();
        }

        // mark as shutdown
        m_shutdown = true;

        // save settings (but only when application was really alive)
        if (m_parsed && m_saveSettingsOnShutdown)
        {
            const CStatusMessage m = this->supportsContexts() ?
                                     this->getIContextApplication()->saveSettings() :
                                     CSettingsCache::instance()->saveToStore();
            CLogMessage(getLogCategories()).preformatted(m);
        }

        // from here on we really rip apart the application object
        // and it should no longer be used
        sApp = nullptr;
        disconnect(this);

        if (this->supportsContexts())
        {
            // clean up facade
            m_coreFacade->gracefulShutdown();
            m_coreFacade.reset();
        }

        if (m_webDataServices)
        {
            m_webDataServices->gracefulShutdown();
            m_webDataServices.reset();
        }

        if (m_setupReader)
        {
            m_setupReader->gracefulShutdown();
            m_setupReader.reset();
        }

        if (m_networkWatchDog)
        {
            m_networkWatchDog->quitAndWait();
            m_networkWatchDog.reset();
        }

        m_fileLogger->close();
    }

    void CApplication::setupHandlingIsCompleted(bool available)
    {
        if (available)
        {
            // start follow ups when setup is avaialable
            const CStatusMessageList msgs = this->asyncWebAndContextStart();
            m_started = msgs.isSuccess();
        }

        emit this->setupHandlingCompleted(available);

        if (m_signalStartup)
        {
            emit this->startUpCompleted(m_started);
        }
    }

    void CApplication::onStartUpCompleted()
    {
        // void
    }

    void CApplication::onChangedNetworkAccessibility(QNetworkAccessManager::NetworkAccessibility accessible)
    {
        switch (accessible)
        {
        case QNetworkAccessManager::Accessible:
            m_accessManager->setNetworkAccessible(accessible); // for some reasons the queried value still is unknown
            CLogMessage(this).info("Network is accessible");
            break;
        case QNetworkAccessManager::NotAccessible:
            CLogMessage(this).error("Network not accessible");
            break;
        default:
            CLogMessage(this).warning("Network accessibility unknown");
            break;
        }
    }

    void CApplication::onChangedInternetAccessibility(bool accessible)
    {
        if (accessible)
        {
            CLogMessage(this).info("Internet reported accessible");
        }
        else
        {
            CLogMessage(this).warning("Internet not accessible");
        }
    }

    void CApplication::onChangedSwiftDbAccessibility(bool accessible, const CUrl &url)
    {
        if (accessible)
        {
            CLogMessage(this).info("swift DB reported accessible: '%1'") << url.toQString();
        }
        else
        {
            CLogMessage(this).warning("swift DB not accessible: '%1'") << url.toQString();
            if (m_networkWatchDog)
            {
                CLogMessage(this).warning(m_networkWatchDog->getCheckInfo());
            }
        }
    }

    CStatusMessageList CApplication::asyncWebAndContextStart()
    {
        if (m_started) { return CStatusMessage(this).info("Already started "); }

        // follow up startups
        CStatusMessageList msgs = this->startWebDataServices();
        if (msgs.isFailure()) return msgs;
        msgs.push_back(this->startCoreFacadeAndWebDataServices());
        return msgs;
    }

    void CApplication::severeStartupProblem(const CStatusMessage &message)
    {
        CLogMessage::preformatted(message);
        this->cmdLineErrorMessage(message.getMessage());
        this->exit(EXIT_FAILURE);

        // if I get here the event loop was not yet running
        std::exit(EXIT_FAILURE);
    }

    CApplication *BlackCore::CApplication::instance()
    {
        return sApp;
    }

    const QString &CApplication::executable()
    {
        static const QString e(QFileInfo(QCoreApplication::applicationFilePath()).completeBaseName());
        return e;
    }

    const BlackMisc::CLogCategoryList &CApplication::getLogCategories()
    {
        static const CLogCategoryList l({ CLogCategory("swift.application"), CLogCategory("swift." + executable())});
        return l;
    }

    // ---------------------------------------------------------------------------------
    // Parsing
    // ---------------------------------------------------------------------------------

    bool CApplication::addParserOption(const QCommandLineOption &option)
    {
        m_allOptions.append(option);
        return m_parser.addOption(option);
    }

    bool CApplication::addParserOptions(const QList<QCommandLineOption> &options)
    {
        m_allOptions.append(options);
        return m_parser.addOptions(options);
    }

    void CApplication::addDBusAddressOption()
    {
        m_cmdDBusAddress = QCommandLineOption({ "dbus", "dbusaddress" },
                                              QCoreApplication::translate("application", "DBus address (session, system, P2P IP e.g. 192.168.23.5)"),
                                              "dbusaddress");
        this->addParserOption(m_cmdDBusAddress);
    }

    void CApplication::addVatlibOptions()
    {
        this->addParserOptions(CNetworkVatlib::getCmdLineOptions());
    }

    QString CApplication::getCmdDBusAddressValue() const
    {
        if (!this->isParserOptionSet(m_cmdDBusAddress)) { return ""; }
        const QString v(this->getParserValue(m_cmdDBusAddress));
        const QString dBusAddress(CDBusServer::normalizeAddress(v));
        return dBusAddress;
    }

    QString CApplication::getCmdSwiftPrivateSharedDir() const
    {
        return m_parser.value(m_cmdSharedDir);
    }

    bool CApplication::isParserOptionSet(const QString &option) const
    {
        return m_parser.isSet(option);
    }

    bool CApplication::isInstallerOptionSet() const
    {
        return this->isParserOptionSet("installer");
    }

    bool CApplication::isParserOptionSet(const QCommandLineOption &option) const
    {
        return m_parser.isSet(option);
    }

    QString CApplication::getParserValue(const QString &option) const
    {
        return m_parser.value(option).trimmed();
    }

    QString CApplication::getParserValue(const QCommandLineOption &option) const
    {
        return m_parser.value(option).trimmed();
    }

    bool CApplication::parseAndStartupCheck()
    {
        if (m_parsed) { return m_parsed; } // already done

        // checks
        if (CBuildConfig::isLifetimeExpired())
        {
            this->cmdLineErrorMessage("Program expired " + CBuildConfig::getEol().toString());
            return false;
        }

        const QStringList verifyErrors = CDirectoryUtils::verifyRuntimeDirectoriesAndFiles();
        if (!verifyErrors.isEmpty())
        {
            this->cmdLineErrorMessage("Missing runtime directories/files: " + verifyErrors.join(", "));
            return false;
        }

        if (m_singleApplication && m_alreadyRunning)
        {
            this->cmdLineErrorMessage("Program must only run once");
            return false;
        }

        // we call parse because we also want to display a GUI error message when applicable
        const QStringList args(QCoreApplication::instance()->arguments());
        if (!m_parser.parse(args))
        {
            this->cmdLineErrorMessage(m_parser.errorText());
            return false;
        }

        // help/version
        if (m_parser.isSet(m_cmdHelp))
        {
            // Important: parser help will already stop application
            this->cmdLineHelpMessage();
            return false;
        }
        if (m_parser.isSet(m_cmdVersion))
        {
            // Important: version will already stop application
            this->cmdLineVersionMessage();
            return false;
        }

        // dev.
        m_devFlag = this->initIsRunningInDeveloperEnvironment();

        // Hookin, other parsing
        if (!this->parsingHookIn()) { return false; }

        // setup reader
        m_setupReader->parseCmdLineArguments();
        m_parsed = true;
        return true;
    }

    bool CApplication::parseAndSynchronizeSetup(int timeoutMs)
    {
        if (!this->parseAndStartupCheck()) return false;
        return !this->synchronizeSetup(timeoutMs).hasErrorMessages();
    }

    bool CApplication::cmdLineErrorMessage(const QString &errorMessage, bool retry) const
    {
        Q_UNUSED(retry); // only works with UI version
        fputs(qPrintable(errorMessage), stderr);
        fputs("\n\n", stderr);
        fputs(qPrintable(m_parser.helpText()), stderr);
        return false;
    }

    bool CApplication::cmdLineErrorMessage(const CStatusMessageList &msgs, bool retry) const
    {
        Q_UNUSED(retry); // only works with UI version
        if (msgs.isEmpty()) { return false; }
        if (!msgs.hasErrorMessages())  { return false; }
        CApplication::cmdLineErrorMessage(
            msgs.toFormattedQString(true)
        );
        return false;
    }

    QString CApplication::cmdLineArgumentsAsString(bool withExecutable)
    {
        QStringList args = QCoreApplication::arguments();
        if (!withExecutable && !args.isEmpty()) args.removeFirst();
        if (args.isEmpty()) return "";
        return args.join(' ');
    }

    ISimulator *CApplication::getISimulator() const
    {
        if (!this->getCoreFacade()) { return nullptr; }
        if (!this->getCoreFacade()->getCContextSimulator()) { return nullptr; }
        return this->getCoreFacade()->getCContextSimulator()->simulator();
    }

    void CApplication::cmdLineHelpMessage()
    {
        m_parser.showHelp(); // terminates
        Q_UNREACHABLE();
    }

    void CApplication::cmdLineVersionMessage() const
    {
        printf("%s %s\n", qPrintable(QCoreApplication::applicationName()), qPrintable(QCoreApplication::applicationVersion()));
    }

    QStringList CApplication::argumentsJoined(const QStringList &newArguments, const QStringList &removeArguments) const
    {
        QStringList joinedArguments = CApplication::arguments();
        QStringList newArgumentsChecked = newArguments;

        // remove the executable argument if it exists at position 0
        if (!joinedArguments.isEmpty() && !joinedArguments.at(0).startsWith("-")) { joinedArguments.removeFirst(); } // was cmd line argument
        if (!newArgumentsChecked.isEmpty() && !newArgumentsChecked.at(0).startsWith("-")) { newArgumentsChecked.removeFirst(); } // was cmd line argument

        // remove all values before checking options
        static const QRegularExpression regExp("^-");
        QStringList toBeRemoved(newArgumentsChecked.filter(regExp));
        toBeRemoved.append(removeArguments.filter(regExp));
        toBeRemoved.append("--installer");
        toBeRemoved.removeDuplicates();

        if (!joinedArguments.isEmpty() && !toBeRemoved.isEmpty())
        {
            // remove all options from removeArguments
            // consider alias names, that is why we check on option
            for (const QCommandLineOption &option : m_allOptions)
            {
                const int n = indexOfCommandLineOption(option, toBeRemoved);
                if (n >= 0)
                {
                    argumentsWithoutOption(option, joinedArguments);
                }
            }
        }

        joinedArguments.append(newArgumentsChecked);
        return joinedArguments;
    }

    // ---------------------------------------------------------------------------------
    // Contexts
    // ---------------------------------------------------------------------------------

    bool CApplication::supportsContexts() const
    {
        if (m_shutdown) { return false; }
        if (m_coreFacade.isNull()) { return false; }
        if (!m_coreFacade->getIContextApplication()) { return false; }
        return (!m_coreFacade->getIContextApplication()->isEmptyObject());
    }

    const IContextNetwork *CApplication::getIContextNetwork() const
    {
        if (!supportsContexts()) { return nullptr; }
        return m_coreFacade->getIContextNetwork();
    }

    const IContextAudio *CApplication::getIContextAudio() const
    {
        if (!supportsContexts()) { return nullptr; }
        return m_coreFacade->getIContextAudio();
    }

    const IContextApplication *CApplication::getIContextApplication() const
    {
        if (!supportsContexts()) { return nullptr; }
        return m_coreFacade->getIContextApplication();
    }

    const IContextOwnAircraft *CApplication::getIContextOwnAircraft() const
    {
        if (!supportsContexts()) { return nullptr; }
        return m_coreFacade->getIContextOwnAircraft();
    }

    const IContextSimulator *CApplication::getIContextSimulator() const
    {
        if (!supportsContexts()) { return nullptr; }
        return m_coreFacade->getIContextSimulator();
    }

    IContextNetwork *CApplication::getIContextNetwork()
    {
        if (!supportsContexts()) { return nullptr; }
        return m_coreFacade->getIContextNetwork();
    }

    IContextAudio *CApplication::getIContextAudio()
    {
        if (!supportsContexts()) { return nullptr; }
        return m_coreFacade->getIContextAudio();
    }

    IContextApplication *CApplication::getIContextApplication()
    {
        if (!supportsContexts()) { return nullptr; }
        return m_coreFacade->getIContextApplication();
    }

    IContextOwnAircraft *CApplication::getIContextOwnAircraft()
    {
        if (!supportsContexts()) { return nullptr; }
        return m_coreFacade->getIContextOwnAircraft();
    }

    IContextSimulator *CApplication::getIContextSimulator()
    {
        if (!supportsContexts()) { return nullptr; }
        return m_coreFacade->getIContextSimulator();
    }

    void CApplication::onCoreFacadeStarted()
    {
        // void
    }

    // ---------------------------------------------------------------------------------
    // Setup
    // ---------------------------------------------------------------------------------

    bool CApplication::hasSetupReader() const
    {
        return m_setupReader;
    }

    CSetupReader *CApplication::getSetupReader() const
    {
        return m_setupReader.data();
    }

    QString CApplication::getLastSuccesfulSetupUrl() const
    {
        if (!this->hasSetupReader()) { return ""; }
        return m_setupReader->getLastSuccessfulSetupUrl();
    }

    QString CApplication::getLastSuccesfulDistributionUrl() const
    {
        if (!this->hasSetupReader()) { return ""; }
        return m_setupReader->getLastSuccessfulUpdateInfoUrl();
    }

    CStatusMessageList CApplication::synchronizeSetup(int timeoutMs)
    {
        this->requestReloadOfSetupAndVersion();
        return this->waitForSetup(timeoutMs);
    }

    CUrlList CApplication::getVatsimMetarUrls() const
    {
        if (m_shutdown) { return CUrlList(); }
        if (m_webDataServices)
        {
            const CUrlList urls(m_webDataServices->getVatsimMetarUrls());
            if (!urls.empty()) { return urls; }
        }
        if (m_setupReader)
        {
            return m_setupReader->getSetup().getVatsimMetarsUrls();
        }
        return CUrlList();
    }

    CUrlList CApplication::getVatsimDataFileUrls() const
    {
        if (m_shutdown) { return CUrlList(); }
        if (m_webDataServices)
        {
            const CUrlList urls(m_webDataServices->getVatsimDataFileUrls());
            if (!urls.empty()) { return urls; }
        }
        if (m_setupReader)
        {
            return m_setupReader->getSetup().getVatsimDataFileUrls();
        }
        return CUrlList();
    }

#ifdef BLACK_USE_CRASHPAD
    base::FilePath qstringToFilePath(const QString &str)
    {
#   ifdef Q_OS_WIN
        return base::FilePath(str.toStdWString());
#   else
        return base::FilePath(str.toStdString());
#   endif
    }
#endif

    BlackMisc::CStatusMessageList CApplication::initCrashHandler()
    {
#ifdef BLACK_USE_CRASHPAD
        // No crash handling for unit tests
        if (this->getApplicationInfo().isUnitTest()) { return CStatusMessage(this).info("No crash handler for unit tests"); }

        static const QString crashpadHandler(CBuildConfig::isRunningOnWindowsNtPlatform() ? "swift_crashpad_handler.exe" : "swift_crashpad_handler");
        static const QString handler = CFileUtils::appendFilePaths(CDirectoryUtils::binDirectory(), crashpadHandler);
        static const QString crashpadPath = CDirectoryUtils::crashpadDirectory();
        static const QString database = CFileUtils::appendFilePaths(crashpadPath, "/database");
        static const QString metrics = CFileUtils::appendFilePaths(crashpadPath, "/metrics");

        if (!QFileInfo::exists(handler))
        {
            return CStatusMessage(this).warning("Crashpad handler '%1' not found. Cannot init handler!") << handler;
        }

        const CUrl serverUrl = this->getGlobalSetup().getCrashReportServerUrl();
        std::map<std::string, std::string> annotations;

        // Caliper (mini-breakpad-server) annotations
        annotations["prod"] = executable().toStdString();
        annotations["ver"] = CBuildConfig::getVersionString().toStdString();

        QDir().mkpath(database);
        m_crashReportDatabase = CrashReportDatabase::Initialize(qstringToFilePath(database));
        auto settings = m_crashReportDatabase->GetSettings();
        settings->SetUploadsEnabled(CBuildConfig::isReleaseBuild() && m_crashDumpUploadEnabled.getThreadLocal());
        m_crashpadClient = std::make_unique<CrashpadClient>();
        m_crashpadClient->StartHandler(qstringToFilePath(handler), qstringToFilePath(database), qstringToFilePath(metrics),
                                       serverUrl.getFullUrl().toStdString(), annotations, {}, false, true);
        return CStatusMessage(this).info("Using crash handler");
#else
        return CStatusMessage(this).info("Not using crash handler");
#endif
    }

    void CApplication::crashDumpUploadEnabledChanged()
    {
#ifdef BLACK_USE_CRASHPAD
        if (!m_crashReportDatabase) { return; }
        auto settings = m_crashReportDatabase->GetSettings();
        settings->SetUploadsEnabled(CBuildConfig::isReleaseBuild() && m_crashDumpUploadEnabled.getThreadLocal());
#endif
    }

    QNetworkReply *CApplication::httpRequestImpl(
        const QNetworkRequest &request, int logId,
        const BlackMisc::CSlot<void (QNetworkReply *)> &callback, int maxRedirects, std::function<QNetworkReply *(QNetworkAccessManager &, const QNetworkRequest &)> requestOrPostMethod)
    {
        if (this->isShuttingDown()) { return nullptr; }
        if (!this->isNetworkAccessible()) { return nullptr; }
        QWriteLocker locker(&m_accessManagerLock);
        Q_ASSERT_X(QCoreApplication::instance()->thread() == m_accessManager->thread(), Q_FUNC_INFO, "Network manager supposed to be in main thread");
        if (QThread::currentThread() != m_accessManager->thread())
        {
            // run in QAM thread
            QTimer::singleShot(0, m_accessManager, std::bind(&CApplication::httpRequestImpl, this, request, logId, callback, maxRedirects, requestOrPostMethod));
            return nullptr; // not yet started
        }

        Q_ASSERT_X(QThread::currentThread() == m_accessManager->thread(), Q_FUNC_INFO, "Network manager thread mismatch");
        QNetworkRequest copiedRequest = CNetworkUtils::getSwiftNetworkRequest(request, this->getApplicationNameAndVersion());

        // If URL is one of the shared URLs, add swift client SSL certificate to request
        CNetworkUtils::setSwiftClientSslCertificate(copiedRequest, this->getGlobalSetup().getSwiftSharedUrls());

        QNetworkReply *reply = requestOrPostMethod(*m_accessManager, copiedRequest);
        reply->setProperty("started", QVariant(QDateTime::currentMSecsSinceEpoch()));
        reply->setProperty(CUrlLog::propertyNameId(), QVariant(logId));
        if (callback)
        {
            Q_ASSERT_X(callback.object(), Q_FUNC_INFO, "Need callback object (to determine thread)");
            connect(reply, &QNetworkReply::finished, callback.object(), [ = ]
            {
                // Called when finished!
                // QNetworkRequest::FollowRedirectsAttribute would allow auto redirect, but we use our approach as it gives us better control
                // \fixme: Check again on Qt 5.9: Added redirects policy to QNetworkAccessManager (ManualRedirectsPolicy, NoLessSafeRedirectsPolicy, SameOriginRedirectsPolicy, UserVerifiedRedirectsPolicy)
                const bool isRedirect = CNetworkUtils::isHttpStatusRedirect(reply);
                if (isRedirect && maxRedirects > 0)
                {
                    const QUrl redirectUrl = CNetworkUtils::getHttpRedirectUrl(reply);
                    if (!redirectUrl.isEmpty())
                    {
                        QNetworkRequest redirectRequest(redirectUrl);
                        const int redirectsLeft = maxRedirects - 1;
                        QTimer::singleShot(0, this, std::bind(&CApplication::httpRequestImpl, this, redirectRequest, logId, callback, redirectsLeft, requestOrPostMethod));
                        return;
                    }
                }
                // called when there are no more callbacks
                callback(reply);

            }, Qt::QueuedConnection);
        }
        return reply;
    }

    void CApplication::tagApplicationDataDirectory()
    {
        const QString d = CDirectoryUtils::normalizedApplicationDataDirectory();
        const QDir dir(d);
        if (!dir.exists() || !dir.isReadable()) { return; }
        const QString aiStr(this->getApplicationInfo().toJsonString());
        const QString filePath(CFileUtils::appendFilePaths(dir.path(), CApplicationInfo::fileName())); // will be overridden by next swift app
        CFileUtils::writeStringToFile(aiStr, filePath);
    }
} // ns

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
#include "blackcore/contextapplication.h"
#include "blackcore/cookiemanager.h"
#include "blackcore/corefacade.h"
#include "blackcore/networkvatlib.h"
#include "blackcore/registermetadata.h"
#include "blackcore/setupreader.h"
#include "blackcore/webdataservices.h"
#include "blackmisc/datacache.h"
#include "blackmisc/dbusserver.h"
#include "blackmisc/directoryutils.h"
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
#include "qcompilerdetection.h"

#include <stdbool.h>
#include <stdio.h>
#include <QCoreApplication>
#include <QDateTime>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslSocket>
#include <QStandardPaths>
#include <QThread>
#include <QTime>
#include <QTimer>
#include <QTranslator>
#include <QWriteLocker>
#include <Qt>
#include <QtGlobal>
#include <cstdlib>

using namespace BlackConfig;
using namespace BlackMisc;
using namespace BlackMisc::Network;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::Simulation;
using namespace BlackMisc::Weather;
using namespace BlackCore;
using namespace BlackCore::Data;
using namespace BlackCore::Db;

BlackCore::CApplication *sApp = nullptr; // set by constructor

namespace BlackCore
{
    CApplication::CApplication(const QString &applicationName, SwiftApplication application, bool init) :
        m_cookieManager( {}, this), m_applicationName(applicationName), m_application(application), m_coreFacadeConfig(CCoreFacadeConfig::allEmpty())
    {
        Q_ASSERT_X(!sApp, Q_FUNC_INFO, "already initialized");
        Q_ASSERT_X(QCoreApplication::instance(), Q_FUNC_INFO, "no application object");

        // init skiped when called from CGuiApplication
        if (init)
        {
            this->init(true);
        }
    }

    void CApplication::init(bool withMetadata)
    {
        if (!sApp)
        {
            if (withMetadata) { CApplication::registerMetadata(); }
            QCoreApplication::setApplicationName(this->m_applicationName);
            QCoreApplication::setApplicationVersion(CVersion::version());
            this->setObjectName(this->m_applicationName);
            const QString executable = QFileInfo(QCoreApplication::applicationFilePath()).fileName();
            if (executable.startsWith("test")) { this->m_unitTest = true; }
            this->initParser();
            this->initLogging();

            //
            // cmd line arguments not yet parsed here
            //

            // Translations
            QFile file(":blackmisc/translations/blackmisc_i18n_de.qm");
            CLogMessage(this).debug() << (file.exists() ? "Found translations in resources" : "No translations in resources");
            QTranslator translator;
            if (translator.load("blackmisc_i18n_de", ":blackmisc/translations/")) { CLogMessage(this).debug() << "Translator loaded"; }
            QCoreApplication::instance()->installTranslator(&translator);

            // Init network
            this->m_cookieManager.setParent(&this->m_accessManager);
            this->m_accessManager.setCookieJar(&this->m_cookieManager);

            // global setup
            sApp = this;
            this->m_setupReader.reset(new CSetupReader(this));
            connect(this->m_setupReader.data(), &CSetupReader::setupAvailable, this, &CApplication::ps_setupAvailable);
            connect(this->m_setupReader.data(), &CSetupReader::updateInfoAvailable, this, &CApplication::updateInfoAvailable);

            this->m_parser.addOptions(this->m_setupReader->getCmdLineOptions());

            // startup done
            connect(this, &CApplication::startUpCompleted, this, &CApplication::ps_startupCompleted);

            // notify when app goes down
            connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &CApplication::gracefulShutdown);
        }
    }

    CApplication::~CApplication()
    {
        this->gracefulShutdown();
    }

    const QString &CApplication::getApplicationNameAndVersion() const
    {
        static const QString s(QCoreApplication::instance()->applicationName() + " " + CVersion::version());
        return s;
    }

    const QString &CApplication::getApplicationNameVersionBetaDev() const
    {
        static const QString s(QCoreApplication::instance()->applicationName() + " " + this->versionStringDevBetaInfo());
        return s;
    }

    CApplication::SwiftApplication CApplication::getSwiftApplication() const
    {
        if (this->isUnitTest()) { return UnitTest; }
        if (this->m_application != Unknown) { return this->m_application; }

        // if not set, guess
        BLACK_VERIFY_X(false, Q_FUNC_INFO, "Missing application");
        const QString a(QCoreApplication::instance()->applicationName().toLower());
        if (a.contains("core"))     { return PilotClientCore; }
        if (a.contains("launcher")) { return Laucher; }
        if (a.contains("gui"))      { return PilotClientGui; }
        if (a.contains("core"))     { return PilotClientCore; }
        return Unknown;
    }

    bool CApplication::isUnitTest() const
    {
        return this->m_unitTest;
    }

    CGlobalSetup CApplication::getGlobalSetup() const
    {
        if (this->m_shutdown) { return CGlobalSetup(); }
        const CSetupReader *r = this->m_setupReader.data();
        if (!r) { return CGlobalSetup(); }
        return r->getSetup();
    }

    CUpdateInfo CApplication::getUpdateInfo() const
    {
        if (this->m_shutdown) { return CUpdateInfo(); }
        const CSetupReader *r = this->m_setupReader.data();
        if (!r) { return CUpdateInfo(); }
        return r->getUpdateInfo();
    }

    bool CApplication::start(bool waitForStart)
    {
        if (!this->m_parsed)
        {
            bool s = this->parse();
            if (!s) { return false; }
        }

        // clear cache?
        if (this->isSetOrTrue(this->m_cmdClearCache))
        {
            QStringList files(CApplication::clearCaches());
            CLogMessage(this).debug() << "Cleared cache, " << files.size() << " files";
        }

        // parsing itself is done
        if (this->m_startSetupReader && !this->m_setupReader->isSetupAvailable())
        {
            const CStatusMessageList msgs(this->requestReloadOfSetupAndVersion());
            CLogMessage::preformatted(msgs);
            if (msgs.isFailure())
            {
                this->cmdLineErrorMessage(msgs.getWarningAndErrorMessages().toSingleMessage().getMessage());
                return false;
            }
        }

        bool s = this->startHookIn();

        // trigger loading and saving of settings in appropriate scenarios
        if (this->m_coreFacadeConfig.getModeApplication() != CCoreFacadeConfig::Remote)
        {
            CStatusMessage m = CSettingsCache::instance()->loadFromStore();
            if (!m.isEmpty())
            {
                m.setCategories(getLogCategories());
                CLogMessage::preformatted(m);
            }

            CSettingsCache::instance()->enableLocalSave();
        }

        if (waitForStart)
        {
            s = this->waitForStart();
        }
        this->m_started = s;
        return s;
    }

    bool CApplication::waitForStart()
    {
        // process events return immediatley if nothing is to be processed
        const QTime dieTime = QTime::currentTime().addMSecs(5000);
        while (QTime::currentTime() < dieTime && !this->m_started && !this->m_startUpCompleted)
        {
            // Alternative: use QEventLoop, which seemed to make the scenario here more complex
            QCoreApplication::instance()->processEvents(QEventLoop::AllEvents, 250);
            QThread::msleep(250); // avoid CPU loop overload by "infinite loop"
        }
        if (!this->m_startUpCompleted)
        {
            CLogMessage(this).error("Waiting for startup timed out");
        }
        return this->m_started;
    }

    bool CApplication::isSetupAvailable() const
    {
        if (this->m_shutdown || !this->m_setupReader) { return false; }
        return this->m_setupReader->isSetupAvailable();
    }

    CStatusMessageList CApplication::requestReloadOfSetupAndVersion()
    {
        if (!this->m_shutdown)
        {
            Q_ASSERT_X(this->m_setupReader, Q_FUNC_INFO, "Missing reader");
            Q_ASSERT_X(this->m_parsed, Q_FUNC_INFO, "Not yet parsed");
            return this->m_setupReader->asyncLoad();
        }
        else
        {
            return CStatusMessage(getLogCategories(), CStatusMessage::SeverityError, "No reader for setup/version");
        }
    }

    bool CApplication::hasWebDataServices() const
    {
        return this->m_webDataServices;
    }

    CWebDataServices *CApplication::getWebDataServices() const
    {
        Q_ASSERT_X(this->m_webDataServices, Q_FUNC_INFO, "Missing web data services");
        return this->m_webDataServices.data();
    }

    bool CApplication::isApplicationThread() const
    {
        return CThreadUtils::isCurrentThreadApplicationThread();
    }

    const QString &CApplication::versionStringDevBetaInfo() const
    {
        if (isRunningInDeveloperEnvironment() && CBuildConfig::isBetaTest())
        {
            static const QString s(CVersion::version() + " [DEV, BETA]");
            return s;
        }
        if (isRunningInDeveloperEnvironment())
        {
            static const QString s(CVersion::version() + " [DEV]");
            return s;
        }
        if (CBuildConfig::isBetaTest())
        {
            static const QString s(CVersion::version() + " [BETA]");
            return s;
        }
        return CVersion::version();
    }

    const QString &CApplication::swiftVersionString() const
    {
        static const QString s(QString("swift %1").arg(versionStringDevBetaInfo()));
        return s;
    }

    const char *CApplication::swiftVersionChar()
    {
        static const QByteArray a(swiftVersionString().toUtf8());
        return a.constData();
    }

    bool CApplication::initIsRunningInDeveloperEnvironment() const
    {
        if (!CBuildConfig::canRunInDeveloperEnvironment()) { return false; }
        if (this->m_unitTest) { return true; }
        if (this->m_parser.isSet(this->m_cmdDevelopment))
        {
            return this->isSetOrTrue(this->m_cmdDevelopment);
        }
        else if (this->isSetupAvailable())
        {
            // assume value from setup
            return this->getGlobalSetup().isDevelopment();
        }
        return false;
    }

    void CApplication::signalStartupAutomatically(bool signal)
    {
        this->m_signalStartup = signal;
    }

    QString CApplication::getEnvironmentInfoString(const QString &separator) const
    {
        QString env("Beta: ");
        env.append(boolToYesNo(CBuildConfig::isBetaTest()));
        env = env.append(" dev.env,: ").append(boolToYesNo(isRunningInDeveloperEnvironment()));
        env = env.append(separator);
        env.append("Windows: ").append(boolToYesNo(CBuildConfig::isRunningOnWindowsNtPlatform()));
        return env;
    }

    QString CApplication::getInfoString(const QString &separator) const
    {
        QString str(CVersion::version());
        str = str.append(" ").append(CBuildConfig::isReleaseBuild() ? "Release build" : "Debug build");
        str = str.append(separator);
        str = str.append(getEnvironmentInfoString(separator));
        str = str.append(separator);
        str.append(CBuildConfig::compiledWithInfo(false));
        return str;
    }

    QNetworkReply *CApplication::getFromNetwork(const CUrl &url, const CSlot<void(QNetworkReply *)> &callback)
    {
        if (this->m_shutdown) { return nullptr; }
        return getFromNetwork(url.toNetworkRequest(), callback);
    }

    QNetworkReply *CApplication::getFromNetwork(const QNetworkRequest &request, const CSlot<void(QNetworkReply *)> &callback)
    {
        if (this->m_shutdown) { return nullptr; }
        QWriteLocker locker(&m_accessManagerLock);
        Q_ASSERT_X(QCoreApplication::instance()->thread() == m_accessManager.thread(), Q_FUNC_INFO, "Network manager supposed to be in main thread");
        if (QThread::currentThread() != this->m_accessManager.thread())
        {
            QTimer::singleShot(0, this, [this, request, callback]() { this->getFromNetwork(request, callback); });
            return nullptr; // not yet started
        }

        Q_ASSERT_X(QThread::currentThread() == m_accessManager.thread(), Q_FUNC_INFO, "Network manager thread mismatch");
        QNetworkRequest r(request); // no QObject
        CNetworkUtils::ignoreSslVerification(r);
        CNetworkUtils::setSwiftUserAgent(r);
        QNetworkReply *reply = this->m_accessManager.get(r);
        if (callback)
        {
            connect(reply, &QNetworkReply::finished, callback.object(), [ = ] { callback(reply); }, Qt::QueuedConnection);
        }
        return reply;
    }

    QNetworkReply *CApplication::postToNetwork(const QNetworkRequest &request, const QByteArray &data, const CSlot<void(QNetworkReply *)> &callback)
    {
        if (this->m_shutdown) { return nullptr; }
        QWriteLocker locker(&m_accessManagerLock);
        Q_ASSERT_X(QCoreApplication::instance()->thread() == m_accessManager.thread(), Q_FUNC_INFO, "Network manager supposed to be in main thread");
        if (QThread::currentThread() != this->m_accessManager.thread())
        {
            QTimer::singleShot(0, this, [this, request, data, callback]() { this->postToNetwork(request, data, callback); });
            return nullptr; // not yet started
        }

        Q_ASSERT_X(QThread::currentThread() == m_accessManager.thread(), Q_FUNC_INFO, "Network manager thread mismatch");
        QNetworkRequest r(request);
        CNetworkUtils::ignoreSslVerification(r);
        CNetworkUtils::setSwiftUserAgent(r);
        QNetworkReply *reply = this->m_accessManager.post(r, data);
        if (callback)
        {
            connect(reply, &QNetworkReply::finished, callback.object(), [ = ] { callback(reply); }, Qt::QueuedConnection);
        }
        return reply;
    }

    QNetworkReply *CApplication::postToNetwork(const QNetworkRequest &request, QHttpMultiPart *multiPart, const CSlot<void(QNetworkReply *)> &callback)
    {
        if (this->m_shutdown) { return nullptr; }
        QWriteLocker locker(&m_accessManagerLock);
        Q_ASSERT_X(QCoreApplication::instance()->thread() == m_accessManager.thread(), Q_FUNC_INFO, "Network manager supposed to be in main thread");
        if (QThread::currentThread() != this->m_accessManager.thread())
        {
            QTimer::singleShot(0, this, [this, request, multiPart, callback]() { this->postToNetwork(request, multiPart, callback); });
            return nullptr; // not yet started
        }

        Q_ASSERT_X(QThread::currentThread() == m_accessManager.thread(), Q_FUNC_INFO, "Network manager thread mismatch");
        QNetworkRequest r(request);
        CNetworkUtils::ignoreSslVerification(r);
        CNetworkUtils::setSwiftUserAgent(r);
        QNetworkReply *reply = this->m_accessManager.post(r, multiPart);
        if (callback)
        {
            connect(reply, &QNetworkReply::finished, callback.object(), [ = ] { callback(reply); });
        }
        return reply;
    }

    void CApplication::deleteAllCookies()
    {
        this->m_cookieManager.deleteAllCookies();
    }

    int CApplication::exec()
    {
        Q_ASSERT_X(instance(), Q_FUNC_INFO, "missing application");
        return QCoreApplication::exec();
    }

    void CApplication::exit(int retcode)
    {
        if (instance())
        {
            instance()->gracefulShutdown();
        }
        // when the event loop is not running, this does nothing
        QCoreApplication::exit(retcode);
    }

    QStringList CApplication::arguments()
    {
        return QCoreApplication::arguments();
    }

    void CApplication::processEventsFor(int milliseconds)
    {
        const QTime end = QTime::currentTime().addMSecs(milliseconds);
        while (QTime::currentTime() <= end)
        {
            QCoreApplication::processEvents();
            QThread::msleep(100);
        }
    }

    bool CApplication::useContexts(const CCoreFacadeConfig &coreConfig)
    {
        Q_ASSERT_X(this->m_parsed, Q_FUNC_INFO, "Call this function after parsing");

        this->m_useContexts = true;
        this->m_coreFacadeConfig = coreConfig;

        if (!this->m_useWebData)
        {
            bool s = this->useWebDataServices(CWebReaderFlags::AllReaders, CDatabaseReaderConfigList::forPilotClient());
            if (!s) { return false; }
        }
        return this->startCoreFacade(); // will do nothing if setup is not yet loaded
    }

    bool CApplication::useWebDataServices(const CWebReaderFlags::WebReader webReaders, const CDatabaseReaderConfigList &dbReaderConfig)
    {
        Q_ASSERT_X(this->m_webDataServices.isNull(), Q_FUNC_INFO, "Services already started");
        BLACK_VERIFY_X(QSslSocket::supportsSsl(), Q_FUNC_INFO, "No SSL");
        if (!QSslSocket::supportsSsl())
        {
            this->cmdLineErrorMessage("No SSL supported, can`t be used");
            return false;
        }

        this->m_webReadersUsed = webReaders;
        this->m_dbReaderConfig = dbReaderConfig;
        this->m_useWebData = true;
        return this->startWebDataServices();
    }

    bool CApplication::startCoreFacade()
    {
        if (!this->m_useContexts) { return true; } // we do not use context, so no need to startup
        if (!this->m_parsed) { return false; }
        if (!this->m_setupReader || !this->m_setupReader->isSetupAvailable()) { return false; }

        Q_ASSERT_X(this->m_coreFacade.isNull(), Q_FUNC_INFO, "Cannot alter facade");
        Q_ASSERT_X(this->m_setupReader, Q_FUNC_INFO, "No facade without setup possible");
        Q_ASSERT_X(this->m_useWebData, Q_FUNC_INFO, "Need web data services");

        this->startWebDataServices();

        CLogMessage(this).info("Will start core facade now");
        this->m_coreFacade.reset(new CCoreFacade(this->m_coreFacadeConfig));
        emit this->coreFacadeStarted();
        return true;
    }

    bool CApplication::startWebDataServices()
    {
        if (!this->m_useWebData) { return true; }
        if (!this->m_parsed) { return false; }
        if (!this->m_setupReader || !this->m_setupReader->isSetupAvailable()) { return false; }

        Q_ASSERT_X(this->m_setupReader, Q_FUNC_INFO, "No web data services without setup possible");
        if (!this->m_webDataServices)
        {
            CLogMessage(this).info("Will start web data services now");
            this->m_webDataServices.reset(
                new CWebDataServices(this->m_webReadersUsed, this->m_dbReaderConfig, {}, this)
            );
        }
        return true;
    }

    void CApplication::initLogging()
    {
        CLogHandler::instance()->install(); // make sure we have a log handler!

        // File logger
        static const QString logPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
                                       "/org.swift-project/" +
                                       CDirectoryUtils::normalizedApplicationDirectory() +
                                       "/logs";
        this->m_fileLogger.reset(new CFileLogger(executable(), logPath));
        this->m_fileLogger->changeLogPattern(CLogPattern().withSeverityAtOrAbove(CStatusMessage::SeverityDebug));
    }

    void CApplication::initParser()
    {
        this->m_parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
        this->m_parser.setApplicationDescription(m_applicationName);
        this->m_cmdHelp = this->m_parser.addHelpOption();
        this->m_cmdVersion = this->m_parser.addVersionOption();

        // dev. system
        this->m_cmdDevelopment = QCommandLineOption({ "dev", "development" },
                                 QCoreApplication::translate("application", "Dev.system feature?"),
                                 "development");
        this->addParserOption(this->m_cmdDevelopment);

        // can read a local bootsrap file
        this->m_cmdSharedDir = QCommandLineOption({ "shared", "shareddir" },
                               QCoreApplication::translate("application", "Local shared directory."),
                               "shared");
        this->addParserOption(this->m_cmdSharedDir);

        // reset caches upfront
        this->m_cmdClearCache = QCommandLineOption({ "ccache", "clearcache" },
                                QCoreApplication::translate("application", "Clear (reset) the caches."),
                                "clearcache");
        this->addParserOption(this->m_cmdClearCache);
    }

    bool CApplication::isSetOrTrue(const QCommandLineOption &option) const
    {
        if (!this->m_parser.isSet(option)) { return false; }

        // explicit value
        const QString v(this->m_parser.value(option).trimmed());
        if (v.isEmpty()) { return true; } // just flag
        if (v.startsWith("-")) { return true; } // just flag, because value is already next parameter
        return stringToBool(v);
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
        if (this->m_shutdown) { return; }
        this->m_shutdown = true;

        // save settings (but only when application was really alive)
        CStatusMessage m;
        if (this->m_parsed)
        {
            if (this->supportsContexts())
            {
                // this will eventually also call saveToStore
                m = this->getIContextApplication()->saveSettings();
            }
            else
            {
                m = CSettingsCache::instance()->saveToStore();
            }
            CLogMessage(getLogCategories()).preformatted(m);
        }

        // from here on we really rip appart the application object
        // and it should no longer be used
        sApp = nullptr;
        disconnect(this);

        if (this->supportsContexts())
        {
            // clean up facade
            this->m_coreFacade->gracefulShutdown();
            this->m_coreFacade.reset();
        }

        if (this->m_webDataServices)
        {
            this->m_webDataServices->gracefulShutdown();
            this->m_webDataServices.reset();
        }

        if (this->m_setupReader)
        {
            this->m_setupReader->gracefulShutdown();
        }

        this->m_fileLogger->close();
    }

    void CApplication::ps_setupAvailable(bool available)
    {
        if (available)
        {
            this->m_started = this->asyncWebAndContextStart();
        }
        this->m_startUpCompleted = true;
        emit setupAvailable(available);

        if (this->m_signalStartup)
        {
            emit this->startUpCompleted(this->m_started);
        }
    }

    void CApplication::ps_startupCompleted()
    {
        // void
    }

    bool CApplication::asyncWebAndContextStart()
    {
        if (this->m_started) { return true; }

        // follow up startups
        bool s = this->startWebDataServices();
        return s && this->startCoreFacade();
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
        return this->m_parser.addOption(option);
    }

    bool CApplication::addParserOptions(const QList<QCommandLineOption> &options)
    {
        return this->m_parser.addOptions(options);
    }

    void CApplication::addDBusAddressOption()
    {
        this->m_cmdDBusAddress = QCommandLineOption({ "dbus", "dbusaddress" },
                                 QCoreApplication::translate("application", "DBus address."),
                                 "dbusaddress");
        this->addParserOption(this->m_cmdDBusAddress);
    }

    void CApplication::addVatlibOptions()
    {
        this->addParserOptions(CNetworkVatlib::getCmdLineOptions());
    }

    QString CApplication::getCmdDBusAddressValue() const
    {
        if (this->isParserOptionSet(this->m_cmdDBusAddress))
        {
            const QString v(this->getParserValue(m_cmdDBusAddress));
            const QString dBusAddress(CDBusServer:: normalizeAddress(v));
            return dBusAddress;
        }
        else
        {
            return "";
        }
    }

    QString CApplication::getCmdSwiftPrivateSharedDir() const
    {
        return this->m_parser.value(this->m_cmdSharedDir);
    }

    bool CApplication::isParserOptionSet(const QString &option) const
    {
        return this->m_parser.isSet(option);
    }

    bool CApplication::isParserOptionSet(const QCommandLineOption &option) const
    {
        return this->m_parser.isSet(option);
    }

    QString CApplication::getParserValue(const QString &option) const
    {
        return this->m_parser.value(option).trimmed();
    }

    QString CApplication::getParserValue(const QCommandLineOption &option) const
    {
        return this->m_parser.value(option).trimmed();
    }

    bool CApplication::parse()
    {
        if (this->m_parsed) { return m_parsed; }
        if (CBuildConfig::isLifetimeExpired())
        {
            this->cmdLineErrorMessage("Program exired " + CBuildConfig::getEol().toString());
            return false;
        }

        // we call parse because we also want to display a GUI error message when applicable
        const QStringList args(QCoreApplication::instance()->arguments());
        if (!this->m_parser.parse(args))
        {
            this->cmdLineErrorMessage(this->m_parser.errorText());
            return false;
        }

        // help/version
        if (this->m_parser.isSet(this->m_cmdHelp))
        {
            // Important parser help will already stop application
            this->cmdLineHelpMessage();
            return true;
        }
        if (this->m_parser.isSet(this->m_cmdVersion))
        {
            this->cmdLineVersionMessage();
            return true;
        }

        // dev.
        this->m_devEnv = this->initIsRunningInDeveloperEnvironment();

        // Hookin, other parsing
        if (!this->parsingHookIn()) { return false; }

        // setup reader
        this->m_startSetupReader = this->m_setupReader->parseCmdLineArguments();
        this->m_parsed = true;
        return true;
    }

    void CApplication::cmdLineErrorMessage(const QString &errorMessage) const
    {
        fputs(qPrintable(errorMessage), stderr);
        fputs("\n\n", stderr);
        fputs(qPrintable(this->m_parser.helpText()), stderr);
    }

    void CApplication::cmdLineHelpMessage()
    {
        this->m_parser.showHelp(); // terminates
        Q_UNREACHABLE();
    }

    void CApplication::cmdLineVersionMessage() const
    {
        printf("%s %s\n", qPrintable(QCoreApplication::applicationName()), qPrintable(QCoreApplication::applicationVersion()));
    }

// ---------------------------------------------------------------------------------
// Contexts
// ---------------------------------------------------------------------------------

    bool CApplication::supportsContexts() const
    {
        if (this->m_shutdown) { return false; }
        if (this->m_coreFacade.isNull()) { return false; }
        if (!this->m_coreFacade->getIContextApplication()) { return false; }
        return (!this->m_coreFacade->getIContextApplication()->isEmptyObject());
    }

    const IContextNetwork *CApplication::getIContextNetwork() const
    {
        if (!supportsContexts()) { return nullptr; }
        return this->m_coreFacade->getIContextNetwork();
    }

    const IContextAudio *CApplication::getIContextAudio() const
    {
        if (!supportsContexts()) { return nullptr; }
        return this->m_coreFacade->getIContextAudio();
    }

    const IContextApplication *CApplication::getIContextApplication() const
    {
        if (!supportsContexts()) { return nullptr; }
        return this->m_coreFacade->getIContextApplication();
    }

    const IContextOwnAircraft *CApplication::getIContextOwnAircraft() const
    {
        if (!supportsContexts()) { return nullptr; }
        return this->m_coreFacade->getIContextOwnAircraft();
    }

    const IContextSimulator *CApplication::getIContextSimulator() const
    {
        if (!supportsContexts()) { return nullptr; }
        return this->m_coreFacade->getIContextSimulator();
    }

    IContextNetwork *CApplication::getIContextNetwork()
    {
        if (!supportsContexts()) { return nullptr; }
        return this->m_coreFacade->getIContextNetwork();
    }

    IContextAudio *CApplication::getIContextAudio()
    {
        if (!supportsContexts()) { return nullptr; }
        return this->m_coreFacade->getIContextAudio();
    }

    IContextApplication *CApplication::getIContextApplication()
    {
        if (!supportsContexts()) { return nullptr; }
        return this->m_coreFacade->getIContextApplication();
    }

    IContextOwnAircraft *CApplication::getIContextOwnAircraft()
    {
        if (!supportsContexts()) { return nullptr; }
        return this->m_coreFacade->getIContextOwnAircraft();
    }

    IContextSimulator *CApplication::getIContextSimulator()
    {
        if (!supportsContexts()) { return nullptr; }
        return this->m_coreFacade->getIContextSimulator();
    }

// ---------------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------------

    CUrlList CApplication::getVatsimMetarUrls() const
    {
        if (this->m_webDataServices)
        {
            const CUrlList urls(this->m_webDataServices->getVatsimMetarUrls());
            if (!urls.empty()) { return urls; }
        }
        if (this->m_setupReader)
        {
            return this->m_setupReader->getSetup().getVatsimMetarsUrls();
        }
        return CUrlList();
    }

    CUrlList CApplication::getVatsimDataFileUrls() const
    {
        if (this->m_webDataServices)
        {
            const CUrlList urls(this->m_webDataServices->getVatsimDataFileUrls());
            if (!urls.empty()) { return urls; }
        }
        if (this->m_setupReader)
        {
            return this->m_setupReader->getSetup().getVatsimDataFileUrls();
        }
        return CUrlList();
    }
} // ns

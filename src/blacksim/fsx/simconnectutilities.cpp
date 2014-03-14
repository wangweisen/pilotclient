#include "simconnectutilities.h"
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>

namespace BlackSim
{
    namespace Fsx
    {
        CSimConnectUtilities::CSimConnectUtilities() { }

        //
        // Filename / path of a "local" SimConnect.cfg file
        //
        QString CSimConnectUtilities::getLocalSimConnectCfgFilename()
        {
            return QCoreApplication::applicationDirPath() + "/SimConnect.cfg";
        }

        //
        // Create a very simple SimConnect.cfg
        //
        bool CSimConnectUtilities::writeSimConnectCfg(const QString &fileName, const QString &ip, quint16 port)
        {
            QString sc = QString("[SimConnect]\nProtocol=Ipv4\nAddress=%1\nPort=%2\n"
                                 "MaxReceiveSize=4096\nDisableNagle=0").arg(ip).arg(port);
            QFile file(fileName);
            bool success = false;
            if ((success = file.open(QIODevice::WriteOnly | QIODevice::Text)))
            {
                QTextStream out(&file);
                out << sc;
                file.close();
            }
            return success;
        }

        //
        // Resolve enum id to string
        // http://labs.qt.nokia.com/2008/10/09/coding-tip-pretty-printing-enum-values/
        //
        const QString CSimConnectUtilities::resolveEnumToString(const DWORD id, const char *enumName)
        {
            int i = CSimConnectUtilities::staticMetaObject.indexOfEnumerator(enumName);
            if (i < 0) return QString("No enumerator for %1").arg(enumName);
            QMetaEnum m = CSimConnectUtilities::staticMetaObject.enumerator(i);
            const char *k = m.valueToKey(id);
            return (k) ? QLatin1String(k) : QString("Id %1 not found for %2").arg(id).arg(enumName);
        }

        //
        // Exception id to something human readible
        //
        const QString CSimConnectUtilities::simConnectExceptionToString(const DWORD id)
        {
            return CSimConnectUtilities::resolveEnumToString(id, "SIMCONNECT_EXCEPTION");
        }

        //
        // The surface types
        //
        const QString CSimConnectUtilities::simConnectSurfaceTypeToString(const DWORD type, bool beautify)
        {
            QString sf = CSimConnectUtilities::resolveEnumToString(type, "SIMCONNECT_SURFACE");
            return beautify ? sf.replace('_', ' ') : sf;
        }

        void CSimConnectUtilities::registerMetadata()
        {
            qRegisterMetaType<CSimConnectUtilities::SIMCONNECT_EXCEPTION>();
            qRegisterMetaType<CSimConnectUtilities::SIMCONNECT_SURFACE>();
        }

    } // namespace
} // namespace

/* Copyright (C) 2019
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution. No part of swift project, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 */

//! \file

#include "resourcesound.h"
#include "audioutilities.h"
#include "blackmisc/fileutils.h"
#include "blackmisc/stringutils.h"
#include <QCoreApplication>

using namespace BlackMisc;
using namespace BlackSound::Wav;

namespace BlackSound
{
    namespace SampleProvider
    {
        CResourceSound::CResourceSound()
        {
            m_data = new CResourceSoundData;
        }

        CResourceSound::CResourceSound(const QString &audioFileName)
        {
            m_data = new CResourceSoundData;
            m_data->fileName = audioFileName;
        }

        bool CResourceSound::load()
        {
            if (m_data->fileName.isEmpty()) { return false; }

            QObject *parent = QCoreApplication::instance();
            Q_ASSERT(parent);
            CWorker *worker = CWorker::fromTask(parent, "loadResourceSound", [this]()
            {
                CWavFile wavFile;

                m_data->samples.clear();
                if (wavFile.open(m_data->fileName))
                {
                    if (wavFile.fileFormat().sampleType() == QAudioFormat::Float)
                    {
                        // Not implemented
                        // m_samples = convertFloatBytesTo16BitPCM(wavFile.audioData());
                    }
                    else
                    {
                        m_data->samples = convertBytesTo32BitFloatPCM(wavFile.audioData());
                    }
                }
            });
            worker->then([this]()
            {
                m_data->isLoaded = true;
            });

            return worker ? true : false;
        }

        bool CResourceSound::isSameFileName(const QString &fn) const
        {
            if (fn.isEmpty()) { return false; }
            return stringCompare(fn, m_data->fileName, CFileUtils::osFileNameCaseSensitivity());
        }
    } // ns
} // ns
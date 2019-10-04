#include "bufferedwaveprovider.h"

#include <QDebug>

namespace BlackSound
{
    namespace SampleProvider
    {
        CBufferedWaveProvider::CBufferedWaveProvider(const QAudioFormat &format, QObject *parent) :
            ISampleProvider(parent)
        {
            // Set buffer size to 10 secs
            m_maxBufferSize = format.bytesForDuration(10 * 1000 * 1000);
        }

        void CBufferedWaveProvider::addSamples(const QVector<float> &samples)
        {
            int delta = m_audioBuffer.size() + samples.size() - m_maxBufferSize;
            if (delta > 0)
            {
                m_audioBuffer.remove(0, delta);
            }
            m_audioBuffer.append(samples);
        }

        int CBufferedWaveProvider::readSamples(QVector<float> &samples, qint64 count)
        {
            qint64 len = qMin(count, static_cast<qint64>(m_audioBuffer.size()));
            samples = m_audioBuffer.mid(0, len);
            // if (len != 0) qDebug() << "Reading" << count << "samples." << m_audioBuffer.size() << "currently in the buffer.";
            m_audioBuffer.remove(0, len);
            return len;
        }

        void CBufferedWaveProvider::clearBuffer()
        {
            m_audioBuffer.clear();
        }
    } // ns
} // ns

/*
* pt_extdevices.h
*
* Copyright (c) 2016 Spranto International Pte Ltd. All Rights Reserved.
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, you can obtain one at http://mozilla.org/MPL/2.0/
*
* Software distributed under the License is distributed on an "AS IS"
* basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
* the License for the specific language governing rights and limitations
* under the License.
*
* Contributor(s): ______________________________________.
*
*
*/

#include <h323.h>

#include "etc/pt_extdevices.h"
#include "etc/h323resampler.h"

///////////////////////////////////////////////////////////////////////////////////////////

H323_MediaManager::H323_MediaManager()
{

}

PBoolean H323_MediaManager::SetAudioFormat(unsigned id, unsigned sampleRate, unsigned bytesPerSample, unsigned noChannels, unsigned sampleTime)
{
    return false;
}

void H323_MediaManager::GetAudioFormat(unsigned id, unsigned & sampleRate, unsigned & bytesPerSample, unsigned & noChannels, unsigned & sampleTime)
{

}

PBoolean H323_MediaManager::SetColourFormat(unsigned id, const PString & colourFormat)
{
    return false;
}

void H323_MediaManager::GetColourFormat(unsigned id, PString & colourFormat)
{
    colourFormat = "YUV420P";
}

PBoolean H323_MediaManager::GetFrameSize(unsigned id, unsigned & width, unsigned & height)
{
    return false;
}

PBoolean H323_MediaManager::Start(unsigned id)
{
    return false;
}

PBoolean H323_MediaManager::Stop(unsigned id)
{
    return false;
}

PBoolean H323_MediaManager::IsRunning(unsigned id)
{
    return false;
}

bool H323_MediaManager::Write(unsigned id, void * data, unsigned size, unsigned width, unsigned height)
{
    return false;
}

bool H323_MediaManager::Read(unsigned id, bool toBlock, void * data, unsigned size)
{
    return false;
}

bool H323_MediaManager::Read(unsigned id, bool toBlock, void * data, unsigned & size, unsigned & width, unsigned & height)
{
    return false;
}

//////////////////////////////////////////////////////////////////////////////

PCREATE_SOUND_PLUGIN(External, PSoundChannel_External)

PSoundChannel_External::PSoundChannel_External()
    :  m_streamID(0), m_manager(NULL),
       m_sampleRate(0), m_channels(1), 
       m_bytesPerSample(2), m_sampleTime(0), m_bufferSize(0), m_intBuffer(0)
#ifdef H323_RESAMPLE
       ,m_resampler(NULL),
        m_buffer(new H323AudioBuffer())
#endif
{

}

PSoundChannel_External::~PSoundChannel_External()
{
    Close();
}

bool PSoundChannel_External::AttachManager(unsigned streamID, H323_MediaManager * manager)
{
    m_manager = manager;
    m_streamID = streamID;

    if (!manager)
        return false;

    m_manager->GetAudioFormat(m_streamID, m_sampleRate, m_bytesPerSample, m_channels, m_sampleTime);

    if (activeDirection == PSoundChannel::Player) {
        m_bufferSize = (m_sampleRate * m_sampleTime * m_bytesPerSample) / 1000;
        m_intBuffer.SetSize(m_bufferSize);
    }

    return true;
}

PStringArray PSoundChannel_External::GetDeviceNames(PSoundChannel::Directions dir)
{
    if (dir == Player)
        return PStringArray("ExternalPlayer");
    else 
        return PStringArray("ExternalRecorder");
}

bool PSoundChannel_External::Open(const Params & params)
{
    activeDirection = params.m_direction;
    return SetFormat(params.m_channels, params.m_sampleRate, params.m_bitsPerSample);
}

PString PSoundChannel_External::GetName() const
{
    return "External";
}

PBoolean PSoundChannel_External::Close()
{
    m_sampleRate = 0;

    if (m_manager)
        m_manager->Stop(m_streamID);

#ifdef H323_RESAMPLE
    if (m_resampler) {
        m_resampler->Close();
        delete m_resampler;
    }

    if (m_buffer)
        delete m_buffer;
#endif
    return true;
}

PBoolean PSoundChannel_External::IsOpen() const
{
    return (m_manager && m_manager->IsRunning(m_streamID));
}

PBoolean PSoundChannel_External::StartRecording()
{
    if (!IsOpen())
       return m_manager->Start(m_streamID);

    return true;
}

PBoolean PSoundChannel_External::Write(const void * buf, PINDEX len)
{
    if (m_sampleRate <= 0)
        return false;

#ifdef H323_RESAMPLE
    unsigned ret = m_resampler->Process((const uint16_t *)buf, len);
    if (ret) {
        m_buffer->write(m_resampler->GetOutBuffer() , ret);

        if (m_buffer->read((uint16_t *)m_intBuffer.GetPointer(), m_intBuffer.GetSize()) > 0)
            m_manager->Write(m_streamID, m_intBuffer.GetPointer(), m_intBuffer.GetSize());
    }
#endif

    lastWriteCount = len;
    m_Pacing.Delay(1000 * len / m_sampleRate / m_channels / m_bytesPerSample);
    return true;
}

PBoolean PSoundChannel_External::Read(void * buf, PINDEX len)
{
    if (m_sampleRate <= 0)
        return false;

    if (!StartRecording())
        return false;

    bool success = false;
#ifdef H323_RESAMPLE
    if (m_manager->Read(m_streamID, false, m_intBuffer.GetPointer(), m_intBuffer.GetSize())) {
        unsigned ret = m_resampler->Process((uint16_t *)m_intBuffer.GetPointer(), m_intBuffer.GetSize());
        if (ret) {
            m_buffer->write(m_resampler->GetOutBuffer(), ret);
            success = (m_buffer->read(buf, len) > 0);
        }
    }
#endif

    if (!success) {
        PTRACE(4, "H323EXT\tReading Empty Audio Frame");
        memset(buf, 0, len);
    }
 
    lastReadCount = len;
    m_Pacing.Delay(1000 * len / m_sampleRate / m_channels / m_bytesPerSample);
    return true;
}

PBoolean PSoundChannel_External::SetFormat(unsigned numChannels, unsigned sampleRate, unsigned bitsPerSample)
{
    if (bitsPerSample % 8 != 0) {
        PTRACE(1, "ExtAudio\tBits per sample must even bytes.");
        return false;
    }

    unsigned bytesPerSample = bitsPerSample / 8;

#ifdef H323_RESAMPLE
    if (m_resampler)
        delete m_resampler;

    m_resampler = new H323AudioResampler();

    if (activeDirection == PSoundChannel::Player) {
        m_resampler->Open(m_bytesPerSample, bytesPerSample, m_sampleRate, sampleRate, m_sampleTime, m_channels, numChannels);
    } else {
        m_bufferSize = (sampleRate * m_sampleTime * bytesPerSample) / 1000;
        m_intBuffer.SetSize(m_bufferSize);
        m_resampler->Open(bytesPerSample, m_bytesPerSample, sampleRate, m_sampleRate, m_sampleTime, numChannels, m_channels);
    }
#endif
    return true;
}

unsigned PSoundChannel_External::GetChannels() const
{
    return m_channels;
}

unsigned PSoundChannel_External::GetSampleRate() const
{
    return m_sampleRate;
}

unsigned PSoundChannel_External::GetSampleSize() const
{
    return m_bytesPerSample * 2;
}

PBoolean PSoundChannel_External::SetBuffers(PINDEX size, PINDEX)
{
    m_bufferSize = size;
    return true;
}

PBoolean PSoundChannel_External::GetBuffers(PINDEX & size, PINDEX & count)
{
    size = m_bufferSize;
    count = 1;
    return true;
}

//////////////////////////////////////////////////////////////////////////////

PCREATE_VIDINPUT_PLUGIN(External);

PVideoInputDevice_External::PVideoInputDevice_External()
: m_streamID(-1), m_manager(0), m_videoFrameSize(0),
  m_initialConverter(NULL), m_readFrameBuffer(0), m_readBufferSize(0),
  m_initialFrameBuffer(0), m_initialBufferSize(0),
  m_initialHeight(0), m_initialWidth(0), m_initialFormat(colourFormat), m_shutdown(false)
{
    m_videoFrameSize = CalculateFrameBytes(frameWidth, frameHeight, colourFormat);
    frameStore.SetSize(m_videoFrameSize);
}


PVideoInputDevice_External::~PVideoInputDevice_External()
{
    Close();
}


bool PVideoInputDevice_External::AttachManager(unsigned streamID, H323_MediaManager * manager)
{
    m_manager = manager;
    m_streamID = streamID;

    if (!manager)
        return false;

    m_manager->GetColourFormat(m_streamID, m_initialFormat);
    if (!m_manager->GetFrameSize(m_streamID, m_initialWidth, m_initialHeight))
        return SetColourFormat(m_initialFormat);

    if (!m_initialWidth || !m_initialHeight)
        return true;

    m_readBufferSize = CalculateFrameBytes(m_initialWidth, m_initialHeight, m_initialFormat);
    m_readFrameBuffer.SetSize(m_readBufferSize);

    // colour converter
    if ((m_initialFormat != colourFormat)) {
        delete converter;
        PVideoFrameInfo src(m_initialWidth, m_initialHeight, m_initialFormat, 25);
        PVideoFrameInfo dst(m_initialWidth, m_initialHeight, colourFormat, 25);
        converter = PColourConverter::Create(src, dst);
        m_initialBufferSize = CalculateFrameBytes(m_initialWidth, m_initialHeight, colourFormat);
        m_initialFrameBuffer.SetSize(m_initialBufferSize);
    }

    // size converter;
    if (m_initialWidth != frameWidth || m_initialHeight != frameHeight) {
        delete m_initialConverter;
        PVideoFrameInfo xsrc(m_initialWidth, m_initialHeight, colourFormat, 25);
        PVideoFrameInfo xdst(frameWidth, frameHeight, colourFormat, 25);
        m_initialConverter = PColourConverter::Create(xsrc, xdst);
        m_videoFrameSize = CalculateFrameBytes(frameWidth, frameHeight, colourFormat);
        frameStore.SetSize(m_videoFrameSize);
    }

    return true;
}


PBoolean PVideoInputDevice_External::Open(const PString & devName, PBoolean /*startImmediate*/)
{
    return true;
}


PBoolean PVideoInputDevice_External::IsOpen()
{
    return (m_manager && m_manager->IsRunning(m_streamID));
}


PBoolean PVideoInputDevice_External::Close()
{
    return Stop();
}


PBoolean PVideoInputDevice_External::Start()
{
    return (m_manager && m_manager->Start(m_streamID));
}


PBoolean PVideoInputDevice_External::Stop()
{
    m_shutdown = true;
    return (m_manager && m_manager->Stop(m_streamID));
}


PBoolean PVideoInputDevice_External::IsCapturing()
{
    return IsOpen();
}


PStringArray PVideoInputDevice_External::GetInputDeviceNames()
{
    return PStringArray("External");
}


PBoolean PVideoInputDevice_External::SetVideoFormat(VideoFormat newFormat)
{
    return PVideoDevice::SetVideoFormat(newFormat);
}


PBoolean PVideoInputDevice_External::SetColourFormat(const PString & newFormat)
{
    return (colourFormat *= newFormat);
}


PBoolean PVideoInputDevice_External::SetFrameRate(unsigned rate)
{
    return true;
}


PBoolean PVideoInputDevice_External::SetFrameSize(unsigned width, unsigned height)
{

    if (width == 0 || height == 0)
        return true;

    if (width == frameWidth && height == frameHeight)
        return true;

    if (!PVideoInputDevice::SetFrameSize(width, height))
        return false;

    delete m_initialConverter;
    PVideoFrameInfo xsrc(m_initialWidth, m_initialHeight, colourFormat, 25);
    PVideoFrameInfo xdst(frameWidth, frameHeight, colourFormat, 25);
    m_initialConverter = PColourConverter::Create(xsrc, xdst);
    m_videoFrameSize = CalculateFrameBytes(frameWidth, frameHeight, colourFormat);

    return frameStore.SetSize(m_videoFrameSize);
}


PINDEX PVideoInputDevice_External::GetMaxFrameBytes()
{
    return GetMaxFrameBytesConverted(CalculateFrameBytes(frameWidth, frameHeight, preferredColourFormat));
}


PBoolean PVideoInputDevice_External::GetFrameData(BYTE * buffer, PINDEX * bytesReturned)
{
    return GetFrameDataNoDelay(buffer, bytesReturned);
}

PBoolean PVideoInputDevice_External::GetFrameDataNoDelay(BYTE * frame, PINDEX * bytesReturned)
{
    if (!m_manager || m_shutdown)
        return false;

    while (!m_manager->Read(m_streamID, false, m_readFrameBuffer.GetPointer(), m_readFrameBuffer.GetSize())) {
        if (m_shutdown)
            return false;
        // Wait till we get a picture
        PThread::Sleep(5);
    }

    if (converter != NULL) {
        converter->Convert(m_readFrameBuffer.GetPointer(), m_initialFrameBuffer.GetPointer());
    } else
        memcpy(m_initialFrameBuffer.GetPointer(), m_readFrameBuffer.GetPointer(), m_readBufferSize);

    if (!m_initialConverter) {
        frame = m_initialFrameBuffer.GetPointer();
        *bytesReturned = m_initialBufferSize;
        return true;
    } else {
        if (m_initialConverter->Convert(m_initialFrameBuffer.GetPointer(), frameStore.GetPointer())) {
            frame = frameStore.GetPointer();
            *bytesReturned = m_videoFrameSize;
            return true;
        }  else 
            return false;
    }
}


//////////////////////////////////////////////////////////////////////////////

PCREATE_VIDOUTPUT_PLUGIN(External);

PVideoOutputDevice_External::PVideoOutputDevice_External()
: m_streamID(-1), m_manager(NULL), m_videoFrameSize(0),
  m_finalConverter(NULL), m_finalFrameBuffer(0), m_finalBufferSize(0),
  m_finalHeight(0), m_finalWidth(0), m_finalFormat(colourFormat)
{
    m_videoFrameSize = CalculateFrameBytes(frameWidth, frameHeight, colourFormat);
    frameStore.SetSize(m_videoFrameSize);
}


PVideoOutputDevice_External::~PVideoOutputDevice_External()
{
    Close();
}


bool PVideoOutputDevice_External::AttachManager(unsigned streamID, H323_MediaManager * manager)
{
    m_manager = manager;
    m_streamID = streamID;

    if (!manager)
        return false;

    m_manager->GetColourFormat(m_streamID, m_finalFormat);
    if (!m_manager->GetFrameSize(m_streamID, m_finalWidth, m_finalHeight))
        return SetColourFormat(m_finalFormat);

    if (!m_finalWidth || !m_finalHeight)
        return true;

    // size converter;
    if (frameWidth != m_finalWidth || frameHeight != m_finalHeight) {
        delete converter;
        PVideoFrameInfo src(frameWidth, frameHeight, colourFormat, 25);
        PVideoFrameInfo dst(m_finalWidth, m_finalHeight, colourFormat, 25);
        converter = PColourConverter::Create(src, dst);
        m_videoFrameSize = CalculateFrameBytes(m_finalWidth, m_finalHeight, colourFormat);
        frameStore.SetSize(m_videoFrameSize);
    }

    // colour converter
    if ((colourFormat != m_finalFormat)) {
        delete m_finalConverter;
        PVideoFrameInfo xsrc(m_finalWidth, m_finalHeight, colourFormat, 25);
        PVideoFrameInfo xdst(m_finalWidth, m_finalHeight, m_finalFormat, 25);
        m_finalConverter = PColourConverter::Create(xsrc, xdst);
        m_finalBufferSize = CalculateFrameBytes(m_finalWidth, m_finalHeight, m_finalFormat);
        m_finalFrameBuffer.SetSize(m_finalBufferSize);
    }

    return true;
}


PBoolean PVideoOutputDevice_External::Open(const PString & /*deviceName*/, PBoolean /*startImmediate*/)
{
    return true;
}

PBoolean PVideoOutputDevice_External::SetColourFormat(const PString & newFormat)
{
    if ((!m_finalWidth || !m_finalHeight) && (newFormat != colourFormat)) {
        delete converter;
        PVideoFrameInfo src(frameWidth, frameHeight, colourFormat, 25);
        PVideoFrameInfo dst(frameWidth, frameHeight, newFormat, 25);
        converter = PColourConverter::Create(src, dst);
        m_videoFrameSize = CalculateFrameBytes(frameWidth, frameHeight, newFormat);
        return frameStore.SetSize(m_videoFrameSize);
    }
    return false;
}


PBoolean PVideoOutputDevice_External::IsOpen()
{
    return (m_manager && m_manager->IsRunning(m_streamID));
}


PBoolean PVideoOutputDevice_External::Close()
{
    delete m_finalConverter;
    return true;
}


PBoolean PVideoOutputDevice_External::Start()
{
    return (m_manager && m_manager->Start(m_streamID));
}

PBoolean PVideoOutputDevice_External::Stop()
{
    return (m_manager && m_manager->Stop(m_streamID));
}


PStringList PVideoOutputDevice_External::GetOutputDeviceNames()
{
    return PStringList("External");
}

PStringArray PVideoOutputDevice_External::GetDeviceNames() const
{
    return GetOutputDeviceNames();
}


PBoolean PVideoOutputDevice_External::SetFrameSize(unsigned width, unsigned height)
{
    if (width == 0 || height == 0)
        return true;

    if (width == frameWidth && height == frameHeight)
        return true;

    if (!PVideoOutputDevice::SetFrameSize(width, height))
        return false;

    if (!m_finalWidth && !m_finalHeight) {
        if (colourFormat != m_finalFormat) {
            delete converter;
            PVideoFrameInfo src(frameWidth, frameHeight, colourFormat, 25);
            PVideoFrameInfo dst(frameWidth, frameHeight, m_finalFormat, 25);
            converter = PColourConverter::Create(src, dst);
            m_videoFrameSize = CalculateFrameBytes(frameWidth, frameHeight, m_finalFormat);
        }
    } else if (frameWidth != m_finalWidth || frameHeight != m_finalHeight) {
        delete converter;
        PVideoFrameInfo fsrc(frameWidth, frameHeight, colourFormat, 25);
        PVideoFrameInfo fdst(m_finalWidth, m_finalHeight, colourFormat, 25);
        converter = PColourConverter::Create(fsrc, fdst);
        m_videoFrameSize = CalculateFrameBytes(m_finalWidth, m_finalHeight, colourFormat);
    } else {
        m_videoFrameSize = CalculateFrameBytes(frameWidth, frameHeight, colourFormat);
    }
  
    return frameStore.SetSize(m_videoFrameSize);
}


PINDEX PVideoOutputDevice_External::GetMaxFrameBytes()
{
    return GetMaxFrameBytesConverted(CalculateFrameBytes(frameWidth, frameHeight, preferredColourFormat));
}


PBoolean PVideoOutputDevice_External::SetFrameData(unsigned x, unsigned y,
    unsigned width, unsigned height, const BYTE * data, PBoolean endFrame)
{
    if (x != 0 || y != 0 || width != frameWidth || height != frameHeight || data == NULL || !endFrame)
        return false;

    if (converter != NULL) {
        converter->Convert(data, frameStore.GetPointer());
    } else
        memcpy(frameStore.GetPointer(), data, m_videoFrameSize);

    if (!m_finalConverter) {
        return (m_manager && m_manager->Write(m_streamID, frameStore.GetPointer(), m_videoFrameSize, width, height));
    } else {
        if (m_finalConverter->Convert(frameStore.GetPointer(), m_finalFrameBuffer.GetPointer())) {
            return (m_manager && m_manager->Write(m_streamID, m_finalFrameBuffer.GetPointer(), m_finalBufferSize, m_finalWidth, m_finalHeight));
        } else {
            return false;
        }
    }
}


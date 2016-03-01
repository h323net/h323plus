
/*
* pt_extdevices.h
*
* Copyright (c) 2015 Spranto International Pte Ltd. All Rights Reserved.
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

PCREATE_SOUND_PLUGIN(IOSSound2, PSoundChannel_External)

PSoundChannel_External::PSoundChannel_External()
    : m_loaded(false), m_running(false), lastReadCount(0)
{

}


PSoundChannel_External::PSoundChannel_External(const PString &device,
    Directions dir,
    unsigned numChannels,
    unsigned sampleRate,
    unsigned bitsPerSample)
{
    Open(device, dir, numChannels, sampleRate, bitsPerSample);
}


PSoundChannel_External::~PSoundChannel_External()
{
    Close();
}

PStringArray PSoundChannel_External::GetDeviceNames(PSoundChannel::Directions _dir)
{

    PStringArray names;
    names.SetSize(1);

    if (_dir == Player) {
        names[0] = "iPhoneSpeaker";
    }
    else {
        names[0] = "iPhoneMicrophone";
    }

    return names;


}

#ifdef ANDROID
int tempPacketCount = 0;
int maxPacketCount = 63;
#endif
PBoolean PSoundChannel_External::Open(const PString & /*_device*/,
    Directions _dir,
    unsigned _numChannels,
    unsigned _sampleRate,
    unsigned _bitsPerSample)
{
    m_dir = _dir;

    Close();
    SetFormat(_numChannels, _sampleRate, _bitsPerSample);


    PTRACE(4, "In PSoundChannel_External::Open() for _dir: " << _dir << " with _numChannels: " << _numChannels << " _sampleRate: " << _sampleRate << " _bitsPerSample: " << _bitsPerSample);

    m_dir = _dir;
    // Start the _device in the _dir (Player or Recorder)
    if (m_mediaCallback) {
        if (_dir == Player) {
  //          OpenH323ProxyAudioConsumerCallback(m_mediaCallback->GetH323ProxyAudioConsumerCallback(), _numChannels, _bitsPerSample, _sampleRate);
  //          StartH323ProxyAudioConsumerCallback(m_mediaCallback->GetH323ProxyAudioConsumerCallback());

        }
        else {
  //          OpenH323ProxyAudioProducerCallback(m_mediaCallback->GetH323ProxyAudioProducerCallback(), _numChannels, _bitsPerSample, _sampleRate);
  //          StartH323ProxyAudioProducerCallback(m_mediaCallback->GetH323ProxyAudioProducerCallback());
        }
        m_loaded = true;
        m_running = true;

#ifdef ANDROID
        tempPacketCount = 0;
#endif

        PTRACE(4, "PSoundIOS\t" << ((_dir == Player) ? "Playback" : "Recording") << " Opened!");

        return true;
    }

    return false;
}

PString PSoundChannel_External::GetName() const
{
    // This name is set from Open()
    return "MacIOSSound2";
}


PBoolean PSoundChannel_External::IsOpen() const
{
    PTRACE(4, "In PSoundDevice_IOS::IsOpen()");

    /*
    if(m_mediaCallback) {
    if(!m_mediaCallback->IsCallInProgress())
    return false;
    }
    */

    // bool whether the system is capturing
    return m_loaded;

}

PBoolean PSoundChannel_External::Abort()
{
    // Not implemented
    return false;
}

PBoolean PSoundChannel_External::Close()
{
    PTRACE(4, "IN PSoundChannel_External::Close Before IsOpen for m_dir: " << m_dir);
    if (!IsOpen())
        return false;

    PTRACE(4, "IN PSoundChannel_External::Close CLOSING DEVICE for m_dir: " << m_dir);
    // Close the sound device
    if (m_mediaCallback && m_running) {
        if (m_dir == Player) {
      //      StopH323ProxyAudioConsumerCallback(m_mediaCallback->GetH323ProxyAudioConsumerCallback());
     //       CloseH323ProxyAudioConsumerCallback(m_mediaCallback->GetH323ProxyAudioConsumerCallback());
        }
        else {
    //        StopH323ProxyAudioProducerCallback(m_mediaCallback->GetH323ProxyAudioProducerCallback());
   //         CloseH323ProxyAudioProducerCallback(m_mediaCallback->GetH323ProxyAudioProducerCallback());
        }
        m_loaded = false;
        m_running = false;
    }
    return true;
}


PBoolean PSoundChannel_External::SetFormat(unsigned /*numChannels*/,
    unsigned /*sampleRate*/,
    unsigned /*bitsPerSample*/)
{
    // Nothing to do here. Values are set directly via call to OpenH323ProxyAudioConsumerCallback

    // Set the audio format
    //  numChannels always 1
    //  sampleRate ms of time usually 20ms
    //  Bits per sample usually 8 but can be 16
    return true;
}

unsigned PSoundChannel_External::GetChannels() const
{
    if (m_mediaCallback) {
        return 0; // GetChannelsH323ProxyAudioProducerCallback(m_mediaCallback->GetH323ProxyAudioProducerCallback());
    }
    return 1; // Should ideally never be called
}

unsigned PSoundChannel_External::GetSampleRate() const
{
    if (m_mediaCallback) {
        return 0; //GetSampleRateH323ProxyAudioProducerCallback(m_mediaCallback->GetH323ProxyAudioProducerCallback());
    }

    return 16000; // Should ideally never be called
}

unsigned PSoundChannel_External::GetSampleSize() const
{
    if (m_mediaCallback) {
        return 0; //GetSampleSizeH323ProxyAudioProducerCallback(m_mediaCallback->GetH323ProxyAudioProducerCallback());
    }

    return 8;  // Should ideally never be called
}

PBoolean PSoundChannel_External::SetBuffers(PINDEX /*size*/, PINDEX /*count*/)
{
    // Set the sound buffers by default it is 3

    return true;
}


PBoolean PSoundChannel_External::GetBuffers(PINDEX & /*size*/, PINDEX & /*count*/)
{
    // Get the sound buffers (not used in h323plus)

    PTRACE(4, "IN PSoundChannel_External::GetBuffers for _dir: " << m_dir);
    return true;
}


PBoolean PSoundChannel_External::Write(const void* buf, PINDEX len)
{

    PTRACE(4, "IN PSoundChannel_External::Write Processing Buffer with len:" << len);

#ifdef ANDROID
    if (tempPacketCount < maxPacketCount) {
        sendwait.Delay(kDefaultPTime);
        ++tempPacketCount;
        return true;
    }
#endif

    if (m_mediaCallback) {
        //PTRACE(4, "IN PSoundChannel_External::Write value of callInProgress is: " << m_mediaCallback->IsCallInProgress());
        if (!m_mediaCallback->IsCallInProgress())
            return false;

#if AUDIO_CONSUMER_DEBUG
        audioConsumerCaptureFile->write((char*)buf, len);
#endif

        if (m_mediaCallback->IsCallInProgress())
           // ConsumeFrameWithH323ProxyAudioConsumerCallback(m_mediaCallback->GetH323ProxyAudioConsumerCallback(), buf, (unsigned)len);

        sendwait.Delay(kDefaultPTime);
    }
    return true;
}

PINDEX PSoundChannel_External::GetLastReadCount() const {
    return lastReadCount;
}

PBoolean PSoundChannel_External::Read(void * buffer, PINDEX bytesReturned)
{
    //PTRACE(4, "IN PSoundChannel_External::Read Processing Buffer");

    if (m_mediaCallback) {
        //PTRACE(4, "IN PSoundChannel_External::Read value of callInProgress is: " << m_mediaCallback->IsCallInProgress());
        if (!m_mediaCallback->IsCallInProgress())
            return false;

        if (!buffer) {
            //PTRACE(4, "IN PSoundChannel_External::Read buffer pointer is NULL");
        }
        else {
            PINDEX length = 0;
            while (m_running && length == 0) {
                length = m_mediaCallback->GetAudioInputBytesAvailableForCapture();
                PThread::Sleep(1);
            }

            if (!m_running)
                return false;

            // TODO: Get here and length is always > 0			 
            PTRACE(4, "IN PSoundChannel_External::Read length: " << length << " bytesReturned: " << bytesReturned);

            // TODO: Only mutex the grabbing of the buffer.
            m_mediaCallback->audioInputMutex.Wait();

            // PINDEX bufferSize = length; 
            PINDEX bufferSize = bytesReturned;

            if (!memcpy(buffer, m_mediaCallback->GetAudioInputCaptureBuffer(), bufferSize)) {
                PTRACE(4, "IN PSoundChannel_External::Read memcpy failed");
                return false;
            }
#if AUDIO_PRODUCER_DEBUG
            audioProducerCaptureFile->write((char*)m_mediaCallback->GetAudioInputCaptureBuffer(), bufferSize);
#endif
            lastReadCount = bytesReturned;
            m_mediaCallback->SetAudioInputBytesAvailableForCapture(0);

            m_mediaCallback->audioInputMutex.Signal();
        }
    }

    return true;
}

PBoolean PSoundChannel_External::PlaySound(const PSound & /*sound*/, PBoolean /*wait*/)
{
    // Not implemented
    return true;
}


PBoolean PSoundChannel_External::PlayFile(const PFilePath & /*filename*/, PBoolean /*wait*/)
{
    // Not implemented
    return true;
}


PBoolean PSoundChannel_External::IsPlayBufferFree()
{
    // Not implemented
    return true;
}


PBoolean PSoundChannel_External::WaitForPlayBufferFree()
{
    // Not implemented
    return true;
}


PBoolean PSoundChannel_External::HasPlayCompleted()
{
    // Not implemented
    return true;
}


PBoolean PSoundChannel_External::WaitForPlayCompletion()
{
    // Not implemented
    return true;
}


PBoolean PSoundChannel_External::RecordSound(PSound & /*sound*/)
{
    // Not implemented
    return PFalse;
}


PBoolean PSoundChannel_External::RecordFile(const PFilePath & /*filename*/)
{
    // Not implemented
    return PFalse;
}


PBoolean PSoundChannel_External::StartRecording()
{
    // Start Recording to file
    return true;
}


PBoolean PSoundChannel_External::IsRecordBufferFull()
{
    // Not implemented
    return true;
}


PBoolean PSoundChannel_External::AreAllRecordBuffersFull()
{
    // Not Implemented
    return true;
}


PBoolean PSoundChannel_External::WaitForRecordBufferFull()
{
    // Not implemented
    return true;
}


PBoolean PSoundChannel_External::WaitForAllRecordBuffersFull()
{
    // Not implemented
    return PFalse;
}


PBoolean PSoundChannel_External::SetVolume(unsigned /*newVal*/)
{
    // Set the current volume
    return false;
}


PBoolean PSoundChannel_External::GetVolume(unsigned & /*devVol*/)
{
    // Get the current volume
    return false;
}
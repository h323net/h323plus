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


#pragma once

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Call back

class H323_MediaManager : public PObject
{
    public:
        H323_MediaManager();

        virtual PBoolean SetAudioFormat(unsigned id, unsigned sampleRate, unsigned bytesPerSample, unsigned noChannels, unsigned sampleTime);
        virtual void     GetAudioFormat(unsigned id, unsigned & sampleRate, unsigned & bytesPerSample, unsigned & noChannels, unsigned & sampleTime);

        virtual PBoolean SetColourFormat(unsigned id, const PString & colourFormat);
        virtual void     GetColourFormat(unsigned id, PString & colourFormat);

        virtual PBoolean GetFrameSize(unsigned id, unsigned & width, unsigned & height);

        virtual bool Write(unsigned id, void * data, unsigned size, unsigned width=0, unsigned height=0);

        virtual bool Read(unsigned id, bool toBlock, void * data, unsigned size);
        virtual bool Read(unsigned id, bool toBlock, void * data, unsigned & size, unsigned & width, unsigned & height);
};


////////////////////////////////////////////////////////////////////////////////////////////////////////
// SoundChannel

class H323AudioResampler;
class PSoundChannel_External : public PSoundChannel
{
    PCLASSINFO(PSoundChannel_External, PSoundChannel);

public:

    PSoundChannel_External();
    ~PSoundChannel_External();

    bool AttachManager(unsigned streamID, H323_MediaManager * manager);

    static PStringArray GetDeviceNames(PSoundChannel::Directions = Player);

    bool Open(const Params & params);
    virtual PString GetName() const;
    PBoolean Close();
    PBoolean IsOpen() const;
    PBoolean Write(const void *, PINDEX len);
    PBoolean Read(void * buf, PINDEX len);
    PBoolean SetFormat(unsigned numChannels, unsigned sampleRate, unsigned bitsPerSample);
    unsigned GetChannels() const;
    unsigned GetSampleRate() const;
    unsigned GetSampleSize() const;
    PBoolean SetBuffers(PINDEX size, PINDEX);
    PBoolean GetBuffers(PINDEX & size, PINDEX & count);

protected:

    unsigned           m_streamID;
    H323_MediaManager* m_manager;

    unsigned       m_sampleRate;
    unsigned       m_channels;
    unsigned       m_bytesPerSample;
    unsigned       m_sampleTime;
    unsigned       m_bufferSize;
    PBYTEArray     m_intBuffer;
    PAdaptiveDelay m_Pacing;

#ifdef H323_RESAMPLE
    H323AudioResampler * m_resampler;
    H323AudioBuffer    * m_buffer;
#endif
};

PPLUGIN_STATIC_LOAD(External, PSoundChannel);

/////////////////////////////////////////////////////////////////////////
// Input Device

class PVideoInputDevice_External : public PVideoInputDevice
{
    PCLASSINFO(PVideoInputDevice_External, PVideoInputDevice);

public:

    PVideoInputDevice_External();

    virtual ~PVideoInputDevice_External();

    bool AttachManager(unsigned streamID, H323_MediaManager * manager);


    PBoolean Open( const PString & deviceName,  PBoolean startImmediate = true);

    PBoolean IsOpen();
    PBoolean Close();
    PBoolean Start();
    PBoolean Stop();
    PBoolean IsCapturing();
    static PStringArray GetInputDeviceNames();

    virtual PStringArray GetDeviceNames() const
    {  return GetInputDeviceNames(); }

    static bool GetDeviceCapabilities(
        const PString & /*deviceName*/, Capabilities * /*caps*/ ) { return false; }

    virtual PBoolean SetVideoFormat(VideoFormat videoFormat);
    virtual PBoolean SetColourFormat(const PString & colourFormat);
    virtual PBoolean SetFrameRate(unsigned rate);
    virtual PBoolean SetFrameSize(unsigned width, unsigned height);
    virtual PINDEX GetMaxFrameBytes();

    virtual PBoolean GetFrameData(
        BYTE * buffer, PINDEX * bytesReturned = NULL );

    virtual PBoolean GetFrameDataNoDelay(
        BYTE * buffer, PINDEX * bytesReturned = NULL );

protected:

    unsigned           m_streamID;
    H323_MediaManager* m_manager;

};

PPLUGIN_STATIC_LOAD(External, PVideoInputDevice);

/////////////////////////////////////////////////////////////////////////
// Output Device

class PVideoOutputDevice_External : public PVideoOutputDevice
{
    PCLASSINFO(PVideoOutputDevice_External, PVideoOutputDevice)

public:

    PVideoOutputDevice_External();
    virtual ~PVideoOutputDevice_External();

    virtual PBoolean Open(const PString & deviceName, PBoolean startImmediate = TRUE);

    PBoolean SetColourFormat(const PString & newFormat);

    virtual PBoolean IsOpen();
    virtual PBoolean Close();

    virtual PBoolean Start();
    virtual PBoolean Stop();

    static PStringList GetOutputDeviceNames();

    virtual PStringArray GetDeviceNames() const;

    virtual PBoolean SetFrameSize(unsigned width, unsigned height);

    virtual PINDEX GetMaxFrameBytes();

    bool AttachManager(unsigned streamID, H323_MediaManager * manager);

    virtual PBoolean SetFrameData(unsigned x, unsigned y,
        unsigned width, unsigned height, const BYTE * data, PBoolean endFrame);

protected:

    unsigned           m_streamID;
    H323_MediaManager* m_manager;

    unsigned           m_videoFrameSize;

    PColourConverter * m_finalConverter;
    PBYTEArray         m_finalFrameBuffer;
    unsigned           m_finalBufferSize;
    unsigned           m_finalHeight;
    unsigned           m_finalWidth;
    PString            m_finalFormat;

};

PPLUGIN_STATIC_LOAD(External, PVideoOutputDevice);

/////////////////////////////////////////////////////////////////////////




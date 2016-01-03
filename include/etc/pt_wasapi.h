/*
* pt_wasapi.h
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
* $Id: pt_wasapi.h,v 1.1.2.3 2016/01/03 07:19:16 shorne Exp $
*
*/


#pragma once

#ifdef H323_WASAPI


class PWASAPIDevice;
class PSoundChannel_WASAPI : public PSoundChannel
{
    PCLASSINFO(PSoundChannel_WASAPI, PSoundChannel);

public:
    PSoundChannel_WASAPI();
    ~PSoundChannel_WASAPI();

    PBoolean Open(const PString & device,
        Directions dir, unsigned numChannels,
        unsigned sampleRate, unsigned bitsPerSample
#if PTLIB_VER >= 2140
        , PPluginManager * pluginMgr = NULL
#endif
        );

    PBoolean IsOpen() const;

    PBoolean Close();

    PBoolean SetVolume(unsigned volume);

    PBoolean GetVolume(unsigned & volume);

    PBoolean SetMute(PBoolean mute);

    PBoolean GetMute(PBoolean & mute);

    PBoolean Read(void * buffer, PINDEX amount);

    PBoolean Write(const void * buffer, PINDEX amount);

    unsigned GetSampleRate() const;

    bool SourceEncoded(bool & lastFrame, unsigned & amount);

    PBoolean DisableDecode();

    static PStringArray GetNames() { return PStringArray("WASAPI"); }

    static PStringArray GetDeviceNames(
        Directions direction,               ///< Direction for device (record or play)
        PPluginManager * pluginMgr = NULL   ///< Plug in manager, use default if NULL
        );

    PStringArray GetAudioDeviceNames(
        Directions direction                ///< Direction for device (record or play)
        );

private:

    PWASAPIDevice *     m_pMMDevice;
    PAdaptiveDelay      m_pacing;

};

PPLUGIN_STATIC_LOAD(WASAPI, PSoundChannel)

#endif  // H323_WASAPI







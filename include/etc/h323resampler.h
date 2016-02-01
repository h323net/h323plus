/*
* h323resampler.h
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

#pragma once

#ifdef H323_RESAMPLE

/* Audio Resampler */

struct H323ResamplerSettings;
class H323AudioResampler : public PObject
{
public:

    H323AudioResampler();

    ~H323AudioResampler();

    void Initialise(uint32_t frame_duration,
            int8_t i_channels,
            int8_t o_channels
            );

    void Open(int32_t in_bytes_per_sample,
            int32_t out_bytes_per_sample,
            uint32_t in_freq,
            uint32_t out_freq,
            size_t frame_duration,
            int8_t i_channels,
            int8_t o_channels,
            uint32_t quality
            );

    size_t Process(const uint16_t* in_data,
                size_t input_size
                );

    void Close();


private:

    H323ResamplerSettings * m_resampler;
    bool                    m_configured;

    PMutex     m_mutex;

};

/* Audio ring buffer */

struct H323SpeexBuffer;
class H323AudioBuffer  : public PObject
{
public:

    H323AudioBuffer();
    ~H323AudioBuffer();

    void init(int size);
    void destroy();
    int write(void *data, int len);
    int writezeros(int len);
    int read(void *data, int len);
    int get_available();
    int resize(int len);

private:
    H323SpeexBuffer * m_st;

};

#endif   // H323_RESAMPLE
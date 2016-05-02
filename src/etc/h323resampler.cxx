/*
* h323resampler.cxx
*
* Copyright (c) 2015 ISVO (Asia) Pte Ltd. All Rights Reserved.
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

#ifdef H323_RESAMPLE

#include <etc/h323resampler.h>

extern "C" {
#include <speex/speex_resampler.h>
}

#define AUDIO_resampler_DEFAULT_QUALITY 5
#define SPEEX_resampler_MAX_QUALITY 10

struct H323ResamplerSettings {
    H323ResamplerSettings()
    : in_bytes_per_sample(0), out_bytes_per_sample(0), in_size(0), out_size(0), 
      frame_duration(0), out_buffer(NULL), in_channels(1), out_channels(1), state(NULL)
    {

    }

    size_t in_bytes_per_sample;
    size_t out_bytes_per_sample;
    size_t in_size;
    size_t out_size;
    size_t frame_duration;
    uint8_t* out_buffer;
    int8_t in_channels;
    int8_t out_channels;

    struct {
        spx_int16_t* ptr;
        size_t size_in_samples;
    } tmp_buffer;

    SpeexResamplerState *state;
};


H323AudioResampler::H323AudioResampler()
    : m_resampler(new H323ResamplerSettings()), m_configured(false)
{
    
}


H323AudioResampler::~H323AudioResampler()
{
    Close();
}


void H323AudioResampler::Open(int32_t in_bytes_per_sample, int32_t out_bytes_per_sample,
    uint32_t in_freq, uint32_t out_freq, size_t frame_duration, int8_t i_channels, int8_t o_channels)
{

    PWaitAndSignal m(m_mutex);

    if (!m_resampler || m_resampler->state != 0) // Already open
        return;

    PTRACE(6, "H323RSP\tH323AudioResampler::Open - for in_freq: " 
        << in_freq << " out_freq: " << out_freq << " frame_duration: " << frame_duration 
        << " in_bytes_per_sample: " << in_bytes_per_sample << " out_bytes_per_sample: " 
        << out_bytes_per_sample << " in channels: " << i_channels << " out channels: " 
        << o_channels);

    int ret = 0;
    if (!(m_resampler->state = speex_resampler_init(i_channels, in_freq, out_freq, SPEEX_resampler_MAX_QUALITY, &ret))) {
        PTRACE(2, "H323RSP\tH323AudioResampler::Open - Error speexm_resampler_init() returned %d", ret);
        return;
    }

    m_resampler->in_bytes_per_sample = in_bytes_per_sample;
    m_resampler->out_bytes_per_sample = out_bytes_per_sample;
    m_resampler->in_size = ((in_freq * frame_duration * in_bytes_per_sample) / 1000) << (i_channels == 2 ? 1 : 0);
    m_resampler->out_size = ((out_freq * frame_duration * out_bytes_per_sample) / 1000) << (o_channels == 2 ? 1 : 0);
    m_resampler->in_channels = i_channels;
    m_resampler->out_channels = o_channels;

    PTRACE(6, "H323RSP\tH323AudioResampler::Open - with in_size: " << m_resampler->in_size << " with out_size : " << m_resampler->out_size );

    if (!(m_resampler->out_buffer = (uint8_t*)realloc(m_resampler->out_buffer, m_resampler->out_size))) {
        PTRACE(2, "H323RSP\tH323AudioResampler::Open - Failed to realloc buffer with size=%u", m_resampler->out_size);
        m_resampler->out_size = 0;
        return;
    }

    if (m_resampler->in_channels != m_resampler->out_channels) {
        m_resampler->tmp_buffer.size_in_samples = ((PMAX(in_freq, out_freq) * frame_duration * PMAX(in_bytes_per_sample, out_bytes_per_sample)) / 1000) << (PMAX(m_resampler->in_channels, m_resampler->out_channels) == 2 ? 1 : 0);
        if (!(m_resampler->tmp_buffer.ptr = (spx_int16_t*)realloc(m_resampler->tmp_buffer.ptr, m_resampler->tmp_buffer.size_in_samples * sizeof(spx_int16_t)))) {
            m_resampler->tmp_buffer.size_in_samples = 0;

            PTRACE(2, "H323RSP\tH323AudioResampler::Open - Failed to allocate tmp_buffer.ptr");
            return;
        }
    }

    m_configured = true;

    return;
}


size_t H323AudioResampler::Process(const uint16_t* in_data, size_t input_size)
{

    PWaitAndSignal m(m_mutex);

    int err;
    if (!m_resampler || !m_resampler->state || !m_resampler->out_buffer || !m_configured) {
        PTRACE(2, "H323RSP\tH323AudioResampler::Process - Invalid parameter");
        return 0;
    }

    if (m_configured) {

        spx_uint32_t out_len = (spx_uint32_t)m_resampler->out_size / m_resampler->out_bytes_per_sample;
        input_size /= m_resampler->in_bytes_per_sample;

        PTRACE(6, "H323RSP\tH323AudioResampler::Process - ENTERING with out_len: " << out_len << " m_resampler->out_size: " << m_resampler->out_size << std::endl);

        if (m_resampler->in_channels == m_resampler->out_channels) {
            err = speex_resampler_process_int(m_resampler->state, 0, (const spx_int16_t *)in_data, (spx_uint32_t *)&m_resampler->in_size, (spx_int16_t *)m_resampler->out_buffer, &out_len);
        }
        else {
            if (m_resampler->in_channels == 1) {
                // in_channels = 1, out_channels = 2
                spx_int16_t* pout_data = (spx_int16_t*)(m_resampler->out_buffer);
                speex_resampler_set_output_stride(m_resampler->state, 2);
                err = speex_resampler_process_int(m_resampler->state, 0,
                    (const spx_int16_t *)in_data, (spx_uint32_t *)&m_resampler->in_size,
                    pout_data, &out_len);
                if (err == RESAMPLER_ERR_SUCCESS) {
                    spx_uint32_t i;
                    // duplicate
                    for (i = 0; i < out_len; i += 2) {
                        pout_data[i + 1] = pout_data[i];
                    }
                }
            }
            else {
                // in_channels = 2, out_channels = 1
                spx_uint32_t out_len_chan2 = (out_len << 1);
                err = speex_resampler_process_int(m_resampler->state, 0,
                    (const spx_int16_t *)in_data, (spx_uint32_t *)&m_resampler->in_size,
                    (spx_int16_t *)m_resampler->tmp_buffer.ptr, &out_len_chan2);
                if (err == RESAMPLER_ERR_SUCCESS) {
                    spx_uint32_t i, j;
                    spx_int16_t* pout_data = (spx_int16_t*)(m_resampler->out_buffer);
                    for (i = 0, j = 0; j < out_len_chan2; ++i, j += 2) {
                        pout_data[i] = m_resampler->tmp_buffer.ptr[j];
                    }
                }
            }
        }

        if (err != RESAMPLER_ERR_SUCCESS) {
            PTRACE(2, "H323RSP\tH323AudioResampler::Process - speex_resampler_process_int() failed with error code %d", err);
            return 0;
        }

        PTRACE(6, "H323RSP\tH323AudioResampler::Process - EXITING with out_len: " << out_len << " m_resampler->out_size: " << m_resampler->out_size);
        return (size_t)m_resampler->out_size;
    }

    return 0;
}


uint8_t* H323AudioResampler::GetOutBuffer()
{
    return m_resampler->out_buffer;
}


void H323AudioResampler::Close()
{
    PWaitAndSignal m(m_mutex);

    if (!m_resampler)
        return;

    if (m_configured) {

        m_configured = false;

        if (m_resampler->out_buffer)
            free(m_resampler->out_buffer);

        if (m_resampler->state)
            speex_resampler_destroy(m_resampler->state);

        if (m_resampler->tmp_buffer.ptr)
            free(m_resampler->tmp_buffer.ptr);

        delete m_resampler;
        m_resampler = NULL;

    }
}


//////////////////////////////////////////////////////////////////////////////////////////////

struct H323SpeexBuffer {
    char *data;
    int   size;
    int   read_ptr;
    int   write_ptr;
    int   available;
};

#define H323_MEMSET(dst, c, n) (memset((dst), (c), (n)*sizeof(*(dst))))
#define H323_COPY(dst, src, n) (memcpy((dst), (src), (n)*sizeof(*(dst)) + 0*((dst)-(src)) ))

H323AudioBuffer::H323AudioBuffer()
: m_st(NULL)
{

}

H323AudioBuffer::~H323AudioBuffer()
{
    destroy();
}

void H323AudioBuffer::init(int size)
{
    m_st = (H323SpeexBuffer*)calloc(sizeof(H323SpeexBuffer),1);
    m_st->data = (char*)calloc(size,1);
    m_st->size = size;
    m_st->read_ptr = 0;
    m_st->write_ptr = 0;
    m_st->available = 0;
}

void H323AudioBuffer::destroy()
{
    if (m_st) {                            // Prakash October 2, 2012
        if (m_st->data) free(m_st->data);
        m_st->data = 0;
        free(m_st);
        m_st = 0;
    }
}

int H323AudioBuffer::write(void *_data, int len)
{
    if (!m_st) return 0;

    int end;
    int end1;
    char *data = (char *)_data; 

    if (len > m_st->size) {
        data += len - m_st->size;
        len = m_st->size;
    }
    end = m_st->write_ptr + len;
    end1 = end;
    if (end1 > m_st->size)
        end1 = m_st->size;
    H323_COPY(m_st->data + m_st->write_ptr, data, end1 - m_st->write_ptr);
    if (end > m_st->size)
    {
        end -= m_st->size;
        H323_COPY(m_st->data, data + end1 - m_st->write_ptr, end);
    }
    m_st->available += len;
    if (m_st->available > m_st->size)
    {
        m_st->available = m_st->size;
        m_st->read_ptr = m_st->write_ptr;
    }
    m_st->write_ptr += len;
    if (m_st->write_ptr > m_st->size)
        m_st->write_ptr -= m_st->size;
    return len;
}

int H323AudioBuffer::writezeros(int len)
{
    /* This is almost the same as for speex_buffer_write() but using
    H323_MEMSET() instead of H323_COPY(). Update accordingly. */
    if (!m_st) 
        return 0;                 

    int end=0;
    int end1=0;
    if (len > m_st->size) {
        len = m_st->size;
    }
    end = m_st->write_ptr + len;
    end1 = end;
    if (end1 > m_st->size)
        end1 = m_st->size;
    H323_MEMSET(m_st->data + m_st->write_ptr, 0, end1 - m_st->write_ptr);
    if (end > m_st->size)
    {
        end -= m_st->size;
        H323_MEMSET(m_st->data, 0, end);
    }
    m_st->available += len;
    if (m_st->available > m_st->size)
    {
        m_st->available = m_st->size;
        m_st->read_ptr = m_st->write_ptr;
    }
    m_st->write_ptr += len;
    if (m_st->write_ptr > m_st->size)
        m_st->write_ptr -= m_st->size;
    return len;
}

int H323AudioBuffer::read(void *_data, int len)
{
    if (!m_st) 
        return 0;

    int end, end1;
    char *data = (char *)_data;

    if (len > m_st->available) {
        if (data)
            H323_MEMSET(data + m_st->available, 0, m_st->size - m_st->available);
        len = m_st->available;
    }
    end = m_st->read_ptr + len;
    end1 = end;
    if (end1 > m_st->size)
        end1 = m_st->size;
    if (data)
        H323_COPY(data, m_st->data + m_st->read_ptr, end1 - m_st->read_ptr);

    if (end > m_st->size)
    {
        end -= m_st->size;
        if (data)
            H323_COPY(data + end1 - m_st->read_ptr, m_st->data, end);
    }
    m_st->available -= len;
    m_st->read_ptr += len;
    if (m_st->read_ptr > m_st->size)
        m_st->read_ptr -= m_st->size;
    return len;
}

int H323AudioBuffer::get_available()
{
    int avail = 0;
    if (m_st) {
        avail = m_st->available;
    }
    return avail;
}

int H323AudioBuffer::resize(int len)
{
    if (!m_st) 
        return 0;   

    int old_len = m_st->size;
    if (len > old_len)
    {
        m_st->data = (char*)realloc(m_st->data, len);
        /* FIXME: move data/pointers properly for growing the buffer */
    }
    else {
        /* FIXME: move data/pointers properly for shrinking the buffer */
        m_st->data = (char*)realloc(m_st->data, len);
    }
    return len;
}


#endif  // H323_RESAMPLE

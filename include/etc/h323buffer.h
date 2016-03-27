/*
* h323buffer.h
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


#pragma once

#ifdef H323_FRAMEBUFFER

#include <queue>

class H323FRAME {
public:

    struct Info {
        unsigned  m_sequence;
        unsigned  m_timeStamp;
        PBoolean  m_marker;
        PInt64    m_receiveTime;
    };

    int operator() (const std::pair<H323FRAME::Info, PBYTEArray>& p1,
        const std::pair<H323FRAME::Info, PBYTEArray>& p2) {
        return (p1.first.m_sequence > p2.first.m_sequence);
    }
};

typedef std::priority_queue< std::pair<H323FRAME::Info, PBYTEArray >,
    std::vector< std::pair<H323FRAME::Info, PBYTEArray> >, H323FRAME > RTP_Sortedframes;


class H323_FrameBuffer : public PThread
{
    PCLASSINFO(H323_FrameBuffer, PThread);

public:
    H323_FrameBuffer();

    ~H323_FrameBuffer();

    void Start();

    PBoolean IsRunning();

    virtual void FrameOut(PBYTEArray & /*frame*/, PInt64 /*receiveTime*/, unsigned /*clock*/, PBoolean /*fup*/, PBoolean /*flow*/) {};

    void Main();

    virtual PBoolean FrameIn(unsigned seq, unsigned time, PBoolean marker, unsigned payload, const PBYTEArray & frame);

private:
    RTP_Sortedframes    m_buffer;
    PBoolean            m_threadRunning;

    unsigned            m_frameMarker;           // Number of complete frames;
    PBoolean            m_frameOutput;           // Signal to start output
    unsigned            m_frameStartTime;        // Time to of first packet
    PInt64              m_StartTimeStamp;        // Start Time Stamp
    float               m_calcClockRate;         // Calculated Clockrate (default 90)
    float               m_packetReceived;        // Packet Received count
    float               m_oddTimeCount;          // Odd time count
    float               m_lateThreshold;         // Threshold (percent) of late packets
    PBoolean            m_increaseBuffer;        // Increase Buffer
    float               m_lossThreshold;         // Percentage loss
    float               m_lossCount;             // Actual Packet lost
    float               m_frameCount;            // Number of Frames Received from last Fast Picture Update
    unsigned            m_lastSequence;          // Last Received Sequence Number
    PInt64              m_RenderTimeStamp;       // local realTime to render.

    PMutex bufferMutex;
    PAdaptiveDelay m_outputDelay;
    PBoolean  m_exit;

};

#endif  // H323_FRAMEBUFFER




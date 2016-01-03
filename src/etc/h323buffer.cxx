/*
* h323buffer.cxx
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
* $Id: h323buffer.cxx,v 1.1.2.1 2015/10/10 08:54:36 shorne Exp $
*
*/

#include <h323.h>

#ifdef H323_FRAMEBUFFER

#include <etc/h323buffer.h>
#include <algorithm>
#include <limits>

H323_FrameBuffer::H323_FrameBuffer()
: PThread(10000, NoAutoDeleteThread, HighestPriority), m_threadRunning(false), m_frameMarker(0), m_frameOutput(false), m_frameStartTime(0),
  m_StartTimeStamp(0), m_calcClockRate(90), m_packetReceived(0), m_oddTimeCount(0), m_lateThreshold(5.0), m_increaseBuffer(false),
  m_lossThreshold(1.0), m_lossCount(0), m_frameCount(0), m_lastSequence(0), m_RenderTimeStamp(0), m_exit(false)
{

}

H323_FrameBuffer::~H323_FrameBuffer()
{
    if (m_threadRunning)
        m_exit = true;
}

void H323_FrameBuffer::Start()
{
    m_threadRunning = true;
    Resume();
}

PBoolean H323_FrameBuffer::IsRunning()
{
    return m_threadRunning;
}

void H323_FrameBuffer::Main() {

    PBYTEArray frame;
    PTimeInterval lastMarker;
    int delay = 0;
    PBoolean fup = false;

    while (!m_exit) {
        while (m_frameOutput && m_frameMarker > 0) {
            if (m_buffer.empty()) {
                if (m_frameMarker > 0)
                    m_frameMarker--;
                break;
            }

            // fixed local render clock
            if (m_RenderTimeStamp == 0)
                m_RenderTimeStamp = PTimer::Tick().GetMilliSeconds();

            PBoolean flow = false;

            H323FRAME::Info info;
            bufferMutex.Wait();
            info = m_buffer.top().first;
            frame.SetSize(m_buffer.top().second.GetSize());
            memcpy(frame.GetPointer(), m_buffer.top().second, m_buffer.top().second.GetSize());
            unsigned lastTimeStamp = info.m_timeStamp;
            m_buffer.pop();
            if (info.m_marker && !m_buffer.empty()) { // Peek ahead for next timestamp
                delay = (m_buffer.top().first.m_timeStamp - lastTimeStamp) / (unsigned)m_calcClockRate;
                if (delay <= 0 || delay > 200 || (lastTimeStamp > m_buffer.top().first.m_timeStamp)) {
                    delay = 0;
                    m_RenderTimeStamp = PTimer::Tick().GetMilliSeconds();
                    fup = true;
                }
            }
            bufferMutex.Signal();

            if (m_exit)
                break;

            m_frameCount++;
            if (m_lastSequence) {
                unsigned diff = info.m_sequence - m_lastSequence - 1;
                if (diff > 0) {
                    PTRACE(5, "RTPBUF\tDetected loss of " << diff << " packets.");
                    m_lossCount = m_lossCount + diff;
                }
            }
            m_lastSequence = info.m_sequence;

            if (!fup)
                fup = ((m_lossCount / m_frameCount)*100.0 > m_lossThreshold);

            FrameOut(frame, info.m_receiveTime, (unsigned)m_calcClockRate, fup, flow);
            frame.SetSize(0);
            if (fup) {
                m_lossCount = m_frameCount = 0;
                fup = false;
            }

            if (info.m_marker && m_frameMarker > 0) {
                if (m_increaseBuffer) {
                    delay = delay * 2;
                    m_increaseBuffer = false;
                }
                m_RenderTimeStamp += delay;
                PInt64 nowTime = PTimer::Tick().GetMilliSeconds();
                unsigned ldelay = (unsigned)((m_RenderTimeStamp > nowTime) ? m_RenderTimeStamp - nowTime : 0);
                if (ldelay > 200 || m_frameMarker > 5) ldelay = 0;
                if (!ldelay)  m_RenderTimeStamp = nowTime;
                m_frameMarker--;
                m_outputDelay.Delay(ldelay);
            }
            else
                PThread::Sleep(2);
        }
        PThread::Sleep(5);
    }
    bufferMutex.Wait();
    while (!m_buffer.empty())
        m_buffer.pop();
    bufferMutex.Signal();

    m_threadRunning = false;
}

PBoolean H323_FrameBuffer::FrameIn(unsigned seq, unsigned time, PBoolean marker, unsigned payload, const PBYTEArray & frame)
{

    if (!m_threadRunning) {  // Start thread on first frame.
        Resume();
        m_threadRunning = true;
    }

    if (m_exit)
        return false;

    PInt64 now = PTimer::Tick().GetMilliSeconds();
    // IF we haven't started or the clockrate goes out of bounds.
    if (!m_frameStartTime) {
        m_frameStartTime = time;
        m_StartTimeStamp = PTimer::Tick().GetMilliSeconds();
    }
    else if (marker && m_frameOutput) {
        m_calcClockRate = (float)(time - m_frameStartTime) / (PTimer::Tick().GetMilliSeconds() - m_StartTimeStamp);
        if (m_calcClockRate > 100 || m_calcClockRate < 40 || (m_calcClockRate == numeric_limits<unsigned int>::infinity())) {
            PTRACE(4, "RTPBUF\tErroneous ClockRate: Resetting...");
            m_calcClockRate = 90;
            m_frameStartTime = time;
            m_StartTimeStamp = PTimer::Tick().GetMilliSeconds();
        }
    }

    H323FRAME::Info info;
    info.m_sequence = seq;
    info.m_marker = marker;
    info.m_timeStamp = time;
    info.m_receiveTime = now;

    PBYTEArray * m_frame = new PBYTEArray(payload + 12);
    memcpy(m_frame->GetPointer(), (PRemoveConst(PBYTEArray, &frame))->GetPointer(), payload + 12);

    bufferMutex.Wait();
    m_packetReceived++;
    if (m_frameOutput && !m_buffer.empty() && info.m_sequence < m_buffer.top().first.m_sequence) {
        m_oddTimeCount++;
        PTRACE(6, "RTPBUF\tLate Packet Received " << (m_oddTimeCount / m_packetReceived)*100.0 << "%");
        if ((m_oddTimeCount / m_packetReceived)*100.0 > m_lateThreshold) {
            PTRACE(4, "RTPBUF\tLate Packet threshold reached increasing buffer.");
            m_increaseBuffer = true;
            m_packetReceived = 0;
            m_oddTimeCount = 0;
        }
    }
    m_buffer.push(pair<H323FRAME::Info, PBYTEArray>(info, *m_frame));
    delete m_frame;
    bufferMutex.Signal();

    if (marker) {
        m_frameMarker++;
        // Make sure we have a min of 3 frames in buffer to start
        if (!m_frameOutput && m_frameMarker > 2)
            m_frameOutput = true;
    }

    return true;
}


#endif  // H323_FRAMEBUFFER


/*
 * h323aec.h
 *
 * Acoustic Echo Cancellation for the h323plus Library.
 *
 * Copyright (c) 2012 ISVO (Asia) Pte Ltd. All Rights Reserved.
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


#ifdef P_USE_PRAGMA
#pragma interface
#endif

extern "C" {
#include <speex/speex_echo.h>
#include <speex/speex_preprocess.h>
}

#include <map>

class H323AEC {
public:
    struct BufferFrame {
        BufferFrame();
        BufferFrame(PINDEX sz);
        ~BufferFrame();

        PInt64      receiveTime;
        PInt64      echoTime;
        PBYTEArray  frame;
    };
};

class H323_AECBuffer : public std::map< unsigned, H323AEC::BufferFrame >
{
public:
    H323_AECBuffer();
    ~H323_AECBuffer();

    void Initialise(PINDEX size, PINDEX byteSize, PINDEX clockRate);
    void ShutDown();

    PBoolean Send(BYTE * buffer, unsigned & length);
    void Receive(BYTE * buffer, unsigned & length);

protected:

    PINDEX m_bufferTime;
    PMutex m_bufferMutex;

private:
    unsigned m_curPos;
};


/** This class implements Acoustic Echo Cancellation
  * The principal is to copy to a buffer incoming audio.
  * after it has been decoded and when recording the audio 
  * to remove the echo pattern from the incoming audio 
  * prior to sending to the encoder..
  */

class H323Aec : public PObject
{
  PCLASSINFO(H323Aec, PObject);
public:

  /**@name Construction */
  //@{
  /**Create a new canceler.
   */
     H323Aec(int _clock = 8000, int _sampletime = 20, int _buffers = 3);
     ~H323Aec();
  //@}

     struct BufferFrame
    {
       PInt64      receiveTime;
       PInt64      echoTime;
       PBYTEArray  frame;
    };

  /**@@name Basic operations */
  //@{
  /**Recording Channel. Should be called prior to encoding audio
   */
    void Send(BYTE * buffer, unsigned & length);

  /**Playing Channel  Should be called after decoding and prior to playing.
   */
    void Receive(BYTE * buffer, unsigned & length);

  //@}

protected:

  H323_AECBuffer           m_echoBuffer;

  SpeexEchoState * m_echoState;
  SpeexPreprocessState * m_preprocessState;

  unsigned m_clockrate;                      // Frame Rate default 8000hz for narrowband audio
  unsigned m_samplesFrame;                   // Buffer Bytes
  unsigned m_BufferBytes;                    // Buffer Bytes

  H323_AECBuffer m_buffer;
  spx_int16_t  * m_echo_buf;                 // Buffer containing the echo
  spx_int16_t  * m_ref_buf;                  // reference recorded sound
  spx_int16_t  * m_temp_buf;                 // temp buffer for preprocess

  unsigned m_tail;                           // Tail of echo to search

};

// End Of File ///////////////////////////////////////////////////////////////

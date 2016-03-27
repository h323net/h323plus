/* h46026mgr.h
 *
 * Copyright (c) 2013 ISVO (Asia) Pte Ltd. All Rights Reserved.
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

#ifndef H_H460_Featurestd26
#define H_H460_Featurestd26

#include <h460/h46026.h>
#include <queue>
#include <vector>
#include <map>

class H46026UDPBuffer {
public:
    H46026UDPBuffer(int sessionId, PBoolean rtp);

    H46026_ArrayOf_FrameData & GetData();

    void SetFrame(const PBYTEArray & data);

    H46026_UDPFrame & GetBuffer();
    void ClearBuffer();

    PINDEX GetSize();
    PINDEX GetPacketCount();

protected:
    PINDEX m_size;
    H46026_UDPFrame m_data;
    PBoolean m_rtp;
};


class socketOrder {
public:
     enum priority {
        Priority_Critical=1,    // Audio RTP
        Priority_Discretion,    // Video RTP
        Priority_High,          // Signaling   
        Priority_Low            // RTCP
     };

     struct MessageHeader {
        unsigned id;
        unsigned crv;
        int sessionId;
        int priority;
        int delay;
        PInt64 packTime;
     };

     int operator() ( const std::pair<PBYTEArray, MessageHeader>& p1,
                      const std::pair<PBYTEArray, MessageHeader>& p2 ) {
           return ((p1.second.priority >= p2.second.priority) && 
                     (p1.second.packTime > p2.second.packTime));
     }

     PString PriorityAsString();
};

typedef std::priority_queue< std::pair<PBYTEArray, socketOrder::MessageHeader >, 
        std::vector< std::pair<PBYTEArray, socketOrder::MessageHeader> >, socketOrder > H46026SocketQueue;

typedef std::map<int,H46026UDPBuffer*> H46026CallMap;
typedef std::map<unsigned, H46026CallMap >  H46026RTPBuffer;

//-------------------------------------------------------------------------------

class H46026_MediaFrame  : public PBYTEArray
{
public:
    H46026_MediaFrame();
    H46026_MediaFrame(const BYTE * data, PINDEX len);

    PBoolean GetMarker() const;
    void PrintInfo(ostream & strm) const;
};

//-------------------------------------------------------------------------------

class Q931;
class H225_H323_UserInformation;
class H46026ChannelManager : public PObject
{  
  PCLASSINFO(H46026ChannelManager, PObject);

public:
    H46026ChannelManager();
    ~H46026ChannelManager();

    enum PacketTypes {
      e_Audio,   /// Audio packet
      e_Video,   /// Video packet
      e_Data,    /// data packet
      e_extVideo /// extended video
    };

    /* Set the pipe bandwidth Default is 384k */
    void SetPipeBandwidth(unsigned bps);

    /** Clear Buffers */
    /* Call this is clear the channel buffers at the end of a call */
    /* This MUST be called at the end of a call */
    void BufferRelease(unsigned crv);

    /** Events */
    /* Retrieve a Signal Message. Note RAS messages are included.
     */
    virtual void SignalMsgIn(const Q931 & /*pdu*/);
    virtual void SignalMsgIn(unsigned /*crv*/, const Q931 & /*pdu*/) {};

    /* Retrieve an RTP/RTCP Frame. The sessionID is the id of the session
     */
    virtual void RTPFrameIn(unsigned /*crv*/, PINDEX /*sessionId*/, PBoolean /*rtp*/, const PBYTEArray & /*data*/);
    virtual void RTPFrameIn(unsigned /*crv*/, PINDEX /*sessionId*/, PBoolean /*rtp*/, const BYTE * /*data*/, PINDEX /*len*/) {};

    /* Fast Picture Update Required on the non-tunneled side.
     */
    virtual void FastUpdatePictureRequired(unsigned /*crv*/, PINDEX /*sessionId*/) {};


    /* Methods */
    /* Sending to the socket */
    /* Post a Signal Message. Note RAS messages must be included.
     */
    PBoolean SignalMsgOut(const Q931 & pdu);
    PBoolean SignalMsgOut(const BYTE * data, PINDEX len);

    /* Post an RTP/RTCP Frame. Need to specify the basic packet type, sessionID and whether RTP or RTCP packet
     */
    PBoolean RTPFrameOut(unsigned crv, PacketTypes id, PINDEX sessionId, PBoolean rtp, const BYTE * data, PINDEX len);
    PBoolean RTPFrameOut(unsigned crv, PacketTypes id, PINDEX sessionId, PBoolean rtp, PBYTEArray & data);

    /* Collect an incoming to send to the socket.
        if the function return 
            True: data has been copied and ready to send on the wire.
            False: there is no data or data is not ready.
        WARNING: This function does not block and returns immediately if false.
        Best practise would be to call from a threaded loop and use PThread::Sleep
     */
    PBoolean SocketOut(BYTE * data, PINDEX & len);
    PBoolean SocketOut(PBYTEArray & data, PINDEX & len);

    /* Receiving from the socket */
    /* Process an incoming message from the socket.
        Returns false if message could not be handled or decoded into Q931.
     */
    PBoolean SocketIn(const BYTE * data, PINDEX len);
    PBoolean SocketIn(const PBYTEArray & data);
    PBoolean SocketIn(const Q931 & q931);

protected:
    PBoolean WriteQueue(const Q931 & msg, const socketOrder::MessageHeader & prior);
    PBoolean WriteQueue(const PBYTEArray & data, const socketOrder::MessageHeader & prior);

    PBoolean PackageFrame(PBoolean rtp, unsigned crv, PacketTypes id, PINDEX sessionId, H46026_UDPFrame & data);
    H46026UDPBuffer * GetRTPBuffer(unsigned crv, int sessionId);

    PBoolean ProcessQueue();

    unsigned NextPacketCounter();

private:
    double                     m_mbps;
    PBoolean                   m_socketPacketReady;
    PInt64                     m_currentPacketTime;
    unsigned                   m_pktCounter;

    H225_H323_UserInformation  m_uuie;
    H46026RTPBuffer            m_rtpBuffer;
    PMutex                     m_writeMutex;
    H46026SocketQueue          m_socketQueue;
    PMutex                     m_queueMutex;
};


#endif  // H_H460_Featurestd26


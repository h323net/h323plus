/*
 * h235chan.h
 *
 * H.235 Secure RTP channel class.
 *
 * h323plus library
 *
 * Copyright (c) 2011 ISVO (Asia) Pte Ltd.
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



#ifdef H323_H235

#include <channels.h>
#include "h235/h235caps.h"
#include "h235/h235crypto.h"

/**This class is a derived Class for encpsulating the IETF Real Time Protocol 
   interface. It's only aim is to expose the Created UDP Data channel derive a
   new class for binding to the OpenSSL TLS system.
 */

class RTP_Session;
class H323SecureRTPChannel  : public H323_RTPChannel
{
  PCLASSINFO(H323SecureRTPChannel, H323_RTPChannel);

  public:
  /**@name Construction/Deconstructor */
  //@{
    /**Create a new channel.
     */
    H323SecureRTPChannel(
      H323Connection & connection,                      ///< Connection to endpoint for channel
      const H323Capability & capability,                ///< Capability channel is using
      Directions direction,                             ///< Direction of channel
      RTP_Session & rtp                                 ///< RTP session for channel    
    );

    /**Destroy Class.
     */
    ~H323SecureRTPChannel();         ///< Destroy the channel
  //@}

  /**@name Overrides from H323_RTPChannel */
  //@{
    /**This is called to clean up any threads on connection termination.
     */
     virtual void CleanUpOnTermination();
  //@}

  /**@name Overrides from class H323Channel */
  //@{
    /**Fill out the OpenLogicalChannel PDU for the particular channel type.
     */
    virtual PBoolean OnSendingPDU(
      H245_OpenLogicalChannel & openPDU  /// Open PDU to send. 
    ) const;

    /**This is called when request to create a channel is received from a
       remote machine and is about to be acknowledged.
     */
    virtual void OnSendOpenAck(
      const H245_OpenLogicalChannel & open,   /// Open PDU
      H245_OpenLogicalChannelAck & ack        /// Acknowledgement PDU
    ) const;

    /**This is called after a request to create a channel occurs from the
       local machine via the H245LogicalChannelDict::Open() function, and
       the request has been acknowledged by the remote endpoint.

       The default makes sure the parameters are compatible and passes on
       the PDU to the rtp session.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_OpenLogicalChannel & pdu,    /// Open PDU
      unsigned & errorCode                    /// Error code on failure
    );

    /**This is called after a request to create a channel occurs from the
       local machine via the H245LogicalChannelDict::Open() function, and
       the request has been acknowledged by the remote endpoint.

       The default makes sure the parameters are compatible and passes on
       the PDU to the rtp session.
     */
    virtual PBoolean OnReceivedAckPDU(
      const H245_OpenLogicalChannelAck & pdu /// Acknowledgement PDU
    );
  //@}

  /**@name Operations */
  //@{
    /**Fill out the OpenLogicalChannel PDU for the particular channel type.
     */
    virtual PBoolean OnSendingPDU(
      H245_H2250LogicalChannelParameters & param  /// Open PDU to send.
      ) const;

    /**This is called when request to create a channel is received from a
       remote machine and is about to be acknowledged.
     */
    virtual void OnSendOpenAck(
      H245_H2250LogicalChannelAckParameters & param /// Acknowledgement PDU
    ) const;

    /**This is called after a request to create a channel occurs from the
       local machine via the H245LogicalChannelDict::Open() function, and
       the request has been acknowledged by the remote endpoint.

       The default behaviour sets the remote ports to send UDP packets to.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_H2250LogicalChannelParameters & param, /// Acknowledgement PDU
      unsigned & errorCode                              /// Error on failure
    );

    /**This is called after a request to create a channel occurs from the
       local machine via the H245LogicalChannelDict::Open() function, and
       the request has been acknowledged by the remote endpoint.

       The default behaviour sets the remote ports to send UDP packets to.
     */
    virtual PBoolean OnReceivedAckPDU(
      const H245_H2250LogicalChannelAckParameters & param /// Acknowledgement PDU
    );

    /**Get the active payload type used by this channel.
       This will use the dynamic payload type configured for the channel, or
       the fixed payload type defined by the media format.
       */
    virtual RTP_DataFrame::PayloadTypes GetRTPPayloadType() const;

    /**Set the dynamic payload type used by this channel.
      */
    virtual PBoolean SetDynamicRTPPayloadType(
      int newType  ///< New RTP payload type number
    );
  //@}

  /**@name Utilities */
  //@{
    /** Read a DataFrame */
    virtual PBoolean ReadFrame(
        DWORD & rtpTimestamp,     /// TimeStamp
        RTP_DataFrame & frame     /// RTP data frame
        );

    /** Write a DataFrame */
    virtual PBoolean WriteFrame(
        RTP_DataFrame & frame     /// RTP data frame
        );
  //@}

protected:
    PString        m_algorithm;
    H235Session    m_encryption;
    int            m_payload;

};


//////////////////////////////////////////////////////////////////////////////////////////////////////
// Generic Security Support

class H323SecureChannel : public H323Channel
{
public:
    H323SecureChannel(H323Connection & conn, const H323Capability & cap, H323Channel * baseChannel);
    ~H323SecureChannel();

    virtual void SetNumber(const H323ChannelNumber & num);
    virtual H323Channel::Directions GetDirection() const;
    virtual unsigned GetSessionID() const;

    virtual PBoolean SetInitialBandwidth();

    virtual RTP_DataFrame::PayloadTypes GetRTPPayloadType() const;

    virtual void Receive();
    virtual void Transmit();
    	
    virtual PBoolean Open();
    virtual PBoolean Start();

    virtual PBoolean OnSendingPDU(H245_OpenLogicalChannel & openPDU) const;

    virtual void OnSendOpenAck(const H245_OpenLogicalChannel & openPDU, H245_OpenLogicalChannelAck & ack) const;

    virtual PBoolean OnReceivedPDU(const H245_OpenLogicalChannel & pdu, unsigned & errorCode);

    virtual PBoolean OnReceivedAckPDU(const H245_OpenLogicalChannelAck & pdu);


    PBoolean ReadFrame(RTP_DataFrame & frame);

    PBoolean WriteFrame(RTP_DataFrame & frame);

    void CleanUpOnTermination();

protected:
    H323Channel *    m_baseChannel;

    PString          m_algorithm;
    H235Session      m_encryption;
    int              m_payload;

};

#endif
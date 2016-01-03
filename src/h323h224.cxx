/*
 * h323h224.cxx
 *
 * H.323 H.224 logical channel establishment implementation for the 
 * OpenH323 Project.
 *
 * Copyright (c) 2006 Network for Educational Technology, ETH Zurich.
 * Written by Hannes Friederich.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * Contributor(s): ______________________________________.
 *
 * $Id: h323h224.cxx,v 1.16.2.1 2015/10/10 08:53:44 shorne Exp $
 *
 */

#include <h323.h>
#include <h323.h>
#include <h245.h>

#ifdef H323_H224

#ifdef __GNUC__
#pragma implementation "h323h224.h"
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4244)
#endif

#include <h323h224.h>

#ifdef H323_H235
#include <h235/h235chan.h>
#endif

H323_H224Capability::H323_H224Capability()
: H323DataCapability(640)
{
//  SetPayloadType((RTP_DataFrame::PayloadTypes)100);
}

H323_H224Capability::~H323_H224Capability()
{
}

PObject::Comparison H323_H224Capability::Compare(const PObject & obj) const
{
  Comparison result = H323DataCapability::Compare(obj);

  if(result != EqualTo)    {
    return result;
  }
    
  PAssert(PIsDescendant(&obj, H323_H224Capability), PInvalidCast);
    
  return EqualTo;
}

PObject * H323_H224Capability::Clone() const
{
  return new H323_H224Capability(*this);
}

unsigned H323_H224Capability::GetSubType() const
{
  return H245_DataApplicationCapability_application::e_h224;
}

PString H323_H224Capability::GetFormatName() const
{
  return "H.224";
}

H323Channel * H323_H224Capability::CreateChannel(H323Connection & connection,
                                                 H323Channel::Directions direction,
                                                 unsigned int sessionID,
                                                 const H245_H2250LogicalChannelParameters * /*params*/) const
{
 
    RTP_Session *session;
    H245_TransportAddress addr;
    connection.GetControlChannel().SetUpTransportPDU(addr, H323Transport::UseLocalTSAP);
    session = connection.UseSession(sessionID, addr, direction);
    
  if(session == NULL) {
    return NULL;
  } 
  
  return new H323_H224Channel(connection, *this, direction, (RTP_UDP &)*session, sessionID);
}

PBoolean H323_H224Capability::OnSendingPDU(H245_DataApplicationCapability & pdu) const
{
  pdu.m_maxBitRate = maxBitRate;
  pdu.m_application.SetTag(H245_DataApplicationCapability_application::e_h224);
    
  H245_DataProtocolCapability & dataProtocolCapability = (H245_DataProtocolCapability &)pdu.m_application;
  dataProtocolCapability.SetTag(H245_DataProtocolCapability::e_hdlcFrameTunnelling);
    
  return TRUE;
}

PBoolean H323_H224Capability::OnSendingPDU(H245_DataMode & pdu) const
{
  pdu.m_bitRate = maxBitRate;
  pdu.m_application.SetTag(H245_DataMode_application::e_h224);
    
  return TRUE;
}

PBoolean H323_H224Capability::OnReceivedPDU(const H245_DataApplicationCapability & pdu)
{

  if (pdu.m_application.GetTag() != H245_DataApplicationCapability_application::e_h224)
      return FALSE;

  const H245_DataProtocolCapability & dataProtocolCapability = pdu.m_application;
  if (dataProtocolCapability.GetTag() != H245_DataProtocolCapability::e_hdlcFrameTunnelling)
      return FALSE;

  maxBitRate = pdu.m_maxBitRate;
  return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////

H323_H224Channel::H323_H224Channel(H323Connection & connection,
                                   const H323Capability & capability,
                                   H323Channel::Directions theDirection,
                                   RTP_UDP & theSession,
                                   unsigned theSessionID)
: H323Channel(connection, capability),
  rtpSession(theSession),
  rtpCallbacks(*(H323_RTP_Session *)theSession.GetUserData())
#ifdef H323_H235
  ,secChannel(NULL)
#endif
{
  direction = theDirection;
  sessionID = theSessionID;
    
  h224Handler = NULL;
    
  rtpPayloadType = (RTP_DataFrame::PayloadTypes)100;
}

H323_H224Channel::~H323_H224Channel()
{
  // h224Handler is deleted by H323Connection
}

H323Channel::Directions H323_H224Channel::GetDirection() const
{
  return direction;
}

PBoolean H323_H224Channel::SetInitialBandwidth()
{
  return TRUE;
}

PBoolean H323_H224Channel::Open()
{
  PBoolean result = H323Channel::Open();
    
  if(result == FALSE) {
    return FALSE;
  }
    
  return TRUE;
}

PBoolean H323_H224Channel::Start()
{
  if (!Open())
    return FALSE;

  PTRACE(4,"H224\tStarting H.224 " << (direction == H323Channel::IsTransmitter ? "Transmitter" : "Receiver") << " Channel");
    
  if (!h224Handler)
      h224Handler = connection.CreateH224ProtocolHandler(direction,sessionID);

  if (!h224Handler) {
      PTRACE(4,"H224\tError starting " << (direction == H323Channel::IsTransmitter ? "Transmitter" : "Receiver"));
      return false;
  }

#ifdef H323_H235
  if (secChannel)
      h224Handler->AttachSecureChannel(secChannel);
#endif
    
  if(direction == H323Channel::IsReceiver) {
    h224Handler->StartReceive();
  }    else {
    h224Handler->StartTransmit();
  }
    
  return TRUE;
}

void H323_H224Channel::Close()
{
  if(terminating) {
    return;
  }
    
  if(h224Handler != NULL) {
    
    if(direction == H323Channel::IsReceiver) {
      h224Handler->StopReceive();
    } else {
      h224Handler->StopTransmit();
    }

    delete h224Handler;
  }
    
 // H323Channel::Close();
}

void H323_H224Channel::Receive()
{

}
void H323_H224Channel::Transmit()
{

}

unsigned H323_H224Channel::GetSessionID() const
{
  return sessionID;
}

PBoolean H323_H224Channel::OnSendingPDU(H245_OpenLogicalChannel & open) const
{
  open.m_forwardLogicalChannelNumber = (unsigned)number;
        
  if(open.HasOptionalField(H245_OpenLogicalChannel::e_reverseLogicalChannelParameters)) {
      
    open.m_reverseLogicalChannelParameters.IncludeOptionalField(
        H245_OpenLogicalChannel_reverseLogicalChannelParameters::e_multiplexParameters);
            
    open.m_reverseLogicalChannelParameters.m_multiplexParameters.SetTag(
        H245_OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters::e_h2250LogicalChannelParameters);

    if (connection.OnSendingOLCGenericInformation(GetSessionID(),open.m_genericInformation,false))
        open.IncludeOptionalField(H245_OpenLogicalChannel::e_genericInformation);
            
    return OnSendingPDU(open.m_reverseLogicalChannelParameters.m_multiplexParameters);
    
  } else {
      
    open.m_forwardLogicalChannelParameters.m_multiplexParameters.SetTag(
        H245_OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters::e_h2250LogicalChannelParameters);

    if (connection.OnSendingOLCGenericInformation(GetSessionID(),open.m_genericInformation,false))
        open.IncludeOptionalField(H245_OpenLogicalChannel::e_genericInformation);
        
    return OnSendingPDU(open.m_forwardLogicalChannelParameters.m_multiplexParameters);
  }
}

void H323_H224Channel::OnSendOpenAck(const H245_OpenLogicalChannel & openPDU, 
                                        H245_OpenLogicalChannelAck & ack) const
{
  // set forwardMultiplexAckParameters option
  ack.IncludeOptionalField(H245_OpenLogicalChannelAck::e_forwardMultiplexAckParameters);
    
  // select H225 choice
  ack.m_forwardMultiplexAckParameters.SetTag(
    H245_OpenLogicalChannelAck_forwardMultiplexAckParameters::e_h2250LogicalChannelAckParameters);
    
  // get H225 params
  H245_H2250LogicalChannelAckParameters & param = ack.m_forwardMultiplexAckParameters;
    
  // set session ID
  param.IncludeOptionalField(H245_H2250LogicalChannelAckParameters::e_sessionID);
  const H245_H2250LogicalChannelParameters & openparam =
      openPDU.m_forwardLogicalChannelParameters.m_multiplexParameters;

  // Set Generic information
  if (connection.OnSendingOLCGenericInformation(GetSessionID(), ack.m_genericInformation,true))
       ack.IncludeOptionalField(H245_OpenLogicalChannel::e_genericInformation);
    
  unsigned sessionID = openparam.m_sessionID;
  param.m_sessionID = sessionID;
    
  OnSendOpenAck(param);
}

PBoolean H323_H224Channel::OnReceivedPDU(const H245_OpenLogicalChannel & open,
                                     unsigned & errorCode)
{
  if(direction == H323Channel::IsReceiver) {
    number = H323ChannelNumber(open.m_forwardLogicalChannelNumber, TRUE);
  }
    
  PBoolean reverse = open.HasOptionalField(H245_OpenLogicalChannel::e_reverseLogicalChannelParameters);
  const H245_DataType & dataType = reverse ? open.m_reverseLogicalChannelParameters.m_dataType
                                           : open.m_forwardLogicalChannelParameters.m_dataType;
    
  if (!capability->OnReceivedPDU(dataType, direction)) {
      
    errorCode = H245_OpenLogicalChannelReject_cause::e_dataTypeNotSupported;
    return FALSE;
  }

  if (open.HasOptionalField(H245_OpenLogicalChannel::e_genericInformation) && 
      !connection.OnReceiveOLCGenericInformation(GetSessionID(),open.m_genericInformation, false)) {
        errorCode = H245_OpenLogicalChannelReject_cause::e_unspecified;
        PTRACE(2, "LogChan\tOnReceivedPDU Invalid Generic Parameters");
        return FALSE;
  }
    
  if (reverse) {
    if (open.m_reverseLogicalChannelParameters.m_multiplexParameters.GetTag() ==
            H245_OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters::e_h2250LogicalChannelParameters) 
    {
      return OnReceivedPDU(open.m_reverseLogicalChannelParameters.m_multiplexParameters, errorCode);
    }
      
  } else {
    if (open.m_forwardLogicalChannelParameters.m_multiplexParameters.GetTag() ==
            H245_OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters::e_h2250LogicalChannelParameters)
    {
      return OnReceivedPDU(open.m_forwardLogicalChannelParameters.m_multiplexParameters, errorCode);
    }
  }

  errorCode = H245_OpenLogicalChannelReject_cause::e_unsuitableReverseParameters;
  return FALSE;
}

PBoolean H323_H224Channel::OnReceivedAckPDU(const H245_OpenLogicalChannelAck & ack)
{
  if (!ack.HasOptionalField(H245_OpenLogicalChannelAck::e_forwardMultiplexAckParameters)) {
    return FALSE;
  }
    
  if (ack.m_forwardMultiplexAckParameters.GetTag() !=
    H245_OpenLogicalChannelAck_forwardMultiplexAckParameters::e_h2250LogicalChannelAckParameters)
  {
    return FALSE;
  }

  if (ack.HasOptionalField(H245_OpenLogicalChannelAck::e_genericInformation) && 
    !connection.OnReceiveOLCGenericInformation(GetSessionID(), ack.m_genericInformation, true)) {
     return FALSE;
  }
    
  return OnReceivedAckPDU(ack.m_forwardMultiplexAckParameters);
}

PBoolean H323_H224Channel::OnSendingPDU(H245_H2250LogicalChannelParameters & param) const
{
  param.m_sessionID = sessionID;
    
  param.IncludeOptionalField(H245_H2250LogicalChannelParameters::e_mediaGuaranteedDelivery);
  param.m_mediaGuaranteedDelivery = FALSE;
    
  // unicast must have mediaControlChannel
  if (rtpSession.GetLocalControlPort() > 0) {
    H323TransportAddress mediaControlAddress(rtpSession.GetLocalAddress(), rtpSession.GetLocalControlPort());
    param.IncludeOptionalField(H245_H2250LogicalChannelParameters::e_mediaControlChannel);
    mediaControlAddress.SetPDU(param.m_mediaControlChannel);
  }
    
  if (direction == H323Channel::IsReceiver  && rtpSession.GetLocalDataPort() > 0) {
    // set mediaChannel
    H323TransportAddress mediaAddress(rtpSession.GetLocalAddress(), rtpSession.GetLocalDataPort());
    param.IncludeOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaChannel);
    mediaAddress.SetPDU(param.m_mediaChannel);
  }  
    
  // Set dynamic payload type, if is one
  int rtpPayloadType = GetDynamicRTPPayloadType();
  
  if (rtpPayloadType >= RTP_DataFrame::DynamicBase && rtpPayloadType < RTP_DataFrame::IllegalPayloadType) {
    param.IncludeOptionalField(H245_H2250LogicalChannelParameters::e_dynamicRTPPayloadType);
    param.m_dynamicRTPPayloadType = rtpPayloadType;
  }

  param.IncludeOptionalField(H245_H2250LogicalChannelParameters::e_transportCapability);
  H245_TransportCapability & cap = param.m_transportCapability;
    cap.IncludeOptionalField(H245_TransportCapability::e_mediaChannelCapabilities); 
    H245_ArrayOf_MediaChannelCapability & mediaCaps = cap.m_mediaChannelCapabilities;
    mediaCaps.SetSize(1);
    H245_MediaChannelCapability & mediaCap = mediaCaps[0];
    mediaCap.IncludeOptionalField(H245_MediaChannelCapability::e_mediaTransport);
    H245_MediaTransportType & transport = mediaCap.m_mediaTransport;
    if (rtpSession.GetLocalDataPort() > 0)
        transport.SetTag(H245_MediaTransportType::e_ip_UDP);
    else
        transport.SetTag(H245_MediaTransportType::e_ip_TCP);

  return TRUE;
}

void H323_H224Channel::OnSendOpenAck(H245_H2250LogicalChannelAckParameters & param) const
{
  // set mediaControlChannel
    if (rtpSession.GetLocalControlPort() > 0) {
        H323TransportAddress mediaControlAddress(rtpSession.GetLocalAddress(), rtpSession.GetLocalControlPort());
        param.IncludeOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaControlChannel);
        mediaControlAddress.SetPDU(param.m_mediaControlChannel);
    }
    
  // set mediaChannel
    if (rtpSession.GetLocalDataPort() > 0) {
        H323TransportAddress mediaAddress(rtpSession.GetLocalAddress(), rtpSession.GetLocalDataPort());
        param.IncludeOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaChannel);
        mediaAddress.SetPDU(param.m_mediaChannel);
    }
    
  // Set dynamic payload type, if is one
  int rtpPayloadType = GetDynamicRTPPayloadType();
  if (rtpPayloadType >= RTP_DataFrame::DynamicBase && rtpPayloadType < RTP_DataFrame::IllegalPayloadType) {
    param.IncludeOptionalField(H245_H2250LogicalChannelAckParameters::e_dynamicRTPPayloadType);
    param.m_dynamicRTPPayloadType = rtpPayloadType;
  }
}

PBoolean H323_H224Channel::OnReceivedPDU(const H245_H2250LogicalChannelParameters & param,
                           unsigned & errorCode)
{
  if (param.m_sessionID != sessionID) {
    errorCode = H245_OpenLogicalChannelReject_cause::e_invalidSessionID;
    return FALSE;
  }
    
  PBoolean ok = FALSE;
    
  if (param.HasOptionalField(H245_H2250LogicalChannelParameters::e_mediaControlChannel)) {
    if (!ExtractTransport(param.m_mediaControlChannel, FALSE, errorCode)) {
      return FALSE;
    }
    ok = TRUE;
  }
    
  if (param.HasOptionalField(H245_H2250LogicalChannelParameters::e_mediaChannel)) {
    if (ok && direction == H323Channel::IsReceiver) {
        // ? TODO what to do here?
    } else if (!ExtractTransport(param.m_mediaChannel, TRUE, errorCode)) {
      return FALSE;
    }
    ok = TRUE;
  }

  if (IsMediaTunneled())
    ok = TRUE;
    
  if (param.HasOptionalField(H245_H2250LogicalChannelParameters::e_dynamicRTPPayloadType)) {
    SetDynamicRTPPayloadType(param.m_dynamicRTPPayloadType);
  }
    
  if (ok) {
    return TRUE;
  }
    
  errorCode = H245_OpenLogicalChannelReject_cause::e_unspecified;
  return FALSE;
}

PBoolean H323_H224Channel::OnReceivedAckPDU(const H245_H2250LogicalChannelAckParameters & param)
{
  if (!param.HasOptionalField(H245_H2250LogicalChannelAckParameters::e_sessionID)) {
  }
    
  if (!IsMediaTunneled()) {
      if (!param.HasOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaControlChannel)) {
        return FALSE;
      }     
      unsigned errorCode;
      if (!ExtractTransport(param.m_mediaControlChannel, FALSE, errorCode)) {
        return FALSE;
      }
        
      if (!param.HasOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaChannel)) {
        return FALSE;
      }
      if (!ExtractTransport(param.m_mediaChannel, TRUE, errorCode)) {
        return FALSE;
      }
  }
    
  if (param.HasOptionalField(H245_H2250LogicalChannelAckParameters::e_dynamicRTPPayloadType)) {
    SetDynamicRTPPayloadType(param.m_dynamicRTPPayloadType);
  }
    
  return TRUE;
}

PBoolean H323_H224Channel::SetDynamicRTPPayloadType(int newType)
{
  if(newType == -1) {
    return TRUE;
  }
    
  if(newType < RTP_DataFrame::DynamicBase || newType >= RTP_DataFrame::IllegalPayloadType) {
    return FALSE;
  }
    
  if(rtpPayloadType < RTP_DataFrame::DynamicBase) {
    return FALSE;
  }
    
  rtpPayloadType = (RTP_DataFrame::PayloadTypes)newType;
    
  return TRUE;
}
/*
OpalMediaStream * H323_H224Channel::GetMediaStream() const
{
  // implemented since declared as an abstract method in H323Channel
  return NULL;
}
*/
PBoolean H323_H224Channel::ExtractTransport(const H245_TransportAddress & pdu,
                                        PBoolean isDataPort,
                                        unsigned & errorCode)
{
  if (pdu.GetTag() != H245_TransportAddress::e_unicastAddress) {
    errorCode = H245_OpenLogicalChannelReject_cause::e_multicastChannelNotAllowed;
    return FALSE;
  }
    
  H323TransportAddress transAddr = pdu;
    
  PIPSocket::Address ip;
  WORD port = 0;
  if (transAddr.GetIpAndPort(ip, port)) {
    return rtpSession.SetRemoteSocketInfo(ip, port, isDataPort);
  }
    
  return FALSE;
}

void H323_H224Channel::SetAssociatedChannel(H323Channel * channel)
{
#ifdef H323_H235
    secChannel = (H323SecureChannel *)channel;
#endif
}

#endif


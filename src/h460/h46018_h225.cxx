/*
 * h460_h225.cxx
 *
 * H.460.18 H225 NAT Traversal class.
 *
 * h323plus library
 *
 * Copyright (c) 2008 ISVO (Asia) Pte. Ltd.
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
 * Portions of this code were written with the assisance of funding from
 * triple-IT. http://www.triple-it.nl.
 *
 * Contributor(s): ______________________________________.
 *
 *
 *
 */

#include <h323.h>

#ifdef H323_H46018

#include <h323pdu.h>
#include <h460/h46018_h225.h>
#include <h460/h46018.h>

#include <h323rtpmux.h>

#define H46019_KEEPALIVE_TIME       19   // Sec between keepalive messages
#define H46019_KEEPALIVE_COUNT      3    // Number of probes per message
#define H46019_KEEPALIVE_INTERVAL   100  // ms between each probe

#define H46024A_MAX_PROBE_COUNT  15
#define H46024A_PROBE_INTERVAL  200

#if PTLIB_VER >= 2130
PCREATE_NAT_PLUGIN(H46019, "H.460.19");
#else
PCREATE_NAT_PLUGIN(H46019);
#endif

///////////////////////////////////////////////////////////////////////////////////

// Listening/Keep Alive Thread

class H46018TransportThread : public PThread
{
   PCLASSINFO(H46018TransportThread, PThread)

   public:
    H46018TransportThread(H323EndPoint & endpoint, H46018Transport * transport);
    ~H46018TransportThread();

    void ConnectionEstablished();

   protected:
    void Main();

    PBoolean    isConnected;
    H46018Transport * transport;

    PDECLARE_NOTIFIER(PTimer, H46018TransportThread, KeepAlive);
    PTimer    m_keepAlive;
    unsigned  m_keepAliveInterval;

    PTime   lastupdate;

};

/////////////////////////////////////////////////////////////////////////////

H46018TransportThread::H46018TransportThread(H323EndPoint & ep, H46018Transport * t)
  : PThread(ep.GetSignallingThreadStackSize(), AutoDeleteThread,
            NormalPriority,"H46019 Answer:%0x"),transport(t)
{  

    isConnected = false;
    m_keepAliveInterval = H46019_KEEPALIVE_TIME;

    // Start the Thread
    Resume();
}

H46018TransportThread::~H46018TransportThread()
{
    m_keepAlive.Stop();
}

void H46018TransportThread::Main()
{
    PTRACE(3, "H46018\tStarted Listening Thread");

    PBoolean ret = true;
    while (transport && transport->IsOpen()) {    // not close due to shutdown
        ret = transport->HandleH46018SignallingChannelPDU(this);

        if (!ret /*&& transport->CloseTransport()*/) {  // Closing down Instruction
            PTRACE(3, "H46018\tShutting down H46018 Thread");
            transport->ConnectionLost(true);
            break;
        } 
    }
    m_keepAlive.Stop();

    PTRACE(3, "H46018\tTransport Closed");
}

void H46018TransportThread::ConnectionEstablished()
{
     PTRACE(3, "H46019\tStarted KeepAlive");
     m_keepAlive.SetNotifier(PCREATE_NOTIFIER(KeepAlive));
     m_keepAlive.RunContinuous(m_keepAliveInterval * 1000);
}

void H46018TransportThread::KeepAlive(PTimer &, H323_INT)
{
  // Send empty RFC1006 TPKT
  BYTE tpkt[4];
  tpkt[0] = 3;  // Version 3
  tpkt[1] = 0;

  PINDEX len = sizeof(tpkt);
  tpkt[2] = (BYTE)(len >> 8);
  tpkt[3] = (BYTE)len;

  PTRACE(5, "H225\tSending KeepAlive TPKT packet");

  if (transport)
     transport->Write(tpkt, len);
}

///////////////////////////////////////////////////////////////////////////////////////


H46018SignalPDU::H46018SignalPDU(const OpalGloballyUniqueID & callIdentifier)
{
    // Build facility msg
    q931pdu.BuildFacility(0, FALSE);

    // Build the UUIE
    m_h323_uu_pdu.m_h245Tunneling.SetValue(true);  // We always H.245 Tunnel
    m_h323_uu_pdu.m_h323_message_body.SetTag(H225_H323_UU_PDU_h323_message_body::e_facility);
    H225_Facility_UUIE & fac = m_h323_uu_pdu.m_h323_message_body;

    PString version = "0.0.8.2250.0." + PString(H225_PROTOCOL_VERSION);
    fac.m_protocolIdentifier.SetValue(version);
    fac.m_reason.SetTag(H225_FacilityReason::e_undefinedReason);
    fac.IncludeOptionalField(H225_Facility_UUIE::e_callIdentifier);
    fac.m_callIdentifier.m_guid = callIdentifier;

    // Put UUIE into user-user of Q931
    BuildQ931();
}

//////////////////////////////////////////////////////////////////////////////////////

H46018Transport::H46018Transport(H323EndPoint & endpoint, 
                                 PIPSocket::Address binding
                )
   : H323TransportTCP(endpoint, binding)
{
    ReadTimeOut = PMaxTimeInterval;
    isConnected = false;
    closeTransport = false;
    remoteShutDown = false;
}

H46018Transport::~H46018Transport()
{
    Close();
}

PBoolean H46018Transport::HandleH46018SignallingSocket(H323SignalPDU & pdu)
{
    for (;;) {

      if (!IsOpen())
          return false;

      H323SignalPDU rpdu;
      if (!rpdu.Read(*this)) { 
              PTRACE(3, "H46018\tSocket Read Failure");
              if (GetErrorNumber(PChannel::LastReadError) == 0) {
             PTRACE(3, "H46018\tRemote SHUT DOWN or Intermediary Shutdown!");
              remoteShutDown = true;
              }
        return false;
      } else if (rpdu.GetQ931().GetMessageType() == Q931::SetupMsg) {
              pdu = rpdu;
              return true;
      } else {
          PTRACE(3, "H46018\tUnknown PDU Received");
              return false;
      }

    }
}

PBoolean H46018Transport::HandleH46018SignallingChannelPDU(PThread * thread)
{

    H323SignalPDU pdu;
    if (!HandleH46018SignallingSocket(pdu)) {
        if (remoteShutDown && !closeTransport)   // Intentional Shutdown?
            Close();
        return false;
    }

    // We are connected
     isConnected = true;

    // Process the Tokens
    unsigned callReference = pdu.GetQ931().GetCallReference();
    PString token = endpoint.BuildConnectionToken(*this, callReference, true);

    H323Connection * connection = endpoint.CreateConnection(callReference, NULL, this, &pdu);
        if (connection == NULL) {
            PTRACE(1, "H46018\tEndpoint could not create connection, " <<
                      "sending release complete PDU: callRef=" << callReference);
            Q931 pdu;
            pdu.BuildReleaseComplete(callReference, true);
            PBYTEArray rawData;
            pdu.Encode(rawData);
            WritePDU(rawData);
            return true;
        }

    PTRACE(3, "H46018\tCreated new connection: " << token);
    connectionsMutex.Wait();
    endpoint.GetConnections().SetAt(token, connection);
    connectionsMutex.Signal();

    connection->AttachSignalChannel(token, this, true);

    if (connection->HandleSignalPDU(pdu)) {
        // All subsequent PDU's should wait forever
        SetReadTimeout(PMaxTimeInterval);
        ((H46018TransportThread *)thread)->ConnectionEstablished();
        connection->HandleSignallingChannel();
    }
    else {
 //       connection->ClearCall(H323Connection::EndedByTransportFail);
        PTRACE(1, "H46018\tSignal channel stopped on first PDU.");
        return false;
    }

    return connection->HadAnsweredCall();
}


PBoolean H46018Transport::WritePDU( const PBYTEArray & pdu )
{
    PWaitAndSignal m(WriteMutex);
    return H323TransportTCP::WritePDU(pdu);

}
    
PBoolean H46018Transport::ReadPDU(PBYTEArray & pdu)
{
    return H323TransportTCP::ReadPDU(pdu);
}

PBoolean H46018Transport::Connect(const OpalGloballyUniqueID & callIdentifier) 
{ 
    PTRACE(4, "H46018\tConnecting to H.460.18 Server");

    if (!H323TransportTCP::Connect())
        return false;
    
    return InitialPDU(callIdentifier);
}

void H46018Transport::ConnectionLost(PBoolean established)
{
    if (closeTransport)
        return;
}

PBoolean H46018Transport::IsConnectionLost()  
{ 
    return false; 
}


PBoolean H46018Transport::InitialPDU(const OpalGloballyUniqueID & callIdentifier)
{
    PWaitAndSignal mutex(IntMutex);

    if (!IsOpen())
        return false;

    H46018SignalPDU pdu(callIdentifier);

    PTRACE(6, "H46018\tCall Facility PDU: " << pdu);

    PBYTEArray rawData;
    pdu.GetQ931().Encode(rawData);

    if (!WritePDU(rawData)) {
        PTRACE(3, "H46018\tError Writing PDU.");
        return false;
    }

    PTRACE(4, "H46018\tSent PDU Call: " << callIdentifier.AsString() << " awaiting response.");

    return true;
}

PBoolean H46018Transport::Close() 
{ 
    PTRACE(4, "H46018\tClosing H46018 NAT channel.");    
    closeTransport = true;
    return H323TransportTCP::Close(); 
}

PBoolean H46018Transport::IsOpen () const
{
    return H323Transport::IsOpen();
}

PBoolean H46018Transport::IsListening() const
{      
    if (isConnected)
        return false;

    if (h245listener == NULL)
        return false;

    return h245listener->IsOpen();
}

/////////////////////////////////////////////////////////////////////////////

H46018Handler::H46018Handler(H323EndPoint & ep)
: EP(ep)
{
    PTRACE(4, "H46018\tCreating H46018 Handler.");

    nat = (PNatMethod_H46019 *)EP.GetNatMethods().LoadNatMethod("H46019");
    m_h46018inOperation = false;

    if (nat != NULL) {
      nat->AttachHandler(this);
      EP.GetNatMethods().AddMethod(nat);
    }

#ifdef H323_H46024A
    m_h46024a = false;
#endif

    SocketCreateThread = NULL;
}

H46018Handler::~H46018Handler()
{
    PTRACE(4, "H46018\tClosing H46018 Handler.");
    EP.GetNatMethods().RemoveMethod("H46019");
}

void H46018Handler::SetTransportSecurity(const H323TransportSecurity & callSecurity)
{
    m_callSecurity = callSecurity;
}

PBoolean H46018Handler::CreateH225Transport(const PASN_OctetString & information)
{

    H46018_IncomingCallIndication callinfo;
    PPER_Stream raw(information);

    if (!callinfo.Decode(raw)) {
        PTRACE(2,"H46018\tUnable to decode incoming call Indication."); 
        return false;
    }

    PTRACE(4, "H46018\t" << callinfo );

    m_address = H323TransportAddress(callinfo.m_callSignallingAddress);
    m_callId = OpalGloballyUniqueID(callinfo.m_callID.m_guid);

    // Fix for Tandberg boxes that send duplicate SCI messages.
    if (m_callId.AsString() == lastCallIdentifer) {
        PTRACE(2,"H46018\tDuplicate Call Identifer " << lastCallIdentifer << " Ignoring request!"); 
        return false;
    }

    PTRACE(5, "H46018\tCreating H225 Channel");

    // We throw the socket creation onto another thread as with UMTS networks it may take several 
    // seconds to actually create the connection and we don't want to wait before signalling back
    // to the gatekeeper. This also speeds up connection time which is also nice :) - SH
    SocketCreateThread = PThread::Create(PCREATE_NOTIFIER(SocketThread), 0, PThread::AutoDeleteThread);

    return true;
}

void H46018Handler::SocketThread(PThread &,  H323_INT)
{
    if (m_callId == PString()) {
        PTRACE(3, "H46018\tTCP Connect Abort: No Call identifier");
        return;
    }

    H46018Transport * transport = new H46018Transport(EP, PIPSocket::Address::GetAny(m_address.GetIpVersion()));
    transport->InitialiseSecurity(&m_callSecurity);
    if (m_callSecurity.IsTLSEnabled() && !m_callSecurity.GetRemoteTLSAddress().IsEmpty()) {
        transport->SetRemoteAddress(m_callSecurity.GetRemoteTLSAddress());
        m_callSecurity.Reset();
    } else
        transport->SetRemoteAddress(m_address);

    if (transport->Connect(m_callId)) {
        PTRACE(3, "H46018\tConnected to " << transport->GetRemoteAddress());
        new H46018TransportThread(EP, transport);
        lastCallIdentifer = m_callId.AsString();
    } else {
        PTRACE(3, "H46018\tCALL ABORTED: Failed connect to " << transport->GetRemoteAddress());
    }

    m_address = H323TransportAddress();
    m_callId = PString();
}

void H46018Handler::Enable()
{
   m_h46018inOperation = true;
   if (nat)
      nat->SetAvailable();
}

PBoolean H46018Handler::IsEnabled()
{
    return m_h46018inOperation;
}

H323EndPoint * H46018Handler::GetEndPoint() 
{ 
    return &EP; 
}

#ifdef H323_H46019M
void H46018Handler::EnableMultiplex(bool enable)
{
   if (nat)
      nat->EnableMultiplex(enable);
}
#endif

#ifdef H323_H46024A
void H46018Handler::H46024ADirect(bool reply, const PString & token)
{
    PWaitAndSignal m(m_h46024aMutex);

    H323Connection * connection = EP.FindConnectionWithLock(token);
    if (connection != NULL) {
        connection->SendH46024AMessage(reply);
        connection->Unlock();
    }
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////


PNatMethod_H46019::PNatMethod_H46019()
: available(false), active(true), handler(NULL)
{
}

PNatMethod_H46019::~PNatMethod_H46019()
{

}

void PNatMethod_H46019::AttachHandler(H46018Handler * _handler)
{
    handler = _handler;

    if (handler->GetEndPoint() == NULL) 
        return;

    WORD portPairBase = handler->GetEndPoint()->GetRtpIpPortBase();
    WORD portPairMax = handler->GetEndPoint()->GetRtpIpPortMax();

    // Initialise
    // ExternalAddress = 0;
    pairedPortInfo.basePort = 0;
    pairedPortInfo.maxPort = 0;
    pairedPortInfo.currentPort = 0;

    // Set the Port Pair Information
    pairedPortInfo.mutex.Wait();

    pairedPortInfo.basePort = (WORD)((portPairBase+1)&0xfffe);
    if (portPairBase == 0) {
        pairedPortInfo.basePort = 0;
        pairedPortInfo.maxPort = 0;
    }
    else if (portPairMax == 0)
        pairedPortInfo.maxPort = (WORD)(pairedPortInfo.basePort+99);
    else if (portPairMax < portPairBase)
        pairedPortInfo.maxPort = portPairBase;
    else
        pairedPortInfo.maxPort = portPairMax;

    pairedPortInfo.currentPort = pairedPortInfo.basePort;

    pairedPortInfo.mutex.Signal();

    available = FALSE;
}

H46018Handler * PNatMethod_H46019::GetHandler()
{
    return handler;
}

PBoolean PNatMethod_H46019::GetExternalAddress(PIPSocket::Address & /*externalAddress*/, /// External address of router
                    const PTimeInterval & /* maxAge */         /// Maximum age for caching
                    )
{
    return FALSE;
}

PBoolean PNatMethod_H46019::CreateSocketPair(PUDPSocket * & socket1,
                    PUDPSocket * & socket2,
                    const PIPSocket::Address & binding,
#if PTLIB_VER >= 2130
                    PObject * userData
#else
                    void * userData
#endif
                    )
{

    if (pairedPortInfo.basePort == 0 || pairedPortInfo.basePort > pairedPortInfo.maxPort)
    {
        PTRACE(1, "H46019\tInvalid local UDP port range "
               << pairedPortInfo.basePort  << '-' << pairedPortInfo.currentPort << '-' << pairedPortInfo.maxPort);
        return FALSE;
    }

    H323MultiplexConnection::SessionInformation * info = (H323MultiplexConnection::SessionInformation *)userData;

#ifdef H323_H46019M
    if (info->GetRecvMultiplexID() > 0) {
        H323MultiplexConnection * muxhandler = info->GetMultiplexConnection();

        if (muxhandler->GetMultiplexSocket(H323UDPSocket::rtp) == NULL) {
            PUDPSocket * & rtp = muxhandler->GetMultiplexSocket(H323UDPSocket::rtp);
            PUDPSocket * & rtcp = muxhandler->GetMultiplexSocket(H323UDPSocket::rtcp);
            rtp = new H46019MultiplexSocket(true);
            rtcp = new H46019MultiplexSocket(false);

#if PTLIB > 2130
            PortInfo muxPortInfo(muxhandler->GetMultiplexPort(false), 
                                 muxhandler->GetMultiplexPort(true));
#else
            PortInfo muxPortInfo;
            muxPortInfo.basePort = muxhandler->GetMultiplexPort(false);
            muxPortInfo.currentPort = muxPortInfo.basePort-1;
            muxPortInfo.maxPort = muxhandler->GetMultiplexPort(true);
#endif

            if ((!OpenSocket(*rtp, muxPortInfo, binding)) ||
                (!OpenSocket(*rtcp, muxPortInfo, binding)) ||
                (rtcp->GetPort() != rtp->GetPort() + 1) )  {
                    delete rtp;
                    delete rtcp;
                    rtp = new H46019MultiplexSocket(true);    
                    rtcp = new H46019MultiplexSocket(false);  
            }
            PTRACE(4, "H46019\tMultiplex UDP ports created "  << rtp->GetPort() << '-' << rtcp->GetPort());
             
            muxhandler->Start();  // Start Multiplexing Listening thread;
        }

       socket1 = new H46019UDPSocket(*handler,info,true);      /// Data 
       socket2 = new H46019UDPSocket(*handler,info,false);     /// Signal

       muxhandler->SetSocketSession(info->GetSessionID(), info->GetRecvMultiplexID(), socket1, socket2);

    } else
#endif
    {
       socket1 = new H46019UDPSocket(*handler,info,true);     /// Data 
       socket2 = new H46019UDPSocket(*handler,info,false);    /// Signal

        /// Make sure we have sequential ports
        while ((!OpenSocket(*socket1, pairedPortInfo,binding)) ||
               (!OpenSocket(*socket2, pairedPortInfo,binding)) ||
               (socket2->GetPort() != socket1->GetPort() + 1) )
            {
                delete socket1;
                delete socket2;
                socket1 = new H46019UDPSocket(*handler,info,true);    /// Data 
                socket2 = new H46019UDPSocket(*handler,info,false);    /// Signal
            }

            PTRACE(5, "H46019\tUDP ports "
                   << socket1->GetPort() << '-' << socket2->GetPort());
    }

    return TRUE;
}

PBoolean PNatMethod_H46019::OpenSocket(PUDPSocket & socket, PortInfo & portInfo, const PIPSocket::Address & binding) const
{
    PWaitAndSignal mutex(portInfo.mutex);

    WORD startPort = portInfo.currentPort;

    do {
        portInfo.currentPort++;
        if (portInfo.currentPort > portInfo.maxPort)
            portInfo.currentPort = portInfo.basePort;

        if (socket.Listen(binding,1, portInfo.currentPort)) {
            socket.SetReadTimeout(500);
            return true;
        }

    } while (portInfo.currentPort != startPort);

    PTRACE(2, "H46019\tFailed to bind to " << binding << " local UDP port range "
        << pairedPortInfo.basePort << '-' << portInfo.currentPort << '-' << portInfo.maxPort);
      return false;
}


bool PNatMethod_H46019::IsAvailable(const PIPSocket::Address & /*address*/) 
{ 
    return handler->IsEnabled() && active;
}

void PNatMethod_H46019::SetAvailable() 
{ 
    if (!available) {
        handler->GetEndPoint()->NATMethodCallBack(GetName(),1,"Available");
        available = TRUE;
    }
}


#ifdef H323_H46019M
void PNatMethod_H46019::EnableMultiplex(bool enable)
{

}
#endif

/////////////////////////////////////////////////////////////////////////////////////////////

#ifdef H323_H46019M
H46019MultiplexSocket::H46019MultiplexSocket()
 : m_subSocket(NULL)
{   
}

H46019MultiplexSocket::H46019MultiplexSocket(bool rtp)
: m_subSocket(NULL)
{
    SetType(rtp ? H323UDPSocket::rtp : H323UDPSocket::rtcp);
}

H46019MultiplexSocket::~H46019MultiplexSocket()
{
    Close();

    if (m_subSocket)
        delete m_subSocket;
}


PBoolean H46019MultiplexSocket::GetLocalAddress(Address & addr, WORD & port)
{
    if (m_subSocket)
        return m_subSocket->GetLocalAddress(addr, port);
    else
        return PUDPSocket::GetLocalAddress(addr, port);
}

PString H46019MultiplexSocket::GetLocalAddress()
{
    if (m_subSocket)
        return m_subSocket->GetLocalAddress();
    else
        return PIPSocket::GetLocalAddress();
}

PBoolean H46019MultiplexSocket::ReadFrom(void *buf, PINDEX len, Address & addr, WORD & pt)
{
    if (m_subSocket)
        return m_subSocket->ReadFrom(buf,len,addr,pt);
    else
        return PUDPSocket::ReadFrom(buf,len,addr,pt);
}

PBoolean H46019MultiplexSocket::WriteTo(const void *buf, PINDEX len, const Address & addr, WORD pt)
{
    PWaitAndSignal m(m_mutex);

    if (m_subSocket)
        return m_subSocket->WriteTo(buf,len,addr,pt);
    else
        return PUDPSocket::WriteTo(buf,len,addr,pt);
}

PBoolean H46019MultiplexSocket::Close()
{
    if (m_subSocket)
        return ((H323UDPSocket*)m_subSocket)->Close();

    return H323UDPSocket::Close();
}
#endif
/////////////////////////////////////////////////////////////////////////////////////////////

H46019UDPSocket::H46019UDPSocket(H46018Handler & _handler, PObject * info, bool _rtpSocket)
: m_Handler(_handler), m_Session(((H323MultiplexConnection::SessionInformation*)info)->GetSessionID()),
  m_Token(((H323MultiplexConnection::SessionInformation*)info)->GetCallToken()),
  m_CallId(((H323MultiplexConnection::SessionInformation*)info)->GetCallIdentifer()),
  m_CUI(((H323MultiplexConnection::SessionInformation*)info)->GetCUI()),
  keepport(0), keeppayload(0), keepTTL(0), keepseqno(0), keepStartTime(NULL), initialKeep(NULL),
#ifdef H323_H46019M
  m_connection(((H323MultiplexConnection::SessionInformation*)info)->GetMultiplexConnection()),
  m_muxHandler(m_connection->GetMultiplexHandler()),
  m_recvMultiplexID(((H323MultiplexConnection::SessionInformation*)info)->GetRecvMultiplexID()), 
  m_sendMultiplexID(0),  m_multiBuffer(0), m_shutDown(false),
#endif
#if defined(H323_H46024A) || defined(H323_H46024B)
  m_CUIrem(PString()), m_locAddr(PIPSocket::GetDefaultIpAny()),  m_locPort(0),
  m_remAddr(PIPSocket::GetDefaultIpAny()),  m_remPort(0), m_detAddr(PIPSocket::GetDefaultIpAny()),  m_detPort(0),
  m_pendAddr(PIPSocket::GetDefaultIpAny()), m_pendPort(0), m_probes(0), SSRC(PRandom::Number()),
#endif
  m_altAddr(PIPSocket::GetDefaultIpAny()), m_altPort(0), m_altMuxID(0),
#ifdef H323_H46024A
  m_state(e_notRequired),
#endif
#ifdef H323_H46024B
  m_h46024b(false),
#endif
  rtpSocket(_rtpSocket)
{
    if (m_recvMultiplexID) {
        PUDPSocket * & muxSocket = m_connection->GetMultiplexSocket(rtpSocket ? H323UDPSocket::rtp : H323UDPSocket::rtcp);
        if (muxSocket)
            muxSocket->GetLocalAddress(m_muxLocalAddress);
    }
}

H46019UDPSocket::~H46019UDPSocket()
{
    Close();
    Keep.Stop();
    delete keepStartTime;

#ifdef H323_H46019M
    if (m_muxHandler)
        m_muxHandler->Unregister(m_recvMultiplexID, rtpSocket ? H323UDPSocket::rtp : H323UDPSocket::rtcp);
#endif

#ifdef H323_H46024A
    m_Probe.Stop();
#endif
}


void H46019UDPSocket::Allocate(const H323TransportAddress & keepalive, unsigned _payload, unsigned _ttl)
{

    PIPSocket::Address ip;  WORD port = 0;
    keepalive.GetIpAndPort(ip,port);
    if (ip.IsValid() && !ip.IsLoopback() && !ip.IsAny() && port > 0) {
        keepip = ip;
        keepport = port;
    }

    if (_payload > 0)
        keeppayload = _payload;

    if (_ttl > 0)
        keepTTL = _ttl;

    PTRACE(4,"H46019UDP\tSetting " << keepip << ":" << keepport << " ping " << keepTTL << " secs.");
}

void H46019UDPSocket::Activate()
{
    InitialiseKeepAlive();
}

void H46019UDPSocket::Activate(const H323TransportAddress & keepalive, unsigned _payload, unsigned _ttl)
{
    Allocate(keepalive,_payload,_ttl);
    InitialiseKeepAlive();
}

PBoolean H46019UDPSocket::Close()
{
#ifdef H323_H46019M
    m_shutDown = true;
#endif
    return H323UDPSocket::Close();
}

void H46019UDPSocket::InitialiseKeepAlive() 
{
    PWaitAndSignal m(PingMutex);

    if (Keep.IsRunning()) {
        PTRACE(6,"H46019UDP\t" << (rtpSocket ? "RTP" : "RTCP") << " ping already running.");
        return;
    }

    if (keepTTL > 0 && keepip.IsValid() && !keepip.IsLoopback() && !keepip.IsAny()) {
        keepseqno = 100;  // Some arbitrary number
        keepStartTime = new PTime();

        PTRACE(4,"H46019UDP\tStart " << (rtpSocket ? "RTP" : "RTCP") << " pinging " 
                        << keepip << ":" << keepport << " every " << keepTTL << " secs.");

        Keep.SetNotifier(PCREATE_NOTIFIER(Ping));
        Keep.RunContinuous(keepTTL * 1000);   // This will fire at keepTTL sec time.

        //  To start before keepTTL interval do a number of special probes to ensure the gatekeeper
        //  is reached to allow media to flow properly.
        initialKeep = PThread::Create(PCREATE_NOTIFIER(StartKeepAlives), 0, PThread::AutoDeleteThread); 

    } else {
        PTRACE(2,"H46019UDP\t"  << (rtpSocket ? "RTP" : "RTCP") << " PING NOT Ready " 
                        << keepip << ":" << keepport << " - " << keepTTL << " secs.");

    }
}

void H46019UDPSocket::StartKeepAlives(PThread &,  H323_INT)
{ 
    // Send a number of keep alive probes to ensure at least one reaches the gatekeeper.
    // This thread MUST terminate prior to Ping timer firing at keepTTL (default 19 sec).
    PINDEX i=0;
    while (i<H46019_KEEPALIVE_COUNT && Keep.IsRunning()) {
        if (i>0) PThread::Sleep(H46019_KEEPALIVE_INTERVAL);
        rtpSocket ? SendRTPPing(keepip,keepport) : SendRTCPPing();
        i++;
    }
}

void H46019UDPSocket::Ping(PTimer &,  H323_INT)
{ 
    rtpSocket ? SendRTPPing(keepip,keepport) : SendRTCPPing();
}

void H46019UDPSocket::SendRTPPing(const PIPSocket::Address & ip, const WORD & port, unsigned id) {

    RTP_DataFrame rtp;

    rtp.SetSequenceNumber(keepseqno);

    rtp.SetPayloadType((RTP_DataFrame::PayloadTypes)keeppayload);
    rtp.SetPayloadSize(0);

    // determining correct timestamp
    PTime currentTime = PTime();
    PTimeInterval timePassed = 0;
    if (keepStartTime) 
        timePassed = currentTime - *keepStartTime;
    rtp.SetTimestamp((DWORD)timePassed.GetMilliSeconds() * 8);

    rtp.SetMarker(TRUE);

    if (!WriteTo(rtp.GetPointer(),
                 rtp.GetHeaderSize()+rtp.GetPayloadSize(),
                 ip, port,id)) {
        switch (GetErrorNumber()) {
        case ECONNRESET :
        case ECONNREFUSED :
            PTRACE(2, "H46019UDP\t" << ip << ":" << port << " not ready.");
            break;

        default:
            PTRACE(1, "H46019UDP\t" << ip << ":" << port 
                << ", Write error on port ("
                << GetErrorNumber(PChannel::LastWriteError) << "): "
                << GetErrorText(PChannel::LastWriteError));
        }
    } else {
        PTRACE(6, "H46019UDP\tRTP KeepAlive sent: " << ip << ":" << port << " " << id << " seq: " << keepseqno);    
        keepseqno++;
    }
}

void H46019UDPSocket::SendRTCPPing() 
{
    RTP_ControlFrame report;
    report.SetPayloadType(RTP_ControlFrame::e_SenderReport);
    report.SetPayloadSize(sizeof(RTP_ControlFrame::SenderReport));

    if (SendRTCPFrame(report, keepip, keepport)) {
        PTRACE(6, "H46019UDP\tRTCP KeepAlive sent: " << keepip << ":" << keepport);    
    }
}


PBoolean H46019UDPSocket::SendRTCPFrame(RTP_ControlFrame & report, const PIPSocket::Address & ip, WORD port, unsigned id) {

     if (!WriteTo(report.GetPointer(),report.GetSize(),
                 ip, port,id)) {
        switch (GetErrorNumber()) {
            case ECONNRESET :
            case ECONNREFUSED :
                PTRACE(2, "H46019UDP\t" << ip << ":" << port << " not ready.");
                break;

            default:
                PTRACE(1, "H46019UDP\t" << ip << ":" << port 
                    << ", Write error on port ("
                    << GetErrorNumber(PChannel::LastWriteError) << "): "
                    << GetErrorText(PChannel::LastWriteError));
        }
        return false;
    } 
    return true;
}

PBoolean H46019UDPSocket::GetLocalAddress(PIPSocket::Address & addr, WORD & port) 
{
#ifdef H323_H46019M
    if (m_recvMultiplexID) {
        addr = m_muxLocalAddress.GetAddress();
        port = m_muxLocalAddress.GetPort();
        return true;
    }
#endif

    if (PUDPSocket::GetLocalAddress(addr, port)) {
#ifdef H323_H46024A
        m_locAddr = addr;
        m_locPort = port;
#endif
        return true;
    }
    return false;
}

unsigned H46019UDPSocket::GetPingPayload()
{
    return keeppayload;
}

void H46019UDPSocket::SetPingPayLoad(unsigned val)
{
    keeppayload = val;
}

unsigned H46019UDPSocket::GetTTL()
{
    return keepTTL;
}

void H46019UDPSocket::SetTTL(unsigned val)
{
    keepTTL = val;
}

#ifdef H323_H46019M

void H46019UDPSocket::GetMultiplexAddress(H323TransportAddress & address, unsigned & multiID, PBoolean OLCack)
{
    multiID = m_recvMultiplexID;

    // Get the local IP/Port of the underlying Multiplexed RTP;
    if (m_recvMultiplexID)
        address = H323TransportAddress(m_muxLocalAddress.GetAddress(), m_muxLocalAddress.GetPort());
}

unsigned H46019UDPSocket::GetRecvMultiplexID() const
{
    return m_recvMultiplexID;
}

unsigned H46019UDPSocket::GetSendMultiplexID() const
{
    return m_sendMultiplexID;
}
    
void H46019UDPSocket::SetMultiplexID(unsigned id, PBoolean isAck)
{
    if (!m_sendMultiplexID) {
        PTRACE(3,"H46019\t" << (rtpSocket ? "RTP" : "RTCP") 
            << " MultiplexID for send Session " << m_Session  
            << " set to " << id);

         m_sendMultiplexID = id;
    } else {
        if (id != m_sendMultiplexID) {
            PTRACE(1,"H46019\tERROR: " << (rtpSocket ? "RTP" : "RTCP") 
                << " MultiplexID OLCack for Send Session " << m_Session  
                << " not match OLC " << id << " was " << m_sendMultiplexID); 
        } else {
            PTRACE(3,"H46019\t" << (rtpSocket ? "RTP" : "RTCP") 
                << " MultiplexID send Session " << m_Session  
                << " already set to " << id);
        }
    }
}

PBoolean H46019UDPSocket::WriteMultiplexBuffer(const void * buf, PINDEX len, const Address & addr, WORD port)
{

    if (rtpSocket && len <= RTP_DataFrame::MinHeaderSize) {
#ifdef H323_H46024B
        if (len == RTP_DataFrame::MinHeaderSize) {  /// Filter out RTP keepAlive Packets
            if (m_h46024b && addr == m_altAddr /*&& port == m_altPort*/) {
                PTRACE(4, "H46024B\ts:" << m_Session << (rtpSocket ? " RTP " : " RTCP ")
                    << "Switching to " << addr << ":" << port << " from " << m_remAddr << ":" << m_remPort);
                m_detAddr = addr;  m_detPort = port;
                SetProbeState(e_direct);
                Keep.Stop();  // Stop the keepAlive Packets
                m_h46024b = false;
            }
        } else
#endif
        {
            PTRACE(4, "H46019\ts:" << m_Session << (rtpSocket ? " RTP " : " RTCP ") << " Received data packet too small : " << len << " bytes.");
        }
        return true;
    }

    H46019MultiPacket packet;
    packet.fromAddr = addr;
    packet.fromPort = port;
    packet.frame.SetSize(len);
    memcpy(packet.frame.GetPointer(), buf, len);

    m_multiMutex.Wait();
      m_multQueue.push(packet);
    m_multiMutex.Signal();
    m_multiBuffer++;

    PTRACE(6, "H46019\tWrite s:" << m_Session << (rtpSocket ? " RTP " : " RTCP ") 
                            << " from " << addr << ":" << port << " " << len << " bytes.");

    return true;
}

PBoolean H46019UDPSocket::ReadMultiplexBuffer(void * buf, PINDEX & len, Address & addr, WORD & port)
{
 
    while (!m_shutDown && (m_multiBuffer == 0)) {
        readBlock.Delay(3);
    }

    if (m_shutDown)
        return false;

    m_multiMutex.Wait();
     H46019MultiPacket & packet = m_multQueue.front();
        addr = packet.fromAddr;
        port = packet.fromPort;
        len = packet.frame.GetSize();
        memcpy(buf, packet.frame.GetPointer(), len);
        m_multQueue.pop();
    m_multiMutex.Signal();
    m_multiBuffer--;

    PTRACE(6, "H46019\tRead s:" << m_Session << " " << (rtpSocket ? "RTP" : "RTCP") 
                            << " from " << addr << ":" << port << " " << len << " bytes.");

    return true;
}


void H46019UDPSocket::ClearMultiplexBuffer()
{
     if (m_multiBuffer > 0) {
       m_multiMutex.Wait();
         while (!m_multQueue.empty()) {
            m_multQueue.pop();
         }
       m_multiMutex.Signal();
     }
     m_multiBuffer = 0;
}
    

PBoolean H46019UDPSocket::DoPseudoRead(int & selectStatus)
{
   if (m_recvMultiplexID == 0)
       return false;

   if (m_shutDown)
       selectStatus += PSocket::Interrupted;
   else
       selectStatus += ((m_multiBuffer > 0) ? (rtpSocket ? -1 : -2) : 0);

   return true;
}


PBoolean H46019UDPSocket::ReadSocket(void * buf, PINDEX & len, Address & addr, WORD & port)
{
    if (m_recvMultiplexID) {
        if (ReadMultiplexBuffer(buf, len, addr, port)) {
            lastReadCount = len;
            return true;
        } else
            return false;
    } else
        return PUDPSocket::ReadFrom(buf, len, addr, port);
}


PBoolean H46019UDPSocket::WriteSocket(const void * buf, PINDEX len, const Address & addr, WORD port, unsigned altMux)
{

#ifdef H323_H46024A
    if (m_remAddr.IsAny()) {
        m_remAddr = addr;
        m_remPort = port;
    }
#endif

    if (len == 1 && m_recvMultiplexID > 0)  // Ignore close packet send.
        return false;

    unsigned mux = m_sendMultiplexID;
    if (altMux) mux = altMux;

    if (m_recvMultiplexID)  // Multiplex Rec'v  Send from same place
        return m_connection->WriteTo(rtpSocket ? H323UDPSocket::rtp : H323UDPSocket::rtcp, mux, buf, len, addr, port);
    else if (m_sendMultiplexID) {  // Send Multiplex only from seperate port
        RTP_MultiDataFrame frame(mux, (const BYTE *)buf, len);                                              
        return PUDPSocket::WriteTo(frame.GetPointer(), frame.GetSize(), addr, port);
    } else
        return PUDPSocket::WriteTo(buf, len, addr, port);
}
#endif

#ifdef H323_H46019M
PBoolean H46019UDPSocket::GetPeerAddress(PIPSocketAddressAndPort & addr)
{
#if defined(H323_H46024A) || defined(H323_H46024B)
    addr.SetAddress(m_remAddr,m_remPort);
    return true;
#else
    return false;
#endif
}
#endif

#ifdef H323_H46024A


PString H46019UDPSocket::ProbeState(probe_state state)
{
    PString probeString = PString();
    switch (state) {
        case e_notRequired:      probeString = "NotRequred";  break;
        case e_initialising:     probeString = "Initialising";  break;  
        case e_idle:             probeString = "Ready";  break;
        case e_probing:          probeString = "Probing";  break;
        case e_verify_receiver:  probeString = "ReceiveVerified";  break;   
        case e_verify_sender:    probeString = "SendVerified";  break;
        case e_wait:             probeString = "WaitingForPacket";  break;
        case e_direct:           probeString = "Direct";  break;
    };
    return probeString;
}

void H46019UDPSocket::SetProbeState(probe_state newstate)
{
    PWaitAndSignal m(probeMutex);

    if (m_state >= newstate) {
        PTRACE(4,"H46024A\ts:" << m_Session << (rtpSocket ? " RTP " : " RTCP ") << "current state not changed from " << ProbeState(m_state));
        return;
    }

    PTRACE(4,"H46024A\ts:" << m_Session << (rtpSocket ? " RTP " : " RTCP ") << "changing state from " << ProbeState(m_state) 
        << " to " << ProbeState(newstate));

    m_state = newstate;
}

int H46019UDPSocket::GetProbeState() const
{
    PWaitAndSignal m(probeMutex);

    return m_state;
}

void H46019UDPSocket::SetAlternateAddresses(const H323TransportAddress & address, const PString & cui, unsigned muxID)
{
    address.GetIpAndPort(m_altAddr,m_altPort);
    m_altMuxID = muxID;

    PTRACE(6,"H46024A\ts:" << m_Session << (rtpSocket ? " RTP " : " RTCP ")  
        << "Remote Alt: " << m_altAddr << ":" << m_altPort << " CUI: " << cui);

    if (!rtpSocket) {
        m_CUIrem = cui;
        if (GetProbeState() < e_idle) {
            SetProbeState(e_idle);
            StartProbe();
        // We Already have a direct connection but we are waiting on the CUI for the reply
        } else if ((GetProbeState() == e_verify_receiver) && !m_pendAddr.IsAny())
            ProbeResponse(false,m_pendAddr,m_pendPort);
    }
}

void H46019UDPSocket::GetAlternateAddresses(H323TransportAddress & address, PString & cui, unsigned & muxID)
{

#ifdef H323_H46019M
    if (m_recvMultiplexID > 0) {
        muxID = m_recvMultiplexID;
        address = H323TransportAddress(m_muxLocalAddress.GetAddress(), m_muxLocalAddress.GetPort());
    }
    else 
#endif
    {
        muxID = 0;
        PIPSocket::Address tempAddr;
        WORD               tempPort;
        if (GetLocalAddress(tempAddr, tempPort))
            address = H323TransportAddress(tempAddr, tempPort);
    }

    if (!rtpSocket)
        cui = m_CUI;
    else
        cui = PString();

    if (GetProbeState() < e_idle)
        SetProbeState(e_initialising);

    PTRACE(6,"H46024A\ts:" << m_Session << (rtpSocket ? " RTP " : " RTCP ") << " Alt:" << address << " CUI " << cui);

}

PBoolean H46019UDPSocket::IsAlternateAddress(const Address & address,WORD port)
{
    return ((address == m_detAddr) && (port == m_detPort));
}

void H46019UDPSocket::StartProbe()
{

    PTRACE(4,"H46024A\ts:" << m_Session << " Start probing " << m_altAddr << ":" << m_altPort << " ID: " << m_altMuxID);

    SetProbeState(e_probing);
    m_probes = 0;
    m_Probe.SetNotifier(PCREATE_NOTIFIER(Probe));
    m_Probe.RunContinuous(H46024A_PROBE_INTERVAL); 
}

void H46019UDPSocket::BuildProbe(RTP_ControlFrame & report, bool reply)
{
    report.SetPayloadType(RTP_ControlFrame::e_ApplDefined);
    report.SetCount((reply ? 1 : 0));  // SubType Probe
    
    report.SetPayloadSize(sizeof(probe_packet));

    probe_packet data;
        data.SSRC = SSRC;
        data.Length = sizeof(probe_packet);
        PString id = "24.1";
        PBYTEArray bytes(id,id.GetLength(), false);
        memcpy(&data.name[0], bytes, 4);

        PMessageDigest::Result bin_digest;
        PMessageDigestSHA1::Encode(m_CallId.AsString() + m_CUIrem, bin_digest);
        memcpy(&data.cui[0], bin_digest.GetPointer(), bin_digest.GetSize());

        memcpy(report.GetPayloadPtr(),&data,sizeof(probe_packet));

}

void H46019UDPSocket::Probe(PTimer &,  H323_INT)
{ 
    m_probes++;

    if (m_probes > H46024A_MAX_PROBE_COUNT) {
        m_Probe.Stop();
        return;
    }

    if (GetProbeState() != e_probing)
        return;

    RTP_ControlFrame report;
    report.SetSize(4+sizeof(probe_packet));
    BuildProbe(report, false);

     if (!WriteTo(report.GetPointer(),report.GetSize(),
                 m_altAddr, m_altPort, m_altMuxID)) {
        switch (GetErrorNumber()) {
            case ECONNRESET :
            case ECONNREFUSED :
                PTRACE(2, "H46024A\t" << m_altAddr << ":" << m_altPort << " not ready.");
                break;

            default:
                PTRACE(1, "H46024A\t" << m_altAddr << ":" << m_altPort 
                    << ", Write error on port ("
                    << GetErrorNumber(PChannel::LastWriteError) << "): "
                    << GetErrorText(PChannel::LastWriteError));
        }
    } else {
        PTRACE(6, "H46024A\ts" << m_Session <<" RTCP Probe sent: " << m_altAddr << ":" << m_altPort << " ID: " << m_altMuxID);
    }
}

void H46019UDPSocket::ProbeResponse(bool reply, const PIPSocket::Address & addr, WORD & port)
{

    if (reply) {  //< Tell remote to wait for connection
        m_Handler.H46024ADirect(true,m_Token);
    } else if (addr.IsValid() && !addr.IsLoopback() && !addr.IsAny()) {
        RTP_ControlFrame reply;
        reply.SetSize(4+sizeof(probe_packet));
        BuildProbe(reply, true);
        if (SendRTCPFrame(reply,addr,port,m_altMuxID)) {
            PTRACE(4, "H46024A\tRTCP Reply packet sent: " << addr << ":" << port);
        }
    } else {
        PTRACE(4, "H46024A\tRTCP Reply packet invalid Address: " << addr);
    }

}

void H46019UDPSocket::H46024Adirect(bool starter)
{
    if (GetProbeState() == e_direct)  // We might already be doing Annex B
        return;

    if (starter) {  // We start the direct channel 
        m_detAddr = m_altAddr;  m_detPort = m_altPort;
        PTRACE(4, "H46024A\ts:" << m_Session << (rtpSocket ? " RTP " : " RTCP ")  
                        << "Switching to " << m_detAddr << ":" << m_detPort);
        SetProbeState(e_direct);
    } else         // We wait for the remote to start channel
        SetProbeState(e_wait);

    Keep.Stop();  // Stop the keepAlive Packets
}
#endif

PBoolean H46019UDPSocket::ReadFrom(void * buf, PINDEX len, Address & addr, WORD & port)
{
    
#ifdef H323_H46019M
    while (ReadSocket(buf, len, addr, port)) {
#else
    while (PUDPSocket::ReadFrom(buf, len, addr, port)) {
#endif
#if defined(H323_H46024A) || defined(H323_H46024B)
        bool reply = false; bool sendResponseNow = false;
      RTP_ControlFrame frame(2048);
        /// Set the detected routed remote address (on first packet received)
        if (m_remAddr.IsAny()) {   
            m_remAddr = addr; 
            m_remPort = port;
        }
#if H323_H46024B
        if (m_h46024b && addr == m_altAddr && port == m_altPort) {
            PTRACE(4, "H46024B\ts:" << m_Session << (rtpSocket ? " RTP " : " RTCP ")  
                << "Switching to " << addr << ":" << port << " from " << m_remAddr << ":" << m_remPort);
            m_detAddr = addr;  m_detPort = port;
            SetProbeState(e_direct);
            Keep.Stop();  // Stop the keepAlive Packets
            m_h46024b = false;
        }
#endif
        /// Check the probe state
        switch (GetProbeState()) {
            case e_initialising:                        // RTCP only
            case e_idle:                                // RTCP only
            case e_probing:                                // RTCP only
            case e_verify_receiver:                        // RTCP only
                frame.SetSize(len);
                memcpy(frame.GetPointer(),buf,len);
                if (ReceivedProbePacket(frame,reply, sendResponseNow)) {
                    if (sendResponseNow)
                        ProbeResponse(reply,addr,port);
                    else {
                        m_pendAddr = addr;  
                        m_pendPort = port;
                    }
                    continue;  // don't forward on probe packets.
                }
                break;
            case e_wait:
                if (addr == keepip) {// We got a keepalive ping...
                     Keep.Stop();  // Stop the keepAlive Packets
                } else if ((addr == m_altAddr) && (port == m_altPort)) {
                    PTRACE(4, "H46024A\ts:" << m_Session << (rtpSocket ? " RTP " : " RTCP ")  << "Already sending direct!");
                    m_detAddr = addr;  m_detPort = port;
                    SetProbeState(e_direct);
                } else if ((addr == m_pendAddr) && (port == m_pendPort)) {
                    PTRACE(4, "H46024A\ts:" << m_Session << (rtpSocket ? " RTP " : " RTCP ")  
                                                << "Switching to Direct " << addr << ":" << port);
                    m_detAddr = addr;  m_detPort = port;
                    SetProbeState(e_direct);
                } else if ((addr != m_remAddr) || (port != m_remPort)) {
                    PTRACE(4, "H46024A\ts:" << m_Session << (rtpSocket ? " RTP " : " RTCP ")  
                        << "Switching to " << addr << ":" << port << " from " << m_remAddr << ":" << m_remPort);
                    m_detAddr = addr;  m_detPort = port;
                    SetProbeState(e_direct);
                } 
                    break;
            case e_direct:    
            default:
                break;
        }
#endif // H46024A/B
        return true;
    } 
       return false; 
}

PBoolean H46019UDPSocket::WriteTo(const void * buf, PINDEX len, const Address & addr, WORD port)
{
    return WriteTo(buf, len, addr, port, 0);
}


PBoolean H46019UDPSocket::WriteTo(const void * buf, PINDEX len, const Address & addr, WORD port, unsigned id)
{
#if defined(H323_H46024A) || defined(H323_H46024B)
    if (GetProbeState() == e_direct)
#ifdef H323_H46019M
        return WriteSocket(buf,len, m_detAddr, m_detPort, m_altMuxID);
#else
        return PUDPSocket::WriteTo(buf,len, m_detAddr, m_detPort);
#endif  // H46019M
    else
#endif  // H46024A/B
#ifdef H323_H46019M
        return WriteSocket(buf,len, addr, port, id);
#else
        return PUDPSocket::WriteTo(buf,len, addr, port);
#endif
}


#ifdef H323_H46024A
PBoolean H46019UDPSocket::ReceivedProbePacket(const RTP_ControlFrame & frame, bool & reply, bool & sendResponse)
{
    if (frame.GetPayloadType() != RTP_ControlFrame::e_ApplDefined) {
        // Not a probe packet ignore
        return false;  
    }

    reply = (frame.GetCount() > 0);


    //Inspect the probe packet
    int cstate = GetProbeState();
    if (cstate == e_notRequired) {
        PTRACE(6, "H46024A\ts:" << m_Session <<" received RTCP probe packet. LOGIC ERROR!");
        return false;  
    }

    if (cstate > e_probing) {
        PTRACE(6, "H46024A\ts:" << m_Session << " received " << (reply ?  "Reply" : "Request") << " RTCP packet. IGNORING! Already authenticated.");
        return false;
    }

    PTRACE(4, "H46024A\ts:" << m_Session <<" RTCP Probe " << (reply ? "Reply" : "Request") << " received.");

    BYTE * data = frame.GetPayloadPtr();
    PBYTEArray bytes(20);
    memcpy(bytes.GetPointer(),data+12, 20);
    PMessageDigest::Result bin_digest;
    PMessageDigestSHA1::Encode(m_CallId.AsString() + m_CUI, bin_digest);
    PBYTEArray val(bin_digest.GetPointer(),bin_digest.GetSize());

    if (bytes != val) {
        PTRACE(4, "H46024A\ts:" << m_Session <<" RTCP Probe " << (reply ? "Reply" : "Request") << " verify FAILURE");
        return false;
    }

    PTRACE(4, "H46024A\ts:" << m_Session <<" RTCP Probe " << (reply ? "Reply" : "Request") << " verified.");

    if (!reply && m_CUIrem.IsEmpty()) {
        PTRACE(4, "H46024A\ts:" << m_Session << " received " << (reply ? "Reply" : "Request") << " RTCP packet too early. local not setup. Waiting for remote signal!");
        SetProbeState(e_verify_receiver);
        return false;
    }

    if (reply)  // We have a reply
        SetProbeState(e_verify_sender);
    else
        SetProbeState(e_verify_receiver);

    sendResponse = true;
    m_Probe.Stop();

    return true;
}
#endif

#ifdef H323_H46024B
void H46019UDPSocket::H46024Bdirect(const H323TransportAddress & address, unsigned muxID)
{
    if (GetProbeState() == e_direct)  // We might already be doing annex A
        return;

    address.GetIpAndPort(m_altAddr,m_altPort);
    m_altMuxID = muxID;

    PTRACE(6,"H46024b\ts: " << m_Session << " RTP Remote Alt: " << m_altAddr << ":" << m_altPort 
                            << " " << m_altMuxID);

    m_h46024b = true;

    // Sending an empty RTP frame to the alternate address
    // will add a mapping to the router to receive RTP from
    // the remote
   for (PINDEX i=0; i<3; i++) {
       SendRTPPing(m_altAddr, m_altPort, m_altMuxID);
       PThread::Sleep(10);
   }
}

#endif  // H323_H46024B

#endif  // H323_H460





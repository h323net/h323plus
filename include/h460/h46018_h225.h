/*
 * h46018_h225.h
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
 */


#ifndef H46018_NAT
#define H46018_NAT

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class H46018SignalPDU  : public H323SignalPDU
{
  public:
    /**@name Construction */
    //@{
    /** Default Contructor
        This creates a H.225.0 SignalPDU to notify
        the server that the opened TCP socket is for
        the call with the callidentifier specified
    */
    H46018SignalPDU(const OpalGloballyUniqueID & callIdentifier);

    //@}
};

class H323EndPoint;
class H46018Handler;
class H46018Transport  : public H323TransportTCP
{
    PCLASSINFO(H46018Transport, H323TransportTCP);

  public:

    enum PDUType {
        e_raw,
    };

    /**@name Construction */
      //@{
    /**Create a new transport channel.
    */
    H46018Transport(
        H323EndPoint & endpoint,        /// H323 End Point object
        PIPSocket::Address binding      /// Bind Interface
    );

    ~H46018Transport();
    //@}

    /**@name Functions */
    //@{
    /**Write a protocol data unit from the transport.
        This will write using the transports mechanism for PDU boundaries, for
        example UDP is a single Write() call, while for TCP there is a TPKT
        header that indicates the size of the PDU.
    */
    virtual PBoolean WritePDU(
        const PBYTEArray & pdu  /// PDU to write
    );

    /**Read a protocol data unit from the transport.
        This will read using the transports mechanism for PDU boundaries, for
        example UDP is a single Read() call, while for TCP there is a TPKT
        header that indicates the size of the PDU.
    */
    virtual PBoolean ReadPDU(
         PBYTEArray & pdu  /// PDU to Read
    );
    //@}

    /**@name NAT Functions */
    //@{
    /**HandleH46018SignallingChannelPDU
        Handle the H46018 Signalling channel
      */
    PBoolean HandleH46018SignallingChannelPDU(PThread * thread);

    /**HandleH46018SignallingSocket
        Handle the H46018 Signalling socket
      */
    PBoolean HandleH46018SignallingSocket(H323SignalPDU & pdu);

    /**InitialPDU
        Send the initialising PDU for the call with the specified callidentifer
      */
    PBoolean InitialPDU(const OpalGloballyUniqueID & callIdentifier);

    /**isCall
       Do we have a call connected?
      */
    PBoolean isCall() { return isConnected; };

    /**ConnectionLost
       Set the connection as being lost
      */
    void ConnectionLost(PBoolean established);

    /**IsConnectionLost
       Is the connection lost?
      */
    PBoolean IsConnectionLost();
    //@}

    // Overrides
    /**Connect to the remote party.
    */
    virtual PBoolean Connect(const OpalGloballyUniqueID & callIdentifier);

    /**Close the channel.(Don't do anything)
    */
    virtual PBoolean Close();

    virtual PBoolean IsListening() const;

    virtual PBoolean IsOpen() const;

    PBoolean CloseTransport() { return closeTransport; };

  protected:

     PMutex connectionsMutex;
     PMutex WriteMutex;
     PMutex IntMutex;
     PTimeInterval ReadTimeOut;
     PSyncPoint ReadMutex;

     PBoolean   isConnected;
     PBoolean   remoteShutDown;
     PBoolean   closeTransport;
};



class H323EndPoint;
class PNatMethod_H46019;
class H46018Handler : public PObject  
{

    PCLASSINFO(H46018Handler, PObject);

  public:
    H46018Handler(H323EndPoint & ep);
    ~H46018Handler();

    void Enable();
    PBoolean IsEnabled();

    H323EndPoint * GetEndPoint();

    PBoolean CreateH225Transport(const PASN_OctetString & information);

#ifdef H323_H46019M
    void EnableMultiplex(bool enable);
#endif

#ifdef H323_H46024A
    void H46024ADirect(bool reply, const PString & token);
#endif

    void SetTransportSecurity(const H323TransportSecurity & callSecurity);
        
  protected:    
    H323EndPoint & EP;
    PNatMethod_H46019 * nat;
    PString lastCallIdentifer;

#ifdef H323_H46024A
    PMutex    m_h46024aMutex;
    bool    m_h46024a;
#endif

  private:
    H323TransportAddress m_address;
    OpalGloballyUniqueID m_callId;
    PThread * SocketCreateThread;
    PDECLARE_NOTIFIER(PThread, H46018Handler, SocketThread);
    PBoolean m_h46018inOperation;

    H323TransportSecurity m_callSecurity;
};


#ifdef H323_H46019M
typedef map<unsigned, PUDPSocket*> muxSocketMap;
typedef map<PString, unsigned> muxPortMap;

class H46019MultiplexSocket;
#endif

class PNatMethod_H46019  : public H323NatMethod
{
    PCLASSINFO(PNatMethod_H46019,H323NatMethod);

  public:
    /**@name Construction */
    //@{
    /** Default Contructor
    */
    PNatMethod_H46019();

    /** Deconstructor
    */
    ~PNatMethod_H46019();
    //@}

    /**@name General Functions */
    //@{
    /**  AttachEndpoint
        Attach endpoint reference
    */
    void AttachHandler(H46018Handler * _handler);

    H46018Handler * GetHandler();

    /**  GetExternalAddress
        Get the external address.
        This function is not used in H46019
    */
    virtual PBoolean GetExternalAddress(
        PIPSocket::Address & externalAddress, ///< External address of router
        const PTimeInterval & maxAge = 1000   ///< Maximum age for caching
      );

    /**  CreateSocketPair
        Create the UDP Socket pair (does nothing)
    */
#if PTLIB_VER < 2130
    virtual PBoolean CreateSocketPair(
        PUDPSocket * & /*socket1*/,            ///< Created DataSocket
        PUDPSocket * & /*socket2*/,            ///< Created ControlSocket
        const PIPSocket::Address & /*binding*/
        ) {  return false; }
#endif

    /**  CreateSocketPair
        Create the UDP Socket pair
    */
    virtual PBoolean CreateSocketPair(
        PUDPSocket * & socket1,            ///< Created DataSocket
        PUDPSocket * & socket2,            ///< Created ControlSocket
        const PIPSocket::Address & binding,  
#if PTLIB_VER >= 2130
        PObject * userData = NULL
#else
        void * userData = NULL
#endif
    );

    /**  isAvailable.
        Returns whether the Nat Method is ready and available in
        assisting in NAT Traversal. The principal is function is
        to allow the EP to detect various methods and if a method
        is detected then this method is available for NAT traversal
        The Order of adding to the PNstStrategy determines which method
        is used
    */
    virtual bool IsAvailable(const PIPSocket::Address & address = PIPSocket::GetDefaultIpAny());

    /* Set Available
        This will enable the natMethod to be enabled when opening the
        sockets
    */
    void SetAvailable();

    /** Activate
        Active/DeActivate the method on a call by call basis
     */
    virtual void Activate(bool act)  { active = act; }

#ifdef H323_H46019M
    /** EnableMultiplex
        Enable Multiplexing for this call
     */
    static void EnableMultiplex(bool enable);
#endif

    /**  OpenSocket
        Create a single UDP Socket 
    */
    PBoolean OpenSocket(PUDPSocket & socket, PortInfo & portInfo, const PIPSocket::Address & binding) const;

    /**  GetMethodName
        Get the NAT method name 
    */
#if PTLIB_VER >= 2130
   virtual PCaselessString GetMethodName() const { return "H46019"; }
#elif PTLIB_VER > 2120
   static PString GetNatMethodName() { return "H46019"; }
   virtual PString GetName() const
            { return GetNatMethodName(); }
#else
    static PStringList GetNatMethodName() {  return PStringArray("H46019"); };
    virtual PString GetName() const
            { return GetNatMethodName()[0]; }
#endif

   // All these are virtual and never used. 
    virtual bool GetServerAddress(
      PIPSocket::Address & address,   ///< Address of server
      WORD & port                     ///< Port server is using.
      ) const { return false; }

    virtual bool GetInterfaceAddress(
      PIPSocket::Address & internalAddress
      ) const { return false; }

    virtual PBoolean CreateSocket(
      PUDPSocket * & socket,
      const PIPSocket::Address & binding = PIPSocket::GetDefaultIpAny(),
      WORD localPort = 0
      ) { return false; }

    virtual RTPSupportTypes GetRTPSupport(
      PBoolean force = PFalse    ///< Force a new check
      )  { return RTPSupported; }

    //@}

#if PTLIB_VER >= 2110
    virtual PString GetServer() const { return PString(); }
    virtual bool GetServerAddress(PIPSocketAddressAndPort & ) const { return false; }
    virtual NatTypes GetNatType(bool) { return UnknownNat; }
    virtual NatTypes GetNatType(const PTimeInterval &) { return UnknownNat; }
    virtual bool SetServer(const PString &) { return false; }
    virtual bool Open(const PIPSocket::Address &) { return false; }
    virtual bool CreateSocket(BYTE component,PUDPSocket * & socket,
            const PIPSocket::Address & binding = PIPSocket::GetDefaultIpAny(),WORD localPort = 0)  { return false; }
    virtual void SetCredentials(const PString &, const PString &, const PString &) {}
protected:
#if PTLIB_VER < 2130
    virtual NatTypes InternalGetNatType(bool, const PTimeInterval &) { return UnknownNat; }
#endif
#if PTLIB_VER >= 2120
    virtual PNATUDPSocket * InternalCreateSocket(Component component, PObject * context)  { return NULL; }
    virtual void InternalUpdate() {};
#endif
#endif


  protected:
    
    PBoolean available;                 ///< Whether this NAT Method is available for call
    PBoolean active;                    ///< Whether the method is active for call
    H46018Handler * handler;            ///< handler

};

#ifndef _WIN32_WCE
    #if PTLIB_VER > 260
       PPLUGIN_STATIC_LOAD(H46019,PNatMethod);
    #else
       PWLIB_STATIC_LOAD_PLUGIN(H46019, PNatMethod);
    #endif
#endif

#ifdef H323_H46019M

struct  H46019MultiPacket {
  PIPSocket::Address fromAddr;
  WORD               fromPort;
  PBYTEArray         frame;
};

typedef std::queue<H46019MultiPacket> H46019MultiQueue;

class H46019MultiplexSocket : public H323UDPSocket
{
  PCLASSINFO(H46019MultiplexSocket, PUDPSocket);

  public:
    H46019MultiplexSocket();

    H46019MultiplexSocket(bool rtp);

    ~H46019MultiplexSocket();

     /**Read a datagram from a remote computer
       @return PTrue if any bytes were sucessfully read.
       */
    virtual PBoolean ReadFrom(
      void * buf,     ///< Data to be written as URGENT TCP data.
      PINDEX len,     ///< Number of bytes pointed to by #buf#.
      Address & addr, ///< Address from which the datagram was received.
      WORD & port     ///< Port from which the datagram was received.
    );

    /**Write a datagram to a remote computer.
       @return PTrue if all the bytes were sucessfully written.
     */
    virtual PBoolean WriteTo(
      const void * buf,   ///< Data to be written as URGENT TCP data.
      PINDEX len,         ///< Number of bytes pointed to by #buf#.
      const Address & addr, ///< Address to which the datagram is sent.
      WORD port           ///< Port to which the datagram is sent.
    );

    virtual PBoolean Close();

    PString GetLocalAddress();
    PBoolean GetLocalAddress(Address & addr, WORD & port);

    PUDPSocket * & GetSubSocket()  { return m_subSocket; }

  private:

    PUDPSocket              *  m_subSocket;
    PMutex                     m_mutex;

};
#endif

class H46019UDPSocket : public H323UDPSocket
{
    PCLASSINFO(H46019UDPSocket, H323UDPSocket);
  public:
    /**@name Construction/Deconstructor */
    /** create a UDP Socket Fully Nat Supported
        ready for H323plus to Call.
    */
    H46019UDPSocket(H46018Handler & _handler, PObject * info, bool _rtpSocket);

    /** Deconstructor to reallocate Socket and remove any exiting
        allocated NAT ports, 
    */
    ~H46019UDPSocket();
    //@}

    /** Close the socket
      */
    PBoolean Close();

    /** Get local Address
      */
    virtual PBoolean GetLocalAddress(Address & addr, WORD & port);

    /**@name Functions */
    //@{

    /** Allocate (FastStart) keep-alive mechanism but don't Activate
    */
    void Allocate(const H323TransportAddress & keepalive, 
            unsigned _payload, 
            unsigned _ttl
            );

    /** Activate (FastStart) keep-alive mechanism.
    */
    void Activate();

    /** Activate keep-alive mechanism.
    */
    void Activate(const H323TransportAddress & keepalive,    ///< KeepAlive Address
            unsigned _payload,            ///< RTP Payload type    
            unsigned _ttl                ///< Time interval for keepalive.
            );

    /** Get the Ping Payload
      */
    unsigned GetPingPayload();

    /** Set the Ping Payload
      */
    void SetPingPayLoad(unsigned val);

    /** Get Ping TTL
      */
    unsigned GetTTL();

    /** Set Ping TTL
      */
    void SetTTL(unsigned val);

#ifdef H323_H46019M

    /** Get Peer Address
      */
    virtual PBoolean GetPeerAddress(
        PIPSocketAddressAndPort & addr    ///< Variable to receive hosts IP address and port.
    );


    /** Get Multiplex Address
      */
    void GetMultiplexAddress(H323TransportAddress & address,       ///< Multiplex Address
                             unsigned & multiID,                   ///< Multiplex ID
                             PBoolean   isOLCack                   ///< Direction
                             );

    void SetMultiplexID(unsigned id, PBoolean isAck);

    unsigned GetRecvMultiplexID() const;
    unsigned GetSendMultiplexID() const;

    PBoolean WriteMultiplexBuffer(
              const void * buf,     ///< Data to be written.
              PINDEX len,           ///< Number of bytes pointed to by #buf#.
              const Address & addr, ///< Address to which the datagram is sent.
              WORD port             ///< Port to which the datagram is sent.
           );

    PBoolean ReadMultiplexBuffer(
              void * buf,     ///< Data to be written.
              PINDEX & len,   ///< Number of bytes pointed to by #buf#.
              Address & addr, ///< Address from which the datagram was received.
              WORD & port     ///< Port from which the datagram was received.
           );

    void ClearMultiplexBuffer();

    virtual PBoolean DoPseudoRead(int & selectStatus);

    PBoolean ReadSocket(
              void * buf,     ///< Data to be written.
              PINDEX & len,   ///< Number of bytes pointed to by #buf#.
              Address & addr, ///< Address from which the datagram was received.
              WORD & port     ///< Port from which the datagram was received.
            );

    PBoolean WriteSocket(
      const void * buf,        ///< Data to be written as URGENT TCP data.
      PINDEX len,              ///< Number of bytes pointed to by #buf#.
      const Address & addr,    ///< Address to which the datagram is sent.
      WORD port,               ///< Port to which the datagram is sent.
      unsigned altMux = 0      ///< Whether use Alternate MUX
    );
#endif


     /**Read a datagram from a remote computer
       @return PTrue if any bytes were sucessfully read.
       */
    virtual PBoolean ReadFrom(
      void * buf,     ///< Data to be written as URGENT TCP data.
      PINDEX len,     ///< Number of bytes pointed to by #buf#.
      Address & addr, ///< Address from which the datagram was received.
      WORD & port     ///< Port from which the datagram was received.
    );

    /**Write a datagram to a remote computer.
       @return PTrue if all the bytes were sucessfully written.
     */
    virtual PBoolean WriteTo(
      const void * buf,     ///< Data to be written as URGENT TCP data.
      PINDEX len,           ///< Number of bytes pointed to by #buf#.
      const Address & addr, ///< Address to which the datagram is sent.
      WORD port             ///< Port to which the datagram is sent.
    );

    /**Write a datagram to a remote computer.
       @return PTrue if all the bytes were sucessfully written.
     */
    virtual PBoolean WriteTo(
      const void * buf,   ///< Data to be written as URGENT TCP data.
      PINDEX len,         ///< Number of bytes pointed to by #buf#.
      const Address & addr, ///< Address to which the datagram is sent.
      WORD port,           ///< Port to which the datagram is sent.
      unsigned id          ///< id (could be MUX ID default = 0)
    );


#if defined(H323_H46024A) || defined(H323_H46024B)

    enum  probe_state {
        e_notRequired,        ///< Polling not required
        e_initialising,        ///< We are initialising (local set but remote not)
        e_idle,                ///< Idle (waiting for first packet from remote)
        e_probing,            ///< Probing for direct route
        e_verify_receiver,    ///< verified receive connectivity    
        e_verify_sender,    ///< verified send connectivity
        e_wait,                ///< we are waiting for direct media (to set address)
        e_direct            ///< we are going direct to detected address
    };

    PString ProbeState(probe_state state);

    struct probe_packet {
        PUInt16b    Length;        // Length
        PUInt32b    SSRC;        // Time Stamp
        BYTE        name[4];    // Name is limited to 32 (4 Bytes)
        BYTE        cui[20];    // SHA-1 is always 160 (20 Bytes)
    };


    /** Set Alternate Direct Address
      */
    virtual void SetAlternateAddresses(
        const H323TransportAddress & address, 
        const PString & cui, 
        unsigned muxID
    );

     /** Set Alternate Direct Address
      */
    virtual void GetAlternateAddresses(
        H323TransportAddress & address, 
        PString & cui, 
        unsigned & muxID
    );

    /** Callback to check if the address received is a permitted alternate
      */
    virtual PBoolean IsAlternateAddress(
        const Address & address,    ///< Detected IP Address.
        WORD port                    ///< Detected Port.
        );

    /** Start sending media/control to alternate address
      */
    void H46024Adirect(bool starter);
#endif

#ifdef H323_H46024B
    /** Start Probing to alternate address
      */
    void H46024Bdirect(const H323TransportAddress & address, unsigned muxID);
#endif
    //@}

  protected:

 // H.460.19 Keepalives
    void InitialiseKeepAlive();    ///< Start the keepalive
    void SendRTPPing(const PIPSocket::Address & ip, const WORD & port, unsigned id = 0);
    void SendRTCPPing();
    PBoolean SendRTCPFrame(RTP_ControlFrame & report, const PIPSocket::Address & ip, WORD port, unsigned id = 0);
    PMutex PingMutex;

#ifdef H323_H46024A
    // H46024 Annex A support
    PBoolean ReceivedProbePacket(const RTP_ControlFrame & frame, bool & reply, bool & sendResponse);
    void BuildProbe(RTP_ControlFrame & report, bool reply);
    void StartProbe();
    void ProbeResponse(bool reply, const PIPSocket::Address & addr, WORD & port);
    void SetProbeState(probe_state newstate);
    int GetProbeState() const;
    PMutex probeMutex;
#endif

private:
    H46018Handler & m_Handler;
    unsigned m_Session;                        ///< Current Session ie 1-Audio 2-video
    PString m_Token;                        ///< Current Connection Token
    OpalGloballyUniqueID m_CallId;            ///< CallIdentifier
    PString m_CUI;                            ///< Local CUI (for H.460.24 Annex A)

 // H.460.19 Keepalives
    PIPSocket::Address keepip;                ///< KeepAlive Address
    WORD keepport;                            ///< KeepAlive Port
    unsigned keeppayload;                    ///< KeepAlive RTP payload
    unsigned keepTTL;                        ///< KeepAlive TTL
    WORD keepseqno;                            ///< KeepAlive sequence number
    PTime * keepStartTime;                    ///< KeepAlive start time for TimeStamp.

    PThread * initialKeep;                    ///< Initial keepalive thread.
    PDECLARE_NOTIFIER(PThread, H46019UDPSocket, StartKeepAlives); ///< First keep alives handling

    PDECLARE_NOTIFIER(PTimer, H46019UDPSocket, Ping);    ///< Timer to notify to poll for External IP
    PTimer    Keep;                                        ///< Polling Timer

#ifdef H323_H46019M
    H323MultiplexConnection* m_connection;          ///< Multiplex Connection
    H323_MultiplexHandler  * m_muxHandler;          ///< Multiplex Handler
    unsigned         m_recvMultiplexID;             ///< Multiplex ID
    unsigned         m_sendMultiplexID;             ///< Multiplex ID
    PIPSocketAddressAndPort m_muxLocalAddress;      ///< Multiplex local address
    H46019MultiQueue m_multQueue;                   ///< Incoming frame Queue
    unsigned         m_multiBuffer;                 ///< Multiplex BufferSize
    PMutex           m_multiMutex;                  ///< MultiQueue mutex
    PBoolean         m_shutDown;                    ///< Shutdown
#endif

#if defined(H323_H46024A) || defined(H323_H46024B)
    // H46024 Annex A support
    PString m_CUIrem;                                        ///< Remote CUI
    PIPSocket::Address m_locAddr;  WORD m_locPort;            ///< local Address (address used when starting socket)
    PIPSocket::Address m_remAddr;  WORD m_remPort;            ///< Remote Address (address used when starting socket)
    PIPSocket::Address m_detAddr;  WORD m_detPort;            ///< detected remote Address (as detected from actual packets)
    PIPSocket::Address m_pendAddr;  WORD m_pendPort;        ///< detected pending RTCP Probe Address (as detected from actual packets)
    PDECLARE_NOTIFIER(PTimer, H46019UDPSocket, Probe);        ///< Thread to probe for direct connection
    PTimer m_Probe;                                            ///< Probe Timer
    PINDEX m_probes;                                        ///< Probe count
    DWORD SSRC;                                                ///< Random number
#endif
    PIPSocket::Address m_altAddr;  
	WORD m_altPort;                                           ///< supplied remote Address (as supplied in Generic Information)
	unsigned m_altMuxID;

#ifdef H323_H46024A
    probe_state m_state;
#endif

#ifdef H323_H46024B
    PBoolean    m_h46024b;
#endif

	PAdaptiveDelay readBlock;
    bool rtpSocket;

};

#endif // H46018_NAT





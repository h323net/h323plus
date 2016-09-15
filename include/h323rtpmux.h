/*
 * h323rtpmux.h
 *
 * Copyright (c) 2016 ISVO (Asia) Pte Ltd. All Rights Reserved.
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

//////////////////////////////////////////////////////////////////////////////////////

class H323_MultiplexSocket : public H323UDPSocket
{
    PCLASSINFO(H323_MultiplexSocket, H323UDPSocket);

public:

    H323_MultiplexSocket(Type type);

    ~H323_MultiplexSocket();

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

    /**Close the Socket, shutting down the underlying socket.
        @return true if the channel successfully closed.
    */
    virtual PBoolean Close();

    /**Get the Internet Protocol address and port for the underlying socket.
       @return IP String or empty string if the IP number was not available.
    */
    PString GetLocalAddress();

    /**Get the Internet Protocol address and port for the underlying socket.
    @return true if the IP number was found.
    */
    virtual PBoolean GetLocalAddress(Address & addr, WORD & port);

    /**Get the underlying subsocket
    @return pointer to the socket address
    */
    PUDPSocket * & GetSubSocket() { return m_subSocket; }

private:

    PUDPSocket              *  m_subSocket;     ///< The underlying socket the mux socket is wrapping
    PMutex                     m_mutex;

};

/////////////////////////////////////////////////////////////////////////////////////////////

/** H323_MultiplexThread
H323plus Multiplex Thread base class
*/

class H323_MultiplexHandler;
class H323_MultiplexThread
{
public:

    H323_MultiplexThread(H323_MultiplexHandler & handler);

    virtual ~H323_MultiplexThread();

    void Shutdown();

    virtual void Work() = 0;

protected:

    H323_MultiplexHandler                        &  m_handler;      ///< Multiplex Handler
    PSyncPointAck                                   m_shutdown;     ///< Close

};


////////////////////////////////////////////////////////////////////////////////////////////
/** H323RTPMultiplexHandler
    H323plus multiplex media Handler

    This class manages Multiplexing of media

*/

class H323_MultiplexHandler : public PObject
{
public:

    /**@name Construction */
    //@{
    /** Class Constructor  
    */
    H323_MultiplexHandler(H323UDPSocket::Type type);

    /** Class Destructor
    */
    virtual ~H323_MultiplexHandler();
    //@}

    /**@name Handler Start/Stop */
    //@{
    /** Start the multiplex handler
    */
    bool Start();

    /** Stop the multiplex handler
    */
    bool Stop();

    /** Close the multiplex handler
    */
    void Close();
    //@}

    /**@name Register Sockets */
    //@{

    /**Register a socket for multiplex
      */
    void Register(unsigned id, H323UDPSocket::Type type, PUDPSocket * socket);

    /**Unregister a socket for multiplex
      */
    void Unregister(unsigned id, H323UDPSocket::Type type);

    /**Get the socket for a given direction
      */
    H323UDPSocket * GetSocket(unsigned id, H323UDPSocket::Type type, const PIPSocket::Address & addr, WORD port);

    /** Get Pointer to subsocket for the given Type
    Use this to create the actual socket.
    */
    PUDPSocket * & GetMultiplexSocket(H323UDPSocket::Type type);

    /** Get Port
    */
    unsigned GetPort(bool max);
    //@}

    /**@name Structures */
    //@{
    /** Read Parameters.
    */
    struct ReadParams {
        ReadParams();
#if PTRACING
        void Print(PStringStream & strm) const;
#endif
        void *              m_buffer;              ///< Data to read
        PINDEX              m_length;              ///< Maximum length of data
        PIPSocket::Address  m_addr;                ///< Remote IP address data came from, or is written to
        WORD                m_port;                ///< Remote port data came from, or is written to
        PINDEX              m_lastCount;           ///< Actual length of data read/written
        PTimeInterval       m_timeout;             ///< Time to wait for data
        PChannel::Errors    m_errorCode;           ///< Error code for read/write
        int                 m_errorNumber;         ///< Error number (OS specific) for read/write
    };

    /** Write Parameters.
    */
    struct WriteParams {
        WriteParams();
        PBYTEArray          m_buffer;              ///< Data to write
        PIPSocket::Address  m_addr;                ///< Remote IP address data came from, or is written to
        WORD                m_port;                ///< Remote port data came from, or is written to
    };

    /** Port Information.
    */ 
    class PortInfo  : public PObject {
    public:
        /** Port information */
        PortInfo();
        /** Set Ports */
        void SetPorts(
            unsigned base,                  ///< Base port
            unsigned max                    ///< Max Port
            );
        /** Get Base Port */
        unsigned GetBase();
        /** Get Next Port */
        unsigned GetNext(
            unsigned increment              ///< Port Increment
            );
        /** Get Max Port */
        unsigned GetMax();
    private:
        unsigned   m_base;                  ///< Base port                          
        unsigned   m_max;                   ///< Max Port
        unsigned   m_current;               ///< Current Port
    };
    //@}

    //@}

    typedef std::map<H323UDPSocket::Type, PUDPSocket *> SocketBundle;
    typedef std::map<unsigned, SocketBundle> SocketMap;
    typedef std::queue< std::pair<H323UDPSocket::Type, WriteParams> > SocketQueue;

    
    /**@name Read/Write Thread Management */
    //@{
    /** Get the Mux Socket Bundle
      */
    SocketBundle & GetMultiplexSockets();

    /** Get the Socket Map for the receiving direction
    */
    SocketMap & GetSocketMap();

    /** Get the SendQueue
    */
    SocketQueue & GetSendQueue();

    /** Get Socket Mutex
      */
    PMutex & GetSocketMutex();

    /** Get Send Mutex
    */
    PMutex & GetSendMutex();
    //@}

    /**@name Ports and Multiplex functions */
    //@{
    /** Set multiplex Port numbers
    */
    virtual void SetPorts(unsigned min, unsigned max);

    /** Get Multiplex Port
    */
    unsigned GetMultiplexPort(H323UDPSocket::Type type);

    /** SetMultiplexSend whether we are using Multiplex Send
    */
    void SetMultiplexSend(PBoolean enable);

    /** IsMultiplexSend Query whether we are using Multiplex Send
      */
    PBoolean IsMultiplexSend();
    //@}

    /**@name Derived classes functions */
    //@{

    /**OnDataArrival: Override to process incoming data from the sockets
    */
    virtual bool OnDataArrival(
        H323UDPSocket::Type type,     ///< Multiplex Type
        const ReadParams & param      ///< Read Parameters
        ) = 0;

    /**Write a datagram to a remote computer.
    @return true if all the bytes were sucessfully written.
    */
    virtual bool WriteTo(
        H323UDPSocket::Type type,    ///< Send Type
        unsigned id,                        ///< Multiplex id
        const void * buf,                   ///< Data to be written as URGENT TCP data.
        PINDEX len,                         ///< Number of bytes pointed to by #buf#.
        const PIPSocket::Address & addr,    ///< Address to which the datagram is sent.
        WORD port                           ///< Port to which the datagram is sent.
        ) = 0;

    /**GetMultiplexIdentifier
    @return the next multiplex identifier (if supported otherwise return 0)
    */
    virtual unsigned NextMultiplexID(unsigned userData=0) { return 0; }
    //@}

protected:

    /**@name Internal functions */
    //@{
    /*QueueSendData: Queue data to be sent
    */
    bool QueueSendData(
        H323UDPSocket::Type type,    ///< Multiplex Type
        const WriteParams & param           ///< Write Parameters
        );

    /*ResolveSession: Resolve an unknow received session
    */
    H323UDPSocket * ResolveSession(unsigned id, H323UDPSocket::Type type, const PIPSocket::Address & addr, WORD port);

    /*DetectSourceAddress: Resolve a session sent with no MuxID
    */
    H323UDPSocket * DetectSourceAddress(H323UDPSocket::Type type, const PIPSocket::Address & addr, WORD port);
    //@}

    SocketBundle         m_muxSockets;      ///< Multiplex Sockets
    SocketMap            m_socketReadMap;   ///< Mapped Read Sockets
    SocketQueue          m_sendQueue;       ///< Media send queue
    PortInfo             m_portInfo;        ///< Port assignment info

private:

    bool                 m_muxSend;     ///< Whether the handler can send and receive mux
    bool                 m_isRunning;   ///< Is the thread pool running
    PMutex               m_mutex;       ///< Socket registering/unregistering Mutex 
    PMutex               m_sendmutex;   ///< Packet sending Mutex

    std::list<H323_MultiplexThread*>          m_workers;  ///< Workers

#if PTLIB_VER > 2130
    PQueuedThreadPool<H323_MultiplexThread>  m_pool;  ///< Send/Receive Thread Pool
#endif

};


/////////////////////////////////////////////////////////////////////////////////////////////

/** H323_MultiplexReceive
H323plus Multiplex Receive Thread
*/
class H323_MultiplexReceive : public H323_MultiplexThread
{
public:

    H323_MultiplexReceive(H323_MultiplexHandler & handler)
        : H323_MultiplexThread(handler), m_muxSockets(handler.GetMultiplexSockets()) { }

    virtual void Work();

    bool ReadFromSocketList(PSocket::SelectList & readers, H323_MultiplexHandler::ReadParams & param);

private:

    H323_MultiplexHandler::SocketBundle          &  m_muxSockets;   ///< Multiplex Socket Bundle

};


///////////////////////////////////////////////////////////////////////////////////////////////

/** H323_MultiplexSend
H323plus Multiplex Send Thread
*/
class H323_MultiplexSend : public H323_MultiplexThread
{
public:

    H323_MultiplexSend(H323_MultiplexHandler & handler)
        : H323_MultiplexThread(handler),
          m_muxSockets(handler.GetMultiplexSockets()),
          m_mutex(handler.GetSendMutex()), m_sendQueue(handler.GetSendQueue()) { }

    virtual void Work();

private:
    H323_MultiplexHandler::SocketBundle      &  m_muxSockets;   ///< Multiplex Socket Bundle
    PMutex                                   &  m_mutex;        ///< MultiplexHandler Mutex 
    H323_MultiplexHandler::SocketQueue       &  m_sendQueue;    ///< MultiplexHandler send queue

};

///////////////////////////////////////////////////////////////////////////////////////////////

class H323MultiplexConnection : public PObject
{
public:

    H323MultiplexConnection(H323_MultiplexHandler * handler);

    virtual ~H323MultiplexConnection();

    /** Socket Pairs for Connection
      */
    struct SocketPair
    {
        SocketPair(PUDPSocket * _rtp, PUDPSocket * _rtcp)
            : m_rtp(_rtp), m_rtcp(_rtcp) {}

        PUDPSocket     * m_rtp;
        PUDPSocket     * m_rtcp;
    };

    typedef std::map<unsigned, SocketPair> SocketPairs;


    /**Session Information
    This contains session information which is passed to the socket handler
    when creating RTP socket pairs.
    */
    class SessionInformation : public PObject
    {
    public:
        SessionInformation(H323MultiplexConnection * handler, const OpalGloballyUniqueID & id, unsigned crv, const PString & token, unsigned session);

        unsigned GetCallReference();

        const PString & GetCallToken();

        unsigned GetSessionID() const;

        const OpalGloballyUniqueID & GetCallIdentifer();

        void SetSendMultiplexID(unsigned id);

        unsigned GetRecvMultiplexID() const;

        const PString & GetCUI();

        H323MultiplexConnection * GetMultiplexConnection();

    protected:
        H323MultiplexConnection * m_muxHandler;
        OpalGloballyUniqueID m_callID;
        unsigned m_crv;
        PString m_callToken;
        unsigned m_sessionID;
        unsigned m_recvMultiID;
        unsigned m_sendMultiID;
        PString m_CUI;
    };

    /** Get Multiplex handler
      */
    H323_MultiplexHandler * GetMultiplexHandler();


    /** Get Session pair
      */
    SocketPair * GetSocketPair(unsigned id);

    /** Start the multiplex handler
    */
    bool Start();

    /** Build Session Information object
        This is called every time a NAT socket is created.
      */
    SessionInformation * BuildSessionInformation(unsigned sessionID, const H323Connection * con);

    /** Set the Sockets for the session
      */
    void SetSocketSession(unsigned sessionid, unsigned recvMux, PUDPSocket * _rtp, PUDPSocket * _rtcp);

    /** remove the Socket references for the session (do not remove the 
    */
    void RemoveSocketSession(unsigned sessionid);

    /** Get Pointer to subsocket for the given Type
    Use this to create the actual socket.
    */
    PUDPSocket * & GetMultiplexSocket(H323UDPSocket::Type type);

    /** Get Multiplex Port
    */
    unsigned GetMultiplexPort(H323UDPSocket::Type type);

    /** Get Port
    */
    unsigned GetMultiplexPort(bool max);

    /** Get Socket Pairs
      */
    SocketPairs & GetSocketPairs();

    /** Remove all the socket pairs (when migrating from fast to slow connect)
    */
    void ClearPairs();

    /**Write a datagram to a remote computer.
    @return true if all the bytes were sucessfully written.
    */
    virtual bool WriteTo(
        H323UDPSocket::Type type,    ///< Send Type
        unsigned id,                        ///< Multiplex id
        const void * buf,                   ///< Data to be written.
        PINDEX len,                         ///< Number of bytes pointed to by #buf#.
        const PIPSocket::Address & addr,    ///< Address to which the datagram is sent.
        WORD port                           ///< Port to which the datagram is sent.
        );

protected:

    unsigned GetNextMultiplexID();

    H323_MultiplexHandler * m_handler;

    SocketPairs             m_sockets;
    PMutex                  m_mutex;

};

///////////////////////////////////////////////////////////////////////////////////////////////

class H460_MultiplexHandler : public H323_MultiplexHandler
{
public:

    /**@name Construction */
    //@{
    /** Class Constructor
    */
    H460_MultiplexHandler();

    /** Class Destructor
    */
    virtual ~H460_MultiplexHandler();
    //@}

    /**@name Derived classes functions */
    //@{

    /** Set multiplex Port numbers
    */
    virtual void SetPorts(unsigned min, unsigned max);

    /**OnDataArrival: Override to process incoming data from the sockets
    */
    virtual bool OnDataArrival(H323UDPSocket::Type type, const ReadParams & param);

    /**Write a datagram to a remote computer.
    @return true if all the bytes were sucessfully written.
    */
    virtual bool WriteTo(
                H323UDPSocket::Type type,    ///< Send Type
                unsigned id,                        ///< Multiplex id
                const void * buf,                   ///< Data to be written.
                PINDEX len,                         ///< Number of bytes pointed to by #buf#.
                const PIPSocket::Address & addr,    ///< Address to which the datagram is sent.
                WORD port                           ///< Port to which the datagram is sent.
                );

    /**GetMultiplexIdentifier
    @return the next multiplex identifier (if supported otherwise return 0)
    */
    virtual unsigned NextMultiplexID(unsigned userData);
    //@}

protected:
    PortInfo             m_multiplexId;    ///< Multiplex identifier

};


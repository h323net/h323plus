/*
 * h323mux.cxx
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


#include <h323.h>

#ifdef H323_NAT 

#include <h323rtpmux.h>
#include <map>

#include "h460/h46018_h225.h"

////////////////////////////////////////////////////////////////////////////

template <class PAIR>
class deletepair { // PAIR::second_type is a pointer type
public:
    void operator()(const PAIR & p) { delete p.second; }
};

template <class M>
inline void DeleteObjectsInMap(M & m)
{
    typedef typename M::value_type PAIR;
    std::for_each(m.begin(), m.end(), deletepair<PAIR>());
    m.clear();
}

////////////////////////////////////////////////////////////////////////////

H323_MultiplexThread::H323_MultiplexThread(H323_MultiplexHandler & handler)
    : m_handler(handler)
{ 

}

H323_MultiplexThread::~H323_MultiplexThread()
{
    Shutdown();
}

void H323_MultiplexThread::Shutdown()
{
    m_shutdown.Signal();
}

////////////////////////////////////////////////////////////////////////////


bool H323_MultiplexReceive::ReadFromSocketList(PSocket::SelectList & readers,
                                            H323_MultiplexHandler::ReadParams & param)
{

    param.m_lastCount = 0;
    param.m_errorCode = PSocket::Select(readers, param.m_timeout);

    switch (param.m_errorCode) {
    case PChannel::NoError:
        break;

    case PChannel::NotOpen: // Interface went down
        PTRACE(3, "RTPMUX\tUDP Port not open.");
        param.m_errorCode = PChannel::Interrupted;
        return false;
    default:
        return true;
    }

    if (readers.IsEmpty()) {
        param.m_errorCode = PChannel::Timeout;
        return false;
    }

    for (PSocket::SelectList::iterator it = readers.begin(); it != readers.end(); ++it) {

        H323UDPSocket & socket = dynamic_cast<H323UDPSocket &>(*it);

        bool ok = socket.ReadFrom(param.m_buffer, param.m_length, param.m_addr, param.m_port);
        param.m_lastCount = socket.GetLastReadCount();
        param.m_errorCode = socket.GetErrorCode(PChannel::LastReadError);
        param.m_errorNumber = socket.GetErrorNumber(PChannel::LastReadError);

        if (ok && (param.m_lastCount > 0) && m_handler.OnDataArrival(socket.GetType(), param))
            return true;

        switch (param.m_errorCode) {
#if PTLIB_VER > 2130
        case PChannel::Unavailable:
            PTRACE(3, "RTPMUX\tUDP Port on remote not ready.");
            break;
#endif
        case PChannel::BufferTooSmall:
            PTRACE(2, "RTPMUX\tRead UDP packet too large for buffer of " << param.m_length << " bytes.");
            break;

        case PChannel::NotFound:
            PTRACE(4, "RTPMUX\tInterface went down");
            param.m_errorCode = PChannel::Interrupted;
            return false;
            break;

        default:
            PTRACE(1, "RTPMUX\tSocket read UDP error ("
                << socket.GetErrorNumber(PChannel::LastReadError) << "): "
                << socket.GetErrorText(PChannel::LastReadError));
        }
    }

    return true;
}


void H323_MultiplexReceive::Work()
{
    PBYTEArray buffer(2000);
    H323_MultiplexHandler::ReadParams param;
    param.m_buffer = buffer.GetPointer();
    param.m_length = buffer.GetSize();
    
    bool ok = true;
    do {
        if (ok) {
            PSocket::SelectList readers;

            for (H323_MultiplexHandler::SocketBundle::iterator it = m_muxSockets.begin(); it != m_muxSockets.end(); ++it) {
                PUDPSocket * & socket = ((H323_MultiplexSocket *)it->second)->GetSubSocket();
                if (socket)
                    readers += *socket;
            }
            if (!ReadFromSocketList(readers, param)) {
                PTRACE(4, "RTPMUX\tError Reading: Reason " << param.m_errorCode);
                ok = false;
            }

        }  else
            PThread::Sleep(5);

    } while (!m_shutdown.Wait(0));

    PTRACE(4, "RTPMUX\tShutting down multiplex read thread: Reason " << param.m_errorCode);
    m_shutdown.Acknowledge();
}


////////////////////////////////////////////////////////////////////////////


void H323_MultiplexSend::Work()
{
    bool ok = true;

    do {

        if (ok && m_sendQueue.size() > 0) {
            m_mutex.Wait();
                H323UDPSocket::Type & type = m_sendQueue.front().first;
                H323_MultiplexHandler::WriteParams & params = m_sendQueue.front().second;
                PUDPSocket * socket = ((H323_MultiplexSocket *)m_muxSockets[type])->GetSubSocket();
                ok = (socket && socket->IsOpen());
                if (ok && !socket->WriteTo(params.m_buffer.GetPointer(), params.m_buffer.GetSize(), params.m_addr, params.m_port)) {
                    PTRACE(1, "MUX\tError sending " << type << " " << params.m_buffer.GetSize() << " bits to: "
                        << params.m_addr << ":" << params.m_port);
                }
                m_sendQueue.pop();
            m_mutex.Signal();
        }  else
            PThread::Sleep(5);

    } while (!m_shutdown.Wait(0));

    PTRACE(4, "RTPMUX\tShutting down multiplex Send thread");
    m_shutdown.Acknowledge();
}

////////////////////////////////////////////////////////////////////////////


H323_MultiplexHandler::H323_MultiplexHandler(H323UDPSocket::Type type)
    : m_muxSend(false), m_isRunning(false)
{
    switch (type) {
        case H323UDPSocket::rtp:
            m_muxSockets.insert(
                std::pair<H323UDPSocket::Type, H323UDPSocket *>(H323UDPSocket::rtp, new H323_MultiplexSocket(H323UDPSocket::rtp)));
        case H323UDPSocket::rtcp:
            m_muxSockets.insert(
                std::pair<H323UDPSocket::Type, H323UDPSocket *>(H323UDPSocket::rtcp, new H323_MultiplexSocket(H323UDPSocket::rtcp)));
            break;
        case H323UDPSocket::sctp:
            m_muxSockets.insert(
                std::pair<H323UDPSocket::Type, H323UDPSocket *>(H323UDPSocket::sctp, new H323_MultiplexSocket(H323UDPSocket::sctp)));
            break;
        default:
            break;
    }
}

H323_MultiplexHandler::~H323_MultiplexHandler()
{
    Close();
}


bool H323_MultiplexHandler::Start()
{
    if (!m_isRunning) {
#if PTLIB_VER > 2130
        m_pool.SetMaxWorkers(2);
#endif
        m_workers.push_back(new H323_MultiplexReceive(*this));
        if (m_muxSend)
            m_workers.push_back(new H323_MultiplexSend(*this));

#if PTLIB_VER > 2130
        std::list<H323_MultiplexThread*>::iterator i;
        for (i = m_workers.begin(); i != m_workers.end(); ++i) {
            m_pool.AddWork(*i);
        }
#endif

        m_isRunning = true;
    }

    return true;
}

bool H323_MultiplexHandler::Stop()
{
    if (m_isRunning) {
        for (H323_MultiplexHandler::SocketBundle::iterator it = m_muxSockets.begin(); it != m_muxSockets.end(); ++it)
            it->second->Close();

        std::list<H323_MultiplexThread*>::iterator i;
        for (i = m_workers.begin(); i != m_workers.end(); ++i) {
#if PTLIB_VER > 2130
            m_pool.RemoveWork(*i);
#else
            delete *i;
#endif
        }
        m_workers.clear();
        m_isRunning = false;
    }
    return true;
}


void H323_MultiplexHandler::Close()
{
    Stop();

    m_mutex.Wait();
        DeleteObjectsInMap(m_muxSockets);
        m_socketReadMap.empty();
        m_sendQueue.empty();
    m_mutex.Signal();
}


void H323_MultiplexHandler::SetMultiplexSend(PBoolean enable)
{
    m_muxSend = enable;
}


PBoolean H323_MultiplexHandler::IsMultiplexSend()
{
    return m_muxSend;
}


H323_MultiplexHandler::SocketBundle & H323_MultiplexHandler::GetMultiplexSockets()
{
    return m_muxSockets;
}


H323_MultiplexHandler::SocketMap & H323_MultiplexHandler::GetSocketMap()
{
     return m_socketReadMap;
}


H323_MultiplexHandler::SocketQueue & H323_MultiplexHandler::GetSendQueue()
{
    return m_sendQueue;
}


PMutex & H323_MultiplexHandler::GetSocketMutex()
{
    return m_mutex;
}

PMutex & H323_MultiplexHandler::GetSendMutex()
{
    return m_sendmutex;
}


H323UDPSocket * H323_MultiplexHandler::DetectSourceAddress(H323UDPSocket::Type type, const PIPSocket::Address & addr, WORD port)
{
    PIPSocketAddressAndPort daddr;
    daddr.SetAddress(addr, port);

    H323_MultiplexHandler::SocketMap::iterator it;
    for (it = m_socketReadMap.begin(); it != m_socketReadMap.end(); ++it) {
        H323_MultiplexHandler::SocketBundle::iterator its = it->second.find(type);
        PIPSocketAddressAndPort raddr;
        its->second->GetPeerAddress(raddr);
        if (raddr.AsString() == daddr.AsString())  {
            PTRACE(2, "H46019M\tFIX: " << type << " No MuxID: " << daddr);
            return (H323UDPSocket *)its->second;
        }
    }
    PTRACE(2, "H46019M\tERROR: " << type << " No MuxID: " << daddr);
    return NULL;
}


H323UDPSocket * H323_MultiplexHandler::ResolveSession(unsigned id, H323UDPSocket::Type type, const PIPSocket::Address & addr, WORD port)
{

    H323_MultiplexHandler::SocketMap::iterator it;
    for (it = m_socketReadMap.begin(); it != m_socketReadMap.end(); ++it) {
        H323_MultiplexHandler::SocketBundle::iterator its = it->second.find(type);
        if (its != it->second.end() && ((H46019UDPSocket *)its->second)->GetSendMultiplexID() == id) {
            unsigned wrong_id = its->first;
            PUDPSocket * socket = its->second;
            PTRACE(2, "H46019M\tFIX: s:" << id << " " << type << " Incorrect MuxID: " << wrong_id << " corrected to: " << id);
                Register(id, type, socket);
                Unregister(wrong_id, type);
            return (H323UDPSocket *)socket;
        }
    }
    return DetectSourceAddress(type, addr, port);
}


H323UDPSocket * H323_MultiplexHandler::GetSocket(unsigned id, H323UDPSocket::Type type, const PIPSocket::Address & addr, WORD port)
{
    PWaitAndSignal m(m_mutex);

    SocketMap::iterator it = m_socketReadMap.find(id);
    if (it == m_socketReadMap.end()) {
        PTRACE(2, "H46019M\tERROR: Multiplex Socket: " << type << " Not found:");
        return NULL;
    }

    SocketBundle::iterator its = m_socketReadMap[id].find(type);
    if (its != m_socketReadMap[id].end())
        return (H323UDPSocket *)its->second;
    else
        return ResolveSession(id, type, addr, port);
}


PUDPSocket * & H323_MultiplexHandler::GetMultiplexSocket(H323UDPSocket::Type type)
{
    return ((H323_MultiplexSocket*)m_muxSockets[type])->GetSubSocket();
}


void H323_MultiplexHandler::Register(unsigned id, H323UDPSocket::Type type, PUDPSocket * socket)
{
    PWaitAndSignal m(m_mutex);

    SocketMap::iterator it = m_socketReadMap.find(id);
    if (it == m_socketReadMap.end()) {
        SocketBundle bundle;
        bundle.insert(std::pair<H323UDPSocket::Type, H323UDPSocket *>(type, (H323UDPSocket *)socket));
        m_socketReadMap.insert(std::pair<unsigned, SocketBundle>(id, bundle));
        return;
    }
    m_socketReadMap[id].insert(std::pair<H323UDPSocket::Type, H323UDPSocket *>(type, (H323UDPSocket *)socket));
}


void H323_MultiplexHandler::Unregister(unsigned id, H323UDPSocket::Type type)
{
    PWaitAndSignal m(m_mutex);

    SocketMap::iterator it = m_socketReadMap.find(id);
    if (it == m_socketReadMap.end())
        return;

    SocketBundle::iterator its = m_socketReadMap[id].find(type);
    if (its == m_socketReadMap[id].end())
        return;

    m_socketReadMap[id].erase(its);
    if (m_socketReadMap[id].size() == 0)
        m_socketReadMap.erase(it);
}


bool H323_MultiplexHandler::QueueSendData(H323UDPSocket::Type type, const WriteParams & param)
{

    SocketBundle::iterator its = m_muxSockets.find(type);
    if (its == m_muxSockets.end())
        return false;

    m_sendmutex.Wait();
    m_sendQueue.push(std::pair<H323UDPSocket::Type, WriteParams>(type, param));
    m_sendmutex.Signal();

    return true;
}


H323_MultiplexHandler::ReadParams::ReadParams()
    : m_buffer(NULL), m_length(0), m_addr(0),
    m_port(0), m_lastCount(0), m_timeout(PMaxTimeInterval),
    m_errorCode(PChannel::NoError), m_errorNumber(0)
{

}

#if PTRACING
void H323_MultiplexHandler::ReadParams::Print(PStringStream & strm) const
{
    strm << m_addr << ":" << m_port << " sz " << m_lastCount;  
    if (m_errorCode)
        strm << " errCode:" << m_errorCode;
    if (m_errorNumber)
        strm << " errNo:" << m_errorNumber;
}
#endif

H323_MultiplexHandler::WriteParams::WriteParams()
    : m_buffer(0), m_addr(0), m_port(0)
{

}

H323_MultiplexHandler::PortInfo::PortInfo()
    : m_base(0), m_max(65536), m_current(m_base-1)
{

}


void H323_MultiplexHandler::PortInfo::SetPorts(unsigned base, unsigned max)
{
    m_base = base;
    m_max = max;
    m_current = m_base - 1;
}


unsigned H323_MultiplexHandler::PortInfo::GetNext(unsigned increment)
{
    if (m_current < m_base || m_current > (m_max - increment))
        m_current = m_base-1;  // Make sure the current will be equal to the base

    m_current = m_current + increment;
    return m_current;
}


unsigned H323_MultiplexHandler::PortInfo::GetBase()
{
    return m_base;
}


unsigned H323_MultiplexHandler::PortInfo::GetMax()
{
    return m_max;
}


void H323_MultiplexHandler::SetPorts(unsigned min, unsigned max)
{
    m_portInfo.SetPorts(min, max);
}

unsigned H323_MultiplexHandler::GetMultiplexPort(H323UDPSocket::Type type)
{
    if (type == H323UDPSocket::rtp)
        return m_portInfo.GetBase();
    else if (type == H323UDPSocket::rtcp)
        return m_portInfo.GetBase() + 1;
    else
        return 0;
}

unsigned H323_MultiplexHandler::GetPort(bool max)
{
    if (max)
        return m_portInfo.GetMax();
    else
        return m_portInfo.GetBase();
}

//////////////////////////////////////////////////////////////////////////////////////////


H323_MultiplexSocket::H323_MultiplexSocket(H323UDPSocket::Type type)
    : m_subSocket(NULL)
{

}


H323_MultiplexSocket::~H323_MultiplexSocket()
{
    Close();

    if (m_subSocket)
        delete m_subSocket;
}


PBoolean H323_MultiplexSocket::GetLocalAddress(Address & addr, WORD & port)
{
    return (m_subSocket && m_subSocket->GetLocalAddress(addr, port));
}


PString H323_MultiplexSocket::GetLocalAddress()
{
    if (!m_subSocket)
        return "";

    return m_subSocket->GetLocalAddress();
}


PBoolean H323_MultiplexSocket::ReadFrom(void *buf, PINDEX len, Address & addr, WORD & pt)
{
    PWaitAndSignal m(m_mutex);

    return (m_subSocket && m_subSocket->ReadFrom(buf, len, addr, pt));
}


PBoolean H323_MultiplexSocket::WriteTo(const void *buf, PINDEX len, const Address & addr, WORD pt)
{
    PWaitAndSignal m(m_mutex);

    return (m_subSocket && m_subSocket->WriteTo(buf, len, addr, pt));
}


PBoolean H323_MultiplexSocket::Close()
{
    PWaitAndSignal m(m_mutex);

    return (m_subSocket && ((H323UDPSocket*)m_subSocket)->Close());
}


//////////////////////////////////////////////////////////////////////////////////////////

H323MultiplexConnection::H323MultiplexConnection(H323_MultiplexHandler * handler)
    : m_handler(handler)
{

}

H323MultiplexConnection::~H323MultiplexConnection()
{
    m_mutex.Wait();
        m_sockets.clear();
    m_mutex.Signal();
}

H323MultiplexConnection::SessionInformation::SessionInformation(H323MultiplexConnection * handler, const OpalGloballyUniqueID & id, unsigned crv, const PString & token, unsigned session)
    : m_muxHandler(handler), m_callID(id), m_crv(crv), m_callToken(token), m_sessionID(session), m_recvMultiID(m_muxHandler->GetNextMultiplexID()), m_sendMultiID(0)
{

#ifdef H323_H46024A
    // Some random number bases on the session id (for H.460.24A)
    int rand = PRandom::Number((session * 100), ((session + 1) * 100) - 1);
    m_CUI = PString(rand);
    PTRACE(4, "H46024A\tGenerated CUI s: " << session << " value: " << m_CUI);
#else
    m_CUI = PString();
#endif
}


unsigned H323MultiplexConnection::SessionInformation::GetCallReference()
{
    return m_crv;
}


const PString & H323MultiplexConnection::SessionInformation::GetCallToken()
{
    return m_callToken;
}


unsigned H323MultiplexConnection::SessionInformation::GetSessionID() const
{
    return m_sessionID;
}


void H323MultiplexConnection::SessionInformation::SetSendMultiplexID(unsigned id)
{
    m_sendMultiID = id;
}


unsigned H323MultiplexConnection::SessionInformation::GetRecvMultiplexID() const
{
    return m_recvMultiID;
}


const OpalGloballyUniqueID & H323MultiplexConnection::SessionInformation::GetCallIdentifer()
{
    return m_callID;
}


const PString & H323MultiplexConnection::SessionInformation::GetCUI()
{
    return m_CUI;
}


H323MultiplexConnection * H323MultiplexConnection::SessionInformation::GetMultiplexConnection()
{
    return m_muxHandler;
}


H323_MultiplexHandler * H323MultiplexConnection::GetMultiplexHandler()
{
    return m_handler;
}


H323MultiplexConnection::SocketPairs & H323MultiplexConnection::GetSocketPairs()
{
    return m_sockets;
}


bool H323MultiplexConnection::Start()
{
    return m_handler->Start();
}


H323MultiplexConnection::SocketPair * H323MultiplexConnection::GetSocketPair(unsigned id)
{
    SocketPairs::iterator sockets_iter = m_sockets.find(id);
    if (sockets_iter == m_sockets.end())
        return NULL;
    else
        return &(sockets_iter->second);
}


void H323MultiplexConnection::ClearPairs()
{
    PWaitAndSignal m(m_mutex);

    m_sockets.clear();
}


H323MultiplexConnection::SessionInformation * H323MultiplexConnection::BuildSessionInformation(unsigned sessionID, const H323Connection * con)
{
    return new SessionInformation(this, con->GetCallIdentifier(), con->GetCallReference(), con->GetCallToken(), sessionID);
}


void H323MultiplexConnection::SetSocketSession(unsigned sessionid, unsigned recvMux, PUDPSocket * _rtp, PUDPSocket * _rtcp)
{
    PWaitAndSignal m(m_mutex);

    m_handler->Register(recvMux, H323UDPSocket::rtp, _rtp);
    m_handler->Register(recvMux, H323UDPSocket::rtcp, _rtcp);

    m_sockets.insert(pair<unsigned, SocketPair>(sessionid, SocketPair(_rtp, _rtcp)));
}


void H323MultiplexConnection::RemoveSocketSession(unsigned sessionid)
{
    SocketPairs::iterator sockets_iter = m_sockets.find(sessionid);
    if (sockets_iter != m_sockets.end())
        m_sockets.erase(sockets_iter);
}


PUDPSocket * & H323MultiplexConnection::GetMultiplexSocket(H323UDPSocket::Type type)
{
    return m_handler->GetMultiplexSocket(type);
}


unsigned H323MultiplexConnection::GetMultiplexPort(H323UDPSocket::Type type)
{
    return m_handler->GetMultiplexPort(type);
}


unsigned H323MultiplexConnection::GetMultiplexPort(bool max)
{
    return m_handler->GetPort(max);
}


bool H323MultiplexConnection::WriteTo(H323UDPSocket::Type type, unsigned id, const void * buf, PINDEX len, const PIPSocket::Address & addr, WORD port)
{
    return m_handler->WriteTo(type, id, buf, len, addr, port);
}


unsigned H323MultiplexConnection::GetNextMultiplexID()
{
    if (m_handler)
        return m_handler->NextMultiplexID(1);
    else
        return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////

#define H460_MUX_HEADER_SIZE 4
#define H460_MUX_BASE_PORT  2776


H460_MultiplexHandler::H460_MultiplexHandler()
: H323_MultiplexHandler(H323UDPSocket::rtp)
{
    SetPorts(H460_MUX_BASE_PORT, H460_MUX_BASE_PORT + 10);
    m_multiplexId.SetPorts(PRandom::Number(999900), (unsigned)1000000);
}


H460_MultiplexHandler::~H460_MultiplexHandler()
{
    Close();
}

void H460_MultiplexHandler::SetPorts(unsigned min, unsigned max)
{
    H323_MultiplexHandler::SetPorts(min,max);
}





bool H460_MultiplexHandler::OnDataArrival(H323UDPSocket::Type type, const ReadParams & param)
{
    if (param.m_lastCount <= H460_MUX_HEADER_SIZE) {
        PTRACE(2, "H460MUX\tPacket ignored. Too small.");
        return false;
    }

    PBYTEArray x((BYTE *)param.m_buffer, H460_MUX_HEADER_SIZE);
    unsigned id = *(PUInt32b *)&x[0];

#if PTRACING
    if (PTrace::GetLevel() > 5) {
        PStringStream info;
        info << "In: " << id << " ";
        param.Print(info);
        PTRACE(6, "H460MUX\t" << info);
    }
#endif

    PUDPSocket * socket = GetSocket(id, type, param.m_addr, param.m_port);
    if (!socket)
        return false;
        
    return ((H46019UDPSocket *)socket)->WriteMultiplexBuffer((BYTE *)param.m_buffer + H460_MUX_HEADER_SIZE, param.m_lastCount - H460_MUX_HEADER_SIZE, param.m_addr, param.m_port);

}


bool H460_MultiplexHandler::WriteTo(H323UDPSocket::Type type, unsigned id, const void * buf, PINDEX len, const PIPSocket::Address & addr, WORD port)
{
    WriteParams param;
    if (id > 0) {
        switch (type) {
            case H323UDPSocket::rtp: param.m_buffer = RTP_MultiDataFrame(id, (const BYTE *)buf, len); break;
            case H323UDPSocket::rtcp: param.m_buffer = RTP_MultiControlFrame(id, (const BYTE *)buf, len); break;
            default: return false;
        }
    } else
        param.m_buffer = PBYTEArray((const BYTE *)buf, len);

    param.m_addr = addr;
    param.m_port = port;

    return QueueSendData(type, param);
}


unsigned H460_MultiplexHandler::NextMultiplexID(unsigned userData)
{
    return m_multiplexId.GetNext(userData);
}


#endif // H323_NAT 


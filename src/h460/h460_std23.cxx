/* H460_std23.cxx
 *
 * Copyright (c) 2009 ISVO (Asia) Pte Ltd. All Rights Reserved.
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

#ifdef H323_H46023

#include "h460/h460_std23.h"

#ifdef H323_H46018
  #include <h460/h460_std18.h>
#endif
#ifdef H323_H46019M
  #include <h460/h46018_h225.h>
#endif
#ifdef H323_UPnP
 #include <h460/upnpcp.h>
#endif

#include <h323rtpmux.h>

#if _WIN32
#pragma message("H.460.23/.24 Enabled. Contact consulting@h323plus.org for licensing terms.")
#else
#warning("H.460.23/.24 Enabled. Contact consulting@h323plus.org for licensing terms.")
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4239)
#endif


// H.460.23 NAT Detection Feature
#define remoteNATOID        1       // bool if endpoint has remote NAT support
#define AnnexAOID           2       // bool if endpoint supports H.460.24 Annex A
#define localNATOID         3       // bool if endpoint is NATed
#define NATDetRASOID        4       // Detected RAS H225_TransportAddress 
#define STUNServOID         5       // H225_TransportAddress of STUN Server 
#define NATTypeOID          6       // integer 8 Endpoint NAT Type
#define AnnexBOID           7       // bool if endpoint supports H.460.24 Annex B


// H.460.24 P2Pnat Feature
#define NATProxyOID          1       // PBoolean if gatekeeper will proxy
#define remoteMastOID        2       // PBoolean if remote endpoint can assist local endpoint directly
#define mustProxyOID         3       // PBoolean if gatekeeper must proxy to reach local endpoint
#define calledIsNatOID       4       // PBoolean if local endpoint is behind a NAT/FW
#define NatRemoteTypeOID     5       // integer 8 reported NAT type
#define apparentSourceOID    6       // H225_TransportAddress of apparent source address of endpoint
#define SupAnnexAOID         7       // PBoolean if local endpoint supports H.460.24 Annex A
#define NATInstOID           8       // integer 8 Instruction on how NAT is to be Traversed
#define SupAnnexBOID         9       // bool if endpoint supports H.460.24 Annex B


//////////////////////////////////////////////////////////////////////

#if PTLIB_VER >= 2130
PCREATE_NAT_PLUGIN(H46024, "H.460.24");
#else
PCREATE_NAT_PLUGIN(H46024);
#endif

PNatMethod_H46024::PNatMethod_H46024()
: mainThread(NULL)
{
    natType = PSTUNClient::UnknownNat;
    isAvailable = false;
    isActive = false;
    feat = NULL;
    locBindAddress = PIPSocket::GetDefaultIpAny();
}

PNatMethod_H46024::~PNatMethod_H46024()
{
    natType = PSTUNClient::UnknownNat;
    delete mainThread;
}

void PNatMethod_H46024::SetPortInformation(PortInfo & pairedPortInfo, WORD portPairBase, WORD portPairMax)
{
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

}

void PNatMethod_H46024::Start(const PString & server,H460_FeatureStd23 * _feat)
{
    feat = _feat;

   H323EndPoint * ep = feat->GetEndPoint();

   SetServer(server);
#if PTLIB_VER >= 2120
   locBindAddress = PIPSocket::GetRouteInterfaceAddress(m_serverAddress.GetAddress());
#endif

#ifdef H323_H46019M
    WORD muxBase = ep->GetMultiplexPort();
    SetPortInformation(multiplexPorts,muxBase-2, muxBase+2);
    SetPortInformation(standardPorts, ep->GetRtpIpPortBase(), ep->GetRtpIpPortMax());
    SetPortRanges(muxBase-2, muxBase+2, muxBase-2, muxBase+20);
#else
    SetPortRanges(ep->GetRtpIpPortBase(), ep->GetRtpIpPortMax(), ep->GetRtpIpPortBase(), ep->GetRtpIpPortMax());
#endif

    mainThread  = PThread::Create(PCREATE_NOTIFIER(MainMethod), 0,  
                       PThread::NoAutoDeleteThread, PThread::NormalPriority, "H.460.24");
}



PSTUNClient::NatTypes PNatMethod_H46024::NATTest()
{

    PSTUNClient::NatTypes testtype;
    WORD testport;
#ifdef H323_H46019M
    testport = (WORD)feat->GetEndPoint()->GetMultiplexPort()-1;
#else
    PRandom rand;
    testport = (WORD)rand.Generate(singlePortInfo.basePort , singlePortInfo.maxPort);   
#endif

    singlePortInfo.currentPort = testport;
    PTRACE(4,"H46023\tSTUN Test Port " << singlePortInfo.currentPort);

    testtype = GetNatType(true);

#ifdef H323_H46019M
    // if we have a cone NAT check the RTCP Port to see if not existing binding
    if (testtype == PSTUNClient::ConeNat || testtype == PSTUNClient::UnknownNat) {
        PThread::Sleep(10);
        PTRACE(4,"H46023\tCone NAT Detected rechecking. Test Port " << singlePortInfo.currentPort+1);
        PSTUNClient::NatTypes test2 = GetNatType(true);
        if (test2 > testtype)
            testtype = test2;
    }
#endif

    return testtype;
}

int recheckTime = 300000;    // 5 minutes

void PNatMethod_H46024::MainMethod(PThread &,  H323_INT)
{

    while (natType == PSTUNClient::UnknownNat ||
                natType == PSTUNClient::ConeNat) {
        PSTUNClient::NatTypes testtype = NATTest();
        if (natType != testtype) {
            natType = testtype;
            PIPSocket::Address extIP;
            if (GetExternalAddress(extIP)) {
                feat->GetEndPoint()->NATMethodCallBack(GetName(),2,natType);
                feat->OnNATTypeDetection(natType, extIP);
            }
        }

        if (natType == PSTUNClient::ConeNat) {
            isAvailable = true;
            PThread::Sleep(recheckTime);
            continue;
        }

        isAvailable = false;
        if (natType == PSTUNClient::UnknownNat) {
            PTRACE(1,"H46024\tNAT Test failed to resolve NAT Type");
            break;
        }
    }

}

bool PNatMethod_H46024::IsAvailable(const PIPSocket::Address & /*binding*/)
{
    if (!isActive)
        return false;

    return isAvailable;
}

void PNatMethod_H46024::SetAvailable() 
{ 
    feat->GetEndPoint()->NATMethodCallBack(GetName(),1,"Available");
    isAvailable = true; 
}

void PNatMethod_H46024::Activate(bool act)
{
    if (act && !isAvailable)  // Special case where activated but not available.
       isAvailable = true;  

    isActive = act;
}

PSTUNClient::NatTypes PNatMethod_H46024::GetNATType()
{
    return natType;
}

WORD PNatMethod_H46024::CreateRandomPortPair(unsigned int start, unsigned int end)
{
    WORD num;
    PRandom rand;
    num = (WORD)rand.Generate(start,end);
    if (num %2 != 0) 
        num++;  // Make sure the number is even

    return num;
}


PBoolean PNatMethod_H46024::CreateSocketPair(PUDPSocket * & socket1,
                                             PUDPSocket * & socket2,
                                             const PIPSocket::Address & binding,
#if PTLIB_VER >= 2130
                                             PObject * userData
#else
                                             void * userData
#endif

                                             )
{
    PWaitAndSignal m(portMute);

#ifdef H323_H46019M
    H323MultiplexConnection::SessionInformation * info = (H323MultiplexConnection::SessionInformation *)userData;
    if (info->GetRecvMultiplexID() > 0) {

        PNatMethod_H46019 * handler =
            (PNatMethod_H46019 *)feat->GetEndPoint()->GetNatMethods().GetMethodByName("H46019");

        H323MultiplexConnection * muxhandler = info->GetMultiplexConnection();

        if (muxhandler->GetMultiplexSocket(H323UDPSocket::rtp) == NULL) {
            PUDPSocket * & rtp = muxhandler->GetMultiplexSocket(H323UDPSocket::rtp);
            PUDPSocket * & rtcp = muxhandler->GetMultiplexSocket(H323UDPSocket::rtcp);

            pairedPortInfo.currentPort = muxhandler->GetMultiplexPort(H323UDPSocket::rtp)-1;

#if PTLIB_VER >= 2130
            if (!PSTUNClient::CreateSocketPair(rtp, rtcp, binding, NULL))
#else
            if (!PSTUNClient::CreateSocketPair(rtp, rtcp, binding))
#endif
                return false;

            PIPSocket::Address stunAddress;
            rtp->GetLocalAddress(stunAddress);
            PTRACE(1, "H46024\tMux STUN Created: " << stunAddress << " "
                << rtp->GetPort() << '-' << rtcp->GetPort());

            muxhandler->Start();  // Start Multiplexing Listening thread;
        }

        socket1 = new H46019UDPSocket(*handler->GetHandler(), info, true);      /// Data 
        socket2 = new H46019UDPSocket(*handler->GetHandler(), info, false);     /// Signal

        muxhandler->SetSocketSession(info->GetSessionID(), info->GetRecvMultiplexID(), socket1, socket2);

    } else {
        // Set standard ports here
        SetPortRanges(standardPorts.basePort, standardPorts.maxPort, standardPorts.basePort, standardPorts.maxPort);
#else
    {
#endif

#if PTLIB_VER >= 2130
        if (!PSTUNClient::CreateSocketPair(socket1,socket2,binding, NULL))
#else
        if (!PSTUNClient::CreateSocketPair(socket1,socket2,binding))
#endif
             return false;
    }

    return true;
}


#if PTLIB_VER >= 2120
void PNatMethod_H46024::InternalUpdate()
{
    m_interface = locBindAddress;
    PSTUNClient::InternalUpdate();
}


#define PTLIB_CLASSIC_STUN_FIX(num) \
    BYTE id##num = (BYTE)PRandom::Number(); \
    PSTUNMessage request##num(PSTUNMessage::BindingRequest, &id##num);


PNatMethod::NatTypes PNatMethod_H46024::DoRFC3489Discovery(
    PSTUNUDPSocket * socket,
    const PIPSocketAddressAndPort & serverAddress,
    PIPSocketAddressAndPort & baseAddressAndPort,
    PIPSocketAddressAndPort & externalAddressAndPort
    )
{
    socket->SetReadTimeout(replyTimeout);

    socket->GetLocalAddress(baseAddressAndPort);
    socket->PUDPSocket::InternalSetSendAddress(serverAddress);

    // RFC3489 discovery

    /* test I - the client sends a STUN Binding Request to a server, without
    any flags set in the CHANGE-REQUEST attribute, and without the
    RESPONSE-ADDRESS attribute. This causes the server to send the response
    back to the address and port that the request came from. */

PTLIB_CLASSIC_STUN_FIX(I)  ///<--- PTLIB FIX HERE
//    PSTUNMessage requestI(PSTUNMessage::BindingRequest);
    requestI.AddAttribute(PSTUNChangeRequest(false, false));

    PSTUNMessage responseI;
    if (!responseI.Poll(*socket, requestI, m_pollRetries)) {
        PTRACE(2, "STUN\tSTUN server " << serverAddress << " did not respond.");
        return PNatMethod::UnknownNat;
    }

    return FinishRFC3489Discovery(responseI, socket, externalAddressAndPort);
}

PNatMethod::NatTypes PNatMethod_H46024::FinishRFC3489Discovery(
    PSTUNMessage & responseI,
    PSTUNUDPSocket * socket,
    PIPSocketAddressAndPort & externalAddressAndPort
    )
{
    // check if server returned "420 Unknown Attribute" - that probably means it cannot do CHANGE_REQUEST even with no changes
    bool canDoChangeRequest = true;

    PSTUNErrorCode * errorAttribute = (PSTUNErrorCode *)responseI.FindAttribute(PSTUNAttribute::ERROR_CODE);
    if (errorAttribute != NULL) {
        bool ok = false;
        if (errorAttribute->GetErrorCode() == 420) {
            // try again without CHANGE request
            PSTUNMessage request(PSTUNMessage::BindingRequest);
            ok = responseI.Poll(*socket, request, m_pollRetries);
            if (ok) {
                errorAttribute = (PSTUNErrorCode *)responseI.FindAttribute(PSTUNAttribute::ERROR_CODE);
                ok = errorAttribute == NULL;
                canDoChangeRequest = false;
            }
        }
        if (!ok) {
            PTRACE(2, "STUN\tSTUN server " << socket->GetSendAddress() << " returned unexpected error " << errorAttribute->GetErrorCode() << ", reason = '" << errorAttribute->GetReason() << "'");
            return PNatMethod::BlockedNat;
        }
    }

    PSTUNAddressAttribute * mappedAddress = (PSTUNAddressAttribute *)responseI.FindAttribute(PSTUNAttribute::XOR_MAPPED_ADDRESS);
    if (mappedAddress == NULL) {
        mappedAddress = (PSTUNAddressAttribute *)responseI.FindAttribute(PSTUNAttribute::MAPPED_ADDRESS);
        if (mappedAddress == NULL) {
            PTRACE(2, "STUN\tExpected (XOR)mapped address attribute from " << m_serverAddress);
            return PNatMethod::UnknownNat; // Protocol error
        }
    }

    mappedAddress->GetIPAndPort(externalAddressAndPort);

    bool notNAT = (socket->GetPort() == externalAddressAndPort.GetPort()) && PIPSocket::IsLocalHost(externalAddressAndPort.GetAddress());

    // can only guess based on a single sample
    if (!canDoChangeRequest) {
        PNatMethod::NatTypes natType = notNAT ? PNatMethod::OpenNat : PNatMethod::SymmetricNat;
        PTRACE(3, "STUN\tSTUN server has only one address - best guess is that NAT is " << PNatMethod::GetNatTypeString(natType));
        return natType;
    }

    PTRACE(3, "STUN\tTest I response received - sending test II (change port and address)");

    /* Test II - the client sends a Binding Request with both the "change IP"
    and "change port" flags from the CHANGE-REQUEST attribute set. */

PTLIB_CLASSIC_STUN_FIX(II)  ///<--- PTLIB FIX HERE
//    PSTUNMessage requestII(PSTUNMessage::BindingRequest);

    requestII.AddAttribute(PSTUNChangeRequest(true, true));
    PSTUNMessage responseII;
    bool testII = responseII.Poll(*socket, requestII, m_pollRetries);

    PTRACE(3, "STUN\tTest II response " << (testII ? "" : "not ") << "received");

    if (notNAT) {
        PNatMethod::NatTypes natType = (testII ? PNatMethod::OpenNat : PNatMethod::PartiallyBlocked);
        // Is not NAT or symmetric firewall
        PTRACE(2, "STUN\tTest I and II indicate nat is " << PNatMethod::GetNatTypeString(natType));
        return natType;
    }

    if (testII)
        return PNatMethod::ConeNat;

    PSTUNAddressAttribute * changedAddress = (PSTUNAddressAttribute *)responseI.FindAttribute(PSTUNAttribute::CHANGED_ADDRESS);
    if (changedAddress == NULL) {
        changedAddress = (PSTUNAddressAttribute *)responseI.FindAttribute(PSTUNAttribute::OTHER_ADDRESS);
        if (changedAddress == NULL) {
            PTRACE(3, "STUN\tTest II response indicates no alternate address in use - testing finished");
            return PNatMethod::UnknownNat; // Protocol error
        }
    }

    PTRACE(3, "STUN\tSending test I to alternate server");

    // Send test I to another server, to see if restricted or symmetric
    PIPSocket::Address secondaryServer = changedAddress->GetIP();
    WORD secondaryPort = changedAddress->GetPort();
    socket->PUDPSocket::InternalSetSendAddress(PIPSocketAddressAndPort(secondaryServer, secondaryPort));

PTLIB_CLASSIC_STUN_FIX(I2)  ///<--- PTLIB FIX HERE
//    PSTUNMessage requestII(PSTUNMessage::BindingRequest);
    requestI2.AddAttribute(PSTUNChangeRequest(false, false));

    PSTUNMessage responseI2;
    if (!responseI2.Poll(*socket, requestI2, m_pollRetries)) {
        PTRACE(3, "STUN\tPoll of secondary server " << secondaryServer << ':' << secondaryPort
            << " failed, NAT partially blocked by firewall rules.");
        return PNatMethod::PartiallyBlocked;
    }

    mappedAddress = (PSTUNAddressAttribute *)responseI2.FindAttribute(PSTUNAttribute::XOR_MAPPED_ADDRESS);
    if (mappedAddress == NULL) {
        mappedAddress = (PSTUNAddressAttribute *)responseI2.FindAttribute(PSTUNAttribute::MAPPED_ADDRESS);
        if (mappedAddress == NULL) {
            PTRACE(2, "STUN\tExpected (XOR)mapped address attribute from " << m_serverAddress);
            return PNatMethod::UnknownNat; // Protocol error
        }
    }

    {
        PIPSocketAddressAndPort ipAndPort;
        mappedAddress->GetIPAndPort(ipAndPort);
        if (ipAndPort != externalAddressAndPort)
            return PNatMethod::SymmetricNat;
    }

    socket->PUDPSocket::InternalSetSendAddress(m_serverAddress);

PTLIB_CLASSIC_STUN_FIX(III)  ///<--- PTLIB FIX HERE
//    PSTUNMessage requestIII(PSTUNMessage::BindingRequest);
    requestIII.SetAttribute(PSTUNChangeRequest(false, true));
    PSTUNMessage responseIII;

    return responseIII.Poll(*socket, requestIII, m_pollRetries) ? PNatMethod::RestrictedNat : PNatMethod::PortRestrictedNat;
}

#endif

//////////////////////////////////////////////////////////////////////

H460_FEATURE(Std23);

H460_FeatureStd23::H460_FeatureStd23()
: H460_FeatureStd(23)
{
  PTRACE(6,"H46023\tInstance Created");

  FeatureCategory = FeatureSupported;

  EP = NULL;
  alg = false;
  isavailable = true;
  isEnabled = false;
  natType = PSTUNClient::UnknownNat;
  externalIP = PIPSocket::GetDefaultIpAny();
  useAlternate = 0;
  natNotify = false;

}

H460_FeatureStd23::~H460_FeatureStd23()
{
}

void H460_FeatureStd23::AttachEndPoint(H323EndPoint * _ep)
{
    EP = _ep;
    isEnabled = EP->H46023IsEnabled();

    // Ignore if already manually using STUN
    isavailable = (EP->GetSTUN() == NULL);
}

PBoolean H460_FeatureStd23::OnSendGatekeeperRequest(H225_FeatureDescriptor & pdu) 
{ 
    if (!isEnabled)
        return false;

    if (!isavailable)
        return FALSE;

    H460_FeatureStd feat = H460_FeatureStd(23); 
    pdu = feat;
    return TRUE; 
}

PBoolean H460_FeatureStd23::OnSendRegistrationRequest(H225_FeatureDescriptor & pdu) 
{ 
    if (!isEnabled)
        return false;

    if (!isavailable)
            return FALSE;

    // Build Message
    H460_FeatureStd feat = H460_FeatureStd(23); 

    if ((EP->GetGatekeeper() == NULL) ||
        (!EP->GetGatekeeper()->IsRegistered())) {
          // First time registering
          // We always support remote
            feat.Add(remoteNATOID,H460_FeatureContent(true));
#ifdef H323_H46024A
            feat.Add(AnnexAOID,H460_FeatureContent(true));
#endif
#ifdef H323_H46024B
            feat.Add(AnnexBOID,H460_FeatureContent(true));
#endif
    } else {
        if (alg) {
              // We should be disabling H.460.23/.24 support but 
              // we will disable H.460.18/.19 instead :) and say we have no NAT..
                feat.Add(NATTypeOID,H460_FeatureContent(1,8)); 
                feat.Add(remoteNATOID,H460_FeatureContent(false)); 
                isavailable = false;
                alg = false;
        } else {
            if (natNotify || AlternateNATMethod()) {
                feat.Add(NATTypeOID,H460_FeatureContent(natType,8)); 
                natNotify = false;
            }
        }
    }

    pdu = feat;

    return true; 
}

void H460_FeatureStd23::OnReceiveGatekeeperConfirm(const H225_FeatureDescriptor & /*pdu*/) 
{
    isEnabled = true;
}

void H460_FeatureStd23::OnReceiveRegistrationConfirm(const H225_FeatureDescriptor & pdu)
{
    isEnabled = true;

   H460_FeatureStd & feat = (H460_FeatureStd &)pdu;

   // Ignore whether the gatekeeper detected as being behind NAT
   // The STUN test will confirm - SH
   //PBoolean NATdetect = false;
   //if (feat.Contains(localNATOID))
   //    NATdetect = feat.Value(localNATOID);

       if (feat.Contains(STUNServOID)) {
          H323TransportAddress addr = feat.Value(STUNServOID);
          StartSTUNTest(addr.Mid(3));
       }

       if (feat.Contains(NATDetRASOID)) {
           H323TransportAddress addr = feat.Value(NATDetRASOID);
           PIPSocket::Address ip;
           addr.GetIpAddress(ip);
           if (DetectALG(ip)) {
             // if we have an ALG then to be on the safe side
             // disable .23/.24 so that the ALG has more chance
             // of behaving properly...
               alg = true;
               DelayedReRegistration();
           }
       }
}


void H460_FeatureStd23::OnNATTypeDetection(PSTUNClient::NatTypes type, const PIPSocket::Address & ExtIP)
{
    if (natType == type)
        return;

    externalIP = ExtIP;

    if (natType == PSTUNClient::UnknownNat) {
        PTRACE(4,"H46023\tSTUN Test Result: " << type << " forcing reregistration.");
#ifdef H323_UPnP
        if (type > PSTUNClient::ConeNat) {
           PString name = PString();
           if (IsAlternateAvailable(name))
               EP->NATMethodCallBack(name,1,"Available");
           else
               EP->InitialiseUPnP();
        }
#endif
        natType = type;  // first time detection
    } else {
        PTRACE(2,"H46023\tBAD NAT Detected: Was " << natType << " Now " << type << " Disabling H.460.23/.24");
        natType = PSTUNClient::UnknownNat;  // Leopard changed it spots (disable H.460.23/.24)
    }
    
    natNotify = true;
    EP->ForceGatekeeperReRegistration();
}

bool H460_FeatureStd23::DetectALG(const PIPSocket::Address & detectAddress)
{

#ifdef H323_IPV6
  // Again horrible code should be able to get interface listing for a given protocol - SH
  PBoolean ipv6IPv4Discover = false;
  if (detectAddress.GetVersion() == 4 && PIPSocket::GetDefaultIpAddressFamily() == AF_INET6) {
      PIPSocket::SetDefaultIpAddressFamilyV4();
      ipv6IPv4Discover = true;
  }
#endif
    bool found = true;
    PIPSocket::InterfaceTable if_table;
    if (!PIPSocket::GetInterfaceTable(if_table)) {
        PTRACE(1, "H46023\tERROR: Can't get interface table");
        found = false;
    } else {  
        for (PINDEX i=0; i< if_table.GetSize(); i++) {
            if (detectAddress == if_table[i].GetAddress()) {
                PTRACE(4, "H46023\tNo Intermediary device detected between EP and GK");
                found = false;
                break;
            }
        }
    }
#ifdef H323_IPV6
  if (ipv6IPv4Discover)
      PIPSocket::SetDefaultIpAddressFamilyV6();
#endif
    if (found) {
        PTRACE(4, "H46023\tWARNING: Intermediary device detected!");
        EP->NATMethodCallBack("ALG",1,"Available");
        return true;
    }

    return false;
}



void H460_FeatureStd23::StartSTUNTest(const PString & server)
{
    PString s;
#ifdef H323_DNS
    PStringList SRVs;
    PStringList x = server.Tokenise(":");
    PString number = "h323:user@" + x[0];
    if (PDNS::LookupSRV(number,"_stun._udp.",SRVs))
        s = SRVs[0];
    else
#endif
        s = server;

    // Remove any previous NAT methods.
    EP->GetNatMethods().RemoveMethod("H46024");
    natType = PSTUNClient::UnknownNat;

    PNatMethod_H46024 * xnat = (PNatMethod_H46024 *)EP->GetNatMethods().LoadNatMethod("H46024");
#ifdef H323_UPnP
    PString name = PString();
    if (IsAlternateAvailable(name)) {
          EP->NATMethodCallBack(name,1,"Available");
          EP->ForceGatekeeperReRegistration();
    } else
#endif
        xnat->Start(s,this);

    EP->GetNatMethods().AddMethod(xnat);
}

bool H460_FeatureStd23::IsAvailable()
{
    return isEnabled;
}

#if H323_UPnP
bool H460_FeatureStd23::IsAlternateAvailable(PString & name)
{
    PNatMethod_UPnP * upnpMethod = (PNatMethod_UPnP *)EP->GetNatMethods().GetMethodByName("UPnP");
    if (upnpMethod && upnpMethod->IsAvailable(PIPSocket::Address::GetAny(4))) {
          PTRACE(4,"H46023\tSTUN Setting alternate: UPnP");
          name = upnpMethod->GetName();
          natType = PSTUNClient::ConeNat;
          natNotify = true;
          useAlternate = 1;
          upnpMethod->Activate(true);
          return true;
    }
    return false;
}
#endif

void H460_FeatureStd23::DelayedReRegistration()
{
    PThread::Sleep(1000);
    EP->ForceGatekeeperReRegistration();  // We have an ALG so notify the gatekeeper   
}

bool H460_FeatureStd23::AlternateNATMethod()
{
#ifdef H323_UPnP
    if (natType <= PSTUNClient::ConeNat || useAlternate > 0)
        return false;

    H323NatList & natlist = EP->GetNatMethods().GetNATList();

    for (PINDEX i=0; i< natlist.GetSize(); i++) {
#if PTLIB_VER >= 2130
        PString methName = natlist[i].GetMethodName();
#else
        PString methName = natlist[i].GetName();
#endif
        if (methName == "UPnP" && 
            natlist[i].GetRTPSupport() == PSTUNClient::RTPSupported) {
            PIPSocket::Address extIP;
            natlist[i].GetExternalAddress(extIP);
            if (extIP.IsAny() || !extIP.IsValid() || externalIP.IsLoopback() || extIP == externalIP) {
                PTRACE(4,"H46023\tUPnP Change NAT from " << natType << " to " << PSTUNClient::ConeNat);
                natType = PSTUNClient::ConeNat;
                useAlternate = 1;
                natlist[i].Activate(true);
                EP->NATMethodCallBack(methName,1,"Available");
                return true;
            } else {
                PTRACE(4,"H46023\tUPnP Unavailable subNAT STUN: " << externalIP << " UPnP " << extIP);
                useAlternate = 2;
            }
        }
    }
#endif
    return false;
}

bool H460_FeatureStd23::UseAlternate()
{
    return (useAlternate == 1);
}

bool H460_FeatureStd23::IsUDPAvailable()
{
    return (natType < PSTUNClient::BlockedNat);
}

///////////////////////////////////////////////////////////////////


H460_FEATURE(Std24);

H460_FeatureStd24::H460_FeatureStd24()
: H460_FeatureStd(24),
  EP(NULL), CON(NULL), natconfig(H460_FeatureStd24::e_unknown), 
  nattype(0), isEnabled(false), useAlternate(false)
{
 PTRACE(6,"H46024\tInstance Created");

 FeatureCategory = FeatureSupported;
}

H460_FeatureStd24::~H460_FeatureStd24()
{
}

void H460_FeatureStd24::AttachEndPoint(H323EndPoint * _ep)
{
   EP = _ep; 
    // We only enable IF the gatekeeper supports H.460.23
    H460_FeatureSet * gkfeat = EP->GetGatekeeperFeatures();
    if (gkfeat && gkfeat->HasFeature(23)) {
        H460_FeatureStd23 * feat = (H460_FeatureStd23 *)gkfeat->GetFeature(23);
        isEnabled = feat->IsAvailable();
        useAlternate = feat->UseAlternate();
    } else {
        PTRACE(4,"H46024\tH.460.24 disabled as H.460.23 is disabled!");
        isEnabled = false;
    }
}

void H460_FeatureStd24::AttachConnection(H323Connection * _conn)
{
   CON = _conn; 
}

PBoolean H460_FeatureStd24::OnSendAdmissionRequest(H225_FeatureDescriptor & pdu) 
{ 
    // Ignore if already not enabled or manually using STUN
    if (!isEnabled) 
        return FALSE;

#ifdef H323_H46023
    if (!EP->H46023NatMethodSelection(GetFeatureName()[0]))
        return false;
#endif

    PWaitAndSignal m(h460mute);

    // Build Message
    PTRACE(6,"H46024\tSending ARQ ");
    H460_FeatureStd feat = H460_FeatureStd(24);

    if (natconfig != e_unknown) {
       feat.Add(NATInstOID,H460_FeatureContent((unsigned)natconfig,8));
    }

    pdu = feat;
    return TRUE;  
}

void H460_FeatureStd24::OnReceiveAdmissionConfirm(const H225_FeatureDescriptor & pdu)
{
     H460_FeatureStd & feat = (H460_FeatureStd &)pdu;

    if (feat.Contains(NATInstOID)) {
        PTRACE(6,"H46024\tReading ACF");
        unsigned NATinst = feat.Value(NATInstOID); 
        natconfig = (NatInstruct)NATinst;
        HandleNATInstruction(natconfig);
    }
}

void H460_FeatureStd24::OnReceiveAdmissionReject(const H225_FeatureDescriptor & pdu) 
{
     PTRACE(6,"H46024\tARJ Received");
     HandleNATInstruction(H460_FeatureStd24::e_natFailure);
}

PBoolean H460_FeatureStd24::OnSendSetup_UUIE(H225_FeatureDescriptor & pdu)
{ 
  // Ignore if already not enabled or manually using STUN
  if (!isEnabled) 
        return FALSE;

 PTRACE(6,"H46024\tSend Setup");
    if (natconfig == H460_FeatureStd24::e_unknown)
        return FALSE;

    H460_FeatureStd feat = H460_FeatureStd(24);
    
    int remoteconfig;
    switch (natconfig) {
        case H460_FeatureStd24::e_noassist:
            remoteconfig = H460_FeatureStd24::e_noassist;
            break;
        case H460_FeatureStd24::e_localMaster:
            remoteconfig = H460_FeatureStd24::e_remoteMaster;
            break;
        case H460_FeatureStd24::e_remoteMaster:
            remoteconfig = H460_FeatureStd24::e_localMaster;
            break;
        case H460_FeatureStd24::e_localProxy:
            remoteconfig = H460_FeatureStd24::e_remoteProxy;
            break;
        case H460_FeatureStd24::e_remoteProxy:
            remoteconfig = H460_FeatureStd24::e_localProxy;
            break;
        default:
            remoteconfig = natconfig;
    }

    feat.Add(NATInstOID,H460_FeatureContent(remoteconfig,8));
    pdu = feat;
    return TRUE; 
}
    
void H460_FeatureStd24::OnReceiveSetup_UUIE(const H225_FeatureDescriptor & pdu)
{

    PWaitAndSignal m(h460mute);

     H460_FeatureStd & feat = (H460_FeatureStd &)pdu;

     if (feat.Contains(NATInstOID)) {
      PTRACE(6,"H46024\tReceive Setup");

        unsigned NATinst = feat.Value(NATInstOID); 
        natconfig = (NatInstruct)NATinst;
        HandleNATInstruction(natconfig);
    }

}

void H460_FeatureStd24::HandleNATInstruction(NatInstruct _config)
{
        
        PTRACE(4,"H46024\tNAT Instruction Received: " << _config);
        switch (_config) {
            case H460_FeatureStd24::e_localMaster:
                PTRACE(4,"H46024\tLocal NAT Support: H.460.24 ENABLED");
                CON->SetRemoteNAT(true);
#ifdef H323_H46019M
                CON->H46019SetOffload();
#endif
                SetNATMethods(e_enable);
                break;

            case H460_FeatureStd24::e_remoteMaster:
                PTRACE(4,"H46024\tRemote NAT Support: ALL NAT DISABLED");
#ifdef H323_H46019M
                CON->H46019SetOffload();
#endif
                if (IsNatSendAvailable()) {  // If we can use STUN do it!
                    CON->SetRemoteNAT(false);
                    SetNATMethods(e_enable);
                } else
                    SetNATMethods(e_disable);
                break;

            case H460_FeatureStd24::e_remoteProxy:
                PTRACE(4,"H46024\tRemote Proxy Support: H.460.24 DISABLED");
                SetNATMethods(e_default);
                break;

            case H460_FeatureStd24::e_localProxy:
                PTRACE(4,"H46024\tCall Local Proxy: H.460.24 DISABLED");
                SetNATMethods(e_default);
                break;

#ifdef H323_H46024A
            case H460_FeatureStd24::e_natAnnexA:
                PTRACE(4,"H46024\tSame NAT: H.460.24 AnnexA ENABLED");
                CON->H46024AEnabled();
                SetNATMethods(e_AnnexA);
                break;
#endif

#ifdef H323_H46024B
            case H460_FeatureStd24::e_natAnnexB:
                PTRACE(4,"H46024\tSame NAT: H.460.24 AnnexA ENABLED");
                CON->H46024BEnabled();
                //CON->H46024AEnabled();  // Might be on same internal network
                SetNATMethods(e_AnnexB);
                break;
#endif

            case H460_FeatureStd24::e_natFailure:
                PTRACE(4,"H46024\tCall Failure Detected");
                EP->FeatureCallBack(GetFeatureName()[0],1,"Call Failure");
                break;
            case H460_FeatureStd24::e_noassist:
                PTRACE(4,"H46024\tNAT Call direct");
            default:
                PTRACE(4,"H46024\tNo Assist: H.460.24 DISABLED.");
                CON->DisableNATSupport();
                SetNATMethods(e_default);
                break;
        }
}

void H460_FeatureStd24::SetH46019State(bool state)
{
#ifdef H323_H46018
    if (CON->GetFeatureSet()->HasFeature(19)) {
        H460_Feature * feat = CON->GetFeatureSet()->GetFeature(19);

        PTRACE(4,"H46024\t" << (state ? "En" : "Dis") << "abling H.460.19 support for call");
        ((H460_FeatureStd19 *)feat)->SetAvailable(state);
    }
#endif
}

PBoolean H460_FeatureStd24::IsNatSendAvailable()
{
   H323NatList & natlist = EP->GetNatMethods().GetNATList();

   PBoolean available = false;
   PINDEX i=0;
   for (i=0; i< natlist.GetSize(); i++) {
#if PTLIB_VER >= 2130
        if (natlist[i].GetMethodName() == "H46024") break;
#else
        if (natlist[i].GetName() == "H46024") break;
#endif
   }
   if (i < natlist.GetSize()) {
     PNatMethod_H46024 & meth = (PNatMethod_H46024 &)natlist[i];
      switch (meth.GetNatType(false)) {
        case PSTUNClient::ConeNat :
        case PSTUNClient::RestrictedNat :
        case PSTUNClient::PortRestrictedNat :
          available = true;
          break;
        case PSTUNClient::SymmetricNat :
        default :   // UnknownNet, SymmetricFirewall, BlockedNat
          break;
      }
   }
   return available;
}

void H460_FeatureStd24::SetNATMethods(H46024NAT state)
{

    H323NatList & natlist = EP->GetNatMethods().GetNATList();

    for (PINDEX i=0; i< natlist.GetSize(); i++) {
#if PTLIB_VER >= 2130
        PString name = natlist[i].GetMethodName();
#else
        PString name = natlist[i].GetName();
#endif
        switch (state) {
            case H460_FeatureStd24::e_AnnexA:   // To do Annex A Implementation.
            case H460_FeatureStd24::e_AnnexB:   // To do Annex B Implementation.
            case H460_FeatureStd24::e_default:
                if (name == "H46024" || name == "UPnP")
                    natlist[i].Activate(false);
                break;
            case H460_FeatureStd24::e_enable:
                if (name == "H46024" && !useAlternate)
                    natlist[i].Activate(true);
                else if (name == "UPnP" && useAlternate)
                    natlist[i].Activate(true);
                else
                    natlist[i].Activate(false);
                break;
            case H460_FeatureStd24::e_disable:
#ifdef H323_H46019M
                if (name == "H46019" && CON->IsH46019Multiplexed())
                    natlist[i].Activate(true);
                else
#endif
                    natlist[i].Activate(false);
                break;
            default:
                break;
        }
    }

   //PTRACE(6,"H46024\tNAT Methods " << GetH460NATString(state));
   for (PINDEX i=0; i< natlist.GetSize(); i++) {
#if PTLIB_VER >= 2130
       PString name = natlist[i].GetMethodName();
#else
       PString name = natlist[i].GetName();
#endif
       PTRACE(6, "H323\tNAT Method " << i << " " << name << " Ready: "
                  << (natlist[i].IsAvailable(PIPSocket::Address::GetAny(4)) ? "Yes" : "No"));  
   }
}

PString H460_FeatureStd24::GetNATStrategyString(NatInstruct method)
{
  static const char * const Names[10] = {
        "Unknown Strategy",
        "No Assistance",
        "Local Master",
        "Remote Master",
        "Local Proxy",
        "Remote Proxy",
        "Full Proxy",
        "AnnexA SameNAT",
        "AnnexB NAToffload",
        "Call Failure!"
  };

  if (method < 10)
    return Names[method];

   return psprintf("<NAT Strategy %u>", method);
}

PString H460_FeatureStd24::GetH460NATString(H46024NAT method)
{
    static const char * const Names[5] = {
        "default"
        "enable"
        "AnnexA"
        "AnnexB"
        "disable"
    };

  if (method < 5)
    return Names[method];

   return psprintf("<H460NAT %u>", method);
}



#ifdef _MSC_VER
#pragma warning(default : 4239)
#endif

#endif // Win32_WCE

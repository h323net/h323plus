/* H460_std23.h
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

#ifndef H_H460_Featurestd23
#define H_H460_Featurestd23


//#include <h460/h4601.h>
#include <ptclib/pstun.h>

#if _MSC_VER
#pragma once
#endif


class H323EndPoint;
class H460_FeatureStd23;
class PNatMethod_H46024 : public PSTUNClient
{

    PCLASSINFO(PNatMethod_H46024, PSTUNClient);

public:
    PNatMethod_H46024();

    ~PNatMethod_H46024();
   
#if PTLIB_VER >= 2130
    struct PortInfo {
        PortInfo(WORD base = 0, WORD max = 0)
            : basePort(base), maxPort(max == 0 ? base : max), currentPort(base) {}
        PMutex mutex;
        WORD   basePort;
        WORD   maxPort;
        WORD   currentPort;
    };
#endif


#if PTLIB_VER >= 2130
    virtual PCaselessString GetMethodName() const { return "H46024"; }
    virtual PString GetName() const { return GetMethodName(); }
#elif PTLIB_VER > 2120
    static PString GetNatMethodName() { return "H46024"; }
    virtual PString GetName() const
    {
        return GetNatMethodName();
    }
#else
    static PStringList GetNatMethodName() { return PStringArray("H46024"); };
    virtual PString GetName() const
    {
        return GetNatMethodName()[0];
    }
#endif

    // Start the Nat Method testing
    void Start(const PString & server, H460_FeatureStd23 * _feat);

    // Whether the NAT method is Available
    virtual bool IsAvailable(
        const PIPSocket::Address & binding = PIPSocket::GetDefaultIpAny()  ///< Interface to see if NAT is available on
        );

    // Create the socket pair
    virtual PBoolean CreateSocketPair(
        PUDPSocket * & socket1,
        PUDPSocket * & socket2,
        const PIPSocket::Address & binding = PIPSocket::GetDefaultIpAny(),
#if PTLIB_VER >= 2130
        PObject * userData = NULL
#else
        void * userData = NULL
#endif
        );

    // Whether the NAT Method is available
    void SetAvailable();

    // Whether the NAT method is activated for this call
    virtual void Activate(bool act);

    // Reportable NAT Type
    PSTUNClient::NatTypes GetNATType();

    // Set Port Information
    void SetPortInformation(PortInfo & pairedPortInfo, WORD portPairBase, WORD portPairMax);

    //  CreateRandomPortPair
    WORD CreateRandomPortPair(unsigned int start, unsigned int end);

#if PTLIB_VER >= 2110
    virtual PString GetServer() const { return PString(); }
    virtual bool GetServerAddress(PIPSocketAddressAndPort &) const { return false; }
    virtual NatTypes GetNatType(bool force) { return PSTUNClient::GetNatType(force); }
    virtual NatTypes GetNatType(const PTimeInterval &) { return UnknownNat; }
    virtual bool SetServer(const PString & server) { return PSTUNClient::SetServer(server); }
    virtual bool Open(const PIPSocket::Address &) { return false; }
    virtual bool CreateSocket(BYTE, PUDPSocket * &, const PIPSocket::Address, WORD) { return false; }
    virtual void SetCredentials(const PString &, const PString &, const PString &) {}
#endif

#if PTLIB_VER >= 2120
    /** Overrides from PSTUN to correct transaction ID
        Default PTLIB sets the RFC5389 MAGIC cookie
     */
    virtual PNatMethod::NatTypes DoRFC3489Discovery(
        PSTUNUDPSocket * socket,
        const PIPSocketAddressAndPort & serverAddress,
        PIPSocketAddressAndPort & baseAddressAndPort,
        PIPSocketAddressAndPort & externalAddressAndPort
        );

    virtual PNatMethod::NatTypes FinishRFC3489Discovery(
        PSTUNMessage & responseI,
        PSTUNUDPSocket * socket,
        PIPSocketAddressAndPort & externalAddressAndPort
        );
#endif

protected:

#if PTLIB_VER >= 2120
        virtual void InternalUpdate();
#endif

        // Do a NAT test
        PSTUNClient::NatTypes NATTest();


#if PTLIB_VER >= 2130
        PortInfo singlePortInfo;
        PortInfo pairedPortInfo;
#endif
private:
        PThread *               mainThread;
        PDECLARE_NOTIFIER(PThread, PNatMethod_H46024, MainMethod);
        bool                    isActive;
        bool                    isAvailable;
        PSTUNClient::NatTypes   natType;
        H460_FeatureStd23 *     feat;
        PMutex                  portMute;

#ifdef H323_H46019M
        PortInfo                standardPorts;
        PortInfo                multiplexPorts;
#endif
        PIPSocket::Address      locBindAddress;
        friend class H323EndPoint;
};

///////////////////////////////////////////////////////////////////////////////

class H460_FeatureStd23 : public H460_FeatureStd 
{
    PCLASSINFO(H460_FeatureStd23, H460_FeatureStd);

public:

    H460_FeatureStd23();
    virtual ~H460_FeatureStd23();

    // Universal Declarations Every H460 Feature should have the following
    virtual void AttachEndPoint(H323EndPoint * _ep);

    static PStringArray GetFeatureName() { return PStringArray("Std23"); };
    static PStringArray GetFeatureFriendlyName() { return PStringArray("P2Pnat Detect-H.460.23"); };
    static int GetPurpose()    { return FeatureRas; };
    virtual int GetFeaturePurpose()  { return H460_FeatureStd23::GetPurpose(); } 
    static PStringArray GetIdentifier() { return PStringArray("23"); };

    virtual PBoolean CommonFeature() { return isEnabled; }

    // Messages
    // GK -> EP
    virtual PBoolean OnSendGatekeeperRequest(H225_FeatureDescriptor & pdu);
    virtual void OnReceiveGatekeeperConfirm(const H225_FeatureDescriptor & pdu);

    virtual PBoolean OnSendRegistrationRequest(H225_FeatureDescriptor & pdu);
    virtual void OnReceiveRegistrationConfirm(const H225_FeatureDescriptor & pdu);

    H323EndPoint * GetEndPoint() { return (H323EndPoint *)EP; }

    // Reporting the NAT Type
    void OnNATTypeDetection(PSTUNClient::NatTypes type, const PIPSocket::Address & ExtIP);

    bool IsAvailable();
    bool IsUDPAvailable();

    bool AlternateNATMethod();
    bool UseAlternate();

protected:
    bool DetectALG(const PIPSocket::Address & detectAddress);
    void StartSTUNTest(const PString & server);
    
#ifdef H323_UPnP
    bool IsAlternateAvailable(PString & name);
#endif
    void DelayedReRegistration();
 
private:
    H323EndPoint *            EP;
    PSTUNClient::NatTypes    natType;
    PIPSocket::Address       externalIP;
    PBoolean                 natNotify;
    PBoolean                 alg;
    PBoolean                 isavailable;
    PBoolean                 isEnabled; 
    int                      useAlternate;

};

// Need to declare for Factory Loader
#if !defined(_WIN32_WCE)
    #if PTLIB_VER > 260
       PPLUGIN_STATIC_LOAD(Std23, H460_Feature);
    #else
       PWLIB_STATIC_LOAD_PLUGIN(Std23, H460_Feature);
    #endif
#endif


////////////////////////////////////////////////////////

class H323EndPoint;
class H323Connection;
class H460_FeatureStd24 : public H460_FeatureStd 
{
    PCLASSINFO(H460_FeatureStd24, H460_FeatureStd);

public:

    H460_FeatureStd24();
    virtual ~H460_FeatureStd24();

    // Universal Declarations Every H460 Feature should have the following
    virtual void AttachEndPoint(H323EndPoint * _ep);
    virtual void AttachConnection(H323Connection * _ep);

    static PStringArray GetFeatureName() { return PStringArray("Std24"); }
    static PStringArray GetFeatureFriendlyName() { return PStringArray("P2Pnat Media-H.460.24"); }
    static int GetPurpose()    { return FeatureSignal; }
    virtual int GetFeaturePurpose()  { return H460_FeatureStd24::GetPurpose(); } 
    static PStringArray GetIdentifier() { return PStringArray("24"); }
    virtual PBoolean CommonFeature() { return isEnabled; }

    enum NatInstruct {
        e_unknown,
        e_noassist,
        e_localMaster,
        e_remoteMaster,
        e_localProxy,
        e_remoteProxy,
        e_natFullProxy,
        e_natAnnexA,                // Same NAT
        e_natAnnexB,                // NAT Offload
        e_natFailure = 100
    };

    static PString GetNATStrategyString(NatInstruct method);

    enum H46024NAT {
        e_default,        // This will use the underlying NAT Method
        e_enable,        // Use H.460.24 method (STUN)
        e_AnnexA,       // Disable H.460.24 method but initiate AnnexA
        e_AnnexB,        // Disable H.460.24 method but initiate AnnexB
        e_disable        // Disable all and remote will do the NAT help        
    };

    static PString GetH460NATString(H46024NAT method);

    // Messages
    virtual PBoolean OnSendAdmissionRequest(H225_FeatureDescriptor & pdu);
    virtual void OnReceiveAdmissionConfirm(const H225_FeatureDescriptor & pdu);
    virtual void OnReceiveAdmissionReject(const H225_FeatureDescriptor & pdu);

    virtual PBoolean OnSendSetup_UUIE(H225_FeatureDescriptor & pdu);
    virtual void OnReceiveSetup_UUIE(const H225_FeatureDescriptor & pdu);

protected:
    void HandleNATInstruction(NatInstruct natconfig);
    void SetNATMethods(H46024NAT state);
    void SetH46019State(bool state);
    PBoolean IsNatSendAvailable();
 
private:
    H323EndPoint * EP;
    H323Connection * CON;
    NatInstruct natconfig;
    PMutex h460mute;
    int nattype;
    bool isEnabled;
    bool useAlternate;

};

inline ostream & operator<<(ostream & strm, H460_FeatureStd24::NatInstruct method) { return strm << H460_FeatureStd24::GetNATStrategyString(method); }

// Need to declare for Factory Loader
#if !defined(_WIN32_WCE)
    #if PTLIB_VER > 260
       PPLUGIN_STATIC_LOAD(Std24, H460_Feature);
    #else
       PWLIB_STATIC_LOAD_PLUGIN(Std24, H460_Feature);
    #endif
#endif

#endif 


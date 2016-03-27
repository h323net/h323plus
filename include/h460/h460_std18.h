/* H460_std18.h
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

#ifndef H_H460_FeatureStd18
#define H_H460_FeatureStd18


#if _MSC_VER
#pragma once
#endif 

/////////////////////////////////////////////////////////////////

class MyH323EndPoint;
class MyH323Connection;
class H46018Handler;
class H460_FeatureStd18 : public H460_FeatureStd 
{
    PCLASSINFO(H460_FeatureStd18,H460_FeatureStd);

public:

    H460_FeatureStd18();
    virtual ~H460_FeatureStd18();

    // Universal Declarations Every H460 Feature should have the following
    virtual void AttachEndPoint(H323EndPoint * _ep);

    static PStringArray GetFeatureName() { return PStringArray("Std18"); }
    static PStringArray GetFeatureFriendlyName() { return PStringArray("NatTraversal-H.460.18"); }
    static int GetPurpose()	{ return FeatureRas; }
    virtual int GetFeaturePurpose()  { return H460_FeatureStd18::GetPurpose(); } 
	static PStringArray GetIdentifier() { return PStringArray("18"); }

	virtual PBoolean CommonFeature() { return isEnabled; }

    void SetTransportSecurity(const H323TransportSecurity & callSecurity);

    /////////////////////
    // H.460.18 Messages
    virtual PBoolean OnSendGatekeeperRequest(H225_FeatureDescriptor & pdu);
    virtual void OnReceiveGatekeeperConfirm(const H225_FeatureDescriptor & pdu);

    virtual PBoolean OnSendRegistrationRequest(H225_FeatureDescriptor & pdu);
    virtual void OnReceiveRegistrationConfirm(const H225_FeatureDescriptor & pdu);

    virtual void OnReceiveServiceControlIndication(const H225_FeatureDescriptor & pdu);

private:
    H323EndPoint * EP;

    H46018Handler * handler;
    PBoolean isEnabled;

};

// Need to declare for Factory Loader
#ifndef _WIN32_WCE
	#if PTLIB_VER > 260
	   PPLUGIN_STATIC_LOAD(Std18, H460_Feature);
	#else
	   PWLIB_STATIC_LOAD_PLUGIN(Std18, H460_Feature);
	#endif
#endif

/////////////////////////////////////////////////////////////////

class MyH323EndPoint;
class MyH323Connection;
class H460_FeatureStd19 : public H460_FeatureStd 
{
    PCLASSINFO(H460_FeatureStd19,H460_FeatureStd);

public:

    H460_FeatureStd19();
    virtual ~H460_FeatureStd19();

    // Universal Declarations Every H460 Feature should have the following
    virtual void AttachEndPoint(H323EndPoint * _ep);
    virtual void AttachConnection(H323Connection * _con);

    static PStringArray GetFeatureName() { return PStringArray("Std19"); };
    static PStringArray GetFeatureFriendlyName() { return PStringArray("NatTraversal-H.460.19"); };
    static int GetPurpose()	{ return FeatureSignal; };
    virtual int GetFeaturePurpose()  { return H460_FeatureStd19::GetPurpose(); } 
	static PStringArray GetIdentifier() { return PStringArray("19"); };

	virtual PBoolean CommonFeature() { return remoteSupport; }

    virtual PBoolean FeatureAdvertised(int mtype);

    /////////////////////
    // H.460.19 Messages
    virtual PBoolean OnSendSetup_UUIE(H225_FeatureDescriptor & pdu);
    virtual void OnReceiveSetup_UUIE(const H225_FeatureDescriptor & pdu);

    virtual PBoolean OnSendCallProceeding_UUIE(H225_FeatureDescriptor & pdu);
    virtual void OnReceiveCallProceeding_UUIE(const H225_FeatureDescriptor & pdu);

    virtual PBoolean OnSendAlerting_UUIE(H225_FeatureDescriptor & pdu);
    virtual void OnReceiveAlerting_UUIE(const H225_FeatureDescriptor & pdu);

    virtual PBoolean OnSendCallConnect_UUIE(H225_FeatureDescriptor & pdu);
    virtual void OnReceiveCallConnect_UUIE(const H225_FeatureDescriptor & pdu);

	////////////////////
	// H.460.24 Override
	void SetAvailable(bool avail);
    void EnableMultiplex();

private:
    H323EndPoint * EP;
    H323Connection * CON;

    PBoolean isEnabled;
	PBoolean isAvailable;
    PBoolean remoteSupport;
    PBoolean multiSupport;
};

// Need to declare for Factory Loader
#ifndef _WIN32_WCE
	#if PTLIB_VER > 260
	   PPLUGIN_STATIC_LOAD(Std19, H460_Feature);
	#else
	   PWLIB_STATIC_LOAD_PLUGIN(Std19, H460_Feature);
	#endif
#endif

#endif


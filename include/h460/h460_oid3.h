/*
 * h460_oid3.h
 *
 * H460 Presence implementation class.
 *
 * h323plus library
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

#ifndef H_H460_Featureoid3
#define H_H460_Featureoid3

#include <h460/h460p.h>

#include <map>

#if _MSC_VER
#pragma once
#endif 

/////////////////////////////////////////////////////////////

typedef std::map<PString,PString> PresenceInstructList;

class H460_FeatureOID3;
class H460PresenceHandler : public H323PresenceHandler
{
  public:

   H460PresenceHandler(H323EndPoint & _ep);
   ~H460PresenceHandler();

   void AttachFeature(H460_FeatureOID3 * _feat);

   void SetRegistered(bool registered);

   void SetPresenceState(const PStringList & epalias, 
						unsigned localstate, 
						const PString & localdisplay,
                        PBoolean updateOnly = false
                        );

   void AddInstruction(const PString & epalias, 
						H323PresenceHandler::InstType instType, 
						const PresenceInstructList & subscribe,
                        PBoolean autoSend = true);

   void AddAuthorization(const OpalGloballyUniqueID id,
						const PString & epalias,
						PBoolean approved,
						const PStringList & subscribe);
						

   PStringList & GetSubscriptionList();
   PStringList & GetBlockList();

  // Inherited
	virtual void OnNotification(MsgType tag,
								const H460P_PresenceNotification & notify,
								const H225_AliasAddress & addr
								);
	virtual void OnSubscription(MsgType tag,
								const H460P_PresenceSubscription & subscription,
								const H225_AliasAddress & addr
								);
	virtual void OnInstructions(MsgType tag,
								const H460P_ArrayOf_PresenceInstruction & instruction,
								const H225_AliasAddress & addr
								);

	// Events to notify endpoint
	void PresenceRcvNotification(const H225_AliasAddress & addr, const H323PresenceNotification & notify);
	void PresenceRcvAuthorization(const H225_AliasAddress & addr, const H323PresenceSubscription & subscript);
	void PresenceRcvInstruction(const H225_AliasAddress & addr, const H323PresenceInstruction & instruct);

	void AddEndpointFeature(int feat);
	void AddEndpointH460Feature(const H225_GenericIdentifier & featid, const PString & display);
	void AddEndpointGenericData(const H225_GenericData & data);

	localeInfo & GetLocationInfo() { return EndpointLocale; }

 private:
 	// Lists
	PStringList PresenceSubscriptions;
	PStringList PresenceBlockList;
    std::list<H460P_PresenceFeature>   EndpointFeatures;
	localeInfo  EndpointLocale;

	H225_ArrayOf_GenericData genericData;

	PDECLARE_NOTIFIER(PTimer, H460PresenceHandler, dequeue);
	PTimer	m_queueTimer;	

    H323EndPoint & ep;
    H460_FeatureOID3 * feat;
};

/////////////////////////////////////////////////////////////

class H323EndPoint;
class H460_FeatureOID3 : public H460_FeatureOID 
{
    PCLASSINFO(H460_FeatureOID3,H460_FeatureOID);

public:

    H460_FeatureOID3();
    virtual ~H460_FeatureOID3();

    virtual void AttachEndPoint(H323EndPoint * _ep);

    static PStringArray GetFeatureName() { return PStringArray("OID3"); };
    static PStringArray GetFeatureFriendlyName() { return PStringArray("Presence"); };
    static int GetPurpose();
    virtual int GetFeaturePurpose()  { return H460_FeatureOID3::GetPurpose(); } 
    static PStringArray GetIdentifier();

    virtual PBoolean CommonFeature() { return remoteSupport; }

    virtual PBoolean OnSendGatekeeperRequest(H225_FeatureDescriptor & pdu);
    virtual PBoolean OnSendRegistrationRequest(H225_FeatureDescriptor & pdu);
    virtual void OnReceiveRegistrationConfirm(const H225_FeatureDescriptor & pdu);

    virtual PBoolean OnSendServiceControlIndication(H225_FeatureDescriptor & pdu);
    virtual void OnReceiveServiceControlIndication(const H225_FeatureDescriptor & pdu);

private:

    PBoolean remoteSupport;
    H460PresenceHandler * handler;

    static PBoolean isLoaded;
};

#ifndef _WIN32_WCE
	#if PTLIB_VER > 260
	   PPLUGIN_STATIC_LOAD(OID3, H460_Feature);
	#else
	   PWLIB_STATIC_LOAD_PLUGIN(OID3, H460_Feature);
	#endif
#endif


#endif

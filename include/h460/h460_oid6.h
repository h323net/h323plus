/* H460_OID6.h
 *
 * Copyright (c) 2012 ISVO (Asia) Pte Ltd. All Rights Reserved.
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

#ifndef H_H460_FeatureOID6
#define H_H460_FeatureOID6

#if _MSC_VER
#pragma once
#endif 

class H323EndPoint;
class H460_FeatureOID6 : public H460_FeatureOID 
{
    PCLASSINFO(H460_FeatureOID6,H460_FeatureOID);

public:

    H460_FeatureOID6();
    virtual ~H460_FeatureOID6();

    // Universal Declarations Every H460 Feature should have the following
    virtual void AttachEndPoint(H323EndPoint * _ep);

    static PStringArray GetFeatureName() { return PStringArray("OID6"); };
    static PStringArray GetFeatureFriendlyName() { return PStringArray("Registration Priority & Preemption"); };
    static int GetPurpose()    { return FeatureRas; };
    virtual int GetFeaturePurpose()  { return H460_FeatureOID6::GetPurpose(); } 
    static PStringArray GetIdentifier();

    virtual PBoolean CommonFeature() { return remoteSupport; }

   // Messages
   // GK -> EP
    virtual PBoolean OnSendGatekeeperRequest(H225_FeatureDescriptor & pdu);
    virtual void OnReceiveGatekeeperConfirm(const H225_FeatureDescriptor & pdu);

    virtual PBoolean OnSendRegistrationRequest(H225_FeatureDescriptor & pdu);
    virtual void OnReceiveRegistrationRequest(const H225_FeatureDescriptor & pdu);

    virtual void OnReceiveRegistrationReject(const H225_FeatureDescriptor & pdu);
    virtual void OnReceiveUnregistrationRequest(const H225_FeatureDescriptor & pdu);

 
private:
    PBoolean remoteSupport;
    H323EndPoint * EP;

};

// Need to declare for Factory Loader
#ifndef _WIN32_WCE
    #if PTLIB_VER > 260
       PPLUGIN_STATIC_LOAD(OID6, H460_Feature);
    #else
       PWLIB_STATIC_LOAD_PLUGIN(OID6, H460_Feature);
    #endif
#endif
#endif

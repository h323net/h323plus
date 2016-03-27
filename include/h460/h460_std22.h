/* h460_std22.h
 *
 * Copyright (c) 2015 ISVO (Asia) Pte Ltd. All Rights Reserved.
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

#ifndef H_H460_FeatureStd22
#define H_H460_FeatureStd22


#if _MSC_VER
#pragma once
#endif 

class H323TransportSecurity;
class H460_FeatureStd22 : public H460_FeatureStd 
{
    PCLASSINFO(H460_FeatureStd22,H460_FeatureStd);

public:

    H460_FeatureStd22();
    virtual ~H460_FeatureStd22();

    virtual PObject * Clone() const;

    // Universal Declarations Every H460 Feature should have the following
    virtual void AttachEndPoint(H323EndPoint * _ep);
    virtual void AttachConnection(H323Connection * _con);
 
    static PStringArray GetFeatureName() { return PStringArray("Std22"); };
    static PStringArray GetFeatureFriendlyName() { return PStringArray("H.225.0 Sec-H.460.22"); };
    static int GetPurpose();
    virtual int GetFeaturePurpose()  { return H460_FeatureStd22::GetPurpose(); } 
	static PStringArray GetIdentifier() { return PStringArray("22"); };

    virtual PBoolean FeatureAdvertised(int mtype);
	virtual PBoolean CommonFeature() { return isEnabled; }

	// Messages
    virtual PBoolean OnSendGatekeeperRequest(H225_FeatureDescriptor & pdu);
    virtual void OnReceiveGatekeeperConfirm(const H225_FeatureDescriptor & pdu);

    virtual PBoolean OnSendRegistrationRequest(H225_FeatureDescriptor & pdu);
    virtual void OnReceiveRegistrationConfirm(const H225_FeatureDescriptor & pdu);

    virtual PBoolean OnSendAdmissionRequest(H225_FeatureDescriptor & pdu);
    virtual void OnReceiveAdmissionConfirm(const H225_FeatureDescriptor & pdu);

    virtual void OnReceiveServiceControlIndication(const H225_FeatureDescriptor & pdu);

private:
    H323EndPoint * EP;
    H323Connection * CON;
    PBoolean isEnabled;
};

// Need to declare for Factory Loader
#ifndef _WIN32_WCE
	#if PTLIB_VER > 260
	   PPLUGIN_STATIC_LOAD(Std22, H460_Feature);
	#else
	   PWLIB_STATIC_LOAD_PLUGIN(Std22, H460_Feature);
	#endif
#endif

#endif


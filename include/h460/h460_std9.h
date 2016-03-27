/* H460_std9.h
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

#ifndef H_H460_FeatureStd9
#define H_H460_FeatureStd9

//#include <h460/h4601.h>

#if _MSC_VER
#pragma once
#endif 


class H4609_ArrayOf_RTCPMeasures;
class H460_FeatureStd9 : public H460_FeatureStd 
{
    PCLASSINFO(H460_FeatureStd9,H460_FeatureStd);

public:

    H460_FeatureStd9();
    virtual ~H460_FeatureStd9();

    // Universal Declarations Every H460 Feature should have the following
    virtual void AttachEndPoint(H323EndPoint * _ep);
    virtual void AttachConnection(H323Connection * _con);

    static PStringArray GetFeatureName() { return PStringArray("Std9"); }
    static PStringArray GetFeatureFriendlyName() { return PStringArray("QoS Monitoring-H.460.9"); }
    static int GetPurpose()	{ return FeatureSignal; }
    virtual int GetFeaturePurpose()  { return H460_FeatureStd9::GetPurpose(); } 
	static PStringArray GetIdentifier() { return PStringArray("9"); }

    virtual PBoolean FeatureAdvertised(int mtype);
	virtual PBoolean CommonFeature() { return qossupport; }

	// Messages
    virtual PBoolean OnSendAdmissionRequest(H225_FeatureDescriptor & pdu);
    virtual void OnReceiveAdmissionConfirm(const H225_FeatureDescriptor & pdu);

	// Send QoS information
	virtual PBoolean OnSendInfoRequestResponseMessage(H225_FeatureDescriptor & pdu);
    virtual PBoolean OnSendDisengagementRequestMessage(H225_FeatureDescriptor & pdu);

private:
	PBoolean GenerateReport(H4609_ArrayOf_RTCPMeasures & report);
	PBoolean WriteStatisticsReport(H460_FeatureStd & msg, PBoolean final);

    H323EndPoint * EP;
    H323Connection * CON;
    PBoolean qossupport;
	PBoolean finalonly;

};

// Need to declare for Factory Loader
#ifndef _WIN32_WCE
	#if PTLIB_VER > 260
	   PPLUGIN_STATIC_LOAD(Std9, H460_Feature);
	#else
	   PWLIB_STATIC_LOAD_PLUGIN(Std9, H460_Feature);
	#endif
#endif

#endif


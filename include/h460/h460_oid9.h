/* H460_OID9.h
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

#ifndef H_H460_FeatureOID9
#define H_H460_FeatureOID9

#if _MSC_VER
#pragma once
#endif 

class H323EndPoint;
class H323Connection;
class H460_FeatureOID9 : public H460_FeatureOID 
{
    PCLASSINFO(H460_FeatureOID9,H460_FeatureOID);

public:

    H460_FeatureOID9();
    virtual ~H460_FeatureOID9();

    // Universal Declarations Every H460 Feature should have the following
    virtual void AttachEndPoint(H323EndPoint * _ep);
    virtual void AttachConnection(H323Connection * _con);

    static PStringArray GetFeatureName() { return PStringArray("OID9"); }
    static PStringArray GetFeatureFriendlyName() { return PStringArray("Vendor Information"); }
    static int GetPurpose()    { return FeatureSignal; }
    virtual int GetFeaturePurpose()  { return H460_FeatureOID9::GetPurpose(); } 
    static PStringArray GetIdentifier();

    virtual PBoolean CommonFeature() { return false; }  // Remove this feature if required.

    // Messages
    virtual PBoolean OnSendAdmissionRequest(H225_FeatureDescriptor & pdu);
    virtual void OnReceiveAdmissionConfirm(const H225_FeatureDescriptor & pdu);

 
private:
    H323EndPoint   * m_ep;
    H323Connection * m_con;

};

// Need to declare for Factory Loader
#ifndef _WIN32_WCE
    #if PTLIB_VER > 260
       PPLUGIN_STATIC_LOAD(OID9, H460_Feature);
    #else
       PWLIB_STATIC_LOAD_PLUGIN(OID9, H460_Feature);
    #endif
#endif


#endif

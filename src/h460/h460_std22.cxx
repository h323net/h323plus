/* h460_std22.cxx
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

#include <h323.h>

#ifdef H323_TLS

#include <h460/h460_std22.h>
#include <h460/h4609.h>
#ifdef H323_H46018
#include <h460/h460_std18.h>
#endif


#ifdef _MSC_VER
#pragma warning(disable : 4239)
#endif

// Must Declare for Factory Loader.
H460_FEATURE(Std22);

#define Std22_TLS               1
#define Std22_IPSec             2
#define Std22_Priority          1
#define Std22_Address           2


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
H460_FeatureStd22::H460_FeatureStd22()
: H460_FeatureStd(22), EP(NULL), CON(NULL), isEnabled(false)
{
  PTRACE(6,"H46022\tInstance Created");
  FeatureCategory = FeatureSupported;
}

H460_FeatureStd22::~H460_FeatureStd22()
{

}

void H460_FeatureStd22::AttachEndPoint(H323EndPoint * _ep)
{
   PTRACE(6,"H46022\tEndpoint Attached");
   EP = _ep; 
}

void H460_FeatureStd22::AttachConnection(H323Connection * _con)
{
   CON = _con;
}

int H460_FeatureStd22::GetPurpose()
{
    return FeatureBaseClone; 
}

PObject * H460_FeatureStd22::Clone() const
{
  return new H460_FeatureStd22(*this);
}

PBoolean H460_FeatureStd22::FeatureAdvertised(int mtype)
{
     switch (mtype) {
        case H460_MessageType::e_gatekeeperRequest:
        case H460_MessageType::e_gatekeeperConfirm:
        case H460_MessageType::e_registrationRequest:
        case H460_MessageType::e_registrationConfirm:
        case H460_MessageType::e_admissionRequest:
        case H460_MessageType::e_admissionConfirm:
            return true;
        default:
            return false;
     }
}

void BuildFeature(H323TransportSecurity * transec, H323EndPoint * ep, H460_FeatureStd & feat, PBoolean address = true)
{

    if (transec->IsTLSEnabled()) {
        const H323Listener * tls = ep->GetListeners().GetTLSListener();
        if (tls) {
            H460_FeatureStd sets;
            sets.Add(Std22_Priority,H460_FeatureContent(1,8)); // Priority 1
            if (address)
                sets.Add(Std22_Address,H460_FeatureContent(tls->GetTransportAddress()));
            feat.Add(Std22_TLS,H460_FeatureContent(sets.GetCurrentTable()));
        }
    }

    // NOT YET Supported...Disabled in H323TransportSecurity.
    if (transec->IsIPSecEnabled()) {
        H460_FeatureStd sets;
        sets.Add(Std22_Priority,H460_FeatureContent(2,8)); // Priority 2
        feat.Add(Std22_IPSec,H460_FeatureContent(sets.GetCurrentTable()));
    } 
}

void ReadFeature(H323TransportSecurity * transec, H460_FeatureStd * feat)
{
    if (feat->Contains(Std22_TLS)) {
        H460_FeatureParameter tlsparam = feat->Value(Std22_TLS);
        transec->EnableTLS(true);
        H460_FeatureStd settings;
        settings.SetCurrentTable(tlsparam);
        if (settings.Contains(Std22_Address))
            transec->SetRemoteTLSAddress(settings.Value(Std22_Address));
    }

    // NOT YET Supported...Disabled in H323TransportSecurity.
    if (feat->Contains(Std22_IPSec))
        transec->EnableIPSec(true);
}

PBoolean H460_FeatureStd22::OnSendGatekeeperRequest(H225_FeatureDescriptor & pdu)
{
    if (!EP || !EP->GetTransportSecurity()->HasSecurity())
        return false;

#ifdef H323_H46017
    // H.460.22 is incompatible with H.460.17
    if (EP->TryingWithH46017() || EP->RegisteredWithH46017())
        return false;
#endif

    isEnabled = false;
    H460_FeatureStd feat = H460_FeatureStd(22);  
    BuildFeature(EP->GetTransportSecurity(), EP, feat, false);

    pdu = feat;
	return true;

}

void H460_FeatureStd22::OnReceiveGatekeeperConfirm(const H225_FeatureDescriptor & pdu)
{
    // Do nothing
}

PBoolean H460_FeatureStd22::OnSendRegistrationRequest(H225_FeatureDescriptor & pdu)
{
    if (!EP || !EP->GetTransportSecurity()->HasSecurity())
        return false;

#ifdef H323_H46017
    // H.460.22 is incompatible with H.460.17
    if (EP->TryingWithH46017() || EP->RegisteredWithH46017())
        return false;
#endif

    isEnabled = false;
    H460_FeatureStd feat = H460_FeatureStd(22);  
    BuildFeature(EP->GetTransportSecurity(), EP, feat);

    pdu = feat;
	return true;
}

void H460_FeatureStd22::OnReceiveRegistrationConfirm(const H225_FeatureDescriptor & pdu)
{
   isEnabled = true;
}

PBoolean H460_FeatureStd22::OnSendAdmissionRequest(H225_FeatureDescriptor & pdu)
{
    if (!isEnabled)
        return false;

    H460_FeatureStd feat = H460_FeatureStd(22);
    BuildFeature(EP->GetTransportSecurity(), EP, feat, false);
    pdu = feat;

    return true;
}

void H460_FeatureStd22::OnReceiveAdmissionConfirm(const H225_FeatureDescriptor & pdu)
{

   H460_FeatureStd * feat = PRemoveConst(H460_FeatureStd,&(const H460_FeatureStd &)pdu);

   H323TransportSecurity m_callSecurity(EP);
   ReadFeature(&m_callSecurity,feat);

   if (CON)
       CON->SetTransportSecurity(m_callSecurity);
}

void H460_FeatureStd22::OnReceiveServiceControlIndication(const H225_FeatureDescriptor & pdu) 
{
   H460_FeatureStd * feat = PRemoveConst(H460_FeatureStd,&(const H460_FeatureStd &)pdu);

   H323TransportSecurity m_callSecurity(EP);
   ReadFeature(&m_callSecurity,feat);

#ifdef H323_H46018
    if (EP && EP->GetGatekeeper()->GetFeatures().HasFeature(18)) {
        H460_Feature * feat = EP->GetGatekeeper()->GetFeatures().GetFeature(18);
        if (feat)
            ((H460_FeatureStd18 *)feat)->SetTransportSecurity(m_callSecurity);
    }
#endif
};

#ifdef _MSC_VER
#pragma warning(default : 4239)
#endif

#endif


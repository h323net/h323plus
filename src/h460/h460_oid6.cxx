/* H460_oid6.cxx
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

#include <h323.h>

#ifdef H323_H460PRE

#include "h460/h460_oid6.h"
#include <h323.h>

#ifdef _MSC_VER
#pragma warning(disable : 4239)
#endif

static const char * baseOID = "1.3.6.1.4.1.17090.0.6";        // Advertised Feature
static const char * priorOID = "1";                              // Priority Value
static const char * preemptOID = "2";                          // Preempt Value
static const char * priNotOID = "3";                          // Prior Notification notify
static const char * preNotOID = "4";                          // Preempt Notification


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

// Must Declare for Factory Loader.
H460_FEATURE(OID6);

H460_FeatureOID6::H460_FeatureOID6()
: H460_FeatureOID(baseOID), remoteSupport(false), EP(NULL)
{
 PTRACE(6,"OID6\tInstance Created");

 FeatureCategory = FeatureSupported;

}

H460_FeatureOID6::~H460_FeatureOID6()
{
}

PStringArray H460_FeatureOID6::GetIdentifier()
{
    return PStringArray(baseOID);
}

void H460_FeatureOID6::AttachEndPoint(H323EndPoint * _ep)
{
    EP = _ep; 
}

PBoolean H460_FeatureOID6::OnSendGatekeeperRequest(H225_FeatureDescriptor & pdu) 
{ 
    H460_FeatureOID feat = H460_FeatureOID(baseOID); 
    pdu = feat;

    return true; 
}
    
void H460_FeatureOID6::OnReceiveGatekeeperConfirm(const H225_FeatureDescriptor & pdu) 
{
    remoteSupport = true;
}


PBoolean H460_FeatureOID6::OnSendRegistrationRequest(H225_FeatureDescriptor & pdu) 
{ 
 // Build Message
    H460_FeatureOID feat = H460_FeatureOID(baseOID); 

    if ((EP->GetGatekeeper() != NULL) && 
        (EP->GetGatekeeper()->IsRegistered())) {
            pdu = feat;
            return true;
    }

    feat.Add(priorOID,H460_FeatureContent(EP->GetRegistrationPriority(),8));
    PBoolean preempt = EP->GetPreempt();
    feat.Add(preemptOID,H460_FeatureContent(preempt));
    pdu = feat;

    EP->SetPreempt(false);
    return true; 
}

void H460_FeatureOID6::OnReceiveRegistrationRequest(const H225_FeatureDescriptor & pdu) 
{ 
}

void H460_FeatureOID6::OnReceiveRegistrationReject(const H225_FeatureDescriptor & pdu)
{
  PTRACE(4,"OID6\tReceived Registration Request");

     H460_FeatureOID & feat = (H460_FeatureOID &)pdu;

    if (feat.Contains(preNotOID))   // Preemption notification
             EP->OnNotifyPreempt(false);
}

void H460_FeatureOID6::OnReceiveUnregistrationRequest(const H225_FeatureDescriptor & pdu)
{
  PTRACE(4,"OID6\tReceived Unregistration Request");

     H460_FeatureOID & feat = (H460_FeatureOID &)pdu;

    if (feat.Contains(priNotOID))    // Priority notification
             EP->OnNotifyPriority();

    if (feat.Contains(preNotOID))   // Preemption notification
             EP->OnNotifyPreempt(true);

    EP->SetPreempted(true);
}

#ifdef _MSC_VER
#pragma warning(default : 4239)
#endif

#endif
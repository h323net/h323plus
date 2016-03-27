/* H460_oid9.cxx
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

#ifdef H323_H460COM

#include "h460/h460_oid9.h"

// Compatibility Feature
#define baseOID "1.3.6.1.4.1.17090.0.9"  // Remote Vendor Information
#define VendorProdOID      "1"    // PASN_String of productID
#define VendorVerOID       "2"    // PASN_String of versionID

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

// Must Declare for Factory Loader.
H460_FEATURE(OID9);

H460_FeatureOID9::H460_FeatureOID9()
: H460_FeatureOID(baseOID), m_ep(NULL), m_con(NULL)
{

 PTRACE(6,"OID9\tInstance Created");

 FeatureCategory = FeatureSupported;

}

H460_FeatureOID9::~H460_FeatureOID9()
{
}

PStringArray H460_FeatureOID9::GetIdentifier()
{
    return PStringArray(baseOID);
}

void H460_FeatureOID9::AttachEndPoint(H323EndPoint * _ep)
{
   m_ep = _ep; 
}

void H460_FeatureOID9::AttachConnection(H323Connection * _conn)
{
   m_con = _conn;
}

PBoolean H460_FeatureOID9::OnSendAdmissionRequest(H225_FeatureDescriptor & pdu) 
{ 
    // Build Message
    PTRACE(6,"OID9\tSending ARQ ");
    H460_FeatureOID feat = H460_FeatureOID(baseOID);

    pdu = feat;
    return TRUE;  
}

void H460_FeatureOID9::OnReceiveAdmissionConfirm(const H225_FeatureDescriptor & pdu)
{
    PTRACE(6,"OID9\tReading ACF");
    H460_FeatureOID & feat = (H460_FeatureOID &)pdu;
    
    PString m_product,m_version = PString();
    
    if (feat.Contains(VendorProdOID)) 
       m_product = (const PString &)feat.Value(VendorProdOID);
    
    if (feat.Contains(VendorVerOID)) 
       m_version = (const PString &)feat.Value(VendorVerOID);

    m_con->OnRemoteVendorInformation(m_product, m_version);
}

#endif
/* h341.cxx
 *
 * Copyright (c) 2007 ISVO (Asia) Pte Ltd. All Rights Reserved.
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

#ifdef H323_H341

#include <ptclib/psnmp.h>
#include "h341/h341.h"
#include "h341/h341_oid.h"


H323_H341Server::H323_H341Server(WORD listenPort)
: PSNMPServer(PIPSocket::GetDefaultIpAny(), listenPort)   
{

}

H323_H341Server::~H323_H341Server()
{

}


static PBoolean ValidateOID(H323_H341Server::messagetype reqType,
						        PSNMP::BindingList & varlist,
								PSNMP::ErrorType & errCode)
{

    PSNMP::BindingList::const_iterator Iter = varlist.begin();
    PBoolean found = FALSE;
    do {
     for (Iter = varlist.begin(); Iter != varlist.end(); ++Iter) {
       for (PINDEX i = 0; i< PARRAYSIZE(H341_Field); i++) {
          if (H341_Field[i].oid != Iter->first) 
			   continue;

	      found = TRUE;
		  switch (reqType) {
            case H323_H341Server::e_request:
            case H323_H341Server::e_nextrequest:
			  if (H341_Field[i].access == H341_NoAccess) {
				  PTRACE(4,"H341\tAttribute request FAILED: No permitted access " << Iter->first );
                  errCode = PSNMP::GenErr;
                  return FALSE;    
              }
              break;
            case H323_H341Server::e_set:
              if (H341_Field[i].access == H341_ReadOnly) {
				  PTRACE(4,"H341\tAttribute set FAILED: Read Only " << Iter->first );
                  errCode = PSNMP::ReadOnly;
                  return FALSE;    
              }
              break;
            default:   // Unknown request
			  PTRACE(4,"H341\tGENERAL FAILURE: Unknown request");
              errCode = PSNMP::GenErr;
              return FALSE;    
		  }
             
		  if (Iter->second.GetTag() != (unsigned)H341_Field[i].type ) {
			  PTRACE(4,"H341\tAttribute FAILED Not valid field type " << Iter->first);
              errCode = PSNMP::BadValue;
              return FALSE;    
		  }
		  break;
	   }
	   if (found) break;
	 }
	} while (Iter != varlist.end() && !found);

	if (!found) {
	   	PTRACE(4,"H341\tRequest FAILED: Not valid attribute " << Iter->first);
        errCode = PSNMP::NoSuchName;
        return FALSE;
	}

    return TRUE;
}

PBoolean H323_H341Server::OnGetRequest(PINDEX /*reqID*/, PSNMP::BindingList & vars, PSNMP::ErrorType & errCode)
{
	messagetype reqType = H323_H341Server::e_request;
	if (!ValidateOID(reqType,vars, errCode))
		     return FALSE;

	return OnRequest(reqType, vars,errCode);

}

PBoolean H323_H341Server::OnGetNextRequest(PINDEX /*reqID*/, PSNMP::BindingList & vars, PSNMP::ErrorType & errCode)
{
	messagetype reqType = H323_H341Server::e_nextrequest;
	if (!ValidateOID(reqType,vars, errCode))
		     return FALSE;

	return OnRequest(reqType, vars,errCode);
}

PBoolean H323_H341Server::OnSetRequest(PINDEX /*reqID*/, PSNMP::BindingList & vars, PSNMP::ErrorType & errCode)
{
	messagetype reqType = H323_H341Server::e_set;
	if (!ValidateOID(reqType,vars, errCode))
		     return FALSE;

	return OnRequest(reqType, vars,errCode);
}

#endif




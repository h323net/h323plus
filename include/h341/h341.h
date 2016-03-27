/* h341.h
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
#pragma once

#ifdef H323_H341

#ifndef _H323_H341
#define _H323_H341

#include <ptclib/psnmp.h>

class H323_H341Server : public PSNMPServer
{
  public:
    H323_H341Server(WORD listenport = 161);
    ~H323_H341Server();

   enum messagetype {
     e_request,
     e_nextrequest,
     e_set
   };
 
    // Inherited from PSNMPServer
    PBoolean OnGetRequest     (PINDEX reqID, PSNMP::BindingList & vars, PSNMP::ErrorType & errCode);
    PBoolean OnGetNextRequest (PINDEX reqID, PSNMP::BindingList & vars, PSNMP::ErrorType & errCode);
    PBoolean OnSetRequest     (PINDEX reqID, PSNMP::BindingList & vars, PSNMP::ErrorType & errCode);

    //Events
    virtual PBoolean Authorise(const PIPSocket::Address & /*received*/) 
				                             { return FALSE; }

	virtual PBoolean OnRequest(H323_H341Server::messagetype /*msgtype*/, 
		                              PSNMP::BindingList & /*vars*/,
						                PSNMP::ErrorType & /*errCode*/) 
	                                          { return FALSE; }

  protected:

};

#endif // _H323_H341

#endif



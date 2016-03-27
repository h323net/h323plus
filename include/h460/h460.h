// H460.h:
/*
 * H.460 Implementation for the h323plus Library.
 *
 * Copyright (c) 2004 ISVO (Asia) Pte Ltd. All Rights Reserved.
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

#ifndef _H460_H
#define _H460_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

class H460_MessageType
{
  public:
    enum {
      e_gatekeeperRequest           = 0xf0,
      e_gatekeeperConfirm           = 0xf1,
      e_gatekeeperReject            = 0xf2,
      e_registrationRequest         = 0xf3,
      e_registrationConfirm         = 0xf4, 
      e_registrationReject          = 0xf5,
      e_admissionRequest            = 0xf6,
      e_admissionConfirm            = 0xf7,
      e_admissionReject             = 0xf8,
      e_locationRequest             = 0xf9,
      e_locationConfirm             = 0xfa,
      e_locationReject              = 0xfb,
      e_nonStandardMessage          = 0xfc,
      e_serviceControlIndication    = 0xfd,
      e_serviceControlResponse      = 0xfe,
      e_unregistrationRequest       = 0xe0,
      e_inforequest                 = 0xe1,
      e_inforequestresponse         = 0xe2,
      e_disengagerequest            = 0xe3,
      e_disengageconfirm            = 0xe4,
      e_setup			            = 0x05,   // Match Q931 message id
      e_callProceeding	            = 0x02,   // Match Q931 message id
      e_connect	                    = 0x07,   // Match Q931 message id
      e_alerting                    = 0x01,   // Match Q931 message id
      e_facility                    = 0x62,   // Match Q931 message id
      e_releaseComplete	            = 0x5a,   // Match Q931 message id
      e_unallocated                 = 0xff
    };
};


#endif //_H460_H


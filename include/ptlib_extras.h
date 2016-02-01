
// ptlib_extras.h:
/*
 * Ptlib Extras Implementation for the h323plus Library.
 *
 * Copyright (c) 2011 ISVO (Asia) Pte Ltd. All Rights Reserved.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the General Public License (the  "GNU License"), in which case the
 * provisions of GNU License are applicable instead of those
 * above. If you wish to allow use of your version of this file only
 * under the terms of the GNU License and not to allow others to use
 * your version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice and
 * other provisions required by the GNU License. If you do not delete
 * the provisions above, a recipient may use your version of this file
 * under either the MPL or the GNU License."
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is derived from and used in conjunction with the 
 * H323plus Project (www.h323plus.org/)
 *
 * The Initial Developer of the Original Code is ISVO (Asia) Pte Ltd.
 *
 *
 * Contributor(s): ______________________________________.
 *
 * $Id: ptlib_extras.h,v 1.46.2.2 2016/01/03 07:19:16 shorne Exp $
 *
 */

#ifndef _PTLIB_EXTRAS_H
#define _PTLIB_EXTRAS_H

// Work arounds for different PTLIB versions
#include <etc/pt_stl.h>             // Change to STL based Dictionary in PTLIB v2.12.x
#include <etc/pt_nat.h>             // Fixes for removed items in NAT support in v2.13.x
#include <etc/pt_colour.h>          // Added NV21 colour format for android
#include <etc/h323resampler.h>      // Added Audio buffer and resampler
#include <etc/h323buffer.h>         // Added Frame Buffer and sequencer

// Extra devices
#include <etc/pt_wasapi.h>          // Added Audio driver for Win7+ devices

// Datastore
#include <etc/h323datastore.h>     // Datastore

/////////////////////////////////////////////////////////////////////////////////////////////////////

// Change in definition of INT in PTLIB v2.12.x
#if PTLIB_VER < 2120
#define H323_INT INT
#else
#define H323_INT P_INT_PTR
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////


#endif // _PTLIB_EXTRAS_H








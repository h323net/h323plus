
// ptlib_extras.h:
/*
 * Ptlib Extras Implementation for the h323plus Library.
 *
 * Copyright (c) 2011 ISVO (Asia) Pte Ltd. All Rights Reserved.
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
//#include <etc/pt_extdevices.h>      // Added External Input/Output devices

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








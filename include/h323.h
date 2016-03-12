/*
 * h323.h
 *
 * H.323 protocol handler
 *
 * H323Plus Library
 *
 * Copyright (c) 1998-2000 Equivalence Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions of this code were written with the assisance of funding from
 * Vovida Networks, Inc. http://www.vovida.com.
 *
 * Contributor(s): ______________________________________.
 *
 * $Id $
 *
 */

#ifndef _H323_H
#define _H323_H

// PTLIB H323Plus options
#include "ptlib.h"

#if defined(P_ANDROID)
#define P_FORCE_STATIC_PLUGIN   1
#endif

// Plugin subsystem
#include <ptlib/pluginmgr.h>
#include <ptlib/plugin.h>

// AV subsystem
#include <ptlib/sound.h>
#include <ptlib/video.h>

// DNS
#include <ptclib/pdns.h>
#include <ptclib/url.h>

// Utilties
#include <ptlib/safecoll.h>
#include <ptclib/random.h>
#include <ptclib/cypher.h>
#include <ptlib/sockets.h>
#include <ptclib/delaychan.h>
#include <ptclib/pnat.h>

// Support for different versions of PTLIB
#include "openh323buildopts.h"
#include "ptlib_extras.h"

// H323 Components
#ifdef H323_H460
#include "h460/h460.h"
#include "h460/h4601.h"
#endif

#ifdef H323_H235
#include "h235/h235caps.h"
#else
#include "h323caps.h"
#endif

#include "h323con.h"
#include "h323ep.h"
#include "h323pdu.h"
#include "h323rtp.h"
#include "gkclient.h"

#ifdef H323_H224
#include "h323h224.h"
#endif

#include "codec/opalplugin.h"

#endif // _H323_H

/////////////////////////////////////////////////////////////////////////////



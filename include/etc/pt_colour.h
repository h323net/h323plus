/*
* pt_colour.h
*
* Copyright (c) 2015 Spranto International Pte Ltd. All Rights Reserved.
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
* $Id: pt_colour.h,v 1.1.2.1 2016/01/03 07:19:16 shorne Exp $
*
*/

#pragma once

#include <ptlib/vconvert.h>

class H323ColourConverter : public PColourConverter 
{
    PCLASSINFO(H323ColourConverter, PColourConverter);

public:

    H323ColourConverter(
        const PVideoFrameInfo & src,
        const PVideoFrameInfo & dst
        );

    bool YUV420PtoNV21(const BYTE * srcFrameBuffer,
        BYTE * dstFrameBuffer,
        PINDEX * bytesReturned
        );

    bool NV21toYUV420P(const BYTE * srcFrameBuffer,
        BYTE * dstFrameBuffer,
        PINDEX * bytesReturned
        );
};




/*
* pt_colour.cxx
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

#include "etc/pt_colour.h"


//////////////////////////////////////////////////////////////////////////////////////////////////

#define H323_COLOUR_CONVERTER(from,to) \
  PCOLOUR_CONVERTER2(P_##from##_##to,H323ColourConverter,#from,#to)

H323_COLOUR_CONVERTER(NV21, YUV420P)
{
    return NV21toYUV420P(srcFrameBuffer, dstFrameBuffer, bytesReturned);
}

H323_COLOUR_CONVERTER(YUV420P, NV21)
{
    return YUV420PtoNV21(srcFrameBuffer, dstFrameBuffer, bytesReturned);
}

////////////////////////////////////////////////////////////////////////////////////////////////


H323ColourConverter::H323ColourConverter(const PVideoFrameInfo & src, const PVideoFrameInfo & dst) 
    : PColourConverter(src, dst) 
{ 

}


PStringArray H323ColourConverter::GetColourConverterList(const PString & fmt, bool src)
{
#if PTLIB_VER > 2140
    PStringArray fulllist = PColourConverterRegistration::GetColourConvertersList();

    PStringArray list;
    for (PINDEX i = 0; i < fulllist.GetSize(); ++i) {
        PStringArray info = fulllist[i].Tokenise('\t');
        if (src && (info[0] == fmt))
            list.AppendString(info[1]);
        else if (!src && (info[1] == fmt))
            list.AppendString(info[0]);
    }
    return list;
#else
    return PStringArray();
#endif
}


bool H323ColourConverter::YUV420PtoNV21(const BYTE * srcFrameBuffer, BYTE * dstFrameBuffer, PINDEX * bytesReturned)
{
#if PTLIB_VER >= 2120
    int width = m_srcFrameWidth;
    int height = m_srcFrameHeight;
    int dWidth = m_dstFrameWidth;
    int dHeight = m_dstFrameHeight;
    int dFrameBytes = m_dstFrameBytes;
#else
    int width = srcFrameWidth;
    int height = srcFrameHeight;
    int dWidth = dstFrameWidth;
    int dHeight = dstFrameHeight;
    int dFrameBytes = dstFrameBytes;
#endif

    if ((width != dWidth) || (height != dHeight)) {
        PTRACE(2, "CONV\tERROR NV21 Conversion must be same size!");
        return false;
    }

    const char *src = (const char *)srcFrameBuffer;
    char *dst = (char *)dstFrameBuffer;

    int area = height * width;
    int sqarea = area >> 3;
    int qarea = area >> 2;

    int count = sqarea;
    const unsigned short * su = (const unsigned short *)(src + area);
    const unsigned short * sv = (const unsigned short *)(src + area + qarea);
    unsigned int * uv = (unsigned int *)(dst + area);

    /* copy y as is */
    memcpy(dst, src, area);

    do
    {
        unsigned int u = *su++;
        unsigned int v = *sv++;

        *uv++ = (((u & 0x00FF) << 8) |
            ((u & 0xFF00) << 16) |
            ((v & 0x00FF)) |
            ((v & 0xFF00) << 8));
    } while (--count);

    if (bytesReturned)
        *bytesReturned = dFrameBytes;

    return true;
}


bool H323ColourConverter::NV21toYUV420P(const BYTE * srcFrameBuffer, BYTE * dstFrameBuffer, PINDEX * bytesReturned)
{
#if PTLIB_VER >= 2120
    int width = m_srcFrameWidth;
    int height = m_srcFrameHeight;
    int dWidth = m_dstFrameWidth;
    int dHeight = m_dstFrameHeight;
    int dFrameBytes = m_dstFrameBytes;
#else
    int width = srcFrameWidth;
    int height = srcFrameHeight;
    int dWidth = dstFrameWidth;
    int dHeight = dstFrameHeight;
    int dFrameBytes = dstFrameBytes;
#endif

    if ((width != dWidth) || (height != dHeight)) {
        PTRACE(2, "CONV\tERROR NV21 Conversion must be same size!");
        return false;
    }


    unsigned int YSize = width * height;
    unsigned int UVSize = (YSize >> 1);

    // NV21: Y..Y + VUV...U
    const char *pSrcY = (const char *)srcFrameBuffer;
    const char *pSrcUV = pSrcY + YSize;

    // I420: Y..Y + U.U + V.V
    char *pDstY = (char *)dstFrameBuffer;
    char *pDstU = pDstY + YSize;
    char *pDstV = pDstY + YSize + (UVSize >> 1);

    // copy Y
    memcpy(pDstY, pSrcY, YSize);

    // copy U and V
    for (unsigned k = 0; k < (UVSize >> 1); k++) {
        pDstV[k] = pSrcUV[k * 2];     // copy V
        pDstU[k] = pSrcUV[k * 2 + 1];   // copy U
    }

    if (bytesReturned)
        *bytesReturned = dFrameBytes;

    return true;
}




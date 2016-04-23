
/*
* pt_extdevices.h
*
* Copyright (c) 2016 Spranto International Pte Ltd. All Rights Reserved.
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

#include "etc/pt_extdevices.h"

///////////////////////////////////////////////////////////////////////////////////////////

H323_MediaManager::H323_MediaManager()
{

}

PBoolean H323_MediaManager::SetColourFormat(unsigned id, const PString & colourFormat)
{
    return false;
}

void H323_MediaManager::GetColourFormat(unsigned id, PString & colourFormat)
{
    colourFormat = "YUV420P";
}

PBoolean H323_MediaManager::GetFrameSize(unsigned id, unsigned & width, unsigned & height)
{
    return false;
}

bool H323_MediaManager::Write(unsigned id, void * data, unsigned size, unsigned width, unsigned height)
{
    return false;
}

bool H323_MediaManager::Read(unsigned id, bool toBlock, void * data, unsigned & size, unsigned & width, unsigned & height)
{
    return false;
}

//////////////////////////////////////////////////////////////////////////////

PCREATE_VIDOUTPUT_PLUGIN(External);

PVideoOutputDevice_External::PVideoOutputDevice_External()
: m_streamID(-1), m_manager(NULL), m_videoFrameSize(0),
  m_szConverter(NULL), m_szFrameBuffer(0), m_szBufferSize(0), 
  m_finalHeight(0), m_finalWidth(0), m_finalFormat("YUV420P")
{
    PVideoOutputDevice::SetColourFormat("YUV420P");
}


PVideoOutputDevice_External::~PVideoOutputDevice_External()
{
    Close();
}


bool PVideoOutputDevice_External::AttachManager(unsigned streamID, H323_MediaManager * manager)
{
    m_manager = manager;
    m_streamID = streamID;

    if (!manager)
        return true;

    m_manager->GetColourFormat(m_streamID, m_finalFormat);
    if (!m_manager->GetFrameSize(m_streamID, m_finalWidth, m_finalHeight))
        return SetColourFormat(m_finalFormat);

    PVideoFrameInfo xsrc(m_finalWidth, m_finalWidth, colourFormat, 25);
    PVideoFrameInfo xdst(m_finalWidth, m_finalHeight, m_finalFormat, 25);
    m_szConverter = PColourConverter::Create(xsrc, xdst);
    m_szBufferSize = CalculateFrameBytes(m_finalWidth, m_finalHeight, m_finalFormat);
    m_szFrameBuffer.SetSize(m_szBufferSize);
    return true;
}


PBoolean PVideoOutputDevice_External::Open(const PString & /*deviceName*/, PBoolean /*startImmediate*/)
{
    return true;
}

PBoolean PVideoOutputDevice_External::SetColourFormat(const PString & newFormat)
{
    if (colourFormat *= "YUV420P")
        return PVideoOutputDevice::SetColourFormat(colourFormat);

    return false;
}


PBoolean PVideoOutputDevice_External::IsOpen()
{
    return true;
}


PBoolean PVideoOutputDevice_External::Close()
{
    delete m_szConverter;
    return true;
}


PBoolean PVideoOutputDevice_External::Start()
{
    return true;
}

PBoolean PVideoOutputDevice_External::Stop()
{
    return true;
}


PStringList PVideoOutputDevice_External::GetOutputDeviceNames()
{
    return PStringList("External");
}

PStringArray PVideoOutputDevice_External::GetDeviceNames() const
{
    return GetOutputDeviceNames();
}


PBoolean PVideoOutputDevice_External::SetFrameSize(unsigned width, unsigned height)
{
    if (width == 0 || height == 0)
            return true;

    if (!PVideoOutputDevice::SetFrameSize(width, height))
        return false;

    if (m_szConverter) {
        delete converter;
        PVideoFrameInfo xsrc(frameWidth, frameHeight, colourFormat, 25);
        PVideoFrameInfo xdst(m_finalWidth, m_finalHeight, colourFormat, 25);
        converter = PColourConverter::Create(xsrc, xdst);
        m_videoFrameSize = CalculateFrameBytes(m_finalWidth, m_finalHeight, colourFormat);
    } else {
        m_videoFrameSize = CalculateFrameBytes(frameWidth, frameHeight, colourFormat);
    }
  
    return frameStore.SetSize(m_videoFrameSize);
}


PINDEX PVideoOutputDevice_External::GetMaxFrameBytes()
{
    return GetMaxFrameBytesConverted(CalculateFrameBytes(frameWidth, frameHeight, preferredColourFormat));
}


PBoolean PVideoOutputDevice_External::SetFrameData(unsigned x, unsigned y,
    unsigned width, unsigned height, const BYTE * data, PBoolean endFrame)
{

    if (width > frameWidth || height > frameHeight)
        return false;

    if (x == 0 && width == frameWidth && y == 0 && height == frameHeight) {
        if (converter != NULL)
            converter->Convert(data, frameStore.GetPointer());
        else
            memcpy(frameStore.GetPointer(), data, m_videoFrameSize);
    }

    if (!m_szConverter)
        return (m_manager && m_manager->Write(m_streamID, frameStore.GetPointer(), m_videoFrameSize, width, height));
    else {
        if (m_szConverter->Convert(frameStore.GetPointer(), m_szFrameBuffer.GetPointer()))
            return (m_manager && m_manager->Write(m_streamID, m_szFrameBuffer.GetPointer(), m_szBufferSize, m_finalWidth, m_finalHeight));
        else
            return false;
    }
}


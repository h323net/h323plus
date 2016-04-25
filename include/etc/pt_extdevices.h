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


#pragma once

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Call back

class H323_MediaManager : public PObject
{
    public:
        H323_MediaManager();

        virtual PBoolean SetColourFormat(unsigned id, const PString & colourFormat);
        virtual void     GetColourFormat(unsigned id, PString & colourFormat);

        virtual PBoolean GetFrameSize(unsigned id, unsigned & width, unsigned & height);

        virtual bool Write(unsigned id, void * data, unsigned size, unsigned width, unsigned height);
        virtual bool Read(unsigned id, bool toBlock, void * data, unsigned & size, unsigned & width, unsigned & height);
};


/////////////////////////////////////////////////////////////////////////

class PVideoOutputDevice_External : public PVideoOutputDevice
{
    PCLASSINFO(PVideoOutputDevice_External, PVideoOutputDevice)

public:

    PVideoOutputDevice_External();
    virtual ~PVideoOutputDevice_External();

    virtual PBoolean Open(const PString & deviceName, PBoolean startImmediate = TRUE);

    PBoolean SetColourFormat(const PString & newFormat);

    virtual PBoolean IsOpen();
    virtual PBoolean Close();

    virtual PBoolean Start();
    virtual PBoolean Stop();

    static PStringList GetOutputDeviceNames();

    virtual PStringArray GetDeviceNames() const;

    virtual PBoolean SetFrameSize(unsigned width, unsigned height);

    PINDEX GetMaxFrameBytes();

    bool AttachManager(unsigned streamID, H323_MediaManager * manager);

    virtual PBoolean SetFrameData(unsigned x, unsigned y,
        unsigned width, unsigned height, const BYTE * data, PBoolean endFrame);

protected:

    unsigned           m_streamID;
    H323_MediaManager* m_manager;

    unsigned           m_videoFrameSize;

    PColourConverter * m_finalConverter;
    PBYTEArray         m_finalFrameBuffer;
    unsigned           m_finalBufferSize;
    unsigned           m_finalHeight;
    unsigned           m_finalWidth;
    PString            m_finalFormat;

};

PPLUGIN_STATIC_LOAD(External, PVideoOutputDevice);

/////////////////////////////////////////////////////////////////////////




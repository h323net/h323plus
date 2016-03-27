/* h284.h
 *
 * Copyright (c) 2013 ISVO (Asia) Pte Ltd. All Rights Reserved.
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

#ifndef __H323PLUS_H284_H
#define __H323PLUS_H284_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <list>
#include <map>

#define H284_CLIENT_ID 0x03

////////////////////////////////////////////////////////////////

class H284_Instruction : public PBYTEArray
{
public:
    H284_Instruction();
    
    BYTE GetControlID() const;
    void SetControlID(BYTE cp);

    WORD GetIdentifer() const;
    void SetIdentifier(WORD id);

    unsigned GetAction() const;
    void SetAction(unsigned action);

    DWORD GetPosition() const;
    void SetPosition(DWORD position);

    int GetInstructionType();
    void SetInstructionType(int newType);

protected:
    int m_instType;
};
typedef std::list<H284_Instruction> H284_InstructionList;

////////////////////////////////////////////////////////////////

class H224_H284Handler;
class H284_Frame : public H224_Frame
{
    PCLASSINFO(H284_Frame, H224_Frame);
    
public:    
    H284_Frame();
    ~H284_Frame();

    void     AddInstruction(const H284_Instruction & inst);
    PBoolean ReadInstructions(H224_H284Handler & handler) const;
};

////////////////////////////////////////////////////////////////

class H224_H284Handler;
class H284_ControlPoint : public PBYTEArray 
{
public:

    enum Type {
        e_unknown            = 0,
        e_simple             = 1,
        e_relative           = 2,
        e_absolute           = 3
    };

    enum  Action {
        e_stop               = 0,
        e_positive           = 1,
        e_negative           = 2,
        e_continue           = 3
    };

    H284_ControlPoint(H224_H284Handler & handler, BYTE ctrlID=0);

    virtual PString Name() const;

    virtual PBoolean IsActive() const;

    // Set Data from Implementer
    virtual void SetData(PBoolean absolute = false, PBoolean viewport=false, unsigned step=0, 
        unsigned min=0, unsigned max=0, unsigned current=0, 
        unsigned vportMin=0, unsigned vportMax=0);

    // Set Information internally
    void Set(BYTE id, PBoolean absolute = false, PBoolean viewport=false, WORD step=0, 
        DWORD min=0, DWORD max=0, DWORD current=0, 
        DWORD vportMin=0, DWORD vportMax=0
    );

    // Set Data from Far End
    PBoolean SetData(const BYTE * data, int & length);

    // Send Data to far end
    PBoolean Load(BYTE * data, int & length) const;

    // Build Instruction to Send
    void BuildInstruction(Action act, DWORD value, H284_Instruction & inst);

    // Process incoming instruction
    void HandleInstruction(const H284_Instruction & inst);



    unsigned GetControlType();

    BYTE GetControlID() const;
    PBoolean IsAbsolute() const;
    PBoolean IsViewPort() const;
    WORD GetStep() const;
    DWORD GetMin() const;
    DWORD GetMax() const;
    DWORD GetCurrent() const;
    void SetCurrent(DWORD newPosition);
    DWORD GetViewPortMin() const;
    DWORD GetViewPortMax() const;

protected :
    H224_H284Handler &    m_handler;
    Type                  m_cpType;
    unsigned              m_lastInstruction;
    PBoolean              m_isActive;

};
typedef std::map<BYTE,H284_ControlPoint*> H284_ControlMap;


////////////////////////////////////////////////////////////////

class H224_H284Handler : public H224_Handler
{
  PCLASSINFO(H224_H284Handler, H224_Handler);
    
public:
    
    enum ControlPointID {
        e_IllegalID             = 0x00,
        e_ForwardReverse        = 0x01,
        e_LeftRight             = 0x02,
        e_UpDown                = 0x03,
        e_NeckPan               = 0x07,
        e_NeckTilt              = 0x08,
        e_NeckRoll              = 0x09,
        e_CameraZoom            = 0x0c,
        e_CameraFocus           = 0x0d,
        e_CameraLighting        = 0x0e,
        e_CameraAuxLighting     = 0x0f
    };
    static PString ControlIDAsString(BYTE id);

    H224_H284Handler();
    ~H224_H284Handler();

    virtual PBoolean IsActive(H323Channel::Directions dir) const;

    static PStringList GetHandlerName() {  return PStringArray("H284"); };
    virtual PString GetName() const
            { return GetHandlerName()[0]; }

    virtual BYTE GetClientID() const  
            { return H284_CLIENT_ID; }

    /** Set Remote (receive) Support
    */
    virtual void SetRemoteSupport();

    /** Set Local (transmit) Support
    */
    virtual void SetLocalSupport();

    virtual PBoolean HasRemoteSupport();

    virtual void OnRemoteSupportDetected() {};

    void Add(ControlPointID id);

    void Add(ControlPointID id, PBoolean absolute, PBoolean viewport=false, WORD step=0, 
        DWORD min=0, DWORD max=0, DWORD current=0, DWORD vportMin=0, DWORD vportMax=0
    );
    virtual PBoolean OnAddControlPoint(ControlPointID id,H284_ControlPoint & cp);
    virtual PBoolean OnReceivedControlData(BYTE id, const BYTE * data, int & length);

    PBoolean SendInstruction(ControlPointID id, H284_ControlPoint::Action action, unsigned value);
    virtual void ReceiveInstruction(ControlPointID id, H284_ControlPoint::Action action, unsigned value) const;

    void PostInstruction(H284_Instruction & inst);

    H284_ControlPoint * GetControlPoint(BYTE id);

    virtual void SendExtraCapabilities() const;
    virtual void OnReceivedExtraCapabilities(const BYTE *capabilities, PINDEX size);
    virtual void OnReceivedMessage(const H224_Frame & message);

protected:
    PBoolean            m_remoteSupport;
    unsigned            m_lastInstruction;
    PMutex              m_ctrlMutex;
    H284_ControlMap     m_controlMap;

    H284_Frame          m_transmitFrame;
};

#ifndef _WIN32_WCE
    #if PTLIB_VER > 260
       PPLUGIN_STATIC_LOAD(H284, H224_Handler);
    #else
       PWLIB_STATIC_LOAD_PLUGIN(H284, H224_Handler);
    #endif
#endif

#endif // __H323PLUS_H284_H
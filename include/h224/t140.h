/* t140.h
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

#ifndef __H323PLUS_T140_H
#define __H323PLUS_T140_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#define T140_CLIENT_ID 0x02

class T140_Frame : public H224_Frame
{
    PCLASSINFO(T140_Frame, H224_Frame);
    
public:
    T140_Frame();
    ~T140_Frame();

     enum DataType {
        TextData        = 0,
        BackSpace       = 1,
        NewLine         = 2
     };

    DataType GetDataType() const;
    void SetDataType(DataType type);

    void SetCharacter(const PString & str);
    PString GetCharacter() const;
    
};

class H224_T140Handler : public H224_Handler
{
  PCLASSINFO(H224_T140Handler, H224_Handler);
    
public:
    
    H224_T140Handler();
    ~H224_T140Handler();

    static PStringList GetHandlerName() {  return PStringArray("T140"); };
    virtual PString GetName() const
            { return GetHandlerName()[0]; }

    virtual BYTE GetClientID() const
            { return T140_CLIENT_ID; }
    
    /** Set Remote (receive) Support
    */
    virtual void SetRemoteSupport();

    /** Set Local (transmit) Support
    */
    virtual void SetLocalSupport();

    virtual void OnRemoteSupportDetected() {};

    virtual PBoolean HasRemoteSupport();
    virtual void SendExtraCapabilities() const;
    virtual void OnReceivedExtraCapabilities(const BYTE *capabilities, PINDEX size);
    virtual void OnReceivedMessage(const H224_Frame & message);

    void SendBackSpace();
    void SendNewLine();
    void SendCharacter(const PString & text);

    // T.140 Events
    virtual void ReceivedBackSpace() {};
    virtual void ReceivedNewLine() {};
    virtual void ReceivedText(const PString &) {};


protected:
    void SendT140Frame(T140_Frame & frame);

    PBoolean remoteSupport;
    T140_Frame transmitFrame;
};

#if !defined(_WIN32_WCE)
    #if PTLIB_VER > 260
       PPLUGIN_STATIC_LOAD(T140, H224_Handler);
    #else
       PWLIB_STATIC_LOAD_PLUGIN(T140, H224_Handler);
    #endif
#endif



#endif // __H323PLUS_T140_H
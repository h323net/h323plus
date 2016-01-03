/*
* pt_nat.h
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
* $Id: pt_nat.h,v 1.1.2.1 2015/10/10 08:55:03 shorne Exp $
*
*/

#pragma once

#if PTLIB_VER < 2130

#define H323UDPSocket PUDPSocket

#else

class H323UDPSocket : public PUDPSocket
{
    PCLASSINFO(H323UDPSocket, PUDPSocket);

public:
    H323UDPSocket() {}
    virtual PBoolean IsAlternateAddress(const Address & /*address*/, WORD /*port*/) { return false; }
    virtual PBoolean DoPseudoRead(int & /*selectStatus*/) { return false; }

    virtual PBoolean Close() {
#if 0 //P_QWAVE - QoS not supported yet
        return PIPSocket::Close();
#else
        return PSocket::Close();
#endif
    }
};

#endif // H323UDPSocket



#if PTLIB_VER < 2130

#define H323NatMethod   PNatMethod
#define H323NatList     PNatList
#define H323NatStrategy PNatStrategy

#else

class H323NatMethod : public PNatMethod
{
    PCLASSINFO(H323NatMethod, PNatMethod);

public:
    H323NatMethod() : PNatMethod(0) {}

    virtual PString GetServer() const { return PString(); }
    virtual PCaselessString GetMethodName() const { return "dummy"; }

    PString GetName() { return GetMethodName(); }

    virtual bool IsAvailable(
        const PIPSocket::Address & binding = PIPSocket::GetDefaultIpAny()  ///< Interface to see if NAT is available on
        ) = 0;

protected:
    struct PortInfo {
        PortInfo(WORD port = 0)
            : basePort(port), maxPort(port), currentPort(port) {}
        PMutex mutex;
        WORD   basePort;
        WORD   maxPort;
        WORD   currentPort;
    } singlePortInfo, pairedPortInfo;

    virtual PNATUDPSocket * InternalCreateSocket(Component /*component*/, PObject * /*context*/) { return NULL; }
    virtual void InternalUpdate() {};
};

H323LIST(H323NatList, PNatMethod);

class H323NatStrategy : public PObject
{
public:

    H323NatStrategy() {};
    ~H323NatStrategy() { natlist.RemoveAll(); }

    void AddMethod(PNatMethod * method) { natlist.Append(method); }

    PNatMethod * GetMethod(const PIPSocket::Address & address = PIPSocket::GetDefaultIpAny()) {
        for (PINDEX i = 0; i < natlist.GetSize(); ++i) {
            if (natlist[i].IsAvailable(address))
                return &natlist[i];
        }
        return NULL;
    }

    PNatMethod * GetMethodByName(const PString & name) {
        for (PINDEX i = 0; i < natlist.GetSize(); ++i) {
            if (natlist[i].GetMethodName() == name) {
                return &natlist[i];
            }
        }
        return NULL;
    }

    PBoolean RemoveMethod(const PString & meth) {
        for (PINDEX i = 0; i < natlist.GetSize(); ++i) {
            if (natlist[i].GetMethodName() == meth) {
                natlist.RemoveAt(i);
                return true;
            }
        }
        return false;
    }

    void SetPortRanges(WORD portBase, WORD portMax = 0, WORD portPairBase = 0, WORD portPairMax = 0) {
        for (PINDEX i = 0; i < natlist.GetSize(); ++i)
            natlist[i].SetPortRanges(portBase, portMax, portPairBase, portPairMax);
    }

    H323NatList & GetNATList() { return natlist; }

    PNatMethod * LoadNatMethod(const PString & name) {
        PPluginManager * pluginMgr = &PPluginManager::GetPluginManager();
        return (PNatMethod *)pluginMgr->CreatePlugin(name, "PNatMethod");
    }

    static PStringArray GetRegisteredList() {
        PPluginManager * pluginMgr = &PPluginManager::GetPluginManager();
        return pluginMgr->GetPluginsProviding("PNatMethod", false);
    }

private:
    H323NatList natlist;
};

#endif  // H323_NATMETHOD




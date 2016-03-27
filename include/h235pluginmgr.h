/*
 * h235pluginmgr.h
 *
 * h235 Implementation for the h323plus library.
 *
 * Copyright (c) 2006 ISVO (Asia) Pte Ltd. All Rights Reserved.
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
#include <h235auth.h>

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef _MSC_VER
#pragma warning(disable:4100)
#endif

//////////////////////////////////////////////////////////////////////////////////////

struct Pluginh235_Definition;
class H235PluginAuthenticator : public H235Authenticator
{
    PCLASSINFO(H235PluginAuthenticator, H235Authenticator);

  public:

    H235PluginAuthenticator(Pluginh235_Definition * _def);

    H235_ClearToken * CreateClearToken();
    H225_CryptoH323Token * CreateCryptoToken();

    PBoolean Finalise(
      PBYTEArray & rawPDU
    );

	const char * GetName() const { return h235name; };
	void SetName(PString & name) { h235name = name; };

    ValidationResult ValidateClearToken(
      const H235_ClearToken & clearToken
    );

    ValidationResult ValidateCryptoToken(
      const H225_CryptoH323Token & cryptoToken,
      const PBYTEArray & rawPDU
    );

    PBoolean IsCapability(
      const H235_AuthenticationMechanism & mechansim,
      const PASN_ObjectId & algorithmOID
    );

    PBoolean SetCapability(
      H225_ArrayOf_AuthenticationMechanism & mechansims,
      H225_ArrayOf_PASN_ObjectId & algorithmOIDs
    );

    PBoolean UseGkAndEpIdentifiers() const;

    PBoolean IsSecuredPDU(
      unsigned rasPDU,
      PBoolean received
    ) const;

    PBoolean IsSecuredSignalPDU(
      unsigned signalPDU,
      PBoolean received
    ) const;

    PBoolean IsActive() const;

    const PString & GetRemoteId() const;
    void SetRemoteId(const PString & id);

    const PString & GetLocalId() const;
    void SetLocalId(const PString & id);

    const PString & GetPassword() const;
    void SetPassword(const PString & pw);

    int GetTimestampGracePeriod() const;
    void SetTimestampGracePeriod(int grace);


    Application GetApplication(); 

protected:
	PString h235name;
	unsigned type;
    Pluginh235_Definition * def; 
};



/////////////////////////////////////////////////////////////////////////////////////

class h235PluginDeviceManager : public PPluginModuleManager
{
  PCLASSINFO(h235PluginDeviceManager, PPluginModuleManager);
  public:
    h235PluginDeviceManager(PPluginManager * pluginMgr = NULL);
    ~h235PluginDeviceManager();

    void OnLoadPlugin(PDynaLink & dll, INT code);

    virtual void OnShutdown();

    static void Bootstrap();

    virtual PBoolean Registerh235(unsigned int count, void * _h235List);
    virtual PBoolean Unregisterh235(unsigned int count, void * _h235List);

    void CreateH235Authenticator(Pluginh235_Definition * h235authenticator);

};

static PFactory<PPluginModuleManager>::Worker<h235PluginDeviceManager> h235PluginCodecManagerFactory("h235PluginDeviceManager", true);

///////////////////////////////////////////////////////////////////////////////

typedef PFactory<H235Authenticator> h235Factory;

#define H235_REGISTER(cls, h235Name)   static h235Factory::Worker<cls> cls##Factory(h235Name, true); \

#define H235_DEFINE_AUTHENTICATOR(cls, h235Name, fmtName) \
class cls : public H235PluginAuthenticator { \
  public: \
    cls() : H235PluginAuthenticator() { } \
    PString GetName() const \
    { return fmtName; } \
}; \
 H235_REGISTER(cls, capName) \

/////////////////////////////////////////////////////////////////////////////

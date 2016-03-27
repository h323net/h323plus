/*
 * h2356.h
 *
 * H.235.6 Encryption class.
 *
 * h323plus library
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

#ifndef H2356_H
#define H2356_H

#include <h323.h>

#ifdef H323_H235

#pragma once

struct H235_OIDiterator
{
  bool operator()(const char* s1, const char* s2) const
  {
    return strcmp(s1, s2) > 0;
  }
};
class H235_DiffieHellman;
typedef std::map<PString, H235_DiffieHellman*, H235_OIDiterator> H235_DHMap;

class H2356_Authenticator : public H235Authenticator
{
    PCLASSINFO(H2356_Authenticator, H235Authenticator);
  public:

    H2356_Authenticator();

    ~H2356_Authenticator();

    enum h235TokenState { 
      e_clearNone,             // No Token Sent 
      e_clearSent,               // ClearToken Sent
      e_clearReceived,           // ClearToken Received
      e_clearComplete,         // Both Sent and received.
      e_clearDisable           // Disable Exchange
    };

    virtual PObject * Clone() const;

    virtual const char * GetName() const;

    static PStringArray GetAuthenticatorNames();
#if PTLIB_VER >= 2110
    static PBoolean GetAuthenticationCapabilities(Capabilities * ids);
#endif
    static void InitialiseCache(int cipherlength = 128, unsigned maxTokenLength = 1024);
    static void RemoveCache();

    PBoolean IsMatch(const PString & identifier) const; 

    virtual PBoolean PrepareTokens(
      PASN_Array & clearTokens,
      PASN_Array & cryptoTokens,
      PINDEX max_cipherSize
    );

    virtual ValidationResult ValidateTokens(
      const PASN_Array & clearTokens,
      const PASN_Array & cryptoTokens,
      const PBYTEArray & rawPDU
    );

    virtual PBoolean IsCapability(
      const H235_AuthenticationMechanism & mechansim,
      const PASN_ObjectId & algorithmOID
    );

    virtual PBoolean SetCapability(
      H225_ArrayOf_AuthenticationMechanism & mechansim,
      H225_ArrayOf_PASN_ObjectId & algorithmOIDs
    );

    virtual PBoolean IsSecuredPDU(
      unsigned rasPDU,
      PBoolean received
    ) const;

    virtual PBoolean IsSecuredSignalPDU(
      unsigned signalPDU,
      PBoolean received
    ) const;

    virtual PBoolean IsActive() const;
    virtual void Disable();

    virtual PBoolean GetAlgorithms(PStringList & algorithms) const;
    virtual PBoolean GetAlgorithmDetails(const PString & algorithm, PString & sslName, PString & description);

    // get sslName for algorithm OID
    static PString GetAlgFromOID(const PString & oid);
    // get H.235 OID for sslName
    static PString GetOIDFromAlg(const PString & sslName);
    // get DH token OID for algorithm OID
    static PString GetDhOIDFromAlg(const PString & alg);

    // get the algorithmOID and DH session key for encryption
    virtual PBoolean GetMediaSessionInfo(PString & algorithmOID, PBYTEArray & sessionKey);

    // Export the DH parameters to file
    virtual void ExportParameters(const PFilePath & path);

protected:
    void InitialiseSecurity();

private:

    static H235_DHMap                m_dhCachedMap;
    H235_DHMap                       m_dhLocalMap;

    PBoolean                         m_enabled;
    PBoolean                         m_active;
    h235TokenState                   m_tokenState;
    PStringArray                     m_algOIDs;
      
};

////////////////////////////////////////////////////
/// PFactory Loader

typedef H2356_Authenticator H235AuthenticatorStd6;
#if PTLIB_VER >= 2110
#ifndef _WIN32_WCE
   PPLUGIN_STATIC_LOAD(Std6, H235Authenticator);
#endif
#endif


#endif // H323_H235

#endif // H2356_H


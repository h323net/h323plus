//
// h4610.h
//
// Code automatically generated by asnparse.
//

#if ! H323_DISABLE_H461

#ifndef __H461_H
#define __H461_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include "h225.h"


//
// ASSETPDU
//

class H461_ASSETMessage;

class H461_ASSETPDU : public PASN_Array
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H461_ASSETPDU, PASN_Array);
#endif
  public:
    H461_ASSETPDU(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    PASN_Object * CreateObject() const;
    H461_ASSETMessage & operator[](PINDEX i) const;
    PObject * Clone() const;
};


//
// ApplicationIE
//

class H461_AssociateRequest;
class H461_AssociateResponse;
class H461_ArrayOf_ApplicationStatus;
class H461_ArrayOf_ApplicationAvailable;
class H461_Application;
class H461_ApplicationStatus;
class H461_ApplicationInvokeRequest;
class H461_ApplicationInvokeResponse;
class H461_ApplicationInvoke;
class H461_ArrayOf_ApplicationStart;
class H461_ArrayOf_Application;

class H461_ApplicationIE : public PASN_Choice
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H461_ApplicationIE, PASN_Choice);
#endif
  public:
    H461_ApplicationIE(unsigned tag = 0, TagClass tagClass = UniversalTagClass);

    enum Choices {
      e_associateRequest,
      e_associateResponse,
      e_statusRequest,
      e_statusResponse,
      e_listRequest,
      e_listResponse,
      e_callApplist,
      e_preInvokeRequest,
      e_preInvokeResponse,
      e_invokeRequest,
      e_invokeResponse,
      e_invoke,
      e_invokeStartList,
      e_invokeNotify,
      e_stopRequest,
      e_stopNotify,
      e_callRelease
    };

#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator H461_AssociateRequest &() const;
#else
    operator H461_AssociateRequest &();
    operator const H461_AssociateRequest &() const;
#endif
#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator H461_AssociateResponse &() const;
#else
    operator H461_AssociateResponse &();
    operator const H461_AssociateResponse &() const;
#endif
#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator H461_ArrayOf_ApplicationStatus &() const;
#else
    operator H461_ArrayOf_ApplicationStatus &();
    operator const H461_ArrayOf_ApplicationStatus &() const;
#endif
#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator H461_ArrayOf_ApplicationAvailable &() const;
#else
    operator H461_ArrayOf_ApplicationAvailable &();
    operator const H461_ArrayOf_ApplicationAvailable &() const;
#endif
#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator H461_Application &() const;
#else
    operator H461_Application &();
    operator const H461_Application &() const;
#endif
#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator H461_ApplicationStatus &() const;
#else
    operator H461_ApplicationStatus &();
    operator const H461_ApplicationStatus &() const;
#endif
#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator H461_ApplicationInvokeRequest &() const;
#else
    operator H461_ApplicationInvokeRequest &();
    operator const H461_ApplicationInvokeRequest &() const;
#endif
#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator H461_ApplicationInvokeResponse &() const;
#else
    operator H461_ApplicationInvokeResponse &();
    operator const H461_ApplicationInvokeResponse &() const;
#endif
#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator H461_ApplicationInvoke &() const;
#else
    operator H461_ApplicationInvoke &();
    operator const H461_ApplicationInvoke &() const;
#endif
#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator H461_ArrayOf_ApplicationStart &() const;
#else
    operator H461_ArrayOf_ApplicationStart &();
    operator const H461_ArrayOf_ApplicationStart &() const;
#endif
#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator H461_ArrayOf_Application &() const;
#else
    operator H461_ArrayOf_Application &();
    operator const H461_ArrayOf_Application &() const;
#endif

    PBoolean CreateObject();
    PObject * Clone() const;
};


//
// AssociateRequest
//

class H461_AssociateRequest : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H461_AssociateRequest, PASN_Sequence);
#endif
  public:
    H461_AssociateRequest(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    H225_TimeToLive m_timeToLive;

    PINDEX GetDataLength() const;
    PBoolean Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// AssociateResponse
//

class H461_AssociateResponse : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H461_AssociateResponse, PASN_Sequence);
#endif
  public:
    H461_AssociateResponse(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    enum OptionalFields {
      e_statusInterval
    };

    H225_GloballyUniqueID m_associateToken;
    H225_TimeToLive m_timeToLive;
    H225_TimeToLive m_statusInterval;

    PINDEX GetDataLength() const;
    PBoolean Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// ApplicationDisplay
//

class H461_ApplicationDisplay : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H461_ApplicationDisplay, PASN_Sequence);
#endif
  public:
    H461_ApplicationDisplay(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    enum OptionalFields {
      e_language
    };

    PASN_IA5String m_language;
    PASN_BMPString m_display;

    PINDEX GetDataLength() const;
    PBoolean Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// ApplicationState
//

class H461_InvokeFailReason;

class H461_ApplicationState : public PASN_Choice
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H461_ApplicationState, PASN_Choice);
#endif
  public:
    H461_ApplicationState(unsigned tag = 0, TagClass tagClass = UniversalTagClass);

    enum Choices {
      e_available,
      e_unavailable,
      e_inuse,
      e_associated,
      e_invokeFail
    };

#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator H461_InvokeFailReason &() const;
#else
    operator H461_InvokeFailReason &();
    operator const H461_InvokeFailReason &() const;
#endif

    PBoolean CreateObject();
    PObject * Clone() const;
};


//
// InvokeFailReason
//

class H461_InvokeFailReason : public PASN_Choice
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H461_InvokeFailReason, PASN_Choice);
#endif
  public:
    H461_InvokeFailReason(unsigned tag = 0, TagClass tagClass = UniversalTagClass);

    enum Choices {
      e_unavailable,
      e_inuse,
      e_declined,
      e_h2250Error
    };

    PBoolean CreateObject();
    PObject * Clone() const;
};


//
// Application
//

class H461_Application : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H461_Application, PASN_Sequence);
#endif
  public:
    H461_Application(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    H225_GenericIdentifier m_applicationId;

    PINDEX GetDataLength() const;
    PBoolean Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// ApplicationInvokeRequest
//

class H225_GenericIdentifier;

class H461_ApplicationInvokeRequest : public PASN_Choice
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H461_ApplicationInvokeRequest, PASN_Choice);
#endif
  public:
    H461_ApplicationInvokeRequest(unsigned tag = 0, TagClass tagClass = UniversalTagClass);

    enum Choices {
      e_applicationId
    };

#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator H225_GenericIdentifier &() const;
#else
    operator H225_GenericIdentifier &();
    operator const H225_GenericIdentifier &() const;
#endif

    PBoolean CreateObject();
    PObject * Clone() const;
};


//
// ApplicationInvokeResponse
//

class H461_ApplicationInvoke;
class H461_InvokeFailReason;

class H461_ApplicationInvokeResponse : public PASN_Choice
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H461_ApplicationInvokeResponse, PASN_Choice);
#endif
  public:
    H461_ApplicationInvokeResponse(unsigned tag = 0, TagClass tagClass = UniversalTagClass);

    enum Choices {
      e_approved,
      e_declined
    };

#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator H461_ApplicationInvoke &() const;
#else
    operator H461_ApplicationInvoke &();
    operator const H461_ApplicationInvoke &() const;
#endif
#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
    operator H461_InvokeFailReason &() const;
#else
    operator H461_InvokeFailReason &();
    operator const H461_InvokeFailReason &() const;
#endif

    PBoolean CreateObject();
    PObject * Clone() const;
};


//
// ApplicationStart
//

class H461_ApplicationStart : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H461_ApplicationStart, PASN_Sequence);
#endif
  public:
    H461_ApplicationStart(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    H225_GenericIdentifier m_applicationId;
    H225_GloballyUniqueID m_invokeToken;

    PINDEX GetDataLength() const;
    PBoolean Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// ArrayOf_ApplicationStatus
//

class H461_ApplicationStatus;

class H461_ArrayOf_ApplicationStatus : public PASN_Array
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H461_ArrayOf_ApplicationStatus, PASN_Array);
#endif
  public:
    H461_ArrayOf_ApplicationStatus(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    PASN_Object * CreateObject() const;
    H461_ApplicationStatus & operator[](PINDEX i) const;
    PObject * Clone() const;
};


//
// ArrayOf_ApplicationAvailable
//

class H461_ApplicationAvailable;

class H461_ArrayOf_ApplicationAvailable : public PASN_Array
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H461_ArrayOf_ApplicationAvailable, PASN_Array);
#endif
  public:
    H461_ArrayOf_ApplicationAvailable(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    PASN_Object * CreateObject() const;
    H461_ApplicationAvailable & operator[](PINDEX i) const;
    PObject * Clone() const;
};


//
// ArrayOf_ApplicationStart
//

class H461_ApplicationStart;

class H461_ArrayOf_ApplicationStart : public PASN_Array
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H461_ArrayOf_ApplicationStart, PASN_Array);
#endif
  public:
    H461_ArrayOf_ApplicationStart(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    PASN_Object * CreateObject() const;
    H461_ApplicationStart & operator[](PINDEX i) const;
    PObject * Clone() const;
};


//
// ArrayOf_Application
//

class H461_Application;

class H461_ArrayOf_Application : public PASN_Array
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H461_ArrayOf_Application, PASN_Array);
#endif
  public:
    H461_ArrayOf_Application(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    PASN_Object * CreateObject() const;
    H461_Application & operator[](PINDEX i) const;
    PObject * Clone() const;
};


//
// ArrayOf_ApplicationDisplay
//

class H461_ApplicationDisplay;

class H461_ArrayOf_ApplicationDisplay : public PASN_Array
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H461_ArrayOf_ApplicationDisplay, PASN_Array);
#endif
  public:
    H461_ArrayOf_ApplicationDisplay(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    PASN_Object * CreateObject() const;
    H461_ApplicationDisplay & operator[](PINDEX i) const;
    PObject * Clone() const;
};


//
// ArrayOf_AliasAddress
//

class H225_AliasAddress;

class H461_ArrayOf_AliasAddress : public PASN_Array
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H461_ArrayOf_AliasAddress, PASN_Array);
#endif
  public:
    H461_ArrayOf_AliasAddress(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    PASN_Object * CreateObject() const;
    H225_AliasAddress & operator[](PINDEX i) const;
    PObject * Clone() const;
};


//
// ASSETMessage
//

class H461_ASSETMessage : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H461_ASSETMessage, PASN_Sequence);
#endif
  public:
    H461_ASSETMessage(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    enum OptionalFields {
      e_associateToken,
      e_callIdentifier
    };

    H461_ApplicationIE m_application;
    H225_GloballyUniqueID m_associateToken;
    H225_CallIdentifier m_callIdentifier;

    PINDEX GetDataLength() const;
    PBoolean Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// ApplicationStatus
//

class H461_ApplicationStatus : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H461_ApplicationStatus, PASN_Sequence);
#endif
  public:
    H461_ApplicationStatus(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    enum OptionalFields {
      e_display,
      e_avatar,
      e_state
    };

    H225_GenericIdentifier m_applicationId;
    H461_ArrayOf_ApplicationDisplay m_display;
    PASN_IA5String m_avatar;
    H461_ApplicationState m_state;

    PINDEX GetDataLength() const;
    PBoolean Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// ApplicationAvailable
//

class H461_ApplicationAvailable : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H461_ApplicationAvailable, PASN_Sequence);
#endif
  public:
    H461_ApplicationAvailable(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    H225_GenericIdentifier m_applicationId;
    H461_ArrayOf_AliasAddress m_aliasAddress;

    PINDEX GetDataLength() const;
    PBoolean Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


//
// ApplicationInvoke
//

class H461_ApplicationInvoke : public PASN_Sequence
{
#ifndef PASN_LEANANDMEAN
    PCLASSINFO(H461_ApplicationInvoke, PASN_Sequence);
#endif
  public:
    H461_ApplicationInvoke(unsigned tag = UniversalSequence, TagClass tagClass = UniversalTagClass);

    H225_GenericIdentifier m_applicationId;
    H225_GloballyUniqueID m_invokeToken;
    H461_ArrayOf_AliasAddress m_aliasAddress;

    PINDEX GetDataLength() const;
    PBoolean Decode(PASN_Stream & strm);
    void Encode(PASN_Stream & strm) const;
#ifndef PASN_NOPRINTON
    void PrintOn(ostream & strm) const;
#endif
    Comparison Compare(const PObject & obj) const;
    PObject * Clone() const;
};


#endif // __H461_H

#endif // if ! H323_DISABLE_H461


// End of h4610.h

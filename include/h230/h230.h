/*
 * h230.h
 *
 * H.230 Conference control class.
 *
 * h323plus library
 *
 * Copyright (c) 2007 ISVO (Asia) Pte. Ltd.
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

#include <h323.h>
#include <gccpdu.h>
#include <list>

#ifdef H323_H230 

class H230Control  : public PObject
{
   PCLASSINFO(H230Control, PObject);

public:

	enum AddResponse
	{
      e_Addsuccess,
      e_AddinvalidRequester,
      e_AddinvalidNetworkType,
      e_AddinvalidNetworkAddress,
      e_AddaddedNodeBusy,
      e_AddnetworkBusy,
      e_AddnoPortsAvailable,
      e_AddconnectionUnsuccessful
	};

	enum LockResponse
	{
      e_Locksuccess,
      e_LockinvalidRequester,
      e_LockalreadyLocked
	};

	enum EjectResponse
	{
      e_Ejectsuccess,
      e_EjectinvalidRequester,
      e_EjectinvalidNode
	};

	enum  TransferResponse
	{
	  e_Transfersuccess,
      e_TransferinvalidRequester
	};

	class userInfo
	{
	 public:
		int       m_Token;
		PString   m_Number;
		PString   m_Name;
		PString   m_vCard;
	};

	H230Control(const PString & _h323token);


///////////////////////////////////////////
// Endpoint Functions
    PBoolean Invite(const PStringList & aliases);
	PBoolean LockConference();
    PBoolean UnLockConference();
    PBoolean EjectUser(int node);
	PBoolean TransferUser(list<int> node,const PString & number);
	PBoolean TerminalListRequest();
	PBoolean ChairRequest(PBoolean revoke);
	PBoolean ChairAssign(int node);
	PBoolean FloorRequest();
	PBoolean FloorAssign(int node);
	PBoolean WhoIsChair();
	PBoolean UserEnquiry(list<int> node);

// Endpoint Events
	virtual void OnControlsEnabled(PBoolean /*success*/) {};
	virtual void OnConferenceChair(PBoolean /*success*/) {};
	virtual void OnConferenceFloor(PBoolean /*success*/) {};
	virtual void OnInviteResponse(int /*id*/, const PString & /*calledNo*/, AddResponse /*response*/, int /*errCode*/){};
	virtual void OnLockConferenceResponse(LockResponse /*lock*/)  {};
	virtual void OnUnLockConferenceResponse(LockResponse /*lock*/)  {};
	virtual void OnEjectUserResponse(int /*node*/, EjectResponse /*lock*/) {};
	virtual void OnTransferUserResponse(list<int> /*node*/,const PString & /*number*/, TransferResponse /*result*/) {};
	virtual void OnTerminalListResponse(list<int> node) {};
	virtual void ConferenceJoined(int /*terminalId*/){};
	virtual void ConferenceLeft(int /*terminalId*/) {};
	virtual void MakeChairResponse(PBoolean /*success*/) {};
	virtual void ChairAssigned(int /*node*/) {};
	virtual void FloorAssigned(int /*node*/) {};
	virtual void OnChairTokenResponse(int /*id*/, const PString & /*name*/) {};
	virtual void OnFloorRequested(int /*terminalId*/,PBoolean /*cancel*/) {};
	virtual void OnUserEnquiryResponse(const list<userInfo> &) {};


///////////////////////////////////////////
// Server Events
	virtual void OnInvite(const PStringList & /*alias*/) const {};
	virtual void OnLockConference(PBoolean /*state*/) const {};
    virtual void OnEjectUser(int /*node*/) const {};
	virtual void OnTransferUser(list<int> /*node*/,const PString & /*number*/) const {};
	virtual void OnTerminalListRequest() const {};
	virtual void ChairRequested(const int & /*terminalId*/,PBoolean /*cancel*/) {};
	virtual void OnFloorRequest() {};
	virtual void OnChairTokenRequest() const {};
	virtual void OnChairAssign(int /*node*/) const {};
	virtual void OnFloorAssign(int /*node*/) const {};
	virtual void OnUserEnquiry(list<int>) const {};


//  Server Commands
	PBoolean InviteResponse(int /*id*/, const PString & /*calledNo*/, AddResponse /*response*/, int /*errCode*/);
	PBoolean LockConferenceResponse(LockResponse lock);
	PBoolean UnLockConferenceResponse(LockResponse lock);
	PBoolean EjectUserResponse(int node, EjectResponse lock);
	PBoolean TransferUserResponse(list<int> node,const PString & number, TransferResponse result);
	PBoolean TerminalListResponse(list<int> node);
	PBoolean ChairTokenResponse(int termid,const PString & termname);
	PBoolean ChairAssignResponse(int termid,const PString & termname);
	PBoolean FloorAssignResponse(int termid,const PString & termname);
	PBoolean UserEnquiryResponse(const list<userInfo> &);

// Server Indications
	PBoolean ConferenceJoinedInd(int termId);
	PBoolean ConferenceLeftInd(int termId);
	PBoolean ConferenceTokenAssign(int mcuId,int termId);

	void SetChair(PBoolean success);
	void SetFloor(PBoolean success);


////////////////////////////////////////////////
// Common
 // standard incoming requests
    PBoolean OnHandleConferenceRequest(const H245_ConferenceRequest &);
    PBoolean OnHandleConferenceResponse(const H245_ConferenceResponse &);
    PBoolean OnHandleConferenceCommand(const H245_ConferenceCommand &);
    PBoolean OnHandleConferenceIndication(const H245_ConferenceIndication &);

  // Generic incoming requests
	PBoolean OnHandleGenericPDU(const H245_GenericMessage & msg);


 protected:
  // H.245
	PBoolean OnGeneralRequest(int request);
	PBoolean OnGeneralIndication(int req, const H245_TerminalLabel & label);

	PBoolean OnReceiveTerminalListResponse(const H245_ArrayOf_TerminalLabel & list);
	PBoolean OnReceiveChairResponse(const H245_ConferenceResponse_makeMeChairResponse & resp);
	PBoolean OnReceiveChairTokenResponse(const H245_ConferenceResponse_chairTokenOwnerResponse & resp);
	PBoolean OnReceiveChairTokenRequest();
	PBoolean OnReceiveChairAssignRequest(const H245_TerminalLabel & req);
	PBoolean OnReceiveChairAssignResponse(const H245_ConferenceResponse_terminalIDResponse & req);
    PBoolean OnReceiveFloorAssignRequest(const H245_TerminalLabel & req);
    PBoolean OnReceiveFloorAssignResponse(const H245_ConferenceResponse_conferenceIDResponse & resp);
	
  // H.230
	PBoolean ReceivedH230PDU(unsigned msgId, unsigned paramId, const H245_ParameterValue & value);

  // T.124
	PBoolean ReceivedT124PDU(unsigned msgId, unsigned paramId, const H245_ParameterValue & value);
	PBoolean OnReceivedT124Request(const GCC_RequestPDU &);
    PBoolean OnReceivedT124Response(const GCC_ResponsePDU &);
    PBoolean OnReceivedT124Indication(const GCC_IndicationPDU &);

    PBoolean OnConferenceJoinRequest(const GCC_ConferenceJoinRequest &);
    PBoolean OnConferenceAddRequest(const GCC_ConferenceAddRequest &);
    PBoolean OnConferenceLockRequest(const GCC_ConferenceLockRequest &);
    PBoolean OnConferenceUnlockRequest(const GCC_ConferenceUnlockRequest &);
    PBoolean OnConferenceTerminateRequest(const GCC_ConferenceTerminateRequest &);
    PBoolean OnConferenceEjectUserRequest(const GCC_ConferenceEjectUserRequest &);
    PBoolean OnConferenceTransferRequest(const GCC_ConferenceTransferRequest &);

    PBoolean OnConferenceJoinResponse(const GCC_ConferenceJoinResponse & pdu);
    PBoolean OnConferenceAddResponse(const GCC_ConferenceAddResponse & pdu);
    PBoolean OnConferenceLockResponse(const GCC_ConferenceLockResponse & pdu);
    PBoolean OnConferenceUnlockResponse(const GCC_ConferenceUnlockResponse & pdu);
    PBoolean OnConferenceEjectUserResponse(const GCC_ConferenceEjectUserResponse & pdu);
    PBoolean OnConferenceTransferResponse(const GCC_ConferenceTransferResponse & pdu);
    PBoolean OnFunctionNotSupportedResponse(const GCC_FunctionNotSupportedResponse & pdu);


    PBoolean ReceivedPACKPDU(unsigned msgId, unsigned paramId, const H245_ParameterValue & value);

	PBoolean SendPACKGenericRequest(int paramid, const PASN_OctetString & rawpdu);
	PBoolean SendPACKGenericResponse(int paramid, const PASN_OctetString & rawpdu);
	PBoolean OnReceivePACKRequest(const PASN_OctetString & rawpdu);
	PBoolean OnReceivePACKResponse(const PASN_OctetString & rawpdu);

	virtual PBoolean WriteControlPDU(const H323ControlPDU & pdu);

	void SetLocalID(int mcu, int num);
	int GetLocalID();
	void RemoveLocalID();

 	PString m_h323token;
	int m_mcuID;
    int m_userID;

    PBoolean m_ConferenceChair;
	PBoolean m_ConferenceFloor;
};


class H230T124PDU :  public H323ControlPDU
{
    PCLASSINFO(H230T124PDU, H323ControlPDU);

public:
	void BuildRequest(GCC_RequestPDU & pdu);
	void BuildResponse(GCC_ResponsePDU & pdu);
	void BuildIndication(GCC_IndicationPDU & pdu);

protected:
	void BuildGeneric(PASN_OctetString & pdu);

};


class H230Control_EndPoint   : public H230Control 
{
  public:
   PCLASSINFO(H230Control_EndPoint, H230Control);

	  class result {
	   public:
	     result();

         int errCode;
		 int node;
		 PBoolean cancel;
		 PString name;
		 list<int> ids;
		 list<userInfo> info;
	  };

  	H230Control_EndPoint(const PString & _h323token);
	~H230Control_EndPoint();

// Chair Instructions
    PBoolean ReqInvite(const PStringList & aliases);
	PBoolean ReqLockConference();
    PBoolean ReqUnLockConference();
    PBoolean ReqEjectUser(int node);
	PBoolean ReqTransferUser(list<int> node,const PString & number);
	PBoolean ReqChairAssign(int node);
	PBoolean ReqFloorAssign(int node);

// General Requests
	PBoolean ReqTerminalList(list<int> & node);
	PBoolean ReqChair(PBoolean revoke);
	PBoolean ReqFloor();
	PBoolean ReqWhoIsChair(int & node);
	PBoolean ReqUserEnquiry(list<int> node, list<userInfo> & info);

// conference Indications
	virtual void OnControlsEnabled(PBoolean /*success*/) {};
	virtual void OnConferenceChair(PBoolean /*success*/) {};
	virtual void OnConferenceFloor(PBoolean /*success*/) {};


// Inherited
	virtual void ConferenceJoined(int /*terminalId*/){};
	virtual void ConferenceJoinInfo(int /*termId*/, PString /*number*/, PString /*name*/, PString /*vcard*/) {}; 
	virtual void ConferenceLeft(int /*terminalId*/) {};
	virtual void OnFloorRequested(int /*terminalId*/,PBoolean /*cancel*/) {};
	virtual void OnChairAssigned(int /*node*/) {};
	virtual void OnFloorAssigned(int /*node*/) {};
	virtual void OnInviteResponse(int /*id*/, const PString & /*calledNo*/, AddResponse /*response*/, int /*errCode*/) {};

//////////////////////////////////////////////////////////
// Endpoint Responses
	void OnLockConferenceResponse(LockResponse /*lock*/);
	void OnUnLockConferenceResponse(LockResponse /*lock*/);
	void OnEjectUserResponse(int /*node*/, EjectResponse /*lock*/);
	void OnTransferUserResponse(list<int> /*node*/,const PString & /*number*/, TransferResponse /*result*/);
	void OnTerminalListResponse(list<int> node);
	void MakeChairResponse(PBoolean /*success*/);
	void OnChairTokenResponse(int /*id*/, const PString & /*name*/);
	void OnUserEnquiryResponse(const list<userInfo> &);
	void ChairAssigned(int /*node*/);
	void FloorAssigned(int /*node*/);

 protected:
	PMutex requestMutex;
	PSyncPoint responseMutex;
	result * res;

};

#endif

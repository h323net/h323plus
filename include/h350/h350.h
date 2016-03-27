/*
 * h350.h
 *
 * H.350 LDAP interface class.
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

#ifdef H323_H350

#ifndef _H323_H350
#define _H323_H350

#include <ptclib/pldap.h>
#include <map>
#include <list>

class H350_Session : public PLDAPSession
{
  public:
     typedef std::list<PLDAPSchema> LDAP_Record;
     typedef std::map<PString,LDAP_Record> LDAP_RecordList;

	 PBoolean Open(const PString & hostname, WORD port = 389);

	 PBoolean Login(const PString & who, const PString & passwd, PLDAPSession::AuthenticationMethod authMethod=AuthSimple);

	 void NewRecord(LDAP_Record & rec);

	 PBoolean SetAttribute(LDAP_Record & record,const PString & attrib, const PString & value);
     PBoolean SetAttribute(LDAP_Record & record,const PString & attrib, const PBYTEArray & value);

 	 PBoolean GetAttribute(LDAP_Record & record,const PString & attrib, PString & value);
	 PBoolean GetAttribute(LDAP_Record & record,const PString & attrib, PBYTEArray & value);

	 PBoolean PostNew(const PString & dn, const LDAP_Record & record);
	 PBoolean PostUpdate(const PString & dn, const LDAP_Record & record);

	 PBoolean Delete() { return FALSE; }

	 int Search(const PString & base, 
		         const PString & filter, 
				 LDAP_RecordList & results,
				 const PStringArray & attributes = PStringList()
				 );
};

#define H350_Schema(cname)  \
class cname##_schema : public PLDAPSchema \
{   \
  public: static PStringList SchemaName() { return PStringList(cname##_SchemaName); } \
  void AttributeList(attributeList & attrib) { \
   for (PINDEX i = 0; i< PARRAYSIZE(cname##_attributes ); i++) \
	 attrib.push_back(Attribute(cname##_attributes[i].name,(AttributeType)cname##_attributes[i].type)); }; \
}; \
LDAP_Schema(cname); 

#endif // _H323_H350

#endif  


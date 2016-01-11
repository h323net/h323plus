/*
* h323datastore.cxx
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
* $Id: h323datastore.cxx,v 1.1.2.1 2016/01/03 07:19:17 shorne Exp $
*
*/

#include <h323.h>

#ifdef H323_DATASTORE

#include "etc/h323datastore.h"

extern "C" {
#include <sqlcipher/sqlite3.h>
}

#if _WIN32
#pragma comment(lib, H323_DATASTORE_LIB)
#endif

PString defDBKey = "QeHvR96oFYNmLXaMqhSKJNEZjK63ftcQm4RNzYkzTVn";
PString defTable = "settings";
PString defSection = "default";
PString defKey = "FourtyTwo";
PString defValue = "42";

PString defCreateStmt =   // Create default table
"CREATE TABLE '%t' ('id' TEXT NOT NULL, 'section' TEXT NOT NULL, 'key' TEXT NOT NULL, 'value' TEXT, PRIMARY KEY('id', 'section', 'key'))";

PString StmtSections =
"SELECT section from '%t' WHERE id = '%i' order by section;";

PString StmtSectionDel =
"DELETE from '%t' WHERE (id = '%i' and section = '%s');";

PString StmtKeyValues =
"SELECT key, value from '%t' WHERE (id = '%i' and section = '%s') order by key;";

PString StmtValue =
"SELECT value from '%t' WHERE (id = '%i' and section = '%s' and key = '%k');";

PString StmtKey =
"SELECT key from '%t' WHERE (id = '%i' and section = '%s') order by key;";

PString StmtHasKey =
"SELECT value from '%t' WHERE (id = '%i' and section = '%s' and key = '%k');";

PString StmtKeyDel =
"DELETE from '%t' WHERE (id = '%i' and section = '%s' and key = '%k');";

PString StmtNewValue =
"INSERT INTO '%t' (id,section,key,value) VALUES ('%i','%s','%k','%v');";

PString StmtSetValue =
"UPDATE '%t' SET value = '%v' WHERE (id = '%i' and section = '%s' and key = '%k');";


/////////////////////////////////////////////////////////////////////////////////////////

static int sqlite_callback(void * result, int argc, char **argv, char **azColName)
{
    H323DataStore::ResultRow row;
    for (int i = 0; i < argc; i++) {
        H323DataStore::ResultValue val;
        val.first = azColName[i];
        val.second = argv[i] ? argv[i] : "";
        row.push_back(val);
    }
    ((H323DataStore::ResultRows *)result)->push_back(row);
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

PBoolean H323DataStore::ResultRows::HasValue()
{
    return (this->size() > 0);
}

PString H323DataStore::ResultRows::GetValue()
{
    PString val;

    ResultRows::const_iterator it;
    for (it = this->begin(); it != this->end(); ++it) {
        ResultRow row = *it;
        ResultRow::iterator i;
        for (i = row.begin(); i != row.end(); ++i)
            val = i->second;
    }
    return val;
}


PStringToString H323DataStore::ResultRows::GetValueMap()
{
    PStringToString vals;

    ResultRows::const_iterator it;
    for (it = this->begin(); it != this->end(); ++it) {
        ResultRow row = *it;
        ResultRow::iterator i;
        for (i = row.begin(); i != row.end(); ++i)
            vals.SetAt(i->first, i->second);
    }
    return vals;
}
   

PStringArray H323DataStore::ResultRows::GetValueArray(bool val)
{
    PStringArray vals;

    ResultRows::const_iterator it;
    for (it = this->begin(); it != this->end(); ++it) {
        ResultRow row = *it;
        ResultRow::iterator i;
        for (i = row.begin(); i != row.end(); ++i) {
            if (val)
                vals.AppendString(i->second);
            else
                vals.AppendString(i->first);
        }
    }
    return vals;
}

H323DataStore::ResultRows H323DataStore::ResultRows::Query(const QueryVar & vars)
{
    H323DataStore::ResultRows rows;

    // TODO cache Query.
    return rows;
}

/////////////////////////////////////////////////////////////////////////////////////////

H323DataStore::H323DataStore(const PFilePath  & filename, const PString & key, const PString & table, const PString & id, bool cache)
    : PConfig(filename, table), m_curKey(key), m_cache(cache), m_connection(0)
{
    if (defaultSection.IsEmpty())
        defaultSection = defTable;

    m_curSection = defSection;

    m_queryVar["%t"] = defaultSection;
    m_queryVar["%i"] = id;
    m_queryVar["%s"] = defSection;
    //m_queryVar["%k"] = defKey;
    //m_queryVar["%v"] = defValue;

    OpenDatabase();
}


H323DataStore::~H323DataStore()
{
    CloseDatabase();
}


void H323DataStore::SetTable(const PString & table)
{
    m_queryVar["%t"] = table;

    // TODO: ReCache here...
}


PString H323DataStore::GetTable()
{
    return m_queryVar["%t"];
}

void H323DataStore::CreateTable(const PString & table)
{
    SetTable(table);

    ResultRows resultRows;
    ExecuteQuery(defCreateStmt, m_queryVar, resultRows);
}

void H323DataStore::SetDefaultSection(const PString & section)
{
    m_curSection = section;
    m_queryVar["%s"] = section;
}

PString H323DataStore::GetDefaultSection() const
{
    return m_curSection;
}

void H323DataStore::SetDefaultID(const PString & id)
{
    m_queryVar["%i"] = id;
}

PString H323DataStore::GetDefaultID()
{
    return m_queryVar["%i"];
}

PStringArray H323DataStore::GetSections() const
{
    PStringArray sections;

    ResultRows resultRows;
    if (ExecuteQuery(StmtSections, m_queryVar, resultRows))
        sections = resultRows.GetValueArray(false);

    return sections;
}


PStringArray H323DataStore::GetKeys() const
{
    PStringArray keys;

    ResultRows resultRows;
    if (ExecuteQuery(StmtKey, m_queryVar, resultRows))
        keys = resultRows.GetValueArray(false);

    return keys;
}


PStringArray H323DataStore::GetKeys(const PString & section) const
{
    PStringArray keys;

    QueryVar localVar = m_queryVar;
    localVar["%s"] = section;

    ResultRows resultRows;
    if (ExecuteQuery(StmtKey, localVar, resultRows))
        keys = resultRows.GetValueArray(false);

    return keys;
}


PStringToString H323DataStore::GetAllKeyValues() const
{
    PStringToString keyvals;

    ResultRows resultRows;
    if (ExecuteQuery(StmtKeyValues, m_queryVar, resultRows))
        keyvals = resultRows.GetValueMap();

    return keyvals;
}


PStringToString H323DataStore::GetAllKeyValues(const PString & section) const
{
    PStringToString keyvals;

    QueryVar localVar = m_queryVar;
    localVar["%s"] = section;

    ResultRows resultRows;
    if (ExecuteQuery(StmtKeyValues, localVar, resultRows))
        keyvals = resultRows.GetValueMap();

    return keyvals;
}


void H323DataStore::DeleteSection()
{
    ResultRows resultRows;
    ExecuteQuery(StmtSectionDel, m_queryVar, resultRows);
}


void H323DataStore::DeleteSection(const PString & section)
{
    QueryVar localVar = m_queryVar;
    localVar["%s"] = section;

    ResultRows resultRows;
    ExecuteQuery(StmtSectionDel, localVar, resultRows);
}


void H323DataStore::DeleteKey(const PString & key)
{
    QueryVar localVar = m_queryVar;
    localVar["%k"] = key;

    ResultRows resultRows;
    ExecuteQuery(StmtKeyDel, localVar, resultRows);
}


void H323DataStore::DeleteKey(const PString & section, const PString & key)
{
    QueryVar localVar = m_queryVar;
    localVar["%s"] = section;
    localVar["%k"] = key;

    ResultRows resultRows;
    ExecuteQuery(StmtKeyDel, localVar, resultRows);
}


PBoolean H323DataStore::HasKey(const PString & key) const
{
    bool haskey = false;

    QueryVar localVar = m_queryVar;
    localVar["%k"] = key;

    ResultRows resultRows;
    if (ExecuteQuery(StmtHasKey, localVar, resultRows))
        return resultRows.HasValue();

    return haskey;
}


PBoolean H323DataStore::HasKey(const PString & section, const PString & key) const
{
    bool haskey = false;

    QueryVar localVar = m_queryVar;
    localVar["%s"] = section;
    localVar["%k"] = key;

    ResultRows resultRows;
    if (ExecuteQuery(StmtHasKey, localVar, resultRows))
        return resultRows.HasValue();

    return haskey;
}


PString H323DataStore::GetString(const PString & section, const PString & key, const PString & dflt) const
{
    PString str = dflt;

    QueryVar localVar = m_queryVar;
    localVar["%s"] = section;
    localVar["%k"] = key;
    localVar["%v"] = str;

    ResultRows resultRows;
    if (ExecuteQuery(StmtValue, localVar, resultRows)) {
        if (resultRows.HasValue()) {
           str = resultRows.GetValue();
        } else
           ExecuteQuery(StmtNewValue, localVar, resultRows);
    }
    return str;
}


void H323DataStore::SetString(const PString & section, const PString & key, const PString & value)
{
    QueryVar localVar = m_queryVar;
    localVar["%s"] = section;
    localVar["%k"] = key;
    localVar["%v"] = value;

    ResultRows resultRows;
    ExecuteQuery(StmtSetValue, localVar, resultRows);
}


void H323DataStore::OpenDatabase()
{

    PBoolean exists = PFile::Exists(location);

    bool ok = true;
    if (ok && (SQLITE_OK != sqlite3_open(location, &m_connection))) {
        PTRACE(2, "H323DataStore\tConnection to " << location
            << " failed (sqlite3_open failed): " << sqlite3_errmsg(m_connection));
        ok = false;
    }

    if (ok && (SQLITE_OK != sqlite3_key(m_connection, m_curKey.GetPointer(), m_curKey.GetLength()))) {
        PTRACE(2, "H323DataStore\tSetting key for " << location
            << " failed (sqlite3_key failed): " << sqlite3_errmsg(m_connection));
        ok = false;
    }

    if (ok && !exists) {
        ResultRows resultRows;
        ok = ExecuteQuery(defCreateStmt, m_queryVar, resultRows);
    }

    if (!ok) {
        CloseDatabase();
        return;
    }

    if (m_cache) {
        // TODO CACHE Load.
    }
}


void H323DataStore::CloseDatabase()
{
    sqlite3_close(m_connection);
    m_connection = NULL;
}


PString H323DataStore::QueryStatement(const PString & query, const QueryVar & vars) const
{
    PString result = query;
    QueryVar::const_iterator it;
    for (it = vars.begin(); it != vars.end(); ++it)
         result.Replace(it->first, it->second,true);

    return result;
}


PBoolean H323DataStore::ExecuteQuery(const PString & query, const QueryVar & vars, ResultRows & resultRows) const
{
    if (!m_connection)
        return false;

    char *errMsg = NULL;
    PString queryStmt = QueryStatement(query, vars);
    if (SQLITE_OK != sqlite3_exec(m_connection, queryStmt, sqlite_callback, &resultRows, &errMsg)) {
        PTRACE(2, "H323DataStore\tError executing " << query
            << " (sqlite3_exec failed):");
        return false;
    }

    return true;
}

#endif

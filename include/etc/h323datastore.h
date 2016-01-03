/*
* h323datastore.h
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
* $Id: h323datastore.h,v 1.1.2.1 2016/01/03 07:19:16 shorne Exp $
*
*/

#pragma once

#ifdef H323_DATASTORE

struct sqlite3;
class H323DataStore : public PConfig
{
    PCLASSINFO(H323DataStore, PConfig);

public:

    /** Create a new secure configuration database
    */
    H323DataStore(
        const PFilePath  & filename,       ///< Database filename
        const PString    & key = "",       ///< Key to decrypt database
        const PString    & table = "",     ///< Database Table
        const PString    & id = "",        ///< identifier in first column
        bool               cache = false   ///< whether to read all and cache
    );

    ~H323DataStore();

    /**@name Section functions */
    //@{

    virtual void SetTable(
        const PString & table  ///< New table in db.
        );

    virtual PString GetTable();


    void CreateTable(
        const PString & table  ///< New table in db.
        );

    /** Set the default section for variable operations. All functions that deal
    with keys and get or set configuration values will use this section
    unless an explicit section name is specified.

    Note when the <code>Environment</code> source is being used the default
    section may be set but it is ignored.
    */
    virtual void SetDefaultSection(
        const PString & section  ///< New default section name.
        );

    /** Get the default section for variable operations. All functions that deal
    with keys and get or set configuration values will use this section
    unless an explicit section name is specified.

    Note when the <code>Environment</code> source is being used the default
    section may be retrieved but it is ignored.

    @return default section name string.
    */
    virtual PString GetDefaultSection() const;

    /**
    */
    virtual void SetDefaultID(
        const PString & id  ///< New default ID (Primary key) name.
        );

    /**
    */
    virtual PString GetDefaultID();

    /** Get all of the section names currently specified in the file. A section
    is the part specified by the [ and ] characters.

    Note when the <code>Environment</code> source is being used this will
    return an empty list as there are no section present.

    @return list of all section names.
    */
    virtual PStringArray GetSections() const;

    /** Get a list of all the keys in the section. If the section name is not
    specified then use the default section.

    @return list of all key names.
    */
    virtual PStringArray GetKeys() const;
    /** Get a list of all the keys in the section. */

    virtual PStringArray GetKeys(
        const PString & section   ///< Section to use instead of the default.
        ) const;

    /** Get all of the keys in the section and their values. If the section
    name is not specified then use the default section.

    @return Dictionary of all key names and their values.
    */
    virtual PStringToString GetAllKeyValues() const;
    /** Get all of the keys in the section and their values. */

    virtual PStringToString GetAllKeyValues(
        const PString & section   ///< Section to use instead of the default.
        ) const;

    /** Delete all variables in the specified section. If the section name is
    not specified then the default section is deleted.

    Note that the section header is also removed so the section will not
    appear in the GetSections() function.
    */
    virtual void DeleteSection();

    /** Delete all variables in the specified section. */
    virtual void DeleteSection(
        const PString & section   ///< Name of section to delete.
        );

    /** Delete the particular variable in the specified section. If the section
    name is not specified then the default section is used.

    Note that the variable and key are removed from the file. The key will
    no longer appear in the GetKeys() function. If you wish to delete the
    value without deleting the key, use SetString() to set it to the empty
    string.
    */
    virtual void DeleteKey(
        const PString & key       ///< Key of the variable to delete.
        );

    /** Delete the particular variable in the specified section. */
    virtual void DeleteKey(
        const PString & section,  ///< Section to use instead of the default.
        const PString & key       ///< Key of the variable to delete.
        );

    /**Determine if the particular variable in the section is actually present.

    This function allows a caller to distinguish between getting a saved
    value or using the default value. For example if you called
    GetString("MyKey", "DefVal") there is no way to distinguish between
    the default "DefVal" being used, or the user had explicitly saved the
    value "DefVal" into the PConfig.
    */
    virtual PBoolean HasKey(
        const PString & key       ///< Key of the variable.
        ) const;
    /**Determine if the particular variable in the section is actually present. */
    virtual PBoolean HasKey(
        const PString & section,  ///< Section to use instead of the default.
        const PString & key       ///< Key of the variable.
        ) const;
    //@}

    /**@name Get/Set variables */
    //@
    /** Get a string variable determined by the key in the section. */
    virtual PString GetString(
        const PString & section,  ///< Section to use instead of the default.
        const PString & key,      ///< The key name for the variable.
        const PString & dflt      ///< Default value for the variable.
        ) const;

    /** Set a string variable determined by the key in the section. */
    virtual void SetString(
        const PString & section,  ///< Section to use instead of the default.
        const PString & key,      ///< The key name for the variable.
        const PString & value     ///< New value to set for the variable.
        );
    //@}

    typedef std::map< PString, PString >               QueryVar;

    typedef std::pair< PString, PString >              ResultValue;
    typedef std::list< ResultValue >                   ResultRow;

    class ResultRows : public std::list< ResultRow >
    {
    public:
        PBoolean            HasValue();
        PString             GetValue();
        PStringToString     GetValueMap();
        PStringArray        GetValueArray(bool val = true);

        ResultRows          Query(const QueryVar & vars);
    };

    // TODO: Caching functions;

protected:

    void OpenDatabase();

    PString QueryStatement(const PString & query, 
                           const QueryVar & vars
            ) const;

    PBoolean ExecuteQuery(const PString & query, 
                          const QueryVar & vars, 
                          ResultRows & resultRows
            ) const;

    void CloseDatabase();


private:

    PString     m_curKey;           ///< Database key
    PString     m_curSection;       ///< current section (Need for compatibility as PConfig::DefaultSection is table name)

    QueryVar    m_queryVar;

    bool        m_cache;            ///< Whether all the settings are read in and cached
    ResultRows  m_cacheRows;        ///< Rows read into memory

    sqlite3  *  m_connection;



};

#endif



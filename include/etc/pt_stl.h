/*
* pt_stl.h.cxx
*
* Copyright (c) 2015 ISVO (Asia) Pte Ltd. All Rights Reserved.
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

#ifndef H323_STLDICTIONARY

#define H323Dictionary  PDictionary
#define H323DICTIONARY  PDICTIONARY

#define H323List  PList
#define H323LIST  PLIST

#else

#include <map>

template <class PAIR>
class deleteDictionaryEntry {
public:
    void operator()(const PAIR & p) { delete p.second.second; }
};


template <class E>
inline void deleteDictionaryEntries(const E & e)
{
    typedef typename E::value_type PT;
    std::for_each(e.begin(), e.end(), deleteDictionaryEntry<PT>());
}


struct PSTLSortOrder {
    int operator() (unsigned p1, unsigned p2) const {
        return (p1 > p2);
    }
};

template <class K, class D> class PSTLDictionary : public PObject,
public std::map< unsigned, std::pair<K, D*>, PSTLSortOrder >
{
    PCLASSINFO(PSTLDictionary, PObject);

public:
    /**@name Construction */
    //@{
    /**Create a new, empty, dictionary.

    Note that by default, objects placed into the dictionary will be
    deleted when removed or when all references to the dictionary are
    destroyed.
    */
    PSTLDictionary() :disallowDeleteObjects(false) {}

    ~PSTLDictionary() { RemoveAll(); }
    //@}

    /**@name Overrides from class PObject */
    //@{
    /**Make a complete duplicate of the dictionary. Note that all objects in
    the array are also cloned, so this will make a complete copy of the
    dictionary.
    */
    virtual PObject * Clone() const
    {
        return PNEW PSTLDictionary(*this);
    }
    //@}

    /**@name New functions for class */
    //@{
    /**Get the object contained in the dictionary at the \p key
    position. The hash table is used to locate the data quickly via the
    hash function provided by the \p key.

    The last key/data pair is remembered by the class so that subseqent
    access is very fast.

    @return
    reference to the object indexed by the key.
    */
    D & operator[](
        const K & key   ///< Key to look for in the dictionary.
        ) const
    {
        unsigned pos = 0; return *InternalFindKey(key, pos);
    }

    /**Determine if the value of the object is contained in the hash table. The
    object values are compared, not the pointers.  So the objects in the
    collection must correctly implement the <code>PObject::Compare()</code>
    function. The hash table is used to locate the entry.

    @return
    true if the object value is in the dictionary.
    */
    PBoolean Contains(
        const K & key   ///< Key to look for in the dictionary.
        ) const {
        unsigned pos = 0;  return InternalContains(key, pos);
    }

    /**Remove an object at the specified \p key. The returned pointer is then
    removed using the <code>SetAt()</code> function to set that key value to
    NULL. If the <code>AllowDeleteObjects</code> option is set then the
    object is also deleted.

    @return
    pointer to the object being removed, or NULL if the key was not
    present in the dictionary. If the dictionary is set to delete objects
    upon removal, the value -1 is returned if the key existed prior to removal
    rather than returning an illegal pointer
    */
    virtual D * RemoveAt(
        const K & key   ///< Key for position in dictionary to get object.
        ) {
        return InternalRemoveKey(key);
    }

    /**Add a new object to the collection. If the objects value is already in
    the dictionary then the object is overrides the previous value. If the
    <code>AllowDeleteObjects</code> option is set then the old object is also deleted.

    The object is placed in the an ordinal position dependent on the keys
    hash function. Subsequent searches use the hash function to speed access
    to the data item.

    @return
    true if the object was successfully added.
    */
    virtual PBoolean SetAt(
        const K & key,  // Key for position in dictionary to add object.
        D * obj         // New object to put into the dictionary.
        ) {
        return InternalAddKey(key, obj);
    }

    /**Get the object at the specified key position. If the key was not in the
    collection then NULL is returned.

    @return
    pointer to object at the specified key.
    */
    virtual D * GetAt(
        const K & key   // Key for position in dictionary to get object.
        ) {
        unsigned pos = 0; return (D *)InternalFindKey(key, pos);
    }

    /**Get the key in the dictionary at the ordinal index position.

    The ordinal position in the dictionary is determined by the hash values
    of the keys and the order of insertion.

    The last key/data pair is remembered by the class so that subseqent
    access is very fast.

    @return
    reference to key at the index position.
    */
    const K & GetKeyAt(
        PINDEX pos  ///< Ordinal position in dictionary for key.
        ) const
    {
        return InternalGetKeyAt(pos);
    }

    /**Get the data in the dictionary at the ordinal index position.

    The ordinal position in the dictionary is determined by the hash values
    of the keys and the order of insertion.

    The last key/data pair is remembered by the class so that subseqent
    access is very fast.

    @return
    reference to data at the index position.
    */
    D & GetDataAt(
        PINDEX pos  ///< Ordinal position in dictionary for data.
        ) const
    {
        return InternalGetAt(pos);
    }

    PINDEX GetSize() const { return this->size(); }

    /**Remove all of the elements in the collection. This operates by
    continually calling <code>RemoveAt()</code> until there are no objects left.

    The objects are removed from the last, at index
    (GetSize()-1) toward the first at index zero.
    */
    virtual void RemoveAll()
    {
        PWaitAndSignal m(dictMutex);

        if (!disallowDeleteObjects)
            deleteDictionaryEntries(*this);
        this->clear();
    }

    PINLINE void AllowDeleteObjects(
        PBoolean yes = true   ///< New value for flag for deleting objects
        ) {
        disallowDeleteObjects = !yes;
    }

    /**Disallow the deletion of the objects contained in the collection. See
    the <code>AllowDeleteObjects()</code> function for more details.
    */
    void DisallowDeleteObjects() { disallowDeleteObjects = true; }
    //@}

    typedef struct std::pair<K, D*> DictionaryEntry;

protected:

    PBoolean  disallowDeleteObjects;
    PMutex    dictMutex;

    D * InternalFindKey(
        const K & key,   ///< Key to look for in the dictionary.
        unsigned & ref        ///< Returned index
        ) const
    {
        typename std::map< unsigned, std::pair<K, D*>, PSTLSortOrder>::const_iterator i;
        for (i = this->begin(); i != this->end(); ++i) {
            if (i->second.first == key) {
                ref = i->first;
                return i->second.second;
            }
        }
        return NULL;
    };

    D & InternalGetAt(
        unsigned ref        ///< Returned index
        ) const
    {
        PWaitAndSignal m(dictMutex);

        PAssert(ref < this->size(), psprintf("Index out of Bounds ref: %u sz: %u", ref, this->size()));
        typename std::map< unsigned, std::pair<K, D*>, PSTLSortOrder>::const_iterator i = this->find(ref);
        PAssert(i != this->end(), psprintf("Item %u not found in collection sz: %u", ref, this->size()));
        return *(i->second.second);
    };

    const K & InternalGetKeyAt(
        unsigned ref      ///< Ordinal position in dictionary for key.
        ) const
    {
        PWaitAndSignal m(dictMutex);

        PAssert(ref < this->size(), psprintf("Index out of Bounds ref: %u sz: %u", ref, this->size()));
        typename std::map< unsigned, std::pair<K, D*>, PSTLSortOrder>::const_iterator i = this->find(ref);
        PAssert(i != this->end(), psprintf("Item %u not found in collection sz: %u", ref, this->size()));
        return i->second.first;
    }

    D * InternalRemoveResort(unsigned pos) {
        unsigned newpos = pos;
        unsigned sz = (unsigned)this->size();
        D * dataPtr = NULL;
        typename std::map< unsigned, std::pair<K, D*>, PSTLSortOrder >::iterator it = this->find(pos);
        if (it == this->end()) return NULL;
        if (disallowDeleteObjects)
            dataPtr = it->second.second;
        else
            delete it->second.second;
        this->erase(it);

        for (unsigned i = pos + 1; i < sz; ++i) {
            typename std::map< unsigned, std::pair<K, D*>, PSTLSortOrder >::iterator j = this->find(i);
            if (j != this->end()) {
                DictionaryEntry entry = make_pair(j->second.first, j->second.second);
                this->insert(pair<unsigned, std::pair<K, D*> >(newpos, entry));
                newpos++;
                this->erase(j);
            }
        }

        return dataPtr;
    };

    D * InternalRemoveKey(
        const K & key   ///< Key to look for in the dictionary.
        )
    {
        PWaitAndSignal m(dictMutex);

        unsigned pos = 0;
        InternalFindKey(key, pos);
        return InternalRemoveResort(pos);
    }

    PBoolean InternalAddKey(
        const K & key,  // Key for position in dictionary to add object.
        D * obj         // New object to put into the dictionary.
        )
    {
        PWaitAndSignal m(dictMutex);

        unsigned pos = (unsigned)this->size();
        DictionaryEntry entry = make_pair(key, obj);
        this->insert(pair<unsigned, std::pair<K, D*> >(pos, entry));
        return true;
    }


    PBoolean InternalContains(
        const K & key,   ///< Key to look for in the dictionary.
        unsigned & ref        ///< Returned index
        ) const
    {
        return (InternalFindKey(key, ref) != NULL);
    };

};

#define PSTLDICTIONARY(cls, K, D) typedef PSTLDictionary<K, D> cls

#define H323Dictionary  PSTLDictionary
#define H323DICTIONARY  PSTLDICTIONARY


//////////////////////////////////////////////////////////////////////////////////////

template <class PAIR>
class deleteListEntry {
public:
    void operator()(const PAIR & p) { delete p.second; }
};

template <class E>
inline void deleteListEntries(const E & e)
{
    typedef typename E::value_type PT;
    std::for_each(e.begin(), e.end(), deleteListEntry<PT>());
}


template <class D> class PSTLList : public PObject,
public std::map< unsigned, D*, PSTLSortOrder >
{
    PCLASSINFO(PSTLList, PObject);

public:
    /**@name Construction */
    //@{
    /**Create a new, empty, dictionary.

    Note that by default, objects placed into the dictionary will be
    deleted when removed or when all references to the dictionary are
    destroyed.
    */
    PSTLList() :disallowDeleteObjects(false) {}

    PSTLList(int /*dummy*/, const PObject * /*c*/)
        : disallowDeleteObjects(false) { }

    ~PSTLList() { RemoveAll(); }
    //@}

    /**@name Overrides from class PObject */
    //@{
    /**Make a complete duplicate of the dictionary. Note that all objects in
    the array are also cloned, so this will make a complete copy of the
    dictionary.
    */
    virtual PObject * Clone() const
    {
        return PNEW PSTLList(*this);
    }
    //@}

    virtual PINDEX Append(
        D * obj   ///< New object to place into the collection.
        ) {
        return InternalAddKey(obj);
    }

    /**Insert a new object at the specified ordinal index. If the index is
    greater than the number of objects in the collection then the
    equivalent of the <code>Append()</code> function is performed.
    If not greater it will insert at the ordinal index and shuffle down ordinal values.

    @return
    index of the newly inserted object.
    */
    virtual PINDEX InsertAt(
        PINDEX index,   ///< Index position in collection to place the object.
        D * obj         ///< New object to place into the collection.
        ) {
        return InternalSetAt((unsigned)index, obj, false, true);
    }

    /**Remove the object at the specified ordinal index from the collection.
    If the AllowDeleteObjects option is set then the object is also deleted.

    Note if the index is beyond the size of the collection then the
    function will assert.

    @return
    pointer to the object being removed, or NULL if it was deleted.
    */
    virtual D * RemoveAt(
        PINDEX index   ///< Index position in collection to place the object.
        ) {
        return InternalRemoveKey((unsigned)index);
    }


    PBoolean Remove(
        D * obj   ///< Index position in collection to place the object.
        ) {
        unsigned index = 0;
        if (!InternalFindIndex(index, obj))
            return false;
        return (InternalRemoveResort(index) != NULL);
    }


    /**Set the object at the specified ordinal position to the new value. This
    will overwrite the existing entry.
    This method will NOT delete the old object independently of the
    AllowDeleteObjects option. Use <code>ReplaceAt()</code> instead.

    Note if the index is beyond the size of the collection then the
    function will assert.

    @return
    true if the object was successfully added.
    */
    virtual PBoolean SetAt(
        PINDEX index,   ///< Index position in collection to set.
        D * obj         ///< New value to place into the collection.
        ) {
        return InternalSetAt((unsigned)index, obj);
    }

    /**Set the object at the specified ordinal position to the new value. This
    will overwrite the existing entry. If the AllowDeleteObjects option is
    set then the old object is also deleted.

    Note if the index is beyond the size of the collection then the
    function will assert.

    @return
    true if the object was successfully replaced.
    */
    virtual PBoolean ReplaceAt(
        PINDEX index,   ///< Index position in collection to set.
        D * obj         ///< New value to place into the collection.
        ) {
        return InternalSetAt((unsigned)index, obj, true);
    }

    /**Get the object at the specified ordinal position. If the index was
    greater than the size of the collection then NULL is returned.

    The object accessed in this way is remembered by the class and further
    access will be fast. Access to elements one either side of that saved
    element, and the head and tail of the list, will always be fast.

    @return
    pointer to object at the specified index.
    */
    virtual D * GetAt(
        PINDEX index  ///< Index position in the collection of the object.
        ) const {
        return InternalAt((unsigned)index);
    }


    D & operator[](PINDEX i) const { return InternalGetAt((unsigned)i); }

    /**Search the collection for the specific instance of the object. The
    object pointers are compared, not the values. A simple linear search
    from "head" of the list is performed.

    @return
    ordinal index position of the object, or P_MAX_INDEX.
    */
    virtual PINDEX GetObjectsIndex(
        const D * obj  ///< Object to find.
        ) const {
        unsigned index = 0;
        if (InternalFindIndex(index, obj))
            return index;
        else
            return P_MAX_INDEX;
    }

    /**Search the collection for the specified value of the object. The object
    values are compared, not the pointers.  So the objects in the
    collection must correctly implement the <code>PObject::Compare()</code>
    function. A simple linear search from "head" of the list is performed.

    @return
    ordinal index position of the object, or P_MAX_INDEX.
    */
    virtual PINDEX GetValuesIndex(
        const D & obj  ///< Object to find value of.
        ) const {
        unsigned index = 0;
        if (InternalIndex(index, obj))
            return index;
        else
            return P_MAX_INDEX;
    }
    //@}


    PINDEX GetSize() const { return this->size(); }

    PBoolean IsEmpty() const { return (this->size() == 0); }

    /**Remove all of the elements in the collection. This operates by
    continually calling <code>RemoveAt()</code> until there are no objects left.

    The objects are removed from the last, at index
    (GetSize()-1) toward the first at index zero.
    */
    virtual void RemoveAll()
    {
        //PWaitAndSignal m(dictMutex);

        if (IsEmpty()) return;

        if (!disallowDeleteObjects)
            deleteListEntries(*this);
        this->clear();
    }

    void SetSize(PINDEX i)
    {
        if (i == 0) RemoveAll();
    }

    PINLINE void AllowDeleteObjects(
        PBoolean yes = true   ///< New value for flag for deleting objects
        ) {
        disallowDeleteObjects = !yes;
    }

    /**Disallow the deletion of the objects contained in the collection. See
    the <code>AllowDeleteObjects()</code> function for more details.
    */
    void DisallowDeleteObjects() { disallowDeleteObjects = true; }
    //@}

protected:

    PBoolean  disallowDeleteObjects;
    PMutex    dictMutex;

    PBoolean InternalFindIndex(
        unsigned & ref,       ///< Returned index
        const D * data        ///< Data to match
        ) const
    {
        PWaitAndSignal m(dictMutex);

        if (data == NULL)
            return false;

        typename std::map< unsigned, D*, PSTLSortOrder>::const_iterator i;
        for (i = this->begin(); i != this->end(); ++i) {
            if (i->second == data) {
                ref = i->first;
                return true;
            }
        }
        return false;
    };

    PBoolean InternalIndex(
        unsigned & ref,       ///< Returned index
        const D & data        ///< Data to match
        ) const
    {
        PWaitAndSignal m(dictMutex);

        typename std::map< unsigned, D*, PSTLSortOrder>::const_iterator i;
        for (i = this->begin(); i != this->end(); ++i) {
            if (*(i->second) == data) {
                ref = i->first;
                return true;
            }
        }
        return false;
    };


    D & InternalGetAt(
        unsigned ref        ///< Returned index
        ) const
    {
        PWaitAndSignal m(dictMutex);

        PAssert(ref < this->size(), psprintf("Index out of Bounds ref: %u sz: %u", ref, this->size()));
        typename std::map< unsigned, D*, PSTLSortOrder>::const_iterator i = this->find(ref);
        PAssert(i != this->end(), psprintf("Index not found: %u sz: %u", ref, this->size()));
        return *(i->second);
    };

    D * InternalAt(
        unsigned ref        ///< Returned index
        ) const
    {
        PWaitAndSignal m(dictMutex);

        PAssert(ref < this->size(), psprintf("Index out of Bounds ref: %u sz: %u", ref, this->size()));
        typename std::map< unsigned, D*, PSTLSortOrder>::const_iterator i = this->find(ref);
        if (i != this->end()) return i->second;
        else return NULL;
    };


    D * InternalRemoveResort(unsigned pos) {
        unsigned newpos = pos;
        unsigned sz = (unsigned)this->size();
        D * dataPtr = NULL;
        typename std::map< unsigned, D*, PSTLSortOrder >::iterator it = this->find(pos);
        if (it == this->end()) return NULL;
        if (disallowDeleteObjects)
            dataPtr = it->second;
        else
            delete it->second;
        this->erase(it);

        for (unsigned i = pos + 1; i < sz; ++i) {
            typename std::map< unsigned, D*, PSTLSortOrder >::iterator j = this->find(i);
            PAssert(j != this->end(), psprintf("Index not found: %u sz: %u", i, this->size()));
            D* entry = j->second;
            this->insert(std::pair<unsigned, D*>(newpos, entry));
            newpos++;
            this->erase(j);
        }

        return dataPtr;
    };

    D * InternalRemoveKey(
        unsigned pos   ///< Key to look for in the dictionary.
        )
    {
        PWaitAndSignal m(dictMutex);

        return InternalRemoveResort(pos);
    }

    PINDEX InternalAddKey(
        D * obj         // New object to put into list.
        )
    {
        PWaitAndSignal m(dictMutex);

        if (obj == NULL)
            return -1;

        unsigned pos = (unsigned)this->size();
        this->insert(std::pair<unsigned, D*>(pos, obj));
        return pos;
    }

    PINDEX InternalSetAt(
        unsigned ref,                ///< Index position in collection to set.
        D * obj,                     ///< New value to place into the collection.
        PBoolean replace = false,
        PBoolean reorder = false
        )
    {
        if (obj == NULL)
            return -1;

        if (ref >= (unsigned)GetSize())
            return InternalAddKey(obj);

        PWaitAndSignal m(dictMutex);

        if (!reorder) {
            typename std::map< unsigned, D*, PSTLSortOrder>::iterator it = this->find(ref);
            if (it != this->end()) {
                if (replace)
                    delete it->second;
                this->erase(it);
            }
        }
        else {
            unsigned sz = (unsigned)GetSize();
            if (sz > 0) {
                unsigned newpos = sz;
                for (unsigned i = sz; i-- > ref; ) {
                    typename std::map< unsigned, D*, PSTLSortOrder >::iterator it = this->find(i);
                    if (it != this->end()) {
                        D* entry = it->second;
                        this->insert(std::pair<unsigned, D*>(newpos, entry));
                        this->erase(it);
                        newpos--;
                    }
                }
            }
        }
        this->insert(std::pair<unsigned, D*>(ref, obj));
        return ref;
    }
};

#define H323List  PSTLList
#define H323LIST(cls, D) typedef H323List<D> cls;

#endif  // H323_STLDICTIONARY


#define H323_DECLARELIST(cls, T) \
  H323LIST(cls##_PTemplate, T); \
  PDECLARE_CLASS(cls, H323List<T>) \
  protected: \
    cls(int dummy, const cls * c) \
      : H323List<T>(dummy, c) { } \
  public: \
    cls() \
      : H323List<T>() { } \
    virtual PObject * Clone() const \
      { return PNEW cls(0, this); } 




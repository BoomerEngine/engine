/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\view #]
***/

#pragma once

#include "rttiTypeRef.h"

BEGIN_BOOMER_NAMESPACE_EX(rtti)

///---

/// generic holder for RTTI-typed data, "proto variant"
/// NOTE: this class always allocate memory for storage and can't be serialized, use Variant if possible
class CORE_OBJECT_API DataHolder
{
public:
    INLINE DataHolder() {};
    INLINE DataHolder(std::nullptr_t) {};
    DataHolder(Type type, const void* copyFrom = nullptr);
    DataHolder(const DataHolder& other);
    DataHolder(DataHolder&& other);
    ~DataHolder();

    // assign, uses type->copy() of the rtti type
    DataHolder& operator=(const DataHolder& other);
    DataHolder& operator=(DataHolder&& other);

    // compare, uses type->compare()
    bool operator==(const DataHolder& other) const;
    bool operator!=(const DataHolder& other) const;

    ///---

    /// empty ?
    INLINE bool empty() const { return !m_data; }

    /// valid ?
    INLINE operator bool() const { return m_data; }

    /// type of stored data
    INLINE Type type() const { return m_type; }

    /// pointer to stored data
    INLINE const void* data() const { return m_data; }

    /// pointer to stored data (for modifications)
    INLINE void* data() { return m_data; }

    ///--

    // print (value only)
    void print(IFormatStream& f) const;

    // print (debug info - value + type)
    void printWithDebugInfo(IFormatStream& f) const;

    // get a string representation of data
    StringBuf toString() const;

    //--

    // relase all data
    void reset();

    // try to write data, uses type conversion
    bool writeData(const void* fromData, Type type);

    // try to write data, uses type conversion
    bool readData(void* toData, Type type);

    //--

    // try to "cast" to a different type, uses internal casting methods
    // stuff that works: numerical types, to/from string, stringID, casting between shared ptrs, etc
    DataHolder cast(Type type) const;

    //--

    // try to parse data of given type from a string, if successful data is put into data holder
    DataHolder Parse(StringView txt, Type type);

private:
    Type m_type;
    void* m_data = nullptr;

    void init(Type type, const void* data);
};

///---

END_BOOMER_NAMESPACE_EX(rtti)

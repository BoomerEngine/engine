/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types #]
***/

#pragma once

#include "rttiType.h"
#include "core/containers/include/array.h"
#include "core/containers/include/stringID.h"

BEGIN_BOOMER_NAMESPACE()

/// Collection of named values
class CORE_OBJECT_API EnumType : public IType
{
public:
    EnumType(StringID name, uint32_t size, uint64_t nativeHash, bool scripted = false);

    /// get list of enumeration options
    typedef Array< StringID > TNames;
    INLINE const TNames& options() const { return m_options; }

    /// get list of enumeration values
    typedef Array< int64_t > TValues;
    INLINE const TValues& values() const { return m_values; }

    /// get miniumum numerical value in the enum
    INLINE int64_t minValue() const { return m_minValue; }

    /// get maximum numerical value in the enum
    INLINE int64_t maxValue() const { return m_maxValue; }

    /// remove all options
    void clear();

    /// add an option to the enum
    void add(StringID name, int64_t value);

    /// find a numerical value for an option with given name
    /// returns false if matching option was not found
    bool findValue(StringID name, int64_t& outValue) const;

    /// find name for option with given value
    /// returns false if matching option was not found
    bool findName(int64_t value, StringID& outName) const;

    //---

    /// convert enum value to string
    bool toString(const void* value, StringBuf& outName) const;

    /// convert enum value to string ID (faster)
    bool toStringID(const void* value, StringID& outNam) const;

    /// convert enum value to numerical value
    bool toNumber(const void* value, int64_t& outNumber) const;

    /// convert string to enum value
    bool fromString(const StringBuf& name, void* outValue) const;

    /// convert enum value to string ID (faster)
    bool fromStringID(StringID name, void* outValue) const;

    /// convert enum value to numerical value
    bool fromNumber(int64_t number, void* outValue) const;

    //---

    // IType interface
    virtual void construct(void *mem) const override final {};
    virtual void destruct(void *mem) const override final {};

    virtual bool compare(const void* data1, const void* data2) const override final;
    virtual void copy(void* dest, const void* src) const override final;

    virtual void writeBinary(TypeSerializationContext& typeContext, SerializationWriter& file, const void* data, const void* defaultData) const override final;
    virtual void readBinary(TypeSerializationContext& typeContext, SerializationReader& file, void* data) const override final;

    virtual void writeXML(TypeSerializationContext& typeContext, xml::Node& node, const void* data, const void* defaultData) const override final;
    virtual void readXML(TypeSerializationContext& typeContext, const xml::Node& node, void* data) const override final;

    virtual void printToText(IFormatStream& f, const void* data, uint32_t flags = 0) const override final;
    virtual bool parseFromString(StringView txt, void* data, uint32_t flags = 0) const override final;

    virtual DataViewResult describeDataView(StringView viewPath, const void* viewData, DataViewInfo& outInfo) const override final;
    virtual DataViewResult readDataView(StringView viewPath, const void* viewData, void* targetData, Type targetType) const override final;
    virtual DataViewResult writeDataView(StringView viewPath, void* viewData, const void* sourceData, Type sourceType) const override final;

private:
    TNames m_options;
    TValues m_values;

    int64_t m_minValue;
    int64_t m_maxValue;

    void readInt64(const void* data, int64_t& outValue) const;
    void writeInt64(void* data, int64_t value) const;
};

END_BOOMER_NAMESPACE()

/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: reflection #]
***/

#pragma once

#include "reflectionTypeName.h"

#include "base/object/include/rttiBitfieldType.h"

BEGIN_BOOMER_NAMESPACE(base::reflection)

// helper class that can add stuff to the bitfield type
class BASE_REFLECTION_API BitFieldBuilder
{
public:
    BitFieldBuilder(rtti::BitfieldType* enumPtr, uint32_t flagsType);
    ~BitFieldBuilder();

    // apply changes to target class
    // this atomically sets up the base class and the properties
    void submit();

    // add bitfield option
    void addRawOption(const char* rawName, uint64_t value, const char* hint = nullptr);

    // add typed value option
    template< typename T>
    INLINE void addOption(const char* rawName, T value, const char* hint = nullptr)
    {
        addRawOption(rawName, (uint64_t)value, hint);
    }

    void addOldName(const char* oldName);

private:
    struct Option
    {
        StringID m_name;
        uint64_t m_value;
        StringBuf m_hint;
    };

    typedef Array<Option> TOptions;
    TOptions m_options;

    uint32_t m_flagsType;

    rtti::BitfieldType* m_bitFieldType;

    base::Array<base::StringID> m_oldNames;
};

END_BOOMER_NAMESPACE(base::reflection)

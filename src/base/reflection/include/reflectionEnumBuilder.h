/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: reflection #]
***/

#pragma once

#include "reflectionTypeName.h"

#include "base/object/include/rttiEnumType.h"

BEGIN_BOOMER_NAMESPACE(base::reflection)

// helper class that can add stuff to the enum type
class BASE_REFLECTION_API EnumBuilder
{
public:
    EnumBuilder(rtti::EnumType* enumPtr);
    ~EnumBuilder();

    // apply changes to target class
    // this atomically sets up the base class and the properties
    void submit();

    // add enum option
    void addOption(const char* rawName, int64_t value, const char* hint = nullptr);

    // add typed value option
    template< typename T>
    INLINE void addOption(const char* rawName, T value, const char* hint = nullptr)
    {
        addOption(rawName, (int64_t)value, hint);
    }

    void addOldName(const char* oldName);

private:
    struct Option
    {
        StringID m_name;
        int64_t m_value;
        StringBuf m_hint;
    };

    typedef Array<Option> TOptions;
    TOptions m_options;

    rtti::EnumType* m_enumType;

    base::Array<base::StringID> m_oldNames;
};

END_BOOMER_NAMESPACE(base::reflection)
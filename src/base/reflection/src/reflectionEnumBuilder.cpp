/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: reflection #]
***/

#include "build.h"
#include "reflectionEnumBuilder.h" 

#include "base/object/include/rttiEnumType.h"

BEGIN_BOOMER_NAMESPACE(base::reflection)

EnumBuilder::EnumBuilder(rtti::EnumType* enumPtr)
    : m_enumType(enumPtr)
{}

EnumBuilder::~EnumBuilder()
{}

void EnumBuilder::submit()
{
    for (auto& opt : m_options)
        m_enumType->add(opt.m_name, opt.m_value);

    for (auto& oldName : m_oldNames)
        RTTI::GetInstance().registerAlternativeTypeName(m_enumType, oldName);
}

void EnumBuilder::addOldName(const char* oldName)
{
    m_oldNames.pushBackUnique(base::StringID(oldName));
}

void EnumBuilder::addOption(const char* rawName, int64_t value, const char* hint)
{
    StringID name(rawName);

    if (!name.empty())
    {
        for (auto& opt : m_options)
        {
            if (opt.m_name == name)
            {
                opt.m_value = value;
                return;
            }
        }

        Option info;
        info.m_name = name;
        info.m_value = value;
        info.m_hint = StringBuf(hint);
        m_options.pushBack(info);
    }
}

END_BOOMER_NAMESPACE(base::reflection)

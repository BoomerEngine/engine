/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: reflection #]
***/

#include "build.h"
#include "reflectionBitFieldBuilder.h" 

#include "core/object/include/rttiBitfieldType.h"

BEGIN_BOOMER_NAMESPACE()

BitFieldBuilder::BitFieldBuilder(BitfieldType* enumPtr, uint32_t flagsType)
    : m_bitFieldType(enumPtr)
    , m_flagsType(flagsType)
{
    ASSERT(m_bitFieldType->size() <= 8);
    ASSERT(flagsType == 1 || flagsType == 2);
}

BitFieldBuilder::~BitFieldBuilder()
{}

void BitFieldBuilder::submit()
{
    for (auto& opt : m_options)
        m_bitFieldType->add(opt.m_name, opt.m_value);

    for (auto& oldName : m_oldNames)
        RTTI::GetInstance().registerAlternativeTypeName(m_bitFieldType, oldName);
}

void BitFieldBuilder::addOldName(const char* oldName)
{
    m_oldNames.pushBackUnique(StringID(oldName));
}

void BitFieldBuilder::addRawOption(const char* rawName, uint64_t value, const char* hint)
{
    uint64_t bitValue = 0;

    if (m_flagsType == 1)
    {
        bitValue = value;
        ASSERT_EX((value & (value-1)) == 0, "Bit field mask value should be a power of two (single bit)");
    }
    else if (m_flagsType == 2)
    {
        bitValue = 1ULL << value;
    }

    auto lastBitValue  = 1ULL << ((8 * m_bitFieldType->size()) - 1);
    ASSERT(bitValue <= lastBitValue);

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

        for (auto& opt : m_options)
        {
            ASSERT((opt.m_value & bitValue) == 0);
        }

        Option info;
        info.m_name = name;
        info.m_value = bitValue;
        info.m_hint = StringBuf(hint);
        m_options.pushBack(info);
    }
}

END_BOOMER_NAMESPACE()

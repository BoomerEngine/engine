/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: variant #]
***/

#include "build.h"
#include "variantTable.h"

BEGIN_BOOMER_NAMESPACE()

//---

RTTI_BEGIN_TYPE_CLASS(VariantTableEntry);
    RTTI_PROPERTY(name);
    RTTI_PROPERTY(data);
RTTI_END_TYPE();

//---

RTTI_BEGIN_TYPE_CLASS(VariantTable);
    RTTI_PROPERTY(m_parameters);
RTTI_END_TYPE();

VariantTable::VariantTable()
{}

void VariantTable::clear()
{
    m_parameters.reset();
}

bool VariantTable::contains(StringID name) const
{
    DEBUG_CHECK(name);
    for (const auto& entry : m_parameters)
        if (entry.name == name)
            return true;
    return false;
}

bool VariantTable::remove(StringID name)
{
    DEBUG_CHECK(name);

    uint16_t index = 0;
    while (index < m_parameters.size())
    {
        if (m_parameters[index].name == name)
        {
            m_parameters.eraseUnordered(index);
            return true;
        }

        index += 1;
    }

    return false;
}

void VariantTable::setVariant(StringID name, Variant&& value)
{
    DEBUG_CHECK(name);

    for (auto& entry : m_parameters)
    {
        if (entry.name == name)
        {
            entry.data = std::move(value);
            return;
        }
    }

    auto& entry = m_parameters.emplaceBack();
    entry.name = name;
    entry.data = std::move(value);
}

void VariantTable::setVariant(StringID name, const Variant& value)
{
    DEBUG_CHECK(name);

    for (auto& entry : m_parameters)
    {
        if (entry.name == name)
        {
            entry.data = value;
            return;
        }
    }

    auto& entry = m_parameters.emplaceBack();
    entry.name = name;
    entry.data = value;
}

const Variant& VariantTable::getVariant(StringID name) const
{
    for (const auto& entry : m_parameters)
        if (entry.name == name)
            return entry.data;

    return Variant::EMPTY();
}

const Variant* VariantTable::findVariant(StringID name) const
{
    for (const auto& entry : m_parameters)
        if (entry.name == name)
            return &entry.data;

    return nullptr;
}

void VariantTable::apply(const VariantTable& otherTable)
{
    for (auto& entry : otherTable.m_parameters)
        setVariant(entry.name, entry.data);
}

void VariantTable::print(IFormatStream& f, bool oneline) const
{
    if (oneline)
    {
        bool first = false;

        for (auto& entry : m_parameters)
        {
            if (!first)
                f << ", ";
            f.appendf("{}='{}'", entry.name, entry.data);
        }
    }
    else
    {
        for (auto& entry : m_parameters)
            f.appendf("{}='{}'\n", entry.name, entry.data);
    }

}

//---

END_BOOMER_NAMESPACE()

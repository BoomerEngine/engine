/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements #]
***/

#include "build.h"
#include "uiElementStyle.h"
#include "uiElement.h"
#include "uiDataStash.h"
#include "uiStyleValue.h"

BEGIN_BOOMER_NAMESPACE(ui) 

StyleStack::StyleStack(DataStash& stash)
    : m_stash(stash)
{
    m_styleLibraries.pushBack(stash.styles());
}

static void CalcParamCRC(base::Type type, const void* data, base::CRC64& crc)
{
    static const auto fontFamily = base::reflection::GetTypeObject<ui::style::FontFamily>();
    static const auto imageReference = base::reflection::GetTypeObject<ui::style::ImageReference>();
    static const auto renderStyle = base::reflection::GetTypeObject<ui::style::RenderStyle>();
    static const auto stringBuf = base::reflection::GetTypeObject<base::StringBuf>();
    static const auto stringId = base::reflection::GetTypeObject<base::StringID>();
       
    if (type == fontFamily)
    {
        crc << ((const ui::style::FontFamily*)(data))->hash();
    }
    else if (type == imageReference)
    {
        crc << ((const ui::style::ImageReference*)(data))->hash();
    }
    else if (type == renderStyle)
    {
        crc << ((const ui::style::RenderStyle *)(data))->hash();
    }
    else if (type == stringBuf)
    {
        crc << ((const base::StringBuf*)(data))->cRC32();
    }
    else if (type == stringId)
    {
        crc << ((const base::StringID*)(data))->index();
    }
    else
    {
        crc.append(data, type->traits().size);
        //  ASSERT_EX(false, base::TempString("Unsupported UI data type: '{}'", type));
    }
}

static void CalcParamCRC(const base::VariantTable& table, base::CRC64& crc)
{
    crc << table.parameters().size();
    for (const auto& entry : table.parameters())
    {
        crc << entry.name.index();
        crc << entry.data.type().ptr();
        CalcParamCRC(entry.data.type(), entry.data.data(), crc);
    }
}

bool StyleStack::buildTable(uint64_t& outTableKey, ParamTablePtr& outTableData) const
{
    //static ParamTablePtr EmptyParamTable = base::RefNew<style::ParamTable>();

    base::InplaceArray<ParamTablePtr, 4> collectedTables;

    // compile parameter table for each registered library
    uint32_t libIndex = 0;
    uint32_t libStride = m_styleLibraries.size();
    uint32_t selectorCount = m_stack.size();
    for (auto* styles : m_styleLibraries)
    {
        if (auto params = styles->compileParamTable(m_key, m_matches.typedData() + libIndex, libStride, selectorCount))
        {
            if (params && !params->values.empty())
            {
                collectedTables.pushBack(params);
                //TRACE_INFO("Styles from '{}': {}", styles->path(), *params);
            }
        }
        libIndex += 1;
    }

    // calculate hash of the generated parameter set
    // TODO: could be better - here we don't take into account that some styles may get overridden
    base::CRC64 tableKey;
    for (const auto& table : collectedTables)
        CalcParamCRC(table->values, tableKey);

    // easy case - we already have data
    if (outTableKey == tableKey.crc())
    {
        DEBUG_CHECK(outTableData);
        return false; // nothing changed
    }

    // no table
    if (collectedTables.empty())
    {
        outTableData = base::RefNew<style::ParamTable>();
    }
    else if (collectedTables.size() == 1)
    {
        outTableData = collectedTables[0];
    }
    else
    {
        // TODO
        DEBUG_CHECK(!"TODO");
    }

    // data changed
    outTableKey = tableKey.crc();
    return true;
}

/*ParamTablePtr StyleStack::resolveStyle() const
{
        
}*/

void StyleStack::push(const style::SelectorMatchContext* selectors)
{
    if (selectors)
    {
        auto& entry = m_stack.emplaceBack();
        entry.selector = selectors;
        entry.prevKey = m_key;
            
        for (const auto* lib : m_styleLibraries)
        {
            const auto* match = lib->matchSelectors(*selectors);
            m_matches.emplaceBack(match);
        }

        base::CRC64 crc;
        crc << m_key;
        crc << selectors->key();
        m_key = crc;
    }
}

void StyleStack::pop(const style::SelectorMatchContext* selectors)
{
    if (selectors)
    {
        ASSERT(!m_stack.empty());

        const auto& back = m_stack.back();
        ASSERT(back.selector == selectors);
        m_key = back.prevKey;

        for (const auto* lib : m_styleLibraries)
            m_matches.popBack();

        m_stack.popBack();
    }
}

END_BOOMER_NAMESPACE(ui)

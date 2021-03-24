/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: styles\compiler #]
***/

#include "build.h"
#include "uiStyleLibraryTree.h"
#include "core/containers/include/stringBuilder.h"
#include "core/memory/include/linearAllocator.h"

BEGIN_BOOMER_NAMESPACE_EX(ui::style)

namespace prv
{

    //--

    RawBaseElement::RawBaseElement(const TextTokenLocation& location)
        : m_fileLocation(location)
    {}

    //---

    RawProperty::RawProperty(const TextTokenLocation& location, const StringID name, uint32_t numValues, const RawValue** values)
        : RawBaseElement(location)
        , m_name(name)
        , m_values(values)
        , m_numValues(numValues)
    {
        DEBUG_CHECK(name);
        DEBUG_CHECK(values != nullptr);
        DEBUG_CHECK(m_numValues > 0);
    }

    void RawProperty::print(IFormatStream& f) const
    {
        f << m_name;
        f << ": ";

        for (uint32_t i = 0; i < m_numValues; ++i)
        {
            if (i > 0)
                f.append(" ");
            m_values[i]->print(f);
        }

        f.append(";");
    }

    //---

    RawVariable::RawVariable(const TextTokenLocation& location, const StringID name, const RawValue* value)
        : RawBaseElement(location)
        , m_name(name)
        , m_value(value)
    {
        DEBUG_CHECK(name);
        DEBUG_CHECK(value != nullptr);
    }

    void RawVariable::print(IFormatStream& f) const
    {
        f << "$" << m_name;
        f << ": ";
        m_value->print(f);
        f << ";";
    }

    //---

    RawMatchList::RawMatchList()
    {}

    void RawMatchList::add(const StringID id)
    {
        ASSERT(!id.empty());
        m_entries.pushBackUnique(id);
    }

    void RawMatchList::add(const char* txt)
    {
        add(StringID(txt));
    }

    void RawMatchList::merge(const RawMatchList& other)
    {
        for (const auto& entry : other.m_entries)
            add(entry);
    }

    //---

    RawSelector::RawSelector(const TextTokenLocation& location, SelectorCombinatorType combinator)
        : RawBaseElement(location)
        , m_combinator(combinator)
    {}

    void RawSelector::print(IFormatStream& ret) const
    {
        switch (m_combinator)
        {
            case SelectorCombinatorType::AnyParent: ret.append(" "); break;
            case SelectorCombinatorType::DirectParent: ret.append(" > "); break;
            case SelectorCombinatorType::GeneralSibling: ret.append(" ~ "); break;
            case SelectorCombinatorType::AdjecentSibling: ret.append(" + "); break;
        }

        if (m_types.empty())
        {
            ret.append("*");
        }
        else if (m_types.entries().size() == 1)
        {
            ret.append(m_types.entries()[0].c_str());
        }
        else
        {
            ret.append("(");
            for (uint32_t i = 0; i < m_types.entries().size(); ++i)
            {
                if (i > 0)
                    ret.append("|");
                ret.append(m_types.entries()[i].c_str());
            }

            ret.append(")");
        }

        for (const auto& clas : m_classes.entries())
        {
            ret.append(".");
            ret.append(clas.c_str());
        }

        for (const auto& id : m_ids.entries())
        {
            ret.append("#");
            ret.append(id.c_str());
        }

        for (const auto& pseudo : m_pseudoClasses.entries())
        {
            ret.append(":");
            ret.append(pseudo.c_str());
        }
    }

    //---

    RawRule::RawRule(const TextTokenLocation& location)
        : RawBaseElement(location)
    {}

    void RawRule::addSelector(const RawSelector* selector)
    {
        ASSERT(selector != nullptr);
        m_selectors.pushBack(selector);
    }

    void RawRule::replaceTopSelector(const RawSelector* selector)
    {
        ASSERT(selector != nullptr);
        ASSERT(!m_selectors.empty());
        m_selectors.popBack();
        m_selectors.pushBack(selector);
    }

    void RawRule::print(IFormatStream& f) const
    {
        for (const auto* selector : m_selectors)
            selector->print(f);
    }

    //---

    RawRuleSet::RawRuleSet(const TextTokenLocation& location)
        : RawBaseElement(location)
    {}

    void RawRuleSet::addRule(const RawRule* rule)
    {
        ASSERT(rule != nullptr);
        m_rules.pushBack(rule);
    }

    void RawRuleSet::addProperty(const RawProperty* prop)
    {
        ASSERT(prop != nullptr);

        for (uint32_t i = 0; i < m_properties.size(); ++i)
        {
            if (m_properties[i]->name() == prop->name())
            {
                m_properties.erase(i);
                break;
            }
        }

        m_properties.pushBack(prop);
    }

    void RawRuleSet::print(IFormatStream& f) const
    {
        for (uint32_t i = 0; i < m_rules.size(); ++i)
        {
            const auto* rule = m_rules[i];
            if (i > 0)
                f.append(",");
            rule->print(f);
        }

        f.append(" {\n");

        for (const auto* prop : m_properties)
        {
            f.append("    ");
            prop->print(f);
            f.append("\n");
        }

        f.append("}\n");
    }

    //---

    RawLibraryData::RawLibraryData(LinearAllocator& mem)
        : m_mem(mem)
    {}

    void RawLibraryData::addRuleSet(const RawRuleSet* ruleSet)
    {
        DEBUG_CHECK(ruleSet != nullptr);
        if (ruleSet != nullptr)
            m_ruleSets.pushBack(ruleSet);
    }

    void RawLibraryData::addVariable(const RawVariable* variable)
    {
        DEBUG_CHECK(variable != nullptr);
        DEBUG_CHECK(nullptr == findVariable(variable->name()));
        m_variables.pushBack(variable);
        m_variableMap[variable->name()] = variable;
        //TRACE_INFO("Registered variable '{}'", variable->name());
    }

    const RawVariable* RawLibraryData::findVariable(StringID name)
    {
        const RawVariable* ret = nullptr;
        m_variableMap.find(name, ret);
        return ret;
    }

    void RawLibraryData::print(IFormatStream& f) const
    {
        for (const auto* ruleSet : m_ruleSets)
        {
            ruleSet->print(f);
            f.append("\n");
        }
    }

    const char* RawLibraryData::allocString(StringView str)
    {
        if (!str)
            return "";

        return m_mem.strcpy(str.data(), str.length());
    }

    //---

} // prv

END_BOOMER_NAMESPACE_EX(ui::style)

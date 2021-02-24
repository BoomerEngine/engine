/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: styles\compiler #]
***/

#include "build.h"
#include "uiStyleLibraryPacker.h"
#include "uiStyleLibraryTree.h"

#include "base/font/include/font.h"
#include "uiStyleValue.h"
#include "base/image/include/image.h"

BEGIN_BOOMER_NAMESPACE(ui::style)

//---

ContentLoader::ContentLoader(base::StringView directoryPath)
    : m_baseDirectory(directoryPath)
{
}

base::image::ImagePtr ContentLoader::loadImage(base::StringView imageFileName)
{
    auto dir = m_baseDirectory;
    for (uint32_t i = 0; i < 2; ++i)
    {
        if (auto image = base::LoadImageFromDepotPath(base::TempString("{}{}", dir, imageFileName)))
            return image;

        dir = dir.parentDirectory();
    }

    return nullptr;
}

base::FontPtr ContentLoader::loadFont(base::StringView fontFileName)
{
    auto dir = m_baseDirectory;
    for (uint32_t i = 0; i < 2; ++i)
    {
        if (auto image = base::LoadFontFromDepotPath(base::TempString("{}{}", dir, fontFileName)))
            return image;

        dir = dir.parentDirectory();
    }

    return nullptr;
}

//--

namespace prv
{
    //---

    RawValueTable::RawValueTable(ContentLoader& resourceResolver)
        : m_resourceResolver(resourceResolver)
    {}

    static uint64_t CalcParamHash(base::StringID paramId, const RawValue& value)
    {
        base::CRC64 crc;
        crc << paramId;
        value.calcHash(crc);
        return crc.crc();
    }

    int RawValueTable::resolveAndMap(const base::parser::Location& loc, base::StringID paramId, const RawValue& value, base::parser::IErrorReporter& err)
    {
        // get name of the parameter
        auto hash = CalcParamHash(paramId, value);
        int index = 0;
        if (m_valueMap.find(hash, index))
            return index;

        // get target type
        auto styleType = StyleVarTypeForParamName(paramId);

        // parse value
        base::Variant resolvedValue;
        if (!CompileStyleValue(loc, styleType, value, m_resourceResolver, resolvedValue, err))
        {
            err.reportError(loc, base::TempString("Unable to resolve value for '{}', type or content may be invalid", paramId));
            return -1;
        }

        //TRACE_INFO("Resolved value for '{}': '{}'", paramId, resolvedValue);

        // store in map
        index = range_cast<int>(m_resolvedValues.size());
        m_resolvedValues.pushBack(resolvedValue);
        m_valueMap.set(hash, index);
        return index;
    }

    //---

    RawSelectorNode::RawSelectorNode(SelectorID parent, const SelectorMatchParams& matchParams, SelectorCombinatorType combinator)
        : m_parentId(parent)
        , m_matchParameters(matchParams)
        , m_combinator(combinator)
    {}

    void RawSelectorNode::addParameter(const SelectorParam& param)
    {
        for (auto& existingParam : m_parameters)
        {
            if (existingParam.paramName() == param.paramName())
            {
                existingParam = param;
                return;
            }
        }

        m_parameters.pushBack(param);
    }

    //---

    RawSelectorTree::RawSelectorTree(RawValueTable& valueTable)
        : m_values(valueTable)
    {
        m_selectorNodes.reserve(65535);
    }

    uint64_t RawSelectorTree::SelectorKey::calcHash() const
    {
        base::CRC64 crc;
        crc << (uint8_t)m_type;
        crc << m_parent;
        m_params.calcHash(crc);
        return crc.crc();
    }

    SelectorID RawSelectorTree::mapSelectorNode(const SelectorKey& key)
    {
        auto hash = key.calcHash();

        // use existing node
        SelectorID id = 0;
        if (m_selectorMap.find(hash, id))
            return id;

        // create new node
        id = range_cast<SelectorID>(m_selectorNodes.size());
        m_selectorNodes.pushBack(RawSelectorNode(key.m_parent, key.m_params, key.m_type));
        m_selectorMap.set(hash, id);
        return id;
    }


    void RawSelectorTree::mapSelectorParams(const RawSelector* selector, base::Array<SelectorMatchParams>& outParams, base::parser::IErrorReporter& err)
    {
        // get possible types at this selector
        auto typeEntries = selector->types().entries();
        if (typeEntries.empty())
            typeEntries.pushBack(base::StringID());

        // generate combinations
        for (const auto type : typeEntries)
        {
            base::StringID matchClass;
            base::StringID matchClass2;
            base::StringID matchPseudoClass;
            base::StringID matchPseudoClass2;
            base::StringID matchId;

            if (selector->classes().entries().size() > 2)
            {
                err.reportError(selector->location(), "Unable to map selector, exceeded class limit: 2");
                return;
            }

            if (selector->pseudoClasses().entries().size() > 2)
            {
                err.reportError(selector->location(), "Unable to map selector, exceeded pseudo class limit: 2");
                return;
            }

            if (selector->identifiers().entries().size() > 1)
            {
                err.reportError(selector->location(), "Unable to map selector, exceeded identifier limit: 2");
                return;
            }

            if (!selector->classes().empty())
            {
                matchClass = selector->classes().entries()[0];

                if (selector->classes().entries().size() >= 2)
                    matchClass2 = selector->classes().entries()[1];
            }

            if (!selector->pseudoClasses().empty())
            {
                matchPseudoClass = selector->pseudoClasses().entries()[0];

                if (selector->pseudoClasses().entries().size() >= 2)
                    matchPseudoClass2 = selector->pseudoClasses().entries()[1];
            }

            if (!selector->identifiers().empty())
                matchId = selector->identifiers().entries()[0];

            outParams.emplaceBack(type, matchClass, matchClass2, matchId, matchPseudoClass, matchPseudoClass2);
        }
    }

    void RawSelectorTree::mapRuleTrace(const base::Array<const RawSelector*>& selectors, uint32_t selectorIndex, SelectorID parentNodeId, base::Array<SelectorID>& outTerminalSelectors, base::parser::IErrorReporter& err)
    {
        // end of the road
        if (selectorIndex == selectors.size())
        { 
            outTerminalSelectors.pushBack(parentNodeId);
            return;
        }

        // generate parameter combinations for this selector
        const auto* selector = selectors[selectorIndex];
        base::Array<SelectorMatchParams> parameterCombinations;
        mapSelectorParams(selector, parameterCombinations, err);

        // create child nodes and recurse
        for (const auto& paramCombination : parameterCombinations)
        {
            // create child selector node
            SelectorKey key;
            key.m_parent = parentNodeId;
            key.m_type = selector->combinator();
            key.m_params = paramCombination;

            // create/get the selector node
            auto nodeID = mapSelectorNode(key);
            mapRuleTrace(selectors, selectorIndex + 1, nodeID, outTerminalSelectors, err);
        }
    }

    void RawSelectorTree::extractSelectorNodes(base::Array<SelectorNode>& outSelectorNodes) const
    {
        outSelectorNodes.reserve(m_selectorNodes.size());
        for (const auto& node : m_selectorNodes)
            outSelectorNodes.emplaceBack(node.parentId(), node.parameters(), node.matchParameters(), node.combinator());
    }

    static bool FindParametersInGroup(base::StringID name, base::Array<base::StringID>& outParams)
    {
        if (name == "margin"_id)
        {
            outParams.reserve(4);
            outParams.pushBack("margin-left"_id);
            outParams.pushBack("margin-top"_id);
            outParams.pushBack("margin-right"_id);
            outParams.pushBack("margin-bottom"_id);
            return true;
        }
        else if (name == "padding"_id)
        {
            outParams.reserve(4);
            outParams.pushBack("padding-left"_id);
            outParams.pushBack("padding-top"_id);
            outParams.pushBack("padding-right"_id);
            outParams.pushBack("padding-bottom"_id);
            return true;
        }
        else if (name == "border"_id)
        {
            outParams.reserve(4);
            outParams.pushBack("border-left"_id);
            outParams.pushBack("border-top"_id);
            outParams.pushBack("border-right"_id);
            outParams.pushBack("border-bottom"_id);
            return true;
        }
        else if (name == "image-scale"_id)
        {
            outParams.pushBack("image-scale-x"_id);
            outParams.pushBack("image-scale-y"_id);
        }

        return false;
    }

    bool RawSelectorTree::mapRuleSet(const RawRuleSet& ruleSet, base::parser::IErrorReporter& err)
    {
        bool valid = true;

        // map parameters
        base::InplaceArray<SelectorParam, 64> properties;
        for (const auto* prop : ruleSet.properties())
        { 
            // a group ?
            base::InplaceArray<base::StringID, 4> groupParameters;
            if (FindParametersInGroup(prop->name(), groupParameters))
            {
                // to many parameters
                if (prop->numValues() > groupParameters.size())
                {
                    err.reportWarning(prop->location(), base::TempString("To many parameter for '{}', expected {}, got {}", prop->name(), groupParameters.size(), prop->numValues()));
                    continue;
                }

                // process values
                for (uint32_t i=0; i<prop->numValues(); ++i)
                {
                    // find property
                    const auto* propValue = prop->values()[i];
                    auto paramId = groupParameters[i];

                    // map value
                    auto valueId = m_values.resolveAndMap(prop->location(), paramId, *propValue, err);
                    if (valueId == -1)
                    {
                        err.reportError(prop->location(), base::TempString("Unable to parse value for '{}' from '{}'", prop->name(), *propValue));
                        valid = false;
                    }

                    // add to list
                    properties.pushBack(SelectorParam(paramId, range_cast<ValueID>(valueId)));
                }
            }
            else
            {
                // map value
                ASSERT(prop->numValues() >= 1);
                const auto* propValue = prop->values()[0];
                auto valueId = m_values.resolveAndMap(prop->location(), prop->name(), *propValue, err);
                if (valueId == -1)
                {
                    err.reportError(prop->location(), base::TempString("Unable to parse value for '{}' from '{}'", prop->name(), *propValue));
                    valid = false;
                }

                // add to list
                properties.pushBack(SelectorParam(prop->name(), range_cast<ValueID>(valueId)));
            }
        }

        // exit on errors
        if (!valid)
            return false;

        // no parameters to map, skip the entry
        if (properties.empty())
            return true;

        // process the rules
        base::Array<SelectorID> terminalSelectors;
        for (const auto* rule : ruleSet.rules())
        {
            auto startSelectors = terminalSelectors.size();

            for (const auto* selector : rule->selectors())
                mapRuleTrace(rule->selectors(), 0, 0, terminalSelectors, err);

            auto numGenerated = terminalSelectors.size() - startSelectors;
            //TRACE_SPAM("Generated {} terminal selectors for rule '{}'", numGenerated, rule->toString().c_str());
        }

        // set parameters for all terminal selectors
        for (auto selectorId : terminalSelectors)
        {
            auto& selector = m_selectorNodes[selectorId];

            for (const auto& prop : properties)
                selector.addParameter(prop);
        }

        return true;
    }

} // prv

END_BOOMER_NAMESPACE(ui::style)

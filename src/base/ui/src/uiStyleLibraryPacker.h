 /***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: styles\compiler #]
***/

#pragma once

#include "uiStyleValue.h"
#include "uiStyleSelector.h"

#include "base/font/include/font.h"
#include "base/image/include/image.h"

BEGIN_BOOMER_NAMESPACE(ui::style)

///--

/// content loader for stuff used by the style library (images and fonts)
class ContentLoader : public base::NoCopy
{
public:
    ContentLoader(base::StringView directoryPath);

    base::image::ImagePtr loadImage(base::StringView imageFileName);
    base::FontPtr loadFont(base::StringView fontFileName);

private:
    base::StringView m_baseDirectory;
};

//--

namespace prv
{
    class RawSelector;
    class RawRuleSet;

    ///--

    /// packer for values, keeps only unique values
    /// NOTE: we are packing unresolved values, final resolve happens later
    class RawValueTable : public base::NoCopy
    {
    public:
        RawValueTable(ContentLoader& resourceResolver);

        /// get all packed values
        typedef base::Array<base::Variant> TResolvedValues;
        INLINE const TResolvedValues& values() const { return m_resolvedValues; }
        INLINE TResolvedValues& values() { return m_resolvedValues; }

        /// resolve and map value, returns savable ValueID or 0 if something went wrong
        int resolveAndMap(const base::parser::Location& loc, base::StringID paramId, const RawValue& value, base::parser::IErrorReporter& err);

    private:
        // mapping from original RawProperty hash to resolved value
        typedef base::HashMap<uint64_t, int> TValueMap;
        TValueMap m_valueMap;

        // all resolved (ready to save) values
        TResolvedValues m_resolvedValues;

        // resolver for referenced resources
        ContentLoader& m_resourceResolver;
    };

    ///--

    // selector nodes
    struct RawSelectorNode
    {
    public:
        RawSelectorNode(SelectorID parent, const SelectorMatchParams& matchParams, SelectorCombinatorType combinator);

        typedef base::Array<SelectorParam> TParameters;
        INLINE const TParameters& parameters() const { return m_parameters; }

        /// get the relation toward the parent node
        INLINE SelectorCombinatorType combinator() const { return m_combinator; }

        /// get ID of the parent node
        INLINE SelectorID parentId() const { return m_parentId; }

        /// get the match parameters for this selector
        INLINE const SelectorMatchParams& matchParameters() const { return m_matchParameters; }

        //--

        // add parameter to selector parameter table
        // if parameter with given name already exists its value is replaced
        void addParameter(const SelectorParam& param);

    private:
        SelectorID m_parentId;
        SelectorCombinatorType m_combinator;
        SelectorMatchParams m_matchParameters;
        TParameters m_parameters;
    };

    /// selector tree node
    class RawSelectorTree : public base::NoCopy
    {
    public:
        RawSelectorTree(RawValueTable& valueTable);

        // get final selector nodes
        void extractSelectorNodes(base::Array<SelectorNode>& outSelectorNodes) const;

        // insert rule set
        bool mapRuleSet(const RawRuleSet& ruleSet, base::parser::IErrorReporter& err);

    private:
        // mapped values
        RawValueTable& m_values;        

        // mapped selector
        struct SelectorKey
        {
            SelectorID m_parent;
            SelectorCombinatorType m_type;
            SelectorMatchParams m_params;

            INLINE SelectorKey()
                : m_parent(0)
                , m_type(SelectorCombinatorType::AnyParent)
            {}

            uint64_t calcHash() const;
        };

        // selector nodes
        typedef base::Array<RawSelectorNode> TSelectorNodes;
        TSelectorNodes m_selectorNodes;

        // selector table
        typedef base::HashMap<uint64_t, SelectorID> TSelectorMap;
        TSelectorMap m_selectorMap;

        // get entry for given selector key, may return existing node
        SelectorID mapSelectorNode(const SelectorKey& key);

        // get a selector match data
        void mapSelectorParams(const RawSelector* selector, base::Array<SelectorMatchParams>& outParams, base::parser::IErrorReporter& err);

        // map a rule set
        void mapRuleTrace(const base::Array<const RawSelector*>& selectors, uint32_t selectorIndex, SelectorID parentNodeId, base::Array<SelectorID>& outTerminalSelectors, base::parser::IErrorReporter& err);

    };

} // prv

END_BOOMER_NAMESPACE(ui::style)
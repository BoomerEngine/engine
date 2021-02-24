/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: styles #]
***/

#pragma once

#include "uiStyleSelector.h"

#include "base/resource/include/resource.h"
#include "base/system/include/spinLock.h"
#include "base/reflection/include/variant.h"

BEGIN_BOOMER_NAMESPACE(ui)

namespace style
{
    class SelectorMatchContext;

    //---

    /// library of styles, contains a parameters tables with selectors
    class BASE_UI_API Library : public base::res::IResource
    {
        RTTI_DECLARE_VIRTUAL_CLASS(Library, base::res::IResource);

    public:
        Library();
        Library(base::Array<SelectorNode>&& selectorNodes, base::Array<base::Variant>&& values);

        /// compile a list of matched selectors using partial selectors from this library and given evaluation context
        /// returned set contains all selectors that were matched
        /// NOTE: returned selectors are cached, do not delete them
        const SelectorMatch* matchSelectors(const SelectorMatchContext& context) const;

        /// compile a flat parameter table using the master element rule set and a hierarchy walker
        ParamTablePtr compileParamTable(uint64_t compoundHash, const SelectorMatch* const* selectorList, uint32_t selectorStride, uint32_t selectorCount) const;

        //--

    private:
        /// selector nodes, organize selection
        typedef base::Array<SelectorNode> TSelectorNodes;
        TSelectorNodes m_selectors;

        /// value table
        typedef base::Array<base::Variant> TValueTable;
        TValueTable m_values;

        /// cached selector matches 
        typedef base::HashMap<uint64_t, const SelectorMatch*> TSelectorCache;
        mutable TSelectorCache m_selectorMap;
        mutable base::SpinLock m_selectorMapLock;

        /// cached parameter tables
        typedef base::HashMap<uint64_t, ParamTablePtr> TParamTableCache;
        mutable TParamTableCache m_paramTableCache;
        mutable base::SpinLock m_paramTableCacheLock;

        /// empty params
        ParamTablePtr m_emptyParams;

        /// check if the given selector chain (rule) can be applied 
        bool testRule(SelectorID selectorId, const SelectorMatch* const* selectorList, uint32_t selectorStride, int selectorIndex) const;

        ///--

        static void ParentValueTable(base::Array<base::Variant>& values, base::IObject* parent);
    };

    //---

    /// load SCSS file and build style library
    extern BASE_UI_API StyleLibraryPtr LoadStyleLibrary(base::StringView depotFilePath);

    //---

} // style

END_BOOMER_NAMESPACE(ui)

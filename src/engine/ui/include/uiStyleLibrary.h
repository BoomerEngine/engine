/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: styles #]
***/

#pragma once

#include "uiStyleSelector.h"

#include "core/resource/include/resource.h"
#include "core/system/include/spinLock.h"
#include "core/reflection/include/variant.h"

BEGIN_BOOMER_NAMESPACE_EX(ui::style)

class SelectorMatchContext;

//---

/// library of styles, contains a parameters tables with selectors
class ENGINE_UI_API Library : public IResource
{
    RTTI_DECLARE_VIRTUAL_CLASS(Library, IResource);

public:
    Library();
    Library(Array<SelectorNode>&& selectorNodes, Array<Variant>&& values);

    /// compile a list of matched selectors using partial selectors from this library and given evaluation context
    /// returned set contains all selectors that were matched
    /// NOTE: returned selectors are cached, do not delete them
    const SelectorMatch* matchSelectors(const SelectorMatchContext& context) const;

    /// compile a flat parameter table using the master element rule set and a hierarchy walker
    ParamTablePtr compileParamTable(uint64_t compoundHash, const SelectorMatch* const* selectorList, uint32_t selectorStride, uint32_t selectorCount) const;

    //--

private:
    /// selector nodes, organize selection
    typedef Array<SelectorNode> TSelectorNodes;
    TSelectorNodes m_selectors;

    /// value table
    typedef Array<Variant> TValueTable;
    TValueTable m_values;

    /// cached selector matches 
    typedef HashMap<uint64_t, const SelectorMatch*> TSelectorCache;
    mutable TSelectorCache m_selectorMap;
    mutable SpinLock m_selectorMapLock;

    /// cached parameter tables
    typedef HashMap<uint64_t, ParamTablePtr> TParamTableCache;
    mutable TParamTableCache m_paramTableCache;
    mutable SpinLock m_paramTableCacheLock;

    /// empty params
    ParamTablePtr m_emptyParams;

    /// check if the given selector chain (rule) can be applied 
    bool testRule(SelectorID selectorId, const SelectorMatch* const* selectorList, uint32_t selectorStride, int selectorIndex) const;

    ///--

    static void ParentValueTable(Array<Variant>& values, IObject* parent);
};

//---

/// load SCSS file and build style library
extern ENGINE_UI_API StyleLibraryPtr LoadStyleLibrary(StringView depotFilePath);

//---

END_BOOMER_NAMESPACE_EX(ui::style)

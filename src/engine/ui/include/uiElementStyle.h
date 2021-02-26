/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements #]
***/

#pragma once

#include "uiStyleLibrary.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//------

/// style traversal stack, used to cache the "parent" pointers as we go
class ENGINE_UI_API StyleStack : public NoCopy
{
public:
    StyleStack(DataStash& stash);

    //---

    /// get current style key, this encodes all types/ids/class/pseudo class names of all elements in stack - this identifies the style we want to draw with
    /// NOTE: different style key does not necessary mean different parameters - ie. many of the stuff we pushed on the stack may NOT be used by any style selector
    INLINE uint64_t selectorKey() const { return m_key; }

    //---

    /// push element (described as set of selectors) on the stack
    void push(const style::SelectorMatchContext* selectors);

    /// pop element
    void pop(const style::SelectorMatchContext* selectors);

    //----

    /// generate parameter table for current element, returns true if new parameter table was generated
    /// NOTE: parameter table is NOT generated if it's the same as already cached (to prevent needless allocations and copies)
    bool buildTable(uint64_t& outTableKey, ParamTablePtr& outTableData) const;
        
private:
    DataStash& m_stash;

    uint64_t m_key = 0;

    struct Element
    {
        const style::SelectorMatchContext* selector = nullptr;
        uint64_t prevKey = 0;
    };

    typedef InplaceArray<Element, 64> TParentHierarchy;
    TParentHierarchy m_stack;

    InplaceArray<const style::Library*, 4> m_styleLibraries;

    typedef InplaceArray<const style::SelectorMatch*, 256> TSelectorMatches;
    TSelectorMatches m_matches;
};

END_BOOMER_NAMESPACE_EX(ui)

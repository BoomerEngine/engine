/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: styles #]
***/

#include "build.h"
#include "uiStyleLibrary.h"
#include "uiStyleSelector.h"
#include "uiStyleValue.h"
#include "uiWindow.h"

#include "base/io/include/utils.h"
#include "base/io/include/absolutePath.h"
#include "base/io/include/absolutePathBuilder.h"
#include "base/containers/include/stringBuilder.h"
#include "base/system/include/scopeLock.h"
#include "base/canvas/include/canvasGlyphCache.h"
#include "base/font/include/fontGlyphCache.h"
#include "base/resource/include/resourceTags.h"

namespace ui
{
    namespace style
    {

        //---

        RTTI_BEGIN_TYPE_CLASS(Library);
            RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("v4styles");
            RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("UI Styles");
            RTTI_PROPERTY(m_selectors);
            RTTI_PROPERTY(m_values);
        RTTI_END_TYPE();

        //---

        Library::Library()
        {
            m_emptyParams = base::CreateSharedPtr<style::ParamTable>();
        }

        void Library::ParentValueTable(base::Array<base::Variant>& values, base::IObject* parent)
        {
            for (auto& value : values)
            {
                if (value.type() == base::reflection::GetTypeObject<base::FontPtr>())
                {
                    auto& val = *(base::FontPtr*)value.data();
                    val->parent(parent);
                }
                else if (value.type() == base::reflection::GetTypeObject<RenderStyle>())
                {
                    auto& val = *(RenderStyle*)value.data();
                    if (val.image)
                        val.image->parent(parent);
                }
            }
        }

        Library::Library(base::Array<SelectorNode>&& selectorNodes, base::Array<base::Variant>&& values)
        {
            m_emptyParams = base::CreateSharedPtr<style::ParamTable>();
            m_selectors = std::move(selectorNodes);
            m_values = std::move(values);

            ParentValueTable(m_values, this);
        }

        const SelectorMatch* Library::matchSelectors(const SelectorMatchContext& context) const
        {
            base::ScopeLock<base::SpinLock> lock(m_selectorMapLock);

            // compute hash and use existing selector if possible
            auto hash = context.key();
            const SelectorMatch* selector = nullptr;
            if (m_selectorMap.find(hash, selector))
                return selector;

            // collect selectors matching given context
            // TODO: optimize
            base::Array<SelectorID> matchingSelectors;
            for (uint32_t i = 0; i < m_selectors.size(); ++i)
            {
                const auto& selector = m_selectors[i];
                if (selector.matchParameters().matches(context))
                {
                    matchingSelectors.pushBack(range_cast<SelectorID>(i));
                }
            }

            // create the match wrapper
            auto newSelector = MemNew(SelectorMatch, std::move(matchingSelectors), hash);
            m_selectorMap.set(hash, newSelector);
            return newSelector;
        }

        bool Library::testRule(SelectorID selectorId, const SelectorMatch* const* selectorList, uint32_t selectorStride, int selectorIndex) const
        {
            // invalid node, test failed
            if (selectorIndex < 0)
                return false;

            // get all selectors that are matched at this node
            // for a rule to match we need to have a matched selector for a given node in the hierarchy
            const auto* selectorMatch = selectorList[selectorIndex * selectorStride];
            if (!selectorMatch->selectors().contains(selectorId))
                return false;

            // if we got here and there are no more selector rules to check we are done 
            const auto& selector = m_selectors[selectorId];
            auto parentSelectorId = selector.parentId();
            if (parentSelectorId == 0)
                return true;

            // get the way we need to combine the selector
            auto combiner = selector.relation();
            if (combiner == SelectorCombinatorType::AnyParent)
            {
                // any parent will do
                while (--selectorIndex >= 0)
                    if (testRule(parentSelectorId, selectorList, selectorStride, selectorIndex))
                        return true;
            }
            else if (combiner == SelectorCombinatorType::DirectParent)
            {
                // our direct parent must match
                return testRule(parentSelectorId, selectorList, selectorStride, selectorIndex-1);
            }
            else if (combiner == SelectorCombinatorType::AdjecentSibling)
            {
                // TODO: hopefully never
            }
            else if (combiner == SelectorCombinatorType::GeneralSibling)
            {
                // TODO: hopefully never
            }

            // rule is invalid
            return false;
        }
        
        ParamTablePtr Library::compileParamTable(uint64_t compoundHash, const SelectorMatch* const* selectorList, uint32_t selectorStride, uint32_t selectorCount) const
        {
            // no data
            if (!selectorCount)
                return m_emptyParams;

            // lookup in local cache
            ParamTablePtr ret;
            if (m_paramTableCache.find(compoundHash, ret))
                return ret;

            // return table
            ret = base::CreateSharedPtr<ParamTable>();
            m_paramTableCache.set(compoundHash, ret);

            // get the styles for the entry node
            int selectorIndex = (int)selectorCount - 1;
            const auto* startSelectors = selectorList[selectorStride * selectorIndex];
            for (const auto selectorId : startSelectors->selectors())
            {
                // aggregate parameters only from rules that match
                if (testRule(selectorId, selectorList, selectorStride, selectorIndex))
                {
                    const auto& selector = m_selectors[selectorId];
                    for (const auto& param : selector.parameters())
                    {
                        const auto& value = m_values[param.valueId()];
                        ret->values.setVariant(param.paramName(), value);
                        //TRACE_INFO("Style param '{}': '{}'", param.paramName(), value);
                    }
                }
            }

            return ret;
        }

    } // style
} // ui
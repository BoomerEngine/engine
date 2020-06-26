/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: pages #]
*
***/

#include "build.h"
#include "debugPage.h"

namespace base
{
    namespace debug
    {

        //--

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IDebugPage);
            RTTI_METADATA(DebugPageCategoryMetadata).category("Debug");
        RTTI_END_TYPE();

        IDebugPage::IDebugPage()
        {}

        IDebugPage::~IDebugPage()
        {}

        bool IDebugPage::handleInitialize(DebugPageContainer& container)
        {
            return true;
        }

        bool IDebugPage::handleSupported(DebugPageContainer& container)
        {
            return true;
        }

        bool IDebugPage::handleTick(DebugPageContainer& context, float timeDelta)
        {
            return true;
        }

        void IDebugPage::handleRender(DebugPageContainer& context)
        {
        }

        base::StringBuf IDebugPage::title() const
        {
            return base::StringBuf(cls()->findMetadata<DebugPageTitleMetadata>()->title());
        }

        base::StringBuf IDebugPage::category() const
        {
            return base::StringBuf(cls()->findMetadata<DebugPageCategoryMetadata>()->category());
        }

        //--

        RTTI_BEGIN_TYPE_CLASS(DebugPageTitleMetadata);
        RTTI_END_TYPE();

        DebugPageTitleMetadata::DebugPageTitleMetadata()
            : m_title("")
        {}

        //--

        RTTI_BEGIN_TYPE_CLASS(DebugPageCategoryMetadata);
        RTTI_END_TYPE();

        DebugPageCategoryMetadata::DebugPageCategoryMetadata()
            : m_category("Debug")
        {}

        //--

    } // debug
} // plugin
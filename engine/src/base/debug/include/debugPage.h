/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: pages #]
*
***/

#pragma once

#include "base/object/include/rttiMetadata.h"

namespace base
{
    namespace debug
    {

        //----

        /// debug page interface,
        class BASE_DEBUG_API IDebugPage : public base::IReferencable
        {
            RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IDebugPage);

        public:
            IDebugPage();
            virtual ~IDebugPage();

            //! initialize the page, can fail
            virtual bool handleInitialize(DebugPageContainer& container);

            //! check if we can use this debug page
            virtual bool handleSupported(DebugPageContainer& container);

            //! update page content, we can return false to force close the page, useful if we lost the context we are operating on
            virtual bool handleTick(DebugPageContainer& context, float timeDelta);

            //! generate ui
            virtual void handleRender(DebugPageContainer& context);
            
            //--

            //! get debug page title
            base::StringBuf title() const;

            //! get debug page category
            base::StringBuf category() const;
        };

        //----

        /// metadata with debug page title
        class BASE_DEBUG_API DebugPageTitleMetadata : public base::rtti::IMetadata
        {
            RTTI_DECLARE_VIRTUAL_CLASS(DebugPageTitleMetadata, base::rtti::IMetadata);

        public:
            DebugPageTitleMetadata();

            INLINE const char* title() const { return m_title; }
            INLINE void title(const char* title) { m_title = title; }

        private:
            const char* m_title;
        };

        //----

        /// metadata with debug page category
        class BASE_DEBUG_API DebugPageCategoryMetadata : public base::rtti::IMetadata
        {
            RTTI_DECLARE_VIRTUAL_CLASS(DebugPageCategoryMetadata, base::rtti::IMetadata);

        public:
            DebugPageCategoryMetadata();

            INLINE const char* category() const { return m_category; }
            INLINE void category(const char* category) { m_category = category; }

        private:
            const char* m_category;
        };

        //----

    } // debug
} // plugin
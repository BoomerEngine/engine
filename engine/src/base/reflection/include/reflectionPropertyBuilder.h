/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: reflection #]
***/

#pragma once

#include "base/containers/include/stringBuf.h"

namespace base
{
    namespace rtti
    {
        class IType;
        class IMetadata;
    }

    namespace reflection
    {
        // helper class that can add stuff to the class type
        class BASE_REFLECTION_API PropertyBuilder
        {
        public:
            PropertyBuilder(Type propType, const char* name, const char* category, uint32_t offset);
            ~PropertyBuilder();

            void submit(rtti::IClassType* targetClass);

            INLINE PropertyBuilder& name(const char* name)
            {
                DEBUG_CHECK_EX(name != nullptr && *name, "Custom name cannot be empty");
                m_name = StringID(name);
                return *this;
            }

            INLINE PropertyBuilder& category(const char* category)
            {
                m_category = StringID(category);
                return *this;
            }

            INLINE PropertyBuilder& editable(const char* hint = "")
            {
                m_editable = true;
                m_comment = hint;
                return *this;
            }

            INLINE PropertyBuilder& readonly()
            {
                m_editable = true;
                m_readonly = true;
                return *this;
            }

            INLINE PropertyBuilder& inlined()
            {
                m_inlined = true;
                return *this;
            }

            INLINE PropertyBuilder& transient()
            {
                m_transient = true;
                return *this;
            }

            INLINE PropertyBuilder& noScripts()
            {
                m_scriptHidden = true;
                return *this;
            }

            INLINE PropertyBuilder& scriptReadOnly()
            {
                m_scriptReadOnly = true;
                return *this;
            }

            INLINE PropertyBuilder& overriddable()
            {
                m_overriddable = true;
                return *this;
            }

            template< typename T, typename... Args >
            INLINE PropertyBuilder& metadata(Args && ... args)
            {
                auto obj  = MemNew(T,std::forward< Args >(args)...);
                m_metadata.pushBack(static_cast<rtti::IMetadata*>(obj));
                return *this;
            }

        private:
            Type m_type;
            StringID m_name;
            StringID m_category;
            uint32_t m_offset = 0;

            StringBuf m_comment;
            bool m_editable = false;
            bool m_inlined = false;
            bool m_readonly = false;
            bool m_scriptHidden = false;
            bool m_scriptReadOnly = false;
            bool m_transient = false;
            bool m_overriddable = false;

            Array<rtti::IMetadata*> m_metadata;
        };

    } // reflection
} // base

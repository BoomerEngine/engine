/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\reference #]
***/

#pragma once

#include "resourcePath.h"

namespace base
{

    namespace stream
    {
        class ITextReader;
        class ITextWriter;
        class IBinaryReader;
        class IBinaryWriter;
    }

    namespace res
    {
        class IResourceLoader;

        ///--------

        /// base concept of serializable resource reference
        class BASE_RESOURCES_API BaseReference
        {
        public:
            BaseReference();
            BaseReference(const BaseReference& other);
            BaseReference(const ResourcePtr& ptr);
            BaseReference(BaseReference&& other);
            BaseReference(std::nullptr_t);
            ~BaseReference();

            BaseReference& operator=(const BaseReference& other);
            BaseReference& operator=(BaseReference&& other);

            ///--

            // get resource "key" -> load class + resource path
            // NOTE: this is not set for path-less embedded resources
            INLINE const ResourceKey& key() const { return m_key; }

            // acquire the resource
            INLINE const ResourceHandle& acquire() const { return m_handle; }

            // is this a valid reference ? we need either a key or an object
            INLINE bool valid() const { return m_handle; }

            // is this an empty reference ? empty reference is one without key and without object
            INLINE bool empty() const { return !valid(); }

            // is this an inlined object ?
            INLINE bool inlined() const { return m_handle && !m_key; } // bound but not loaded from anywhere

            // validate
            INLINE operator bool() const { return valid(); }

            //---

            // clear reference
            void reset();

            // setup a reference to a specific resource object
            // NOTE: if the resource is "path less" it will be embedded into the containing object when saved (that is if it's parented properly and is part of the object hierarchy being saved)
            void set(const ResourceHandle& object);

            //--

            /// test references for equality, if key is given is checked first, if no key is given then and only then the object pointers are checked
            bool operator==(const BaseReference& other) const;
            bool operator!=(const BaseReference& other) const;

            //--

            // print to text
            // for a key reference it prints: "Texture$engine/textures/lena.png"
            // for inlined objects this just prints: "Texture"
            void print(IFormatStream& f) const;

            //--


            //--

            // cast to reference of specific type
            template< typename T >
            INLINE Ref<T> cast() const;

            //--

        protected:
            ResourcePtr m_handle;
            ResourceKey m_key;

            static const ResourceHandle& EMPTY_HANDLE();
        };

        ///--------

        /// reference to resource, preserves the path even if the actual resource was not loaded (errors)
        template< class T >
        class Ref : public BaseReference
        {
        public:
            INLINE Ref() = default;
            INLINE Ref(std::nullptr_t) {};
            INLINE Ref(Ref<T>&& other) = default;
            INLINE Ref& operator=(const Ref<T>& other) = default;
            INLINE Ref& operator=(Ref<T> && other) = default;
            INLINE Ref(const Ref<T>& other) = default;

            template< typename U >
            INLINE Ref(const Ref<U>& other)
                : BaseReference(other)
            {
                ASSERT_EX(0 == (int64_t) static_cast<T*>((U*)nullptr), "Address to object in the shared pointer cannot change due to case");
                static_assert(std::is_base_of<T, U>::value, "Cannot down-cast a pointer through construction, use rtti_cast");
            }

            template< typename U >
            INLINE Ref(const RefPtr<U>& ptr)
                : BaseReference(ptr)
            {
                ASSERT_EX(0 == (int64_t) static_cast<T*>((U*)nullptr), "Address to object in the shared pointer cannot change due to case");
                static_assert(std::is_base_of<T, U>::value, "Cannot down-cast a pointer through construction, use rtti_cast");
            }

            ///---

            // check for equality
            INLINE bool operator==(const Ref<T>& other) const { return BaseReference::operator==(other); }

            // check for in equality
            INLINE bool operator!=(const Ref<T>& other) const { return BaseReference::operator!=(other); }

            // validate
            INLINE operator bool() const { return valid(); }

            ///---

            // acquire the resource
            INLINE RefPtr<T> acquire() const { return rtti_cast<T>(BaseReference::acquire()); }
        };

        //----------

        template< typename T >
        INLINE Ref<T> BaseReference::cast() const
        {
            return Ref<T>(rtti_cast<T>(acquire()));
        }

        ///--------

        // type naming, generate a rtti type name for a Ref<T> with given class
        extern BASE_RESOURCES_API StringID FormatRefTypeName(StringID className);

        // get a reference type name
        template< typename T >
        INLINE StringID FormatRefTypeName() {
            static const const typeName = FormatRefTypeName(T::GetStaticClass()->name());
            return typeName;
        }

        //--------

    } // res

    //---

    // class ref cast for RTTI SpecificClassType, will null the class ref if class does not match
    // ie. SpecificClassType<Entity> entityClass = rtti_cast<Entity>(anyClass);
    template< class _DestType, class _SrcType >
    INLINE res::Ref< _DestType > rtti_cast(const res::Ref< _SrcType>& srcClassType)
    {
        static_assert(std::is_base_of<_DestType, _SrcType>::value || std::is_base_of<_SrcType, _DestType>::value, "Cannot cast between unrelated class types");
        return srcClassType.cast<_DestType>();
    }

    //---

    // class ref cast for RTTI SpecificClassType, will null the class ref if class does not match
    // ie. SpecificClassType<Entity> entityClass = rtti_cast<Entity>(anyClass);
    template< class _DestType >
    INLINE res::Ref< _DestType > rtti_cast(const res::BaseReference& srcClassType)
    {
        return srcClassType.cast<_DestType>();
    }

    //---

} // base

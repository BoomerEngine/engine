/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\reference #]
***/

#pragma once

#include "resourcePath.h"
#include "resourceReference.h"

namespace base
{

    namespace res
    {

        ///--------

        /// base concept of deferred loading reference to resource
        class BASE_RESOURCE_API BaseAsyncReference
        {
        public:
            BaseAsyncReference();
            BaseAsyncReference(const BaseAsyncReference& other);
            BaseAsyncReference(BaseAsyncReference&& other);
            BaseAsyncReference(const ResourceKey& key);
            ~BaseAsyncReference();

            BaseAsyncReference& operator=(const BaseAsyncReference& other);
            BaseAsyncReference& operator=(BaseAsyncReference&& other);

            ///--

            // get resource "key" -> load class + resource path
            // NOTE: this is not set for path-less embedded resources
            INLINE const ResourceKey& key() const { return m_key; }

            // is this a valid reference ? we need either a key or an object
            INLINE bool valid() const { return m_key; }

            // is this an empty reference ? empty reference is one without key and without object
            INLINE bool empty() const { return !valid(); }

            //---

            // clear reference
            void reset();

            // setup reference to a given resource key
            void set(StringView<char> path, SpecificClassType<IResource> cls);

            // setup reference to a given resource key
            void set(const ResourceKey& key);

            //---

            // load the resource into a "loaded" reference, a custom loader can be provided 
            BaseReference load(IResourceLoader* customLoader = nullptr) const CAN_YIELD;

            //---

            // load asynchronously, call the callback when loaded
            // basically a nice wrapper for a fiber job with a fast short circuit if the resource is already loaded
            // NOTE: THIS DOES NOT CHANGE THE STATE OF THE OBJECT, just calls the callback, use the ensureLoaded() to do that
            void loadAsync(const std::function<void(const BaseReference&)>& onLoadedFunc, IResourceLoader* customLoader = nullptr) const; // <--- look here, a "const"

            //--

            /// test references for equality, if key is given is checked first, if no key is given then and only then the object pointers are checked
            bool operator==(const BaseAsyncReference& other) const;
            bool operator!=(const BaseAsyncReference& other) const;

            //--

            // print to text
            // for a key reference it prints: "Texture$engine/textures/lena.png"
            // for inlined objects this just prints: "Texture"
            void print(IFormatStream& f) const;

            // parse from text representation, will fail if the referenced class name does not match constrained class
            static bool Parse(StringView<char> txt, BaseAsyncReference& outReference, ClassType constrainedClass = nullptr);

            //--

        protected:
            ResourceKey m_key;

            static const BaseAsyncReference& EMPTY_HANDLE();
        };

        ///--------

        /// async reference to resource
        template< class T >
        class AsyncRef : public BaseAsyncReference
        {
        public:
            INLINE AsyncRef() = default;
            INLINE AsyncRef(const AsyncRef<T>& other) = default;
            INLINE AsyncRef(AsyncRef<T>&& other) = default;
            INLINE AsyncRef& operator=(const AsyncRef<T>& other) = default;
            INLINE AsyncRef& operator=(AsyncRef<T> && other) = default;

            INLINE AsyncRef(const ResourceKey& key)
            {
                if (key.cls() && key.cls()->is<T>())
                    set(key);
            }

            INLINE AsyncRef(StringView<char> path)
                : BaseAsyncReference(MakePath<T>(path))
            {}

            INLINE bool operator==(const AsyncRef<T>& other) const { return BaseAsyncReference::operator==(other); }
            INLINE bool operator!=(const AsyncRef<T>& other) const { return BaseAsyncReference::operator!=(other); }

            template< typename U = T >
            INLINE Ref<U> load(IResourceLoader* customLoader = nullptr) const CAN_YIELD
            {
                static_assert(std::is_base_of<T, U>::value || std::is_base_of<U, T>::value, "Types are unrelated");

                if (auto loaded = BaseAsyncReference::load(customLoader).acquire())
                    return base::rtti_cast<U>(loaded);

                return nullptr;
            }
        };

        ///--------

        // type naming, generate a rtti type name for a Ref<T> with given class
        extern BASE_RESOURCE_API StringID FormatAsyncRefTypeName(StringID className);

        // get a reference type name
        template< typename T >
        INLINE StringID FormatAsyncRefTypeName() {
            static const const typeName = FormatAsyncRefTypeName(T::GetStaticClass()->name());
            return typeName;
        }

        //--------

    } // res
} // base

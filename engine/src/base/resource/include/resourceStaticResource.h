/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\static #]
***/

#pragma once

namespace base
{
    namespace res
    {

        ///---

        namespace prv
        {
            class StaticResourceRegistry;
        } // prv

        /// static (engine global) resource reference
        /// has the benefit of being directly loadable
        class BASE_RESOURCE_API IStaticResource : public base::NoCopy
        {
        public:
            IStaticResource(const char* path, bool preload);
            virtual ~IStaticResource();

            /// get the resource path
            INLINE StringView path() const { return m_path; }

            /// get the resource class
            virtual ClassType resourceClass() const = 0;

            /// get the loaded resource, valid only if loaded
            INLINE const ResourceHandle& loadedResource() const { return m_handle; }

            /// should this resource be preloaded ?
            INLINE bool isPreloaded() const { return m_isPreloaded; }


            //---

            /// unload the resource
            void unload();

            /// load the resource and get the loaded object
            ResourceHandle loadAndGet();

            /// reload resource
            void reload(res::IResource* currentResource, res::IResource* newResource);

            //---

            // mount the global resource loader for all of the static resources
            static void BindGlobalLoader(IResourceLoader* loader);

            // collect all static resources that are defined in C++ code
            static void CollectAllResources(Array<IStaticResource*>& outResources);

            // apply a reload to static resources
            static void ApplyReload(res::IResource* currentResource, res::IResource* newResource);

        private:
            const char* m_path;
            SpinLock m_lock;
            ClassType m_class;
            ResourceHandle m_handle;
            IStaticResource* m_next;
            IStaticResource* m_prev;
            bool m_isPreloaded;

            friend class prv::StaticResourceRegistry;
        };

        ///---

        /// typed static resource, can be directly loaded
        template< typename T >
        class StaticResource : public IStaticResource
        {
        public:
            INLINE StaticResource(const char* path, bool preload=false)
                : IStaticResource(path, preload)
            {}

            /// load the resource and get the loaded object
            INLINE RefPtr<T> loadAndGet()
            {
                return rtti_cast<T>(IStaticResource::loadAndGet());
            }

            /// load the resource and get the loaded object
            INLINE Ref<T> loadAndGetAsRef()
            {
                Ref<T> ret(rtti_cast<T>(IStaticResource::loadAndGet()));
                return ret;
            }

            /// get as async ref
            INLINE AsyncRef<T> asyncRef() const
            {
                AsyncRef<T> ret(ResourceKey(path(), T::GetStaticClass()));
                return ret;
            }

        protected:
            virtual ClassType resourceClass() const override final
            {
                return T::GetStaticClass();
            }
        };

    } // res
} // base

/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\reference #]
***/

#include "build.h"

#include "resource.h"
#include "resourceReference.h"
#include "resourceAsyncReference.h"
#include "resourceLoader.h"

namespace base
{
    namespace res
    {

        //---

        BaseAsyncReference::BaseAsyncReference()
        {
        }

        BaseAsyncReference::BaseAsyncReference(const BaseAsyncReference& other)
            : m_key(other.m_key)
        {
        }

        BaseAsyncReference::BaseAsyncReference(const ResourceKey& path)
            : m_key(path)
        {}

        BaseAsyncReference::BaseAsyncReference(BaseAsyncReference&& other)
            : m_key(std::move(other.m_key))
        {}

        BaseAsyncReference::~BaseAsyncReference()
        {
        }

        BaseAsyncReference& BaseAsyncReference::operator=(const BaseAsyncReference& other)
        {
            if (this != &other)
            {
                m_key = other.m_key;
            }

            return *this;
        }

        BaseAsyncReference& BaseAsyncReference::operator=(BaseAsyncReference&& other)
        {
            if (this != &other)
            {
                m_key = std::move(other.m_key);
            }

            return *this;
        }

        void BaseAsyncReference::reset()
        {
            m_key = ResourceKey();
        }

        void BaseAsyncReference::set(ResourcePath path, SpecificClassType<IResource> cls)
        {
            set(ResourceKey(path, cls));
        }
        
        void BaseAsyncReference::set(const ResourceKey& key)
        {
            m_key = key;
        }

        BaseReference BaseAsyncReference::load(IResourceLoader* customLoader /*= nullptr*/) const
        {
            if (m_key)
            {
                auto loadedResource = customLoader ? customLoader->loadResource(m_key) : base::LoadResource(m_key);
                return BaseReference(loadedResource);
            }

            return BaseReference();
        }

        void BaseAsyncReference::loadAsync(const std::function<void(const BaseReference&)>& onLoadedFunc, IResourceLoader* customLoader /*= nullptr*/) const
        {
            if (m_key)
            {
                // the custom loaded case requires more care
                if (customLoader)
                {
                    // the resource may be already loaded, if that's the case we can save from creating a fiber
                    if (auto alreadyLoaed  = customLoader->acquireLoadedResource(m_key))
                    {
                        onLoadedFunc(BaseReference(alreadyLoaed));
                    }
                    else
                    {
                        // we need to run a proper fiber
                        auto key  = m_key;
                        RunFiber("LoadResourceRef") << [key, onLoadedFunc, customLoader](FIBER_FUNC)
                        {
                            auto loadedObject = customLoader->loadResource(key);
                            onLoadedFunc(BaseReference(loadedObject));
                        };
                    }
                }
                else
                {
                    // use general "global" loading function
                    LoadResourceAsync(m_key, onLoadedFunc);
                }
            }
            else
            {
                // empty ref, still we need to call the handler
                onLoadedFunc(BaseReference());
            }
        }
        
        const BaseAsyncReference& BaseAsyncReference::EMPTY_HANDLE()
        {
            static BaseAsyncReference theEmptyHandle;
            return theEmptyHandle;
        }

        bool BaseAsyncReference::operator==(const BaseAsyncReference& other) const
        {
            return m_key == other.m_key;
        }

        bool BaseAsyncReference::operator!=(const BaseAsyncReference& other) const
        {
            return !operator==(other);
        }

        void BaseAsyncReference::print(IFormatStream& f) const
        {
            if (m_key)
                f << m_key;
            else
                f << "null";
        }

        bool BaseAsyncReference::Parse(StringView<char> txt, BaseAsyncReference& outReference, ClassType constrainedClass /*= nullptr*/)
        {
            ResourceKey key;
            if (!ResourceKey::Parse(txt, key))
                return false;

            if (!key.cls() || !key.cls()->is(constrainedClass))
                return false;

            outReference.set(key);
            return true;
        }

        //--

    } // res
} // base

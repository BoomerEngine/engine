/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene\proxy #]
***/

#pragma once

#include "renderingSceneCommand.h"

namespace rendering
{
    namespace scene
    {
        ///--

        /// proxy prototype
        struct RENDERING_SCENE_API IProxy : public base::NoCopy
        {
            IProxy(ProxyType type);
            virtual ~IProxy();

            ObjectRenderID objectId = 0;

            INLINE ProxyType type() const { return proxyType; }

        protected:
            ProxyType proxyType;
        };

        ///--

        /// Helper class to iterate over proxies
        template< typename T >
        class ProxyIterator : public base::NoCopy
        {
        public:
            INLINE ProxyIterator(const SceneObjectCullingEntry* proxyList, uint32_t numFragments)
                : m_proxies(proxyList)
                , m_size(numFragments)
                , m_index(0)
            {}

            INLINE operator bool() const
            {
                return m_index < m_size;
            }

            INLINE uint32_t size() const
            {
                return m_size;
            }

            INLINE uint32_t pos() const
            {
                return m_index;
            }

            INLINE bool advance()
            {
                if (m_index < m_size)
                {
                    m_index += 1;
                    return true;
                }

                return false;
            }

            INLINE const T* operator->() const
            {
                if (m_index < m_size)
                    return (const T*)(m_proxies[m_index].proxy);
                else
                    return nullptr;
            }

            INLINE void operator++()
            {
                if (m_index < m_size)
                    m_index += 1;
            }

        private:
            const SceneObjectCullingEntry* m_proxies;
            const uint32_t m_size;
            uint32_t m_index;
        };

        ///--

        /// generalized proxy handler interface
        class RENDERING_SCENE_API IProxyHandler : public ICommandDispatcher
        {
            RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IProxyHandler);

        public:
            IProxyHandler(ProxyType type);
            virtual ~IProxyHandler();

            // get type of handled proxies
            INLINE ProxyType proxyType() const { return m_type; }

            // get the scene we are part of
            INLINE Scene* scene() const { return m_scene; }

            //--

            // initialize
            virtual void handleInit(Scene* scene);

            // scene is being locked for rendering
            virtual void handleSceneLock();

            // scene was unlocked after rendering
            virtual void handleSceneUnlock();

            // prepare all data needed for rendering of a GENERIC frame with the scene's data
            // NOTE: this is the last moment we can modify the data - all other collect/rendering calls should be read only as they will run on jobs
            virtual void handlePrepare(command::CommandWriter& cmd);

            //--

            // create proxy based on setup
            virtual IProxy* handleProxyCreate(const ProxyBaseDesc& desc) = 0;

            // destroy proxy 
            virtual void handleProxyDestroy(IProxy* proxy) = 0;

            //--

            // process proxy visibility results
            virtual void handleProxyFragments(command::CommandWriter& cmd, FrameView& view, const SceneObjectCullingEntry* proxies, uint32_t numProxies, FragmentDrawList& outFragmentList) const = 0;

        private:
            Scene* m_scene;
            ProxyType m_type;
        };

        ///--

    } // scene
} // rendering


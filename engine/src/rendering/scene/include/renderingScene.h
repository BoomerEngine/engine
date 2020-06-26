/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
***/

#pragma once

#include "base/containers/include/refcounted.h"
#include "base/containers/include/staticStructurePool.h"

namespace rendering
{
    namespace scene
    {

        ///--

        // scene type
        enum class SceneType : uint8_t
        {
            Game, // actual full blown game scene, usually there's only one 
            EditorGame, // in-editor game scene, some features may have bigger budgets
            EditorPreview, // in-editor small preview scene
        };

        // scene setup
        struct SceneSetupInfo
        {
            SceneType type = SceneType::Game; // type of scene
            base::StringBuf name; // context name, in editor this is the name of the asset, for debugging mostly
            base::Matrix sceneToWorld; // placement of the scene in the abstract world space, scenes don't have to be rooted at 0,0,0
        };

        ///--

        // a rendering scene - container for objects of various sorts
        class RENDERING_SCENE_API Scene : public base::IReferencable
        {
        public:
            Scene(const SceneSetupInfo& setup);

            //--

            // type of the scene
            INLINE const SceneType type() const { return m_type; }

            // get list of proxy handlers
            INLINE const IProxyHandler* const* proxyHandlers() const { return &m_proxyHandlers[0]; }

            // get list of fragment handlers
            INLINE const IFragmentHandler* const* fragmentHandlers() const { return &m_fragmentHandlers[0]; }

            // get global object/culling table
            INLINE const SceneObjectRegistry& objects() const { return *m_objects; }
            INLINE SceneObjectRegistry& objects() { return *m_objects; }
                       
            //--

            /// lock scene for rendering, no modifications to scene are possible when scene is locked
            void lockForRendering();

            /// unlock scene after rendering is done
            void unlockAfterRendering();

            /// is the scene locked for rendering ? (DEBUG ONLY)
            INLINE bool lockedForRendering() const { return m_lockCount.load() > 0; }

            //--

            /// create a proxy in the scene based on a description
            /// NOTE: can't be called during rendering
            ProxyHandle proxyCreate(const ProxyBaseDesc& desc);

            /// destroy proxy from the scene
            void proxyDestroy(ProxyHandle handle);

            /// run a command on a proxy
            void proxyCommand(ProxyHandle handle, const Command& command);

            //--

            // prepare scene for rendering, update all required GPU side data
            void prepareForRendering(command::CommandWriter& cmd);

        private:
            virtual ~Scene();

            SceneType m_type = SceneType::Game;
            base::StringBuf m_name;

            std::atomic<int> m_lockCount = 0;

            IProxyHandler* m_proxyHandlers[(uint8_t)ProxyType::MAX];
            IFragmentHandler* m_fragmentHandlers[(uint8_t)FragmentHandlerType::MAX];

            //--

            base::UniquePtr<SceneObjectRegistry> m_objects;

            //---

            struct ProxyEntry
            {
                uint32_t generationIndex = 0;
                IProxy* proxy = nullptr;
                ProxyType type = ProxyType::None;
            };

            uint32_t m_proxyGenerationIndex = 1;
            base::StaticStructurePool<ProxyEntry> m_proxyTable;

            // --

            void destroyAllProxies();

            void createProxyHandlers();
            void createFragmentHandlers();
        };

        ///--

    } // scene
} // rendering


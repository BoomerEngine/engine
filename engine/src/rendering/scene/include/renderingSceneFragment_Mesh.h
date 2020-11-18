/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene\fragments #]
***/

#pragma once

#include "renderingSceneFragment.h"

namespace rendering
{
    namespace scene
    {
        ///--

        /// mesh fragment
        struct Fragment_Mesh : public Fragment
        {
            static const auto FRAGMENT_TYPE = FragmentHandlerType::Mesh;

            ObjectRenderID objectId = 0;
            MeshChunkRenderID meshChunkdId = 0;
            uint32_t subObjectID = 0;
            MaterialCachedTemplate* materialTemplate = nullptr; // cached template for the material - matches the geometry type
            const MaterialDataProxy* materialData = nullptr; // TODO: convert to offset
        };

        ///--

        // a scene container to render mesh objects
        class RENDERING_SCENE_API FragmentHandler_Mesh : public IFragmentHandler
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FragmentHandler_Mesh, IFragmentHandler);

        public:
            FragmentHandler_Mesh();
            virtual ~FragmentHandler_Mesh();

            //--

            // IFragmentHandler
            virtual void handleInit(Scene* scene) override final;
            virtual void handleSceneLock() override final;
            virtual void handleSceneUnlock() override final;
            virtual void handlePrepare(command::CommandWriter& cmd) override final;
            virtual void handleRender(command::CommandWriter& cmd, const FrameView& view, const FragmentRenderContext& context, const Fragment* const* fragments, uint32_t numFragments, FrameFragmentRenderStats& outStats) const override final;

            //--

        private:
            const MeshService* m_meshCache = nullptr;

            void innerRender(command::CommandWriter& cmd, const FrameView& view, const FragmentRenderContext& context, const Fragment* const* fragments, uint32_t numFragments, FrameFragmentRenderStats& outStats) const;

            BufferObjectPtr m_bufferChunkData;
            uint32_t m_bufferChunkDataCapacity = 0;
        };

        ///--
        
    } // scene
} // rendering


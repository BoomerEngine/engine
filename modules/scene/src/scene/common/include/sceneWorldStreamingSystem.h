/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world #]
***/

#pragma once

#include "sceneRuntimeSystem.h"
#include "base/containers/include/hashSet.h"

namespace scene
{
    //---

    struct WorldSectorLoadingState;

    /// helper class for a streamable world sector
    class WorldSectorStreamer : public base::NoCopy
    {
    public:
        WorldSectorStreamer(const WorldSectorDesc& desc, const base::res::ResourcePath& worldPath);
        ~WorldSectorStreamer();

        //--

        /// get the name of this sector
        INLINE const base::StringBuf& name() const { return m_name; }

        /// is this sector still loading ?
        INLINE bool isLoading() const { return m_loadingState != nullptr; }

        /// is this sector loaded ?
        INLINE bool isLoaded() const { return m_loadedData != nullptr; }

        /// get the reference streaming box
        INLINE const base::Box& streamingBox() const { return m_streamingBox; }

        /// start loading of the sector
        /// this request can be canceled before finishing if the requestUnload is called
        void requestLoad();

        /// request sector to be unloaded
        /// this request can be canceled before finishing if the requestLoad is called
        void requestUnload(Scene& scene);

        /// update the loading state, returns true if the sector was loaded
        bool updateLoadingState(Scene& scene);


    private:
        // reference to the sector data
        base::res::Ref<WorldSector> m_dataHandle;

        // loaded sector data (when loaded)
        base::RefPtr<WorldSector> m_loadedData;

        // persistent (always loaded) data
        base::RefPtr<WorldSector> m_persistentData;

        // internal loading state
        base::RefPtr<WorldSectorLoadingState> m_loadingState;

        // at least content observer must be in this area for the sector to be loaded
        base::Box m_streamingBox;

        // get name assigned to this sector
        base::StringBuf m_name;
    };

    //---

    /// scene system that is capable of streaming compiled world
    class SCENE_COMMON_API WorldStreamingSystem : public IRuntimeSystem
    {
        RTTI_DECLARE_VIRTUAL_CLASS(WorldStreamingSystem, IRuntimeSystem);

    public:
        WorldStreamingSystem();
        virtual ~WorldStreamingSystem();

        /// are we loading something at all ?
        bool isLoadingContent() const;

        /// are we loading something for given location ?
        bool isLoadingContentAround(const base::AbsolutePosition& pos) const;

    private:
        /// IRuntimeSystem interface
        virtual bool checkCompatiblity(SceneType type) const override final;
        virtual bool onInitialize(Scene& scene) override final;
        virtual void onPreTick(Scene& scene, const UpdateContext& ctx) override final;
        virtual void onRenderFrame(Scene& scene, rendering::scene::FrameInfo& info) override final;
        virtual void onShutdown(Scene& scene) override final;
        virtual void onWorldContentAttached(Scene& scene, const WorldPtr& worldPtr) override final;
        virtual void onWorldContentDetached(Scene& scene, const WorldPtr& worldPtr) override final;

        //---

        // attached world
        struct WorldInfo
        {
            WorldPtr m_world;
            
            // all registered sectors, NOTE: this is a streaming wrapper for fast streaming
            typedef base::Array<WorldSectorStreamer*> TSectors;
            TSectors m_allSectors;

            // sectors we are loading/loaded
            base::HashSet<WorldSectorStreamer*> m_loadingSectors;
            base::HashSet<WorldSectorStreamer*> m_loadedSectors;
        };

        base::Array<WorldInfo*> m_worlds;

        //---

        void updateStreaming(Scene& scene, const base::AbsolutePosition* observers, uint32_t numObservers);
        void updateStreaming(Scene& scene, WorldInfo* world, const base::AbsolutePosition* observers, uint32_t numObservers);

        static bool CheckVisibility(const base::Box& streamingBox, const base::AbsolutePosition* observers, uint32_t numObservers);
    };

} // scene

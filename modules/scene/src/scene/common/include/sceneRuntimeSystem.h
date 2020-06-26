/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#pragma once

namespace scene
{
    //---

    /// general system update order:
    ///  - pre tick
    ///  - tick (the tick system is ticked, only active nodes are ticked)
    ///  - pre transform
    ///  - transform (the transform system us updated, only on dirty nodes)
    ///  - post transform

    /// scene system, integrates scene with various engine and gameplay systems
    class SCENE_COMMON_API IRuntimeSystem
    {
        RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IRuntimeSystem);

    public:
        IRuntimeSystem();
        virtual ~IRuntimeSystem();

        /// check if the system should be initialized on given scene type
        virtual bool checkCompatiblity(SceneType type) const;

        /// initialize system, can fail
        virtual bool onInitialize(Scene& scene);

        /// shutdown system, called once before destruction of scene
        /// NOTE: always called in revere order than the initialize calls
        virtual void onShutdown(Scene& scene);

        /// called before any component is ticked for this frame
        virtual void onPreTick(Scene& scene, const UpdateContext& ctx);

        /// called as part of tick update
        virtual void onTick(Scene& scene, const UpdateContext& ctx);

        /// called after all components were ticked but before any component was moved
        virtual void onPreTransform(Scene& scene, const UpdateContext& ctx);

        /// called as part of transform update
        virtual void onTransform(Scene& scene, const UpdateContext& ctx);

        /// called after all components were moved
        virtual void onPostTransform(Scene& scene, const UpdateContext& ctx);

        /// called during frame rendering to collect system specific debug fragments
        virtual void onRenderFrame(Scene& scene, rendering::scene::FrameInfo& info);

        /// world content was attached to scene
        virtual void onWorldContentAttached(Scene& scene, const WorldPtr& worldPtr);

        /// world content was detached from scene
        virtual void onWorldContentDetached(Scene& scene, const WorldPtr& worldPtr);
    };

    //---

    /// initialization order
    class SCENE_COMMON_API RuntimeSystemInitializationOrderMetadata : public base::rtti::IMetadata
    {
        RTTI_DECLARE_VIRTUAL_CLASS(RuntimeSystemInitializationOrderMetadata, base::rtti::IMetadata);

    public:
        RuntimeSystemInitializationOrderMetadata();
        virtual ~RuntimeSystemInitializationOrderMetadata();

        INLINE void order(int index)
        {
            m_order = index;
        }

        INLINE int order() const
        {
            return m_order;
        }

    private:
        int m_order;
    };

    //---

} // scene
/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world #]
***/

#pragma once

namespace game
{
    //---

    /// general system update order:
    ///  - pre tick
    ///  - tick (the tick system is ticked, only active nodes are ticked)
    ///  - pre transform
    ///  - transform (the transform system us updated, only on dirty nodes)
    ///  - post transform

    /// scene system, integrates scene with various engine and gameplay systems
    class GAME_SCENE_API IWorldSystem : public base::script::ScriptedObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(IWorldSystem, base::script::ScriptedObject);

    public:
        IWorldSystem();
        virtual ~IWorldSystem();

        /// initialize system, can fail
        virtual bool handleInitialize(World& scene);

        /// shutdown system, called once before destruction of scene
        /// NOTE: always called in revere order than the initialize calls
        virtual void handleShutdown(World& scene);

        /// called before any entity is ticked for this frame
        virtual void handlePreTick(World& scene, const UpdateContext& ctx);

        /// called as part of main tick phase, should start update jobs for the system
        virtual void handleMainTickStart(World& scene, const UpdateContext& ctx);

        /// called as part of main tick phase, should finish the jobs NOTE: touching components is still illegal here
        virtual void handleMainTickFinish(World& scene, const UpdateContext& ctx);

        /// called as part of main tick phase, should publish the result to components/entities
        virtual void handleMainTickPublish(World& scene, const UpdateContext& ctx);

        /// called after all entities were updated 
        virtual void handlePostTick(World& scene, const UpdateContext& ctx);

        /// called after all components were moved
        virtual void handlePostTransform(World& scene, const UpdateContext& ctx);

        /// prepare for rendering - mostly gathers parameters
        virtual void handlePrepareCamera(World& scene, rendering::scene::CameraSetup& outCamera);

        /// called during frame rendering to collect system specific debug fragments
        virtual void handleRendering(World& scene, rendering::scene::FrameParams& info);

        /// world content was attached to scene
        //virtual void onWorldContentAttached(Scene& scene, const WorldPtr& worldPtr);

        /// world content was detached from scene
        //virtual void onWorldContentDetached(Scene& scene, const WorldPtr& worldPtr);
    };

    //---

    /// world system dependency order, determines initialization and shutdown order
    class GAME_SCENE_API DependsOnWorldSystemMetadata : public base::rtti::IMetadata
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DependsOnWorldSystemMetadata, base::rtti::IMetadata);

    public:
        DependsOnWorldSystemMetadata();

        // add class to list
        template< typename T >
        INLINE DependsOnWorldSystemMetadata& dependsOn()
        {
            m_classList.pushBackUnique(T::GetStaticClass());
            return *this;
        }

        // get list of classes the service having this metadata depends on
        INLINE const base::Array<base::SpecificClassType<IWorldSystem>>& classes() const
        {
            return m_classList;
        }

    private:
        base::Array<base::SpecificClassType<IWorldSystem>> m_classList;
    };

    //---

    /// world ticking dependency, determines which systems must tick before or after us
    class GAME_SCENE_API TickOrderWorldSystemMetadata : public base::rtti::IMetadata
    {
        RTTI_DECLARE_VIRTUAL_CLASS(TickOrderWorldSystemMetadata, base::rtti::IMetadata);

    public:
        TickOrderWorldSystemMetadata();

        template< typename T >
        INLINE TickOrderWorldSystemMetadata& before()
        {
            m_beforeClassList.pushBackUnique(T::GetStaticClass());
            return *this;
        }

        template< typename T >
        INLINE TickOrderWorldSystemMetadata& after()
        {
            m_afterClassList.pushBackUnique(T::GetStaticClass());
            return *this;
        }

        INLINE const base::Array<base::SpecificClassType<IWorldSystem>>& beforeClasses() const { return m_beforeClassList; }
        INLINE const base::Array<base::SpecificClassType<IWorldSystem>>& afterClasses() const { return m_afterClassList; }

    private:
        base::Array<base::SpecificClassType<IWorldSystem>> m_beforeClassList;
        base::Array<base::SpecificClassType<IWorldSystem>> m_afterClassList;
    };

    //---

} // game
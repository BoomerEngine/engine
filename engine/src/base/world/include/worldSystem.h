/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world #]
***/

#pragma once

#include "base/script/include/scriptObject.h"

namespace base
{
    namespace world
    {
        //---

        /// general system update order:
        ///  - pre tick
        ///  - tick (the tick system is ticked, only active nodes are ticked)
        ///  - pre transform
        ///  - transform (the transform system us updated, only on dirty nodes)
        ///  - post transform

        /// scene system, integrates scene with various engine and gameplay systems
        class BASE_WORLD_API IWorldSystem : public script::ScriptedObject
        {
            RTTI_DECLARE_VIRTUAL_CLASS(IWorldSystem, script::ScriptedObject);

        public:
            IWorldSystem();
            virtual ~IWorldSystem();
            
            //---

            INLINE World* world() const { return m_world; }

            //---

            /// should this system be auto-intialized ?
            virtual bool autoInitialized() const { return true; }

            /// initialize system, can fail
            virtual bool handleInitialize(World& scene);

            /// shutdown system, called once before destruction of scene
            /// NOTE: always called in revere order than the initialize calls
            virtual void handleShutdown();

            /// called before any entity is ticked for this frame
            virtual void handlePreTick(double dt);

            /// called as part of main tick phase, should start update jobs for the system
            virtual void handleMainTickStart(double dt);

            /// called as part of main tick phase, should finish the jobs NOTE: touching components is still illegal here
            virtual void handleMainTickFinish(double dt);

            /// called as part of main tick phase, should publish the result to components/entities
            virtual void handleMainTickPublish(double dt);

            /// called after all entities were updated 
            virtual void handlePostTick(double dt);

            /// called after all components were moved
            virtual void handlePostTransform(double dt);

            /// called during frame rendering to collect system specific debug fragments
            virtual void handleRendering(rendering::scene::FrameParams& info);

            /// world content was attached to scene
            //virtual void onWorldContentAttached(Scene& scene, const WorldPtr& worldPtr);

            /// world content was detached from scene
            //virtual void onWorldContentDetached(Scene& scene, const WorldPtr& worldPtr);

        private:
            World* m_world = nullptr;
        };

        //---

        /// world system dependency order, determines initialization and shutdown order
        class BASE_WORLD_API DependsOnWorldSystemMetadata : public rtti::IMetadata
        {
            RTTI_DECLARE_VIRTUAL_CLASS(DependsOnWorldSystemMetadata, rtti::IMetadata);

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
            INLINE const Array<SpecificClassType<IWorldSystem>>& classes() const
            {
                return m_classList;
            }

        private:
            Array<SpecificClassType<IWorldSystem>> m_classList;
        };

        //---

        /// world ticking dependency, determines which systems must tick before or after us
        class BASE_WORLD_API TickOrderWorldSystemMetadata : public rtti::IMetadata
        {
            RTTI_DECLARE_VIRTUAL_CLASS(TickOrderWorldSystemMetadata, rtti::IMetadata);

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

            INLINE const Array<SpecificClassType<IWorldSystem>>& beforeClasses() const { return m_beforeClassList; }
            INLINE const Array<SpecificClassType<IWorldSystem>>& afterClasses() const { return m_afterClassList; }

        private:
            Array<SpecificClassType<IWorldSystem>> m_beforeClassList;
            Array<SpecificClassType<IWorldSystem>> m_afterClassList;
        };

        //---

    } // world
} // base
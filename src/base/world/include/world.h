/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world #]
***/

#pragma once

#include "base/memory/include/structurePool.h"
#include "base/object/include/object.h"

namespace base
{
    namespace world
    {

        ///----

        /// scene content observer
        struct BASE_WORLD_API WorldObserver
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(WorldObserver);

            AbsolutePosition position;
            Vector3 velocity;
            float maxStreamingRange = 0.0f;
        };

        ///----

        /// world ticking stats
        struct BASE_WORLD_API WorldStats
        {
            double totalUpdateTime = 0.0;

            uint32_t numEntites = 0;
            uint32_t numEntitiesAttached = 0;
            uint32_t numEntitiesDetached = 0;

            uint32_t numTransformUpdateBaches = 0;
            uint32_t numTransformUpdateEntities = 0;
            double transformUpdateTime = 0.0;

            uint32_t numPreTickLoops = 0;
            uint32_t numPreTickEntites = 0;
            double preTickSystemTime = 0.0;
            double preTickEntityTime = 0.0;
            double preTickFixupTime = 0.0;

            double mainTickTime = 0.0;

            uint32_t numPostTickEntites = 0;
            double postTickSystemTime = 0.0;
            double postTickEntityTime = 0.0;
        };

        ///----
    
        /// runtime game world class
        /// this is the THE place where everything is simulated, rendered, etc
        /// NOTE: we follow classical Entity-Component-System model here: entities groups components, most of the work is done in systems
        /// NOTE: this object is never saved neither are the components inside
        class BASE_WORLD_API World : public IObject
        {
            RTTI_DECLARE_POOL(POOL_WORLD_OBJECTS)
            RTTI_DECLARE_VIRTUAL_CLASS(World, IObject);

        public:
            World();
            virtual ~World();

            //---

            /// get last tick stats
            INLINE const WorldStats& stats() const { return m_stats; }

            //---

            /// get scene system
            /// NOTE: requesting system that does not exist if a fatal error, please check scene flags first
            template< typename T >
            INLINE T* system() const
            {
                static auto userIndex = reflection::ClassID<T>()->userIndex();
                ASSERT_EX(userIndex != -1, "Trying to access unregistered scene system");
                auto system  = (T*) m_systemMap[userIndex];
                ASSERT_EX(!system || system->cls()->is(T::GetStaticClass()), "Invalid system registered");
                return system;
            }

            /// get system pointer
            INLINE IWorldSystem* systemPtr(short index) const
            {
                return m_systemMap[index];
            }

            //---

            /// update scene and all the runtime systems
            /// NOTE: this function will wait for the internal tasks to finish
            CAN_YIELD virtual void update(double dt);

            /// render the world
            CAN_YIELD virtual void render(rendering::scene::FrameParams& info);

            /// render special debug gui with ImGui
            void renderDebugGui();

            ///--

            /// attach entity to the world
            /// NOTE: entity cannot be already attached
            /// NOTE: world will add a reference to the entity object
            /// NOTE: must be called on main thread
            void attachEntity(Entity* entity);

            /// detach previously attached entity from the world
            /// NOTE: world will remove a reference to the entity object (the entity object may get destroyed)
            /// NOTE: must be called on main thread
            void detachEntity(Entity* entity);

            ///---

            /// "spawn" prefab on the world, shorthand for prefab compilation, instantiation and attachment of the entities to the world
            EntityPtr createPrefabInstance(const AbsoluteTransform& placement, const Prefab* prefab, StringID appearance = "default"_id);

            ///---

        private:
            static const uint32_t MAX_SYSTEM = 64;
            IWorldSystem* m_systemMap[MAX_SYSTEM];
            Array<WorldSystemPtr> m_systems; // all systems registered in the scene

            void initializeSystems();
            void destroySystems();

            //--

            // TEMPSHIT!!!!
            Array<EntityPtr> m_addedEntities;
            Array<EntityPtr> m_removedEntities;
            HashSet<EntityPtr> m_entities; // all entities
            bool m_protectedEntityRegion = false;

            void destroyEntities();
            void applyEntityChanges(Array<Entity*>* outCreatedEntities, WorldStats& outStats);

            //--

            WorldStats m_stats;

            void updatePreTick(double dt, WorldStats& outStats);
            void updateSystems(double dt, WorldStats& outStats);
            void updatePostTick(double dt, WorldStats& outStats);

            //--

            struct TransformUpdateRequest
            {
                EntityWeakPtr entity = nullptr;
                AbsoluteTransform newTransform;
                bool hasNewTransform = false;
            };

            mem::StructurePool<TransformUpdateRequest> m_transformRequetsPool;
            HashMap<Entity*, TransformUpdateRequest*> m_transformRequestsMap;
            HashMap<Entity*, TransformUpdateRequest*> m_transformRequestsMap2;

            void cancelAllTransformRequests();
            void cancelTransformRequest(Entity* entity);
            void scheduleEntityForTransformUpdate(Entity* entity);
            void scheduleEntityForTransformUpdateWithTransform(Entity* entity, const AbsoluteTransform& newTransform);
            void serviceTransformRequests(WorldStats& outStats);

            friend class Entity;
        
        };

        //---

    } // base
} // world
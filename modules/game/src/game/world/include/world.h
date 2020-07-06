/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world #]
***/

#pragma once

#include "base/script/include/scriptObject.h"
#include "base/memory/include/structurePool.h"

namespace game
{
    ///----

    /// scene content observer
    struct GAME_WORLD_API WorldObserver
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(WorldObserver);

        base::AbsolutePosition position;
        base::Vector3 velocity;
        float maxStreamingRange = 0.0f;
    };

    ///----

    /// world ticking stats
    struct GAME_WORLD_API WorldStats
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
    
    /// runtime game world, can be created empty or based on a world definition
    /// this is the THE place where everything is simulated and rendered
    /// NOTE: this object is never saved neither are the components inside
    class GAME_WORLD_API World : public base::script::ScriptedObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(World, base::script::ScriptedObject);

    public:
        World(WorldType type = WorldType::Game, const WorldDefinitionPtr& data = nullptr);
        virtual ~World();

        //---

        /// get type of the world
        INLINE WorldType type() const { return m_type; }

        /// get world data
        INLINE const WorldDefinition& data() const { return *m_data; }

        /// get last tick stats
        INLINE const WorldStats& stats() const { return m_stats; }

        //---

        /// get scene system
        /// NOTE: requesting system that does not exist if a fatal error, please check scene flags first
        template< typename T >
        INLINE T* system() const
        {
            static auto userIndex = base::reflection::ClassID<T>()->userIndex();
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

        /// service input (usually passes it to the input system)
        bool processInput(const base::input::BaseEvent& evt);

        /// update scene and all the runtime systems
        /// NOTE: this function will wait for the internal tasks to finish
        CAN_YIELD void update(double dt);

        /// render the scene
        CAN_YIELD void render(rendering::scene::FrameParams& info);

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
        EntityPtr createPrefabInstance(const base::AbsoluteTransform& placement, const Prefab* prefab);

        ///---

    private:
        WorldType m_type; // type of the scene

        //--

        static const uint32_t MAX_SYSTEM = 64;
        IWorldSystem* m_systemMap[MAX_SYSTEM];
        base::Array<WorldSystemPtr> m_systems; // all systems registered in the scene

        void initializeSystems();
        void destroySystems();

        //--

        WorldDefinitionPtr m_data; // immutable

        //--

        // TEMPSHIT!!!!
        base::Array<EntityPtr> m_addedEntities;
        base::Array<EntityPtr> m_removedEntities;
        base::HashSet<EntityPtr> m_entities; // all entities
        bool m_protectedEntityRegion = false;

        void destroyEntities();
        void applyEntityChanges(base::Array<Entity*>* outCreatedEntities, WorldStats& outStats);

        //--

        WorldStats m_stats;

        void updatePreTick(double dt, WorldStats& outStats);
        void updateSystems(double dt, WorldStats& outStats);
        void updatePostTick(double dt, WorldStats& outStats);

        //--

        struct TransformUpdateRequest
        {
            EntityWeakPtr entity = nullptr;
            base::AbsoluteTransform newTransform;
            bool hasNewTransform = false;
        };

        base::mem::StructurePool<TransformUpdateRequest> m_transformRequetsPool;
        base::HashMap<Entity*, TransformUpdateRequest*> m_transformRequestsMap;
        base::HashMap<Entity*, TransformUpdateRequest*> m_transformRequestsMap2;

        void cancelAllTransformRequests();
        void cancelTransformRequest(Entity* entity);
        void scheduleEntityForTransformUpdate(Entity* entity);
        void scheduleEntityForTransformUpdateWithTransform(Entity* entity, const base::AbsoluteTransform& newTransform);
        void serviceTransformRequests(WorldStats& outStats);

        friend class Entity;
        
    };

    //---

} // game
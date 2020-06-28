/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world #]
***/

#pragma once

#include "base/script/include/scriptObject.h"

namespace game
{
    ///----

    /// scene content observer
    struct GAME_SCENE_API WorldObserver
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(WorldObserver);

        base::AbsolutePosition position;
        base::Vector3 velocity;
        float maxStreamingRange = 0.0f;
    };

    ///----
    
    /// runtime game world, can be created empty or based on a world definition
    /// this is the THE place where everything is simulated and rendered
    /// NOTE: this object is never saved neither are the components inside
    class GAME_SCENE_API World : public base::script::ScriptedObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(World, base::script::ScriptedObject);

    public:
        World(WorldType type, const WorldDefinitionPtr& data);
        virtual ~World();

        //---

        /// get type of the world
        INLINE WorldType type() const { return m_type; }

        /// get world data
        INLINE const WorldDefinition& data() const { return *m_data; }

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
        CAN_YIELD void update(const UpdateContext& ctx);

        /// render the scene
        CAN_YIELD void render(rendering::scene::FrameParams& info);

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
        void applyEntityChanges(base::Array<Entity*>* outCreatedEntities);

        //--

        void updatePreTick(const UpdateContext& ctx);
        void updateSystems(const UpdateContext& ctx);
        void updatePostTick(const UpdateContext& ctx);
        void updateTransform(const UpdateContext& ctx);

        //--
        
    };

    //---

} // game
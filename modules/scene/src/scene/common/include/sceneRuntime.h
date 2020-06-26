/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#pragma once

#include "base/script/include/scriptObject.h"

namespace scene
{
    ///----

    /// scene content observer
    struct SCENE_COMMON_API SceneObserver
    {
        int m_id;
        base::AbsolutePosition m_position;
        base::Vector3 m_velocity;
        float m_maxStreamingRange = 0.0f;
    };

    ///----
    
    /// runtime scene
    /// this is the master scene that is simulated and rendered
    /// NOTE: this object is never saved neither are the components inside
    class SCENE_COMMON_API Scene : public base::script::ScriptedObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(Scene, base::script::ScriptedObject);

    public:
        Scene(SceneType type);
        virtual ~Scene();

        /// get type of the scene
        INLINE SceneType type() const { return m_type; }
                
        /// get list of registered content observers
        INLINE const base::Array<SceneObserver>& observers() const { return m_observers; }

        /// get scene system
        /// NOTE: requesting system that does not exist if a fatal error, please check scene flags first
        template< typename T >
        INLINE T* system() const
        {
            auto userIndex = base::reflection::ClassID<T>()->userIndex();
            ASSERT_EX(userIndex != -1, "Trying to access unregistered scene system");
            auto system  = (T*) m_systemMap[userIndex];
            ASSERT_EX(!system || system->cls()->is(T::GetStaticClass()), "Invalid system registered");
            return system;
        }

        /// update scene and all the runtime systems
        /// NOTE: this function will wait for the internal tasks to finish
        CAN_YIELD void update(const UpdateContext& ctx);

        /// render the scene
        CAN_YIELD void render(rendering::scene::FrameInfo& info);

        ///--

        /// create a scene observer, it will affect the loaded content
        int createObserver(const base::AbsolutePosition& initialPosition);

        /// update observer position
        void updateObserver(int id, const base::AbsolutePosition& newPosition);

        /// remove observer
        void removeObserver(int id);

        ///---

        /// attach world content to scene
        void attachWorldContent(const WorldPtr& worldPtr);

        /// detach previously attach world content form the scene
        void detachWorldContent(const WorldPtr& worldPtr);

        ///---

    private:
        ///----

        static const uint32_t MAX_SYSTEM = 32;

        IRuntimeSystem* m_systemMap[MAX_SYSTEM];

        SceneType m_type; // type of the scene

        base::Array<SystemPtr> m_systems; // all systems registered in the scene

        //--

        int m_nextObserverID;
        base::Array<SceneObserver> m_observers;

        base::Array<WorldPtr> m_worldContent;

        //--

        void initializeSceneSystems();
    };

} // scene
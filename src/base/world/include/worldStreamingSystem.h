/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\streaming #]
***/

#pragma once

#include "worldSystem.h"

namespace base
{
    namespace world
    {
        //--

        /// basic streaming observer
        class BASE_WORLD_API StreamingObserver : public IObject
        {
            RTTI_DECLARE_VIRTUAL_CLASS(StreamingObserver, IObject);

        public:
            StreamingObserver();

            /// get current position
            INLINE const Vector3& position() const { return m_position; }

            /// set new position
            void move(const Vector3& position);

        public:
            Vector3 m_position;
        };        

        //--

        /// world streaming system capable of loading dynamic content from compiled scene
        class BASE_WORLD_API StreamingSystem : public IWorldSystem
        {
            RTTI_DECLARE_VIRTUAL_CLASS(StreamingSystem, IWorldSystem);

        public:
            StreamingSystem();
            virtual ~StreamingSystem();

            //--

            /// unbind all loaded content
            void unbindEntities();

            /// attach compiled scene content
            void bindScene(CompiledScene* scene);

            //--

            /// attach streaming observer
            void attachObserver(StreamingObserver* observer);

            /// remove streaming observer
            void dettachObserver(StreamingObserver* observer);

            //--

        protected:
            virtual void handlePreTick(double dt) override;
            virtual void handleShutdown() override;

            //--

            Array<RefPtr<StreamingObserver>> m_observers;

            RefPtr<CompiledScene> m_compiledScene;

            //--

            struct IslandLoading : public IReferencable
            {
                RefPtr<StreamingIsland> data;
                std::atomic<bool> finished = false;
                RefPtr<StreamingIslandInstance> loadedData;
            };

            struct IslandState : public IReferencable
            {
                Box streamingBox;
                RefPtr<StreamingIsland> data;
                RefPtr<IslandLoading> loading;
                RefPtr<StreamingIslandInstance> attached;
            };

            Array<RefPtr<IslandState>> m_islands;

            void updateIslandStates();

            //--

            struct SectorLoading : public IReferencable
            {
                StreamingSectorAsyncRef data;

                std::atomic<bool> finished = false;
                RefPtr<StreamingSector> loadedData;
            };

            struct SectorState : public IReferencable
            {
                Box streamingBox;
                StreamingSectorAsyncRef data;

                RefPtr<SectorLoading> loading;

                Array<RefPtr<IslandState>> loadedIslands;
            };

            Array<RefPtr<SectorState>> m_sectors;

            void updateSectorStates();
            void extractSectorIslands(SectorState* sector, StreamingIsland* island, IslandState* parentIsland);

            //--
        };

        //--

    } // world
} // base
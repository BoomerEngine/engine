/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\data #]
***/

#pragma once

#include "base/resources/include/resource.h"

namespace game
{
    ///----

    /// global scene configuration parameters for a world, cooked and available as part of the compiled world
    /// can be used by various runtime systems to configure them
    class GAME_WORLD_API IWorldParameters : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(IWorldParameters, base::IObject);

    public:
        IWorldParameters();
        virtual ~IWorldParameters();

        /// render debug stuff related to the parameters into the viewport
        /// NOTE: used only in the editor when we are editing the parameter
        virtual void renderDebug(bool edited, rendering::scene::FrameParams& info) const;
    };

    //--

    /// world definition file - contains all the data for the world, usually consumed by the world instance
    class GAME_WORLD_API WorldDefinition : public base::res::IResource
    {
        RTTI_DECLARE_VIRTUAL_CLASS(WorldDefinition, base::res::IResource);

    public:
        WorldDefinition();
        virtual ~WorldDefinition();

        //---

        /// get all world parameter blocks
        typedef base::Array<WorldParametersPtr> TParameters;
        INLINE const TParameters& allParameters() const { return m_unorderedParameters; }

        /// find parameters for given class
        IWorldParameters* findParameters(base::ClassType paramClass) const;

        /// get scene parameters
        template< typename T >
        INLINE T* parameters() const
        {
            auto userIndex = base::reflection::ClassID<T>()->userIndex();
            ASSERT_EX(userIndex != -1, "Trying to access unregistered scene parameter block");
            return static_cast<T*>(m_parametersTable[userIndex]);
        }

        //---

        /// get informations about streaming sectors in the world
        INLINE const WorldSectorsAsyncRef& sectors() const { return m_sectors; }

        //---

    protected:
        /// world parametrization
        TParameters m_unorderedParameters;
        base::Array<IWorldParameters*> m_parametersTable; // ordered list

        /// world sectors
        WorldSectorsAsyncRef m_sectors;

        //--

        void createParametersMap();

        virtual void onPostLoad() override;
    };

    //--

} // game
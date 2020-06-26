/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#pragma once

#include "base/object/include/object.h"

namespace scene
{

    ///----

    /// global scene configuration parameters for a world, cooked and available as part of the compiled world
    /// can be used by various runtime systems to configure them
    class SCENE_COMMON_API IWorldParameters : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(IWorldParameters, base::IObject);

    public:
        IWorldParameters();
        virtual ~IWorldParameters();

        /// render debug stuff related to the parameters into the viewport
        virtual void renderDebug(rendering::scene::FrameInfo& info) const;
    };

    ///----

    /// container for scene parameters
    class SCENE_COMMON_API WorldParameterContainer : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(WorldParameterContainer, base::IObject);

    public:
        WorldParameterContainer();
        virtual ~WorldParameterContainer();

        /// get all parameters
        typedef base::Array<base::RefPtr<IWorldParameters>> TParameters;
        INLINE const TParameters& allParameters() const { return m_parameters; }

        /// find parameters for given class
        const base::RefPtr<IWorldParameters> findParameters(base::ClassType paramClass) const;

        /// get scene parameters
        template< typename T >
        INLINE const T& parameters() const
        {
            auto userIndex = base::reflection::ClassID<T>()->userIndex();
            ASSERT_EX(userIndex != -1, "Trying to access unregistered scene parameter block");
            return *static_cast<const T*>(m_parametersTable[userIndex]);
        }

        // make sure all parameters types are accounted for
        void createParameters();

    private:
        TParameters m_parameters;
        base::Array<const IWorldParameters*> m_parametersTable;

        virtual void onPostLoad() override final;
    };

    ///----

} // scene
/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: driver\objects\sampler #]
***/

#include "build.h"
#include "glSampler.h"
#include "glDriver.h"
#include "glObjectCache.h"

namespace rendering
{
    namespace gl4
    {

        //--

        Sampler::Sampler(Driver* drv, const SamplerState& info)
            : Object(drv, ObjectType::Sampler)
            , m_setup(info)
        {}

        bool Sampler::CheckClassType(ObjectType type)
        {
            return (type == ObjectType::Sampler);
        }

        void Sampler::finalizeCreation()
        {
            if (m_glSampler == 0)
                m_glSampler = driver()->objectCache().resolveSampler(m_setup);
        }

        //--

    } // gl4
} // driver

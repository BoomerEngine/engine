/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects\sampler #]
***/

#include "build.h"
#include "glSampler.h"
#include "glDevice.h"
#include "glObjectCache.h"

namespace rendering
{
    namespace gl4
    {

        //--

        Sampler::Sampler(Device* drv, const SamplerState& info)
            : Object(drv, ObjectType::Sampler)
            , m_setup(info)
        {}

        void Sampler::finalizeCreation()
        {
            if (m_glSampler == 0)
                m_glSampler = device()->objectCache().resolveSampler(m_setup);
        }

        //--

    } // gl4
} // rendering

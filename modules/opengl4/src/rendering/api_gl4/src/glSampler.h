/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: driver\objects\sampler #]
***/

#pragma once

#include "glObject.h"

namespace rendering
{
    namespace gl4
    {
        ///---

        /// wrapper for sampler object
        class RENDERING_API_GL4_API Sampler : public Object
        {
        public:
            Sampler(Driver* drv, const SamplerState& info);

            INLINE GLuint deviceObject() { finalizeCreation(); return m_glSampler; }

            static bool CheckClassType(ObjectType type);

        private:
            GLuint m_glSampler = 0; // only when finalized
            SamplerState m_setup;

            void finalizeCreation();
        };

        ///---

    } // gl4
} // driver


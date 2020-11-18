/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects\shaders #]
***/

#pragma once

#include "glObject.h"

#include "rendering/device/include/renderingShaderLibrary.h"

namespace rendering
{
    namespace gl4
    {
        struct ResolvedParameterBindingState;
        struct ResolvedVertexBindingState;

        /// loaded shaders, this object mainly servers as caching interface to object cache
        class Shaders : public Object
        {
        public:
            Shaders(Device* device, const ShaderLibraryData* data);
            virtual ~Shaders();

            static const auto STATIC_TYPE = ObjectType::Shaders;

            //--

            INLINE const ShaderLibraryData& data() const { return *m_data; }
           
            //--

            const ResolvedParameterBindingState* parameterBindingState(PipelineIndex index) const;

            const ResolvedVertexBindingState* vertexState(PipelineIndex index) const;

            GLuint shaderBundle(PipelineIndex index) const;

            //--            

        private:
            ShaderLibraryDataPtr m_data;

            // objects in side a shader library are indexed by numbers take advantage of it by creating a second cache level
            mutable base::Array<GLuint> m_shaderBundleMap;
            mutable base::Array<ResolvedVertexBindingState*> m_vertexStateMap;
            mutable base::Array<ResolvedParameterBindingState*> m_parameterBindingStateMap;
        };

    } // gl4
} // rendering
        

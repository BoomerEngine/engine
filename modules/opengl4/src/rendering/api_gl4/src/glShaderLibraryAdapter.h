/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: driver\shaders #]
***/

#pragma once

#include "glObject.h"

#include "rendering/driver/include/renderingShaderLibrary.h"

namespace rendering
{
    namespace gl4
    {
        struct ResolvedParameterBindingState;
        struct ResolvedVertexBindingState;

        /// loaded shaders, this object mainly servers as caching interface to object cache
        class ShaderLibraryAdapter : public Object
        {
        public:
            ShaderLibraryAdapter(Driver* driver, const ShaderLibraryData* data);
            virtual ~ShaderLibraryAdapter();

            //--

            INLINE const ShaderLibraryData& data() const { return *m_data; }
           
            //--

            const ResolvedParameterBindingState* parameterBindingState(PipelineIndex index) const;

            const ResolvedVertexBindingState* vertexState(PipelineIndex index) const;

            GLuint shaderBundle(PipelineIndex index) const;

            //--

            static bool CheckClassType(ObjectType type);

        private:
            ShaderLibraryDataPtr m_data;

            // objects in side a shader library are indexed by numbers take advantage of it by creating a second cache level
            mutable base::Array<GLuint> m_shaderBundleMap;
            mutable base::Array<ResolvedVertexBindingState*> m_vertexStateMap;
            mutable base::Array<ResolvedParameterBindingState*> m_parameterBindingStateMap;
        };

    } // gl4
} // driver
        

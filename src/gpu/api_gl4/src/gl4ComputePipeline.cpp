/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "gl4ComputePipeline.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::gl4)
				
//--

ComputePipeline::ComputePipeline(Thread* owner, const Shaders* shaders)
    : IBaseComputePipeline(owner, shaders)
{}

ComputePipeline::~ComputePipeline()
{}

bool ComputePipeline::apply(GLuint& glActiveProgram)
{
    if (GLuint glProgram = shaders()->object())
    {
        if (glProgram != glActiveProgram)
        {
            GL_PROTECT(glBindProgramPipeline(glProgram));
            glActiveProgram = glProgram;
        }

        return true;
    }

    return false;
}

//--

END_BOOMER_NAMESPACE_EX(gpu::api::gl4)
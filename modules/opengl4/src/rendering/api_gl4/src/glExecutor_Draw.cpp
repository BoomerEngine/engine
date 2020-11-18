/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\frame\execution #]
***/

#include "build.h"
#include "glExecutor.h"
#include "glShaders.h"
#include "glDevice.h"
#include "glObjectCache.h"
#include "glObjectRegistry.h"

namespace rendering
{
    namespace gl4
    {
        namespace exec
        {

            //--

            void FrameExecutor::runBindVertexBuffer(const command::OpBindVertexBuffer& op)
            {
                if (op.bindpoint)
                {
                    auto bindPointIndex = m_objectCache.resolveVertexBindPointIndex(op.bindpoint);
                    m_geometry.vertexBindings.prepare(bindPointIndex+1);

                    if (m_geometry.vertexBindings[bindPointIndex] != op.buffer)
                    {
                        m_geometry.vertexBindings[bindPointIndex] = op.buffer;
                        m_geometry.vertexBindingsChanged = true;
                    }
                }
            }

            void FrameExecutor::runBindIndexBuffer(const command::OpBindIndexBuffer& op)
            {
                if (m_geometry.indexStreamBinding != op.buffer || m_geometry.indexFormat != op.format)
                {
                    m_geometry.indexStreamBinding = op.buffer;
                    m_geometry.indexFormat = op.format;
                    m_geometry.indexBindingsChanged = true;
                }
            }

            bool FrameExecutor::applyVertexData(const Shaders& shaders, PipelineIndex vertexStateIndex)
            {
                // get the vertex array object with the cached layout of the vertices
                if (const auto* vertexBinding = shaders.vertexState(vertexStateIndex))
                {
                    GL_PROTECT(glBindVertexArray(vertexBinding->glVertexArrayObject));

                    const auto numStreams = vertexBinding->vertexBindPoints.size();
                    const auto* bindingInfo = vertexBinding->vertexBindPoints.typedData();
                    for (uint32_t i = 0; i < numStreams; ++i, ++bindingInfo)
                    {
                        if (bindingInfo->bindPointIndex < m_geometry.vertexBindings.size())
                        {
                            if (const auto& bufferView = m_geometry.vertexBindings[bindingInfo->bindPointIndex])
                            {
                                auto buffer = resolveUntypedBufferView(bufferView);
                                if (buffer.glBuffer != 0)
                                {
                                    ASSERT_EX(glIsBuffer(buffer.glBuffer), "Object is not a buffer");
                                    GL_PROTECT(glBindVertexBuffer(i, buffer.glBuffer, buffer.offset, bindingInfo->stride));
                                }
                                else
                                {
                                    TRACE_ERROR("Unable to resolve vertex buffer for bind point '{}', object '{}'", bindingInfo->name, bufferView);
                                    return false;
                                }
                            }
                            else
                            {
                                TRACE_ERROR("Missing vertex buffer for bind point '{}'", bindingInfo->name);
                                return false;
                            }
                        }
                        else
                        {
                            TRACE_ERROR("Missing vertex buffer for bind point '{}'", bindingInfo->name);
                            return false;
                        }
                    }

                    // unbind buffers from unused slots
                    for (uint32_t i = numStreams; i < m_geometry.maxBoundVertexStreams; ++i)
                        GL_PROTECT(glBindVertexBuffer(i, 0, 0, 0));
                    m_geometry.maxBoundVertexStreams = numStreams;
                    return true;
                };

                // ups
                return false;
            }

            bool FrameExecutor::applyIndexData()
            {
                bool ret = false;

                if (m_geometry.indexStreamBinding.empty())
                {
                    GL_PROTECT(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
                    m_geometry.indexBufferOffset = 0;
                }
                else
                {
                    auto buffer = resolveUntypedBufferView(m_geometry.indexStreamBinding);
                    if (buffer.glBuffer != 0 && glIsBuffer(buffer.glBuffer))
                    {
                        GL_PROTECT(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer.glBuffer));
                        m_geometry.indexBufferOffset = buffer.offset;
                        ret = true;
                    }
                    else
                    {
                        TRACE_ERROR("Unable to resolve index buffer");
                        m_geometry.indexBufferOffset = 0;
                    }
                }

                return ret;
            }

            bool FrameExecutor::prepareDraw(const Shaders& shaders, PipelineIndex programIndex, bool usesIndices)
            {
                DEBUG_CHECK_EX(programIndex != INVALID_PIPELINE_INDEX, "Draw with invalid program");
                if (programIndex == INVALID_PIPELINE_INDEX)
                    return false;

                if (m_program.activeDrawShaderLibrary != shaders.handle() || m_program.activeDrawShaderBundleIndex != programIndex)
                {
                    m_program.activeDrawShaderLibrary = shaders.handle();
                    m_program.activeDrawShaderBundleIndex = programIndex;
                    m_program.glActiveDrawLinkedProgramObject = shaders.shaderBundle(m_program.activeDrawShaderBundleIndex);
                    m_geometry.vertexBindingsChanged = true;
                    m_geometry.indexBindingsChanged = true;
                    m_params.parameterBindingsChanged = true;
                }

                if (!m_program.glActiveDrawLinkedProgramObject)
                    return false;

                auto& bundleSetup = shaders.data().shaderBundles()[m_program.activeDrawShaderBundleIndex];

                if (m_geometry.vertexBindingsChanged)
                {
                    if (!applyVertexData(shaders, bundleSetup.vertexBindingState))
                        return false;
                    m_geometry.vertexBindingsChanged = false;
                }

                if (m_geometry.indexBindingsChanged)
                {
                    if (usesIndices && !applyIndexData())
                        return false;
                    m_geometry.indexBindingsChanged = false;
                }

                if (m_params.parameterBindingsChanged)
                {
                    if (!applyParameters(shaders, bundleSetup.parameterBindingState))
                        return false;
                    m_params.parameterBindingsChanged = false;
                }

                if (m_program.glActiveDrawLinkedProgramObject != m_program.glActiveProgram)
                {
                    m_program.glActiveProgram = m_program.glActiveDrawLinkedProgramObject;
                    GL_PROTECT(glBindProgramPipeline(m_program.glActiveProgram));
                }

                applyDirtyRenderStates();

                return true;
            }

            bool FrameExecutor::prepareDispatch(const Shaders& shaders, PipelineIndex programIndex)
            {
                DEBUG_CHECK_EX(programIndex != INVALID_PIPELINE_INDEX, "Dispatch with invalid program");
                if (programIndex == INVALID_PIPELINE_INDEX)
                    return false;

                if (m_program.activeComputeShaderLibrary != shaders.handle() || m_program.activeDispatchShaderBundleIndex != programIndex)
                {
                    m_program.activeComputeShaderLibrary = shaders.handle();
                    m_program.activeDispatchShaderBundleIndex = programIndex;
                    m_program.glActiveDispatchLinkedProgramObject = shaders.shaderBundle(programIndex);
                    m_params.parameterBindingsChanged = true;
                }

                if (!m_program.glActiveDispatchLinkedProgramObject)
                    return false;

                auto& bundleSetup = shaders.data().shaderBundles()[m_program.activeDispatchShaderBundleIndex];

                if (m_params.parameterBindingsChanged)
                {
                    if (!applyParameters(shaders, bundleSetup.parameterBindingState))
                        return false;
                    m_params.parameterBindingsChanged = false;
                }

                if (m_program.glActiveDispatchLinkedProgramObject != m_program.glActiveProgram)
                {
                    m_program.glActiveProgram = m_program.glActiveDispatchLinkedProgramObject;
                    GL_PROTECT(glBindProgramPipeline(m_program.glActiveProgram));
                }

                return true;
            }

            void FrameExecutor::runDraw(const command::OpDraw& op)
            {
                if (const auto* shaders = m_objectRegistry.resolveStatic<Shaders>(op.shaderLibrary))
                {
                    if (prepareDraw(*shaders, op.shaderIndex, false))
                    {
                        if (op.numInstances > 1 || op.firstInstance)
                        {
                            GL_PROTECT(glDrawArraysInstancedBaseInstance(m_render.polygon.topology, op.firstVertex, op.vertexCount, op.numInstances, op.firstInstance));
                        }
                        else
                        {
                            GL_PROTECT(glDrawArrays(m_render.polygon.topology, op.firstVertex, op.vertexCount));
                        }
                    }
                }
            }

            void FrameExecutor::runDrawIndexed(const command::OpDrawIndexed& op)
            {
                if (const auto* shaders = m_objectRegistry.resolveStatic<Shaders>(op.shaderLibrary))
                {
                    if (prepareDraw(*shaders, op.shaderIndex, true))
                    {
                        GLenum indexType = 0;
                        uint32_t indexSize = 0;
                        uint32_t indexOffset = m_geometry.indexBufferOffset;
                        if (m_geometry.indexFormat == ImageFormat::R16_UINT)
                        {
                            indexType = GL_UNSIGNED_SHORT;
                            indexSize = 2;
                        }
                        else if (m_geometry.indexFormat == ImageFormat::R32_UINT)
                        {
                            indexType = GL_UNSIGNED_INT;
                            indexSize = 4;
                        }

                        if (op.numInstances > 1 || op.firstInstance > 0)
                        {
                            GL_PROTECT(glDrawElementsInstancedBaseVertexBaseInstance(m_render.polygon.topology, op.indexCount, indexType, (uint8_t*) nullptr + op.firstIndex * indexSize + indexOffset, op.numInstances, op.firstVertex, op.firstInstance));
                        }
                        else
                        {
                            GL_PROTECT(glDrawElementsBaseVertex(m_render.polygon.topology, op.indexCount, indexType, (uint8_t*) nullptr + op.firstIndex * indexSize + indexOffset, op.firstVertex));
                        }
                    }
                }
            }

            void FrameExecutor::runDispatch(const command::OpDispatch& op)
            {
                if (const auto* shaders = m_objectRegistry.resolveStatic<Shaders>(op.shaderLibrary))
                {
                    if (prepareDispatch(*shaders, op.shaderIndex))
                    {
                        GL_PROTECT(glDispatchCompute(op.counts[0], op.counts[1], op.counts[2]));
                    }
                }
            }

        } // exe
    } // gl4
} // rendering

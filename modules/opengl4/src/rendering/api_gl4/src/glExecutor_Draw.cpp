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

            
            void FrameExecutor::applyVertexData(const Shaders& shaders, PipelineIndex vertexStateIndex)
            {
                // get the vertex array object with the cached layout of the vertices
                if (const auto* vertexBinding = shaders.vertexState(vertexStateIndex))
                {
                    GL_PROTECT(glBindVertexArray(vertexBinding->glVertexArrayObject));

                    const auto numStreams = vertexBinding->vertexBindPoints.size();
                    const auto* bindingInfo = vertexBinding->vertexBindPoints.typedData();
                    for (uint32_t i = 0; i < numStreams; ++i, ++bindingInfo)
                    {
						DEBUG_CHECK_EX(bindingInfo->bindPointIndex < m_geometry.vertexBindings.size(), base::TempString("Missing vertex buffer for bind point '{}'", bindingInfo->name));

                        const auto& bufferView = m_geometry.vertexBindings[bindingInfo->bindPointIndex];
						DEBUG_CHECK_EX(bufferView.id, base::TempString("Missing vertex buffer for bind point '{}'", bindingInfo->name));

                        auto buffer = resolveGeometryBufferView(bufferView.id, bufferView.offset);
						DEBUG_CHECK_EX(buffer.glBuffer != 0, base::TempString("Vertex buffer for bind point '{}' unloaded while command buffer was being recorded", bindingInfo->name));
						DEBUG_CHECK_EX(glIsBuffer(buffer.glBuffer), "Object is not a buffer");

                        GL_PROTECT(glBindVertexBuffer(i, buffer.glBuffer, buffer.offset, bindingInfo->stride));
					}

                    // unbind buffers from unused slots
                    for (uint32_t i = numStreams; i < m_geometry.maxBoundVertexStreams; ++i)
                        GL_PROTECT(glBindVertexBuffer(i, 0, 0, 0));
                    m_geometry.maxBoundVertexStreams = numStreams;
                };
            }

            void FrameExecutor::applyIndexData()
            {
                if (m_geometry.indexStreamBinding.id.empty())
                {
                    GL_PROTECT(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
					m_geometry.indexStreamBinding = GeometryBuffer();
                }
                else
                {
                    auto buffer = resolveGeometryBufferView(m_geometry.indexStreamBinding.id, m_geometry.indexStreamBinding.offset);
					DEBUG_CHECK_EX(buffer.glBuffer != 0, "Index buffer unloaded while command buffer was being recorded");
					DEBUG_CHECK_EX(glIsBuffer(buffer.glBuffer), "Object is not a buffer");

                    GL_PROTECT(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer.glBuffer));
                    m_geometry.finalIndexStreamOffset = buffer.offset;
                }
            }

            void FrameExecutor::prepareDraw(const Shaders& shaders, PipelineIndex programIndex, bool usesIndices)
            {
				ASSERT_EX(programIndex != INVALID_PIPELINE_INDEX, "Draw with invalid program");

                if (m_program.activeDrawShaderLibrary != shaders.handle() || m_program.activeDrawShaderBundleIndex != programIndex)
                {
                    m_program.activeDrawShaderLibrary = shaders.handle();
                    m_program.activeDrawShaderBundleIndex = programIndex;
                    m_program.glActiveDrawLinkedProgramObject = shaders.shaderBundle(m_program.activeDrawShaderBundleIndex);
                    m_geometry.vertexBindingsChanged = true;
                    m_geometry.indexBindingsChanged = true;
                    m_descriptors.descriptorsChanged = true;
                }

                /*if (!m_program.glActiveDrawLinkedProgramObject)
                    return false;*/

                auto& bundleSetup = shaders.data().shaderBundles()[m_program.activeDrawShaderBundleIndex];

                if (m_geometry.vertexBindingsChanged)
                {
					applyVertexData(shaders, bundleSetup.vertexBindingState);
                    m_geometry.vertexBindingsChanged = false;
                }

                if (usesIndices && m_geometry.indexBindingsChanged)
                {
					applyIndexData();
					m_geometry.indexBindingsChanged = false;
                }

                if (m_descriptors.descriptorsChanged)
                {
					applyDescriptors(shaders, bundleSetup.parameterBindingState);
					m_descriptors.descriptorsChanged = false;
                }

                if (m_program.glActiveDrawLinkedProgramObject != m_program.glActiveProgram)
                {
                    m_program.glActiveProgram = m_program.glActiveDrawLinkedProgramObject;
                    GL_PROTECT(glBindProgramPipeline(m_program.glActiveProgram));
                }

                applyDirtyRenderStates();
            }

            void FrameExecutor::prepareDispatch(const Shaders& shaders, PipelineIndex programIndex)
            {
                ASSERT_EX(programIndex != INVALID_PIPELINE_INDEX, "Dispatch with invalid program");

                if (m_program.activeComputeShaderLibrary != shaders.handle() || m_program.activeDispatchShaderBundleIndex != programIndex)
                {
                    m_program.activeComputeShaderLibrary = shaders.handle();
                    m_program.activeDispatchShaderBundleIndex = programIndex;
                    m_program.glActiveDispatchLinkedProgramObject = shaders.shaderBundle(programIndex);
                    m_descriptors.descriptorsChanged = true;
                }

                /*if (!m_program.glActiveDispatchLinkedProgramObject)
                    return false;*/

                auto& bundleSetup = shaders.data().shaderBundles()[m_program.activeDispatchShaderBundleIndex];

                if (m_descriptors.descriptorsChanged)
                {
					applyDescriptors(shaders, bundleSetup.parameterBindingState);
					m_descriptors.descriptorsChanged = false;
                }

                if (m_program.glActiveDispatchLinkedProgramObject != m_program.glActiveProgram)
                {
                    m_program.glActiveProgram = m_program.glActiveDispatchLinkedProgramObject;
                    GL_PROTECT(glBindProgramPipeline(m_program.glActiveProgram));
                }
            }

            void FrameExecutor::runDraw(const command::OpDraw& op)
            {
				const auto* shaders = m_objectRegistry.resolveStatic<Shaders>(op.shaderLibrary);
				DEBUG_CHECK_RETURN_EX(shaders, "Shaders unloaded while command buffer was pending processing");

				prepareDraw(*shaders, op.shaderIndex, false);
                if (op.numInstances > 1 || op.firstInstance)
                {
                    GL_PROTECT(glDrawArraysInstancedBaseInstance(m_render.polygon.topology, op.firstVertex, op.vertexCount, op.numInstances, op.firstInstance));
                }
                else
                {
                    GL_PROTECT(glDrawArrays(m_render.polygon.topology, op.firstVertex, op.vertexCount));
                }
            }

            void FrameExecutor::runDrawIndexed(const command::OpDrawIndexed& op)
            {
				const auto* shaders = m_objectRegistry.resolveStatic<Shaders>(op.shaderLibrary);
				DEBUG_CHECK_RETURN_EX(shaders, "Shaders unloaded while command buffer was pending processing");

				prepareDraw(*shaders, op.shaderIndex, true);

                GLenum indexType = 0;
                uint32_t indexSize = 0;
				ASSERT(m_geometry.indexFormat == ImageFormat::R16_UINT || m_geometry.indexFormat == ImageFormat::R32_UINT);
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
                    GL_PROTECT(glDrawElementsInstancedBaseVertexBaseInstance(m_render.polygon.topology, op.indexCount, indexType, 
						(uint8_t*) nullptr + op.firstIndex * indexSize + m_geometry.finalIndexStreamOffset,
						op.numInstances, op.firstVertex, op.firstInstance));
                }
                else
                {
                    GL_PROTECT(glDrawElementsBaseVertex(m_render.polygon.topology, op.indexCount, indexType, 
						(uint8_t*) nullptr + op.firstIndex * indexSize + m_geometry.finalIndexStreamOffset, 
						op.firstVertex));
                }
            }

            void FrameExecutor::runDispatch(const command::OpDispatch& op)
            {
				const auto* shaders = m_objectRegistry.resolveStatic<Shaders>(op.shaderLibrary);
				DEBUG_CHECK_RETURN_EX(shaders, "Shaders unloaded while command buffer was pending processing");

				prepareDispatch(*shaders, op.shaderIndex);
                GL_PROTECT(glDispatchCompute(op.counts[0], op.counts[1], op.counts[2]));
            }

			//--

        } // exe
    } // gl4
} // rendering

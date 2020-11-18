/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\frame\execution #]
***/

#include "build.h"
#include "glExecutor.h"
#include "glObjectCache.h"
#include "glShaders.h"
#include "glUtils.h"
#include "glTempBuffer.h"

namespace rendering
{
    namespace gl4
    {
        namespace exec
        {

            //---

            void FrameExecutor::runBindParameters(const command::OpBindParameters &op)
            {
                DEBUG_CHECK_EX(op.binding, "Empty binding name");
                DEBUG_CHECK_EX(op.view.layout(), "Empty layout");

                if (op.binding && op.view.layout())
                {
                    auto bindingIndex = m_objectCache.resolveParametersBindPointIndex(op.binding, op.view.layout());
                    m_params.paramBindings.prepare(bindingIndex+1);

                    if (m_params.paramBindings[bindingIndex] != op.view)
                    {
                        //TRACE_INFO("Bound params to '{}' at index {}", op.binding, bindingIndex);
                        m_params.paramBindings[bindingIndex] = op.view;
                        m_params.parameterBindingsChanged = true;
                    }
                }
            }

            void FrameExecutor::printParamState()
            {
                TRACE_INFO("Currently known {} parameter bindings points", m_params.paramBindings.size());
                for (uint32_t i = 0; i < m_params.paramBindings.size(); ++i)
                    TRACE_INFO("  [{}]: {} {}", i, m_params.paramBindings[i].layout(), m_params.paramBindings[i].dataPtr());
            }

            bool FrameExecutor::applyParameters(const Shaders& shaders, PipelineIndex parameterBindingStateIndex)
            {
                if (const auto* parameterBinding = shaders.parameterBindingState(parameterBindingStateIndex))
                {
                    for (const auto& elem : parameterBinding->bindingElements)
                    {
                        // do we have parameter ?
                        {
                            const auto hasValidParam = elem.bindPointIndex < m_params.paramBindings.size() && m_params.paramBindings[elem.bindPointIndex];
                            if (!hasValidParam)
                                printParamState();
                            DEBUG_CHECK_EX(hasValidParam, base::TempString("Missing parameters for '{}', bind point {}, expected layout: {}", elem.bindPointName, elem.bindPointIndex, elem.bindPointLayout));
                            if (!hasValidParam)
                                return false;
                        }

                        // validate
                        const auto& params = m_params.paramBindings[elem.bindPointIndex];
                        DEBUG_CHECK_EX(params.layout() == elem.bindPointLayout, base::TempString("Incompatible parameter layout for '{}': '{}' != '{}'", elem.bindPointName, params.layout(), elem.bindPointLayout));
                        if (params.layout() != elem.bindPointLayout)
                            return false;

                        // validate object type
                        const auto& untypedView = *(const ObjectView*)(params.dataPtr() + elem.offsetToView);
                        DEBUG_CHECK_EX(untypedView.type() == elem.objectType, base::TempString("Invalid object view in param table '{}', member '{}' at offset '{}'. Expected {} got {}",
                            elem.bindPointName, elem.paramName, elem.offsetToView, (uint8_t)elem.objectType, (uint8_t)untypedView.type()));
                        if (untypedView.type() != elem.objectType)
                            return false;

                        // bind required resources
                        switch (elem.objectPackingType)
                        {
                            case ResolvedParameterBindingState::PackingType::Constants:
                            {
                                const auto& view = static_cast<const ConstantsView&>(untypedView);
                                DEBUG_CHECK_EX(view, base::TempString("No constants assigned to '{}' in '{}'", elem.paramName, elem.bindPointName));
                                if (view)
                                {
                                    auto assignedRealOffset = *view.offsetPtr() + view.offset();
                                    auto resolvedView = m_tempConstantBuffer->resolveUntypedView(assignedRealOffset, view.size());
                                    m_objectBindings.bindUniformBuffer(elem.objectSlot, resolvedView);
                                }
                                break;
                            }

                            case ResolvedParameterBindingState::PackingType::Buffer:
                            {
                                const auto& view = static_cast<const BufferView&>(untypedView);
                                DEBUG_CHECK_EX(view, base::TempString("No buffer assigned to '{}' in '{}'", elem.paramName, elem.bindPointName));
                                DEBUG_CHECK_EX(view.shadeReadable(), base::TempString("Buffer assigned tp '{}' in '{}' should be shader readable", elem.paramName, elem.bindPointName));
                                if (view && view.shadeReadable())
                                {
                                    auto readWriteMode = elem.objectReadWriteMode;
                                    if (readWriteMode == GL_WRITE_ONLY || readWriteMode == GL_READ_WRITE)
                                    {
                                        DEBUG_CHECK_EX(view.uavCapable(), "Buffer has to be UAV capable to be written by shaders");
                                        if (!view.uavCapable())
                                            readWriteMode = GL_READ_ONLY;
                                    }

                                    auto resolvedView = resolveTypedBufferView(view, elem.objectFormat);
                                    m_objectBindings.bindImage(elem.objectSlot, resolvedView, readWriteMode);
                                }

                                break;
                            }

                            case ResolvedParameterBindingState::PackingType::StorageBuffer:
                            {
                                const auto& view = static_cast<const BufferView&>(untypedView);
                                DEBUG_CHECK_EX(view, base::TempString("No buffer assigned to '{}' in '{}'", elem.paramName, elem.bindPointName));
                                DEBUG_CHECK_EX(view.structured(), base::TempString("Buffer assigned to '{}' in '{}' should be a structured buffer", elem.paramName, elem.bindPointName));
                                DEBUG_CHECK_EX(view.uavCapable(), base::TempString("Buffer assigned to '{}' in '{}' should be UAV capable to be read as structured buffer", elem.paramName, elem.bindPointName));
                                DEBUG_CHECK_EX(view.stride(), "Structured buffer requires view with structure stride");

                                if (view.stride() && view.structured() && view.shadeReadable())
                                {
                                    DEBUG_CHECK_EX(view.uavCapable() || (elem.objectReadWriteMode == GL_READ_ONLY), "Buffer has to be UAV capable to be written by shaders");
                                    auto resolvedView = resolveUntypedBufferView(view);
                                    m_objectBindings.bindStorageBuffer(elem.objectSlot, resolvedView);
                                }
                                break;
                            }

                            case ResolvedParameterBindingState::PackingType::Texture:
                            {
                                const auto& view = static_cast<const ImageView&>(untypedView);
                                DEBUG_CHECK_EX(view, base::TempString("No texture assigned to '{}' in '{}'", elem.paramName, elem.bindPointName));
                                DEBUG_CHECK_EX(view.shaderReadable(), base::TempString("Texture assigned tp '{}' in '{}' should be shader readable", elem.paramName, elem.bindPointName));
                                if (view && view.shaderReadable())
                                {
                                    auto resolvedView = resolveImageView(view);
                                    auto resolvedSamplerView = resolveSampler(view.sampler());
                                    m_objectBindings.bindTexture(elem.objectSlot, resolvedView);
                                    m_objectBindings.bindSampler(elem.objectSlot, resolvedSamplerView); // TODO: static samplers, yeah :D
                                }
                                break;
                            }

                            case ResolvedParameterBindingState::PackingType::Image:
                            {
                                const auto& view = static_cast<const ImageView&>(untypedView);
                                DEBUG_CHECK_EX(view, base::TempString("No image assigned to '{}' in '{}'", elem.paramName, elem.bindPointName));
                                DEBUG_CHECK_EX(view.uavCapable(), base::TempString("Image assigned to '{}' in '{}' should be UAV capable to be accessed as Image", elem.paramName, elem.bindPointName));
                                if (view && view.uavCapable())
                                {
                                    auto resolvedView = resolveImageView(view);

                                    // prepare formated view
                                    ResolvedFormatedView formatedView;
                                    formatedView.glBufferView = resolvedView.glImageView;
                                    formatedView.glViewFormat = TranslateImageFormat(elem.objectFormat);// view.format());

                                    m_objectBindings.bindImage(elem.objectSlot, formatedView, elem.objectReadWriteMode);
                                }
                                break;
                            }
                        }
                    } 
                }

                // no errors
                return true;
            }

            //---

        } // exex
    } // gl4
} // rendering

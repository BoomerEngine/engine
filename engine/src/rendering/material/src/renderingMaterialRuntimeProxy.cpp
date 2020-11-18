/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#include "build.h"
#include "renderingMaterialRuntimeProxy.h"
#include "renderingMaterialRuntimeTechnique.h"
#include "renderingMaterialTemplate.h"
#include "rendering/device/include/renderingConstantsView.h"
#include "rendering/device/include/renderingCommandWriter.h"
#include "rendering/texture/include/renderingTexture.h" 

namespace rendering
{
    ///---

    MaterialDataProxy::MaterialDataProxy(const MaterialTemplate* materialTemplate, bool keepRef, const IMaterial& dataSource)
        : m_layout(materialTemplate->dataLayout())
        , m_sortGroup(materialTemplate->sortGroup())
        , m_materialTemplate(materialTemplate)
        , m_descriptorLayoutID(materialTemplate->dataLayout()->descriptorLayout().layoutId)
    {
        // keep the material template alive
        if (keepRef)
            m_materialTemplateOwnedRef = AddRef(materialTemplate);

        // reserve space for classical constant buffer
        m_constantDataSize = m_layout->descriptorLayout().constantDataSize;
        m_constantData = base::mem::GlobalPool<POOL_MATERIAL_DATA, uint8_t>::Alloc(m_constantDataSize, 16);
        memzero(m_constantData, m_constantDataSize);

        // reserve space for descriptor data
        m_descriptorDataSize = m_layout->descriptorLayout().descriptorSize;
        m_descriptorData = base::mem::GlobalPool<POOL_MATERIAL_DATA, uint8_t>::Alloc(m_descriptorDataSize, 16);
        memzero(m_descriptorData, m_descriptorDataSize);

        // bind default resources
        for (const auto& entry : m_layout->descriptorLayout().resourceEntries)
        {
            switch (entry.type)
            {
                case MaterialDataLayoutParameterType::Texture2D:
                {
                    auto* outputImageView = (ImageView*)(m_descriptorData + entry.descriptorDataOffset);
                    *outputImageView = ImageView::DefaultWhite();

                    break;
                }
            }
        }

        // upload data
        update(dataSource);
    }

    MaterialDataProxy::~MaterialDataProxy()
    {
        base::mem::GlobalPool<POOL_MATERIAL_DATA, uint8_t>::Free(m_constantData);
        m_constantData = nullptr;

        base::mem::GlobalPool<POOL_MATERIAL_DATA, uint8_t>::Free(m_descriptorData);
        m_descriptorData = nullptr;
    }

    void MaterialDataProxy::update(const IMaterial& dataSource)
    {
        auto lock = CreateLock(m_constantDataLock);
        auto oldResources = std::move(m_usedResources);

        // pack constant data
        for (const auto& entry : m_layout->descriptorLayout().constantBufferEntries)
        {
            auto destDataPtr = m_constantData + entry.dataOffset;

            switch (entry.type)
            {
                case MaterialDataLayoutParameterType::Float:
                {
                    dataSource.readParameter(entry.name, destDataPtr, base::reflection::GetTypeObject<float>());
                    break;
                }

                case MaterialDataLayoutParameterType::Vector2:
                {
                    dataSource.readParameter(entry.name, destDataPtr, base::reflection::GetTypeObject<base::Vector2>());
                    break;
                }

                case MaterialDataLayoutParameterType::Vector3:
                {
                    dataSource.readParameter(entry.name, destDataPtr, base::reflection::GetTypeObject<base::Vector3>());
                    break;
                }

                case MaterialDataLayoutParameterType::Vector4:
                {
                    dataSource.readParameter(entry.name, destDataPtr, base::reflection::GetTypeObject<base::Vector4>());
                    break;
                }

                case MaterialDataLayoutParameterType::Color:
                {
                    base::Color color(255, 255, 255, 255);
                    dataSource.readParameter(entry.name, &color, base::reflection::GetTypeObject<base::Color>());
                    *(base::Vector4*)destDataPtr = color.toVectorSRGB();
                    break;
                }

                case MaterialDataLayoutParameterType::Texture2D:
                    break;

                default:
                {
                    DEBUG_CHECK(!"Unknown parameter type for packing in the constant data");
                }
            }
        }

        // pack resources
        for (const auto& entry : m_layout->descriptorLayout().resourceEntries)
        {
            switch (entry.type)
            {
                case MaterialDataLayoutParameterType::Texture2D:
                {
                    TextureRef texture;
                    dataSource.readParameter(entry.name, &texture, base::reflection::GetTypeObject<TextureRef>());

                    ImageView view;
                    if (const auto data = texture.acquire())
                    {
                        if (auto mainView = data->view())
                        {
                            view = mainView;
                        }
                    }

                    if (view && view.viewType() == entry.viewType)
                        m_usedResources.pushBackUnique(texture.acquire());
                    else
                        view = ImageView::DefaultWhite();

                    auto* outputImageView = (ImageView*)(m_descriptorData + entry.descriptorDataOffset);
                    *outputImageView = view;

                    break;
                }
            }
        }
    }

    ParametersView MaterialDataProxy::upload(command::CommandWriter& cmd) const
    {
        // nothing
        if (!m_descriptorData || !m_descriptorDataSize || !m_descriptorLayoutID)
            return ParametersView();

        auto lock = CreateLock(m_constantDataLock);

        // upload constants
        if (m_constantDataSize > 0)
        {
            auto* constantsView = (ConstantsView*)m_descriptorData;
            *constantsView = cmd.opUploadConstants(m_constantData, m_constantDataSize);
        }       

        // create and upload descriptor
        return cmd.opUploadParameters(m_descriptorData, m_descriptorDataSize, m_descriptorLayoutID);
    }

    ///---

} // rendering
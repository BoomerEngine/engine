/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#include "build.h"
#include "renderingMaterialRuntimeLayout.h"
#include "rendering/device/include/renderingConstantsView.h"

namespace rendering
{
    ///---

    RTTI_BEGIN_TYPE_ENUM(MaterialDataLayoutParameterType);
        RTTI_ENUM_OPTION(Float);
        RTTI_ENUM_OPTION(Vector2);
        RTTI_ENUM_OPTION(Vector3);
        RTTI_ENUM_OPTION(Vector4);
        RTTI_ENUM_OPTION(Color);
        RTTI_ENUM_OPTION(Texture2D);
    RTTI_END_TYPE();

    ///---

    MaterialDataLayout::MaterialDataLayout(MaterialDataLayoutID id, uint64_t key, base::Array<MaterialDataLayoutEntry>&& entries)
        : m_entries(std::move(entries))
        , m_id(id)
    {
        // name descriptor based on the layout ID
        m_descriptorName = base::StringID(base::TempString("MaterialData{}", Hex(key)));

        buildDescriptorLayout();
    }

    void MaterialDataLayout::buildDescriptorLayout()
    {
        // reset
        m_descriptor.constantDataSize = 0;
        m_descriptor.descriptorSize = 0;
        m_descriptor.constantBufferEntries.reset();
        m_descriptor.resourceEntries.reset();

        // extract constant buffer entries
        for (const auto& entry : m_entries)
        {
            const auto remainderLeftInVector = 16 - (m_descriptor.constantDataSize % 16);

            uint32_t dataSize = 0;
            uint32_t dataAlign = 4;
            switch (entry.type)
            {
                case MaterialDataLayoutParameterType::Float:
                {
                    dataSize = 4;
                    break;
                }

                case MaterialDataLayoutParameterType::Vector2:
                {
                    if (remainderLeftInVector < 8) dataAlign = 16;
                    dataSize = 8;
                    break;
                }

                case MaterialDataLayoutParameterType::Vector3:
                {
                    if (remainderLeftInVector < 12) dataAlign = 16;
                    dataSize = 12;
                    break;
                }

                case MaterialDataLayoutParameterType::Color:
                case MaterialDataLayoutParameterType::Vector4:
                {
                    dataAlign = 16;
                    dataSize = 16;
                    break;
                }

                case MaterialDataLayoutParameterType::Texture2D:
                {
                    break;
                }
            }

            // store
            if (dataSize != 0)
            {
                auto& descEntry = m_descriptor.constantBufferEntries.emplaceBack();
                descEntry.name = entry.name;
                descEntry.type = entry.type;
                descEntry.dataSize = dataSize;
                descEntry.dataOffset = base::Align(m_descriptor.constantDataSize, dataAlign);
                m_descriptor.constantDataSize = descEntry.dataOffset + descEntry.dataSize;
            }
        }

        // ordered list of view types (for descriptor)
        base::InplaceArray<ObjectViewType, 20> layoutViewTypes;

        // if we have constants leave space for constant buffer
        if (m_descriptor.constantDataSize > 0)
        {
            m_descriptor.descriptorSize += sizeof(ConstantsView);
            layoutViewTypes.pushBack(ObjectViewType::Constants);
        }

        // pack resource entries
        for (const auto& entry : m_entries)
        {
            switch (entry.type)
            {
                case MaterialDataLayoutParameterType::Texture2D:
                {
                    auto& resourceEntry = m_descriptor.resourceEntries.emplaceBack();
                    resourceEntry.descriptorDataOffset = base::Align<uint32_t>(m_descriptor.descriptorSize, 4);
                    resourceEntry.name = entry.name;
                    resourceEntry.type = entry.type;
                    resourceEntry.viewType = ImageViewType::View2D;

                    m_descriptor.descriptorSize = resourceEntry.descriptorDataOffset + sizeof(ImageView);
                    layoutViewTypes.pushBack(ObjectViewType::Image);
                }
            }
        }

        // build descriptor layout
        m_descriptor.layoutId = ParametersLayoutID::FromObjectTypes(layoutViewTypes.typedData(), layoutViewTypes.size());
    }

    void MaterialDataLayout::print(const base::IFormatStream& f) const
    {
        // TODO
    }

    ///---

} // rendering
/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#include "build.h"
#include "renderingMaterialRuntimeLayout.h"

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

    MaterialDataLayout::MaterialDataLayout(MaterialDataLayoutID id, base::Array<MaterialDataLayoutEntry>&& entries)
        : m_entries(std::move(entries))
        , m_id(id)
    {
		BuildDescriptorLayout(m_entries, m_discreteDataLayout);
		BuildBindlessLayout(m_entries, m_bindlessDataLayout);
    }

	void MaterialDataLayout::BuildDescriptorLayout(const base::Array<MaterialDataLayoutEntry>& entries, MaterialDataLayoutDescriptor& outLayout)
	{
		outLayout.constantDataSize = 0;
		outLayout.descriptorSize = 0;
		outLayout.constantBufferEntries.reset();
		outLayout.resourceEntries.reset();

		base::StringBuilder txt;

		base::InplaceArray<DeviceObjectViewType, 20> resourceTypes;
        for (const auto& entry : entries)
        {
            const auto remainderLeftInVector = 16 - (outLayout.constantDataSize % 16);

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
					txt << "T";
					auto& resourceEntry = outLayout.resourceEntries.emplaceBack();
					resourceEntry.name = entry.name;
					resourceEntry.type = entry.type;
					resourceEntry.viewType = ImageViewType::View2D;

					resourceTypes.pushBack(DeviceObjectViewType::SampledImage);
                    break;
                }
            }

            // store
            if (dataSize != 0)
            {
                auto& descEntry = outLayout.constantBufferEntries.emplaceBack();
                descEntry.name = entry.name;
                descEntry.type = entry.type;
                descEntry.dataSize = dataSize;
                descEntry.dataOffset = base::Align(outLayout.constantDataSize, dataAlign);
				outLayout.constantDataSize = descEntry.dataOffset + descEntry.dataSize;
            }
        }

        // if we have constants leave space for constant buffer
		uint32_t descriptorEntryIndex = 0;
		if (outLayout.constantDataSize > 0)
		{
			txt << "C";
			resourceTypes.insert(0, DeviceObjectViewType::ConstantBuffer);
			descriptorEntryIndex = 1;
		}

		// assign descriptor entries
		for (auto& entry : outLayout.resourceEntries)
			entry.descriptorEntryIndex = descriptorEntryIndex++;

        // build descriptor layout
		if (!resourceTypes.empty())
		{
			outLayout.descriptorName = base::StringID(base::TempString("MaterialData{}", txt));
			outLayout.descriptorID = DescriptorID::FromTypes(resourceTypes.typedData(), resourceTypes.size());
			outLayout.descriptorSize = resourceTypes.size();
		}
    }

	void MaterialDataLayout::BuildBindlessLayout(const base::Array<MaterialDataLayoutEntry>& entries, MaterialDataLayoutBindless& outLayout)
	{
		outLayout.constantDataSize = 0;
		outLayout.constantBufferEntries.reset();

		base::StringBuilder txt;

		base::InplaceArray<DeviceObjectViewType, 20> resourceTypes;
		for (const auto& entry : entries)
		{
			const auto remainderLeftInVector = 16 - (outLayout.constantDataSize % 16);

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
					dataAlign = 4;
					dataSize = 4;
					break;
				}
			}

			// store
			if (dataSize != 0)
			{
				auto& descEntry = outLayout.constantBufferEntries.emplaceBack();
				descEntry.name = entry.name;
				descEntry.type = entry.type;
				descEntry.dataSize = dataSize;
				descEntry.dataOffset = base::Align(outLayout.constantDataSize, dataAlign);
				outLayout.constantDataSize = descEntry.dataOffset + descEntry.dataSize;
			}
		}
	}

    ///---

} // rendering
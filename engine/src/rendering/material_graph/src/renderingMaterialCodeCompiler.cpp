/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material #]
***/

#include "build.h"
#include "renderingMaterialCodeCompiler.h"
#include "rendering/mesh/include/renderingMeshFormat.h"
#include "rendering/material/include/renderingMaterialRuntimeLayout.h"
#include "rendering/material/include/renderingMaterialRuntimeTechnique.h"

namespace rendering
{
    //--

    static CodeChunk DefaultValueForStream(CodeChunkType type)
    {
        if (type == CodeChunkType::Numerical1)
            return CodeChunk(0.0f);
        else if (type == CodeChunkType::Numerical2)
            return CodeChunk(base::Vector2(0.0f, 0.0f));
        else if (type == CodeChunkType::Numerical3)
            return CodeChunk(base::Vector3(0.0f, 0.0f, 0.0f));
        else if (type == CodeChunkType::Numerical4)
            return CodeChunk(base::Vector4(0.0f, 0.0f, 0.0f, 0.0f));
        else
            return CodeChunk();
    }

    //--

    MaterialPixelStageCompiler::MaterialPixelStageCompiler(const MaterialDataLayout* dataLayout, base::StringView materialPath, const MaterialCompilationSetup& context)
        : MaterialStageCompiler(dataLayout, ShaderStage::Pixel, materialPath, context)
    {}

    CodeChunk MaterialPixelStageCompiler::vertexData(MaterialVertexDataType typeV)
    {
        int type = (int)typeV;

        // maybe we already did it
        if (const auto* ret = m_requestedVertexMap.find(type))
            return *ret;

        // if it's a vertex stream field than emit it only if the stream has that data
        const auto& info = GetMaterialVertexDataInfo(typeV);
        if (info.vertexStream)
        {
            bool presentInVertexStreams = false;

            const auto vertexFormat = (MeshVertexFormat)context().vertexFormat;
            const auto& vertexFormatInfo = GetMeshVertexFormatInfo(vertexFormat);
            for (uint32_t i = 0; i < vertexFormatInfo.numStreams; ++i)
            {
                if (vertexFormatInfo.streams[i].name == info.name)
                {
                    presentInVertexStreams = true;
                    break;
                }
            }

            if (presentInVertexStreams)
            {
                if (info.renotmalize)
                {
                    const auto renormalizedName = base::StringBuf(base::TempString("{}_NORM", info.name));
                    appendf("{} {} = normalize({});\n", info.shaderType, renormalizedName, info.name);
                    m_requestedVertexMap[type] = CodeChunk(info.type, renormalizedName, true);
                }
                else
                {
                    m_requestedVertexMap[type] = CodeChunk(info.type, info.name, true);
                }
            }
            else
            {
                TRACE_WARNING("VertexStream '{}' is not present when compiling shader '{}'", info.name, path());
                m_requestedVertexMap[type] = DefaultValueForStream(info.type);
                return m_requestedVertexMap[type];
            }
        }
        else
        {
            // assume the VS will produce the data
            if (info.renotmalize)
            {
                const auto renormalizedName = base::StringBuf(base::TempString("{}_NORM", info.name));
                appendf("{} {} = normalize({});\n", info.shaderType, renormalizedName, info.name);
                m_requestedVertexMap[type] = CodeChunk(info.type, renormalizedName, true);
            }
            else
            {
                m_requestedVertexMap[type] = CodeChunk(info.type, info.name, true);
            }
        }

        m_requestedVertexData.pushBackUnique(typeV);
        return m_requestedVertexMap[type];
    }

    void MaterialPixelStageCompiler::assembleFinalShaderCode(base::StringBuilder& outStr, const MaterialTechniqueRenderStates& renderStates) const
    {
        outStr.append("state MaterialStates {\n");

        outStr.appendf("   DepthEnabled = {},\n", renderStates.depthTest);
        outStr.appendf("   DepthWriteEnabled = {},\n", renderStates.depthWrite);

        if (renderStates.depthTest)
        {
            if (renderStates.earlyPixelTests && !renderStates.alphaBlend)
                outStr.append("   DepthFunc = Equal,\n");
            else
                outStr.append("   DepthFunc = LessEqual,\n");
        }

        if (renderStates.twoSided)
        {
            outStr.append("   CullEnabled = false,\n");
        }
        else
        {
            outStr.append("   CullEnabled = true,\n");
            outStr.append("   CullMode = Back,\n");
        }

        if (renderStates.alphaBlend)
        {
            outStr.append("   BlendingEnabled = true,\n");
            outStr.append("   BlendColorSrc0 = One,\n");
            outStr.append("   BlendAlphaSrc0 = One,\n");
            outStr.append("   BlendColorDest0 = OneMinusSrcAlpha,\n");
            outStr.append("   BlendAlphaDest0 = OneMinusSrcAlpha,\n");            
        }
        else
        {
            outStr.append("   BlendingEnabled = false,\n");
        }        

        outStr.append("}\n\n");
        
        outStr.append("attribute(state=MaterialStates)\n");        
        outStr.append("shader MaterialPS {\n");

        // print VS->PS imports
        {
            auto vertexStreams = m_requestedVertexData;
            std::sort(vertexStreams.begin(), vertexStreams.end(), [](MaterialVertexDataType a, MaterialVertexDataType b) { return (int)a < (int)b; });

            for (const auto data : vertexStreams)
            {
                const auto& info = GetMaterialVertexDataInfo(data);
                if (info.shaderType == "uint")
                    outStr.appendf("  attribute(flat) in {} {};\n", info.shaderType, info.name);
                else
                    outStr.appendf("  in {} {};\n", info.shaderType, info.name);
            }
        }

        // print code
        {
            if (renderStates.earlyPixelTests)
                outStr.append("  attribute(early_fragment_tests)\n");

            outStr.append("  void main() {\n");
            if (empty())
                outStr.append("    gl_Target0 = vec4(1,0,1,1);\n");
            else
                outStr.append(view());
            outStr.append("  }\n");
        }

        outStr.append("}\n");
    }

    //--

    MaterialVertexStageCompiler::MaterialVertexStageCompiler(const MaterialDataLayout* dataLayout, base::StringView materialPath, const MaterialCompilationSetup& context, const MaterialPixelStageCompiler& ps)
        : MaterialStageCompiler(dataLayout, ShaderStage::Vertex, materialPath, context)
        , m_ps(ps)
    {
    }

    CodeChunk MaterialVertexStageCompiler::vertexData(MaterialVertexDataType typeV)
    {
        int type = (int)typeV;

        // TODO: some vertex data may not be accessible in vertex shader ?

        // maybe we already did it
        if (const auto* ret = m_requestedVertexMap.find(type))
            return *ret;

        const auto& info = GetMaterialVertexDataInfo(typeV);
        m_requestedVertexMap[type] = CodeChunk(info.type, info.name, true);
        return m_requestedVertexMap[type];
    }

    void MaterialVertexStageCompiler::assembleFinalShaderCode(base::StringBuilder& outStr) const
    {
        const auto vertexFormat = context().vertexFormat;
        const auto& vertexFormatInfo = GetMeshVertexFormatInfo(vertexFormat);
        if (!context().meshletsVertices)
        {
            outStr.appendf("attribute(packing=vertex) struct {} {\n", MeshVertexFormatBindPointName(vertexFormat));


            for (uint32_t i=0; i<vertexFormatInfo.numStreams; ++i)
            {
                const auto& vertexStream = vertexFormatInfo.streams[i];

                outStr.appendf("    attribute(offset={}) ", vertexStream.dataOffset);

                const auto numOutComponents = GetImageFormatInfo(vertexStream.readFormat).numComponents;
                const auto formatClass = GetImageFormatInfo(vertexStream.readFormat).formatClass;

                if (formatClass == ImageFormatClass::UINT)
                {
                    if (numOutComponents == 1)
                        outStr.append("uint ");
                    else
                        outStr.appendf("uvec{} ", numOutComponents);
                }
                else if (formatClass == ImageFormatClass::INT)
                {
                    if (numOutComponents == 1)
                        outStr.append("int ");
                    else
                        outStr.appendf("ivec{} ", numOutComponents);
                }
                else if (formatClass == ImageFormatClass::FLOAT)
                {
                    if (numOutComponents == 1)
                        outStr.append("float ");
                    else
                        outStr.appendf("vec{} ", numOutComponents);
                }

                if (vertexStream.readFunction.empty())
                    outStr.appendf("Raw{};\n", vertexStream.name);
                else
                    outStr.appendf("Raw{};\n", vertexStream.name);
            }

            outStr.append("}\n\n");
        }

        outStr.append("shader MaterialVS {\n");

        // print the outputs to pixel shader
        {
            auto dataStreams = m_ps.vertexData();
            std::sort(dataStreams.begin(), dataStreams.end(), [](MaterialVertexDataType a, MaterialVertexDataType b) { return (int)a < (int)b; });

            if (!dataStreams.empty())
            {
                for (const auto data : dataStreams)
                {
                    const auto& info = GetMaterialVertexDataInfo(data);
                    outStr.appendf("  out {} {};\n", info.shaderType, info.name);
                }

                outStr.append("\n");
            }
        }

        // vertex input
        if (!context().meshletsVertices)
        {
            outStr.appendf("  vertex {} v;\n", MeshVertexFormatBindPointName(vertexFormat));
            outStr.append("\n");
        }

        outStr.append("  void main() {\n");

        // merge requirements to generate
        auto requestedData = m_ps.vertexData();
        for (const auto& data : m_requestedVertexData)
            requestedData.pushBackUnique(data);

        // some world-space stuff will require computing other data
        if (requestedData.contains(MaterialVertexDataType::WorldNormal))
            requestedData.pushBackUnique(MaterialVertexDataType::VertexNormal);
        if (requestedData.contains(MaterialVertexDataType::WorldTangent))
            requestedData.pushBackUnique(MaterialVertexDataType::VertexTangent);
        if (requestedData.contains(MaterialVertexDataType::WorldBitangent))
            requestedData.pushBackUnique(MaterialVertexDataType::VertexBitangent);

        // internally we will also need world position to compute the gl_Position
        requestedData.pushBackUnique(MaterialVertexDataType::WorldPosition);
        requestedData.pushBackUnique(MaterialVertexDataType::VertexPosition);
        requestedData.pushBackUnique(MaterialVertexDataType::ObjectIndex);

        // print variables for all streams that are not outputs
        for (const auto data : requestedData)
        {
            if (!m_ps.vertexData().contains(data))
            {
                const auto& info = GetMaterialVertexDataInfo(data);
                outStr.appendf("    {} {} = ", info.shaderType, info.name);

                if (info.type == CodeChunkType::Numerical1)
                    outStr.append("0;\n");
                else if (info.type == CodeChunkType::Numerical2)
                    outStr.append("vec2(0,0);\n");
                else if (info.type == CodeChunkType::Numerical3)
                    outStr.append("vec3(0,0,0);\n");
                else if (info.type == CodeChunkType::Numerical4)
                    outStr.append("vec4(0,0,0,0);\n");
            }
        }

        // generate either bindless vertex access or a classical vertex attributes
        if (context().meshletsVertices)
        {
            // access draw object
            outStr.append("    ObjectIndex = FetchObjectIndex();\n");

            // unpack vertex attributes
            if (vertexFormatInfo.numStreams)
            {
                outStr.append("    uint vertexOffsetInWords = CalcVertexOffsetInWords();\n");

                for (uint32_t i = 0; i < vertexFormatInfo.numStreams; ++i)
                {
                    const auto& vertexStream = vertexFormatInfo.streams[i];
                    auto dataWordIndex = vertexStream.dataOffset / 4;

                    // if nothing needs the stream don't read it
                    bool isStreamRequested = false;
                    for (const auto data : requestedData)
                    {
                        if (GetMaterialVertexDataInfo(data).name == vertexStream.name)
                        {
                            isStreamRequested = true;
                            break;
                        }
                    }

                    if (vertexStream.readFunction.empty())
                    {
                        const auto numOutComponents = GetImageFormatInfo(vertexStream.readFormat).numComponents;
                        for (uint32_t j = 0; j < numOutComponents; ++j)
                        {
                            if (isStreamRequested)
                            {
                                const base::StringView compNames[] = { "x","y","z","w" };
                                outStr.appendf("    {}.{} = uintBitsToFloat(MeshVertexData[vertexOffsetInWords + {}]);\n", vertexStream.name, compNames[j], dataWordIndex);
                            }
                            dataWordIndex += 1;
                        }
                    }
                    else if (vertexStream.readFormat == ImageFormat::RG32_UINT)
                    {
                        if (isStreamRequested)
                            outStr.appendf("    {} = {}(MeshVertexData[vertexOffsetInWords + {}], MeshVertexData[vertexOffsetInWords + {}]);\n", vertexStream.name, vertexStream.readFunction, dataWordIndex, dataWordIndex + 1);
                        dataWordIndex += 2;
                    }
                    else if (vertexStream.readFormat == ImageFormat::R32_UINT || vertexStream.readFormat == ImageFormat::RG16F)
                    {
                        if (isStreamRequested)
                            outStr.appendf("    {} = {}(MeshVertexData[vertexOffsetInWords + {}]);\n", vertexStream.name, vertexStream.readFunction, dataWordIndex);
                        dataWordIndex += 1;
                    }
                    else
                    {
                        TRACE_ERROR("Unknown unpacking format for '{}'", vertexStream.name);
                    }
                }              
            }
        }
        else
        {
            // access draw object
            outStr.append("    ObjectIndex = gl_InstanceID;\n");

            for (uint32_t i = 0; i < vertexFormatInfo.numStreams; ++i)
            {
                const auto& vertexStream = vertexFormatInfo.streams[i];

                // if nothing needs the stream don't read it
                bool isStreamRequested = false;
                for (const auto data : requestedData)
                {
                    if (GetMaterialVertexDataInfo(data).name == vertexStream.name)
                    {
                        isStreamRequested = true;
                        break;
                    }
                }

                if (isStreamRequested)
                {
                    if (vertexStream.readFunction.empty())
                        outStr.appendf("    {} = v.Raw{};\n", vertexStream.name, vertexStream.name);
                    else if (vertexStream.readFormat == ImageFormat::RG32_UINT)
                        outStr.appendf("    {} = {}(v.Raw{}.x, v.Raw{}.y);\n", vertexStream.name, vertexStream.readFunction, vertexStream.name, vertexStream.name);
                    else
                        outStr.appendf("    {} = {}(v.Raw{});\n", vertexStream.name, vertexStream.readFunction, vertexStream.name);
                }
            }
        }

        // decompress quantized data
        if (vertexFormatInfo.quantizedPosition)
            outStr.append("    VertexPosition = DecompressQuantizedPosition(VertexPosition);\n");

        // calculate local to scene matrix
        {
            outStr.append("    mat4 LocalToScene = ObjectData[ObjectIndex].LocalToScene;\n");
        }

        // calculate world space stuff
        if (requestedData.contains(MaterialVertexDataType::WorldPosition))
            outStr.append("    WorldPosition = (LocalToScene * VertexPosition.xyz1).xyz;\n");

        // export tangent space
        if (requestedData.contains(MaterialVertexDataType::WorldNormal))
            outStr.append("    WorldNormal = (LocalToScene * VertexNormal.xyz0).xyz;\n");
        if (requestedData.contains(MaterialVertexDataType::WorldTangent))
            outStr.append("    WorldTangent = (LocalToScene * VertexTangent.xyz0).xyz;\n");
        if (requestedData.contains(MaterialVertexDataType::WorldBitangent))
            outStr.append("    WorldBitangent = (LocalToScene * VertexBitangent.xyz0).xyz;\n");
        
        // append custom code, this will mostly modify
        {
            outStr.append("    {\n");
            outStr.append("      vec3 WorldVertexOffset = vec3(0,0,0);\n");
            outStr.append(view());
            outStr.append("      WorldPosition += WorldVertexOffset;\n");
            outStr.append("    }\n");
        }

        // calculate screen position
        {
            outStr.append("    gl_Position = WorldToScreen * WorldPosition.xyz1;\n");
        }

        outStr.append("  }\n");
        outStr.append("}\n");
    }

    //--

    MaterialMeshGeometryCompiler::MaterialMeshGeometryCompiler(const MaterialDataLayout* layout, base::StringView materialPath, const MaterialCompilationSetup& context)
        : m_ps(layout, materialPath, context)
        , m_vs(layout, materialPath, context, m_ps)
        , m_dataLayout(layout)
        , m_debugPath(materialPath)
		, m_setup(context)
    {
    }

    void MaterialMeshGeometryCompiler::printMaterialDescriptor(base::StringBuilder& builder) const
    {
		if (!m_dataLayout || m_setup.bindlessTextures)
			return;

		const auto& layout = m_dataLayout->discreteDataLayout();
		if (layout.empty())
			return;

		builder.appendf("descriptor {} {\n", layout.descriptorName);

        // constants
        if (layout.constantDataSize > 0)
        {
            builder.appendf("  ConstantBuffer {\n");

            for (const auto& entry : layout.constantBufferEntries)
            {
                builder.appendf("    attribute(offset={}) ", entry.dataOffset);

                switch (entry.type)
                {
                    case MaterialDataLayoutParameterType::Float: builder.append("float"); break;
                    case MaterialDataLayoutParameterType::Vector2: builder.append("vec2"); break;
                    case MaterialDataLayoutParameterType::Vector3: builder.append("vec3"); break;
                    case MaterialDataLayoutParameterType::Vector4: builder.append("vec4"); break;
                    case MaterialDataLayoutParameterType::Color: builder.append("vec4"); break;
                }

                builder.appendf(" {};\n", entry.name);
            }

            builder.appendf("  }\n");
        }

        // textures
        for (const auto& resource : layout.resourceEntries)
        {
            builder.append("  attribute(sampler=DefaultMaterialTextureSampler) ");

            switch (resource.viewType)
            {
                case ImageViewType::View1D: builder.append("Texture1D "); break;
                case ImageViewType::View1DArray: builder.append("Texture1DArray "); break;
                case ImageViewType::View2D: builder.append("Texture2D "); break;
                case ImageViewType::View2DArray: builder.append("Texture2DArray "); break;
                case ImageViewType::ViewCube: builder.append("TextureCube "); break;
                case ImageViewType::ViewCubeArray: builder.append("TextureCubeArray "); break;
                case ImageViewType::View3D: builder.append("Texture3D "); break;
            }

            builder.appendf("{};\n", resource.name);
        }

        builder.appendf("}\n");
    }

    void MaterialMeshGeometryCompiler::assembleFinalShaderCode(base::StringBuilder& outStr, const MaterialTechniqueRenderStates& renderStates) const
    {
        outStr.appendf("// BoomerEngine MaterialCompiler\n");
        outStr.appendf("// Compiled form '{}'\n", m_ps.path());
        outStr.appendf("\n");

        {
            base::Array<base::StringBuf> includes;
            includes.pushBack("material/material.h");

            if (m_ps.context().meshletsVertices)
                includes.pushBack("material/vertex_bindless.h");
            else
                includes.pushBack("material/vertex_standard.h");

            for (const auto& path : m_ps.includes())
                includes.pushBackUnique(path);

            for (const auto& path : m_vs.includes())
                includes.pushBackUnique(path);

            for (const auto& path : includes)
                outStr.appendf("#include <{}>\n", path);
        }
        
        /*for (const auto& path : m_includes)
            outStr.appendf("#include <{}>\n", path);*/

        outStr.appendf("\n");

        // material descriptor
        printMaterialDescriptor(outStr);

        // pixel shader
        {
            outStr.append("// pixel shader\n");
            m_ps.assembleFinalShaderCode(outStr, renderStates);
            outStr.append("\n");
        }

        // vertex shader
        {
            outStr.append("// vertex shader\n");
            m_vs.assembleFinalShaderCode(outStr);
            outStr.append("\n");
        }

        // DEBUG DUMP
        if (0)
        {
            base::StringBuilder path;
            path.append("Z:/.materialDump/");
            path.append(m_ps.path().afterLast("/").beforeFirst("."));
            path << m_ps.context();
            path.append(".txt");

            base::io::SaveFileFromString(path.view(), outStr.view());
        }
    }

    //--

} // rendering
 
/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material #]
***/

#pragma once

#include "renderingMaterialCode.h"

BEGIN_BOOMER_NAMESPACE()

//---
    
// material pixel shader compiler
class ENGINE_MATERIAL_GRAPH_API MaterialPixelStageCompiler : public MaterialStageCompiler
{
public:
    MaterialPixelStageCompiler(const MaterialDataLayout* dataLayout, StringView materialPath, const MaterialCompilationSetup& context);

    //--

    // get list of vertex data requested by the shader
    INLINE const Array<MaterialVertexDataType>& vertexData() const { return m_requestedVertexData; }

    // request vertex shader to produce an input and to pass that input to pixel shader
    // usually it will be a vertex stream or some global stuff like "WorldPosition"
    virtual CodeChunk vertexData(MaterialVertexDataType type) override final;

    //--

    // generate the final code (shader part only)
    void assembleFinalShaderCode(StringBuilder& builder, const MaterialTechniqueRenderStates& renderStates) const;

private:
    Array<MaterialVertexDataType> m_requestedVertexData;
    HashMap<int, CodeChunk> m_requestedVertexMap;
};

//--

// material vertex shader compiler
class ENGINE_MATERIAL_GRAPH_API MaterialVertexStageCompiler : public MaterialStageCompiler
{
public:
    MaterialVertexStageCompiler(const MaterialDataLayout* dataLayout, StringView materialPath, const MaterialCompilationSetup& context, const MaterialPixelStageCompiler& ps);

    //--

    virtual CodeChunk vertexData(MaterialVertexDataType type) override final;

    //--

    // generate the final code (shader part only)
    void assembleFinalShaderCode(StringBuilder& builder) const;

private:
    const MaterialPixelStageCompiler& m_ps;
    HashMap<int, CodeChunk> m_requestedVertexMap;
    Array<MaterialVertexDataType> m_requestedVertexData;
};

//--

// helper compiler when compiling a mesh geometry based shader
class ENGINE_MATERIAL_GRAPH_API MaterialMeshGeometryCompiler : public NoCopy
{
public:
    MaterialMeshGeometryCompiler(const MaterialDataLayout* layout, StringView materialPath, const MaterialCompilationSetup& context);

    MaterialPixelStageCompiler m_ps;
    MaterialVertexStageCompiler m_vs;

    const MaterialDataLayout* m_dataLayout = nullptr;

    void assembleFinalShaderCode(StringBuilder& builder, const MaterialTechniqueRenderStates& renderStates) const;

private:
    StringBuf m_debugPath;
	MaterialCompilationSetup m_setup;

    void printMaterialDescriptor(StringBuilder& builder) const;
};

//--
    
END_BOOMER_NAMESPACE()

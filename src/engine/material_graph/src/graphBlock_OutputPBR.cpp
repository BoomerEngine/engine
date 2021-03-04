/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material\output #]
***/

#include "build.h"
#include "graph.h"
#include "graphBlock_OutputPBR.h"
#include "code.h"

#include "engine/material/include/runtimeTechnique.h"


BEGIN_BOOMER_NAMESPACE()

///---

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlockOutput_PBR);
    RTTI_METADATA(graph::BlockInfoMetadata).title("PBRMaterial").group("Material outputs");
    RTTI_CATEGORY("Lighting");
    RTTI_PROPERTY(m_applyGlobalDirectionalLighting).editable("This material will be affected by directional lighting (sun)");
    RTTI_PROPERTY(m_applyGlobalAmbient).editable("This material will be affected by the simple global ambient lighting (horizon/zenith)");
    RTTI_PROPERTY(m_applyReflectionProbes).editable("This material will be affected by image based lighting (reflection probes)");
    RTTI_PROPERTY(m_applyLocalLighting).editable("This material will be affected by local lights");
    RTTI_PROPERTY(m_applyAmbientOcclusion).editable("This material will be affected by ambient occlusion");
    RTTI_PROPERTY(m_receiveGlobalShadows).editable("Receive global shadows from cascades");
    RTTI_PROPERTY(m_receiveTerrainShadows).editable("Receive terrain shadows");
    RTTI_PROPERTY(m_receiveLocalShadows).editable("Receive local light shadows");
RTTI_END_TYPE();

MaterialGraphBlockOutput_PBR::MaterialGraphBlockOutput_PBR()
{}

void MaterialGraphBlockOutput_PBR::buildLayout(graph::BlockLayoutBuilder& builder) const
{
    builder.socket("Base Color"_id, MaterialInputSocket());
    builder.socket("Metallic"_id, MaterialInputSocket());
    builder.socket("Specular"_id, MaterialInputSocket());
    builder.socket("Roughness"_id, MaterialInputSocket());
    builder.socket("Emissive"_id, MaterialInputSocket());
    builder.socket("Opacity"_id, MaterialInputSocket());
    builder.socket("Mask"_id, MaterialInputSocket());
    builder.socket("Normal"_id, MaterialInputSocket());
    builder.socket("World Position Offset"_id, MaterialInputSocket());
    //builder.addSocket("RefractionDelta"_id, MaterialInputSocket());
    //builder.addSocket("ReflectionOffset"_id, MaterialInputSocket());
    //builder.addSocket("ReflectionAmount"_id, MaterialInputSocket());
    //builder.addSocket("GlobalReflection"_id, MaterialInputSocket());
    builder.socket("Lighting Opacity"_id, MaterialInputSocket());
    builder.socket("Ambient Occlusion"_id, MaterialInputSocket());
}

CodeChunk MaterialGraphBlockOutput_PBR::compileMainColor(MaterialStageCompiler& compiler, MaterialTechniqueRenderStates& outRenderState) const
{
    auto color = compiler.evalInput(this, "Base Color"_id, Vector4(0.5f, 0.5f, 0.5f, 1.0f)).conform(3);
    const auto metallic = compiler.evalInput(this, "Metallic"_id, 0.0f).conform(1);
    const auto specular = compiler.evalInput(this, "Specular"_id, Vector3(0.5f, 0.5f, 0.5f)).conform(3);
    const auto rougness = compiler.evalInput(this, "Roughness"_id, 0.18f).conform(1);
    auto ambientOcclusion = compiler.evalInput(this, "Ambient Occlusion"_id, 1.0f).conform(1);

    const auto worldPosition = compiler.vertexData(MaterialVertexDataType::WorldPosition);
    const auto worldNormal = compiler.vertexData(MaterialVertexDataType::WorldNormal);

    auto shadeNormal = worldNormal;
    if (hasConnectionOnSocket("Normal"_id))
        shadeNormal = compiler.evalInput(this, "Normal"_id, Vector3(0, 0, 1)).conform(3);
    else
        shadeNormal = compiler.vertexData(MaterialVertexDataType::WorldNormal);

    if (compiler.debugCode())
    {
        color = compiler.var(color);
        compiler.appendf("if (Frame.CheckMaterialDebug(MATERIAL_FLAG_DISABLE_COLOR)) {} = vec3(1,1,1);\n", color);
        compiler.appendf("if (Frame.CheckMaterialDebug(MATERIAL_FLAG_DISABLE_NORMAL)) {} = {};\n", shadeNormal, worldNormal);
    }

    {
        compiler.appendf("PBRPixel pbr;\n");
        compiler.appendf("PackPBR(pbr, {}, {}, {}, {}, {}, {}, {});\n", worldPosition, worldNormal, shadeNormal, color, metallic, specular, rougness);
    }

    auto result = compiler.var(Vector3(0, 0, 0));
    auto resultBack = compiler.var(Vector3(0, 0, 0));

    if (m_applyAmbientOcclusion)
    {
        ambientOcclusion = compiler.var(ambientOcclusion);
        compiler.appendf("{} *= Lighting.SampleGlobalAmbientOcclusion(gl_FragCoord.xy);\n", ambientOcclusion);
    }

    //--

    const auto twoSidedLighting = evalTwoSidedLightingFlag(compiler);
    if (m_applyGlobalDirectionalLighting)
    {
        auto shadowOcclusion = compiler.var(1.0f);

        if (m_receiveGlobalShadows)
            compiler.appendf("{} *= Lighting.SampleGlobalShadowMask(gl_FragCoord.xy);\n", shadowOcclusion);

        compiler.appendf("{} += Lighting.ComputeGlobalLighting(pbr, {});\n", result, shadowOcclusion);

        if (twoSidedLighting)
        {
            compiler.appendf("FlipPBRToSecondSide(pbr);\n");
            compiler.appendf("{} += Lighting.ComputeGlobalLighting(pbr, {});\n", resultBack, shadowOcclusion);
        }
    }

    if (twoSidedLighting)
    {
        auto lightingBlend = compiler.evalInput(this, "Lighting Opacity"_id, Vector3(1.0f, 1.0f, 1.0f)).conform(3);
        auto frontSideLighting = compiler.var(CodeChunk(CodeChunkType::Numerical3, TempString("gl_FrontFacing ? vec3(1) : {}", lightingBlend), false));
        auto backSideLighting = compiler.var(CodeChunk(CodeChunkType::Numerical3, TempString("gl_FrontFacing ? {} : vec3(1)", lightingBlend), false));
        compiler.appendf("{} = ({}*{}) + ({}*{});\n", result, result, frontSideLighting, resultBack, backSideLighting);
    }

    if (m_applyGlobalAmbient)
        compiler.appendf("{} += {} * Lighting.ComputeGlobalAmbient({}, {}, {});\n", result, color, worldPosition, worldNormal, ambientOcclusion);

    //--

    if (compiler.debugCode())
        compiler.appendf("if (Frame.CheckMaterialDebug(MATERIAL_FLAG_DISABLE_LIGHTING)) {} = {};\n", result, color);

    if (hasConnectionOnSocket("Emissive"_id))
    {
        const auto emissive = compiler.evalInput(this, "Emissive"_id, CodeChunk(Vector3(0, 0, 0))).conform(3);
        compiler.appendf("{} += {};\n", result, emissive);
    }

    return result;
}

///---

END_BOOMER_NAMESPACE()

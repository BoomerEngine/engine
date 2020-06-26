/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material\output #]
***/

#include "build.h"
#include "renderingMaterialGraph.h"
#include "renderingMaterialGraphBlock_OutputPBR.h"
#include "renderingMaterialCode.h"

#include "rendering/material/include/renderingMaterialRuntimeTechnique.h"


namespace rendering
{
    ///---

    RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlockOutput_PBR);
        RTTI_METADATA(base::graph::BlockInfoMetadata).title("PBRMaterial").group("Material outputs");
        RTTI_CATEGORY("Lighting");
        RTTI_PROPERTY(m_twoSidedLighting).editable("Material is lit from two sides");
        RTTI_PROPERTY(m_applyGlobalDirectionalLighting).editable("This material will be affected by directional lighting (sun)");
        RTTI_PROPERTY(m_applyGlobalAmbient).editable("This material will be affected by the simple global ambient lighting (horizon/zenith)");
        RTTI_PROPERTY(m_applyReflectionProbes).editable("This material will be affected by image based lighting (reflection probes)");
        RTTI_PROPERTY(m_applyLocalLighting).editable("This material will be affected by local lights");
        RTTI_PROPERTY(m_receiveGlobalShadows).editable("Receive global shadows from cascades");
        RTTI_PROPERTY(m_receiveTerrainShadows).editable("Receive terrain shadows");
        RTTI_PROPERTY(m_receiveLocalShadows).editable("Receive local light shadows");
    RTTI_END_TYPE();

    MaterialGraphBlockOutput_PBR::MaterialGraphBlockOutput_PBR()
    {}

    void MaterialGraphBlockOutput_PBR::buildLayout(base::graph::BlockLayoutBuilder& builder) const
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
        builder.socket("Ambient Occlusion"_id, MaterialInputSocket());
    }

    CodeChunk MaterialGraphBlockOutput_PBR::compileMainColor(MaterialStageCompiler& compiler, MaterialTechniqueRenderStates& outRenderState) const
    {
        compiler.includeHeader("material/pbr.h");
        compiler.includeHeader("material/lighting.h");

        const auto color = compiler.evalInput(this, "Base Color"_id, base::Vector4(0.5f, 0.5f, 0.5f, 1.0f)).conform(3);
        const auto metallic = compiler.evalInput(this, "Metallic"_id, 0.0f).conform(1);
        const auto specular = compiler.evalInput(this, "Specular"_id, 0.5f).conform(1);
        const auto rougness = compiler.evalInput(this, "Roughness"_id, 0.18f).conform(1);
        const auto ambientOcclusion = compiler.evalInput(this, "Ambient Occlusion"_id, 1.0f).conform(1);

        const auto worldPosition = compiler.vertexData(MaterialVertexDataType::WorldPosition);
        const auto worldNormal = compiler.vertexData(MaterialVertexDataType::WorldNormal);

        auto shadeNormal = worldNormal;
        if (hasConnectionOnSocket("Normal"_id))
            shadeNormal = compiler.evalInput(this, "Normal"_id, base::Vector3(0, 0, 1)).conform(3);
        else
            shadeNormal = compiler.vertexData(MaterialVertexDataType::WorldNormal);

        if (compiler.debugCode())
        {
            compiler.appendf("if (Frame.CheckMaterialDebug(MATERIAL_FLAG_DISABLE_COLOR)) {} = vec3(1,1,1);\n", color);
            compiler.appendf("if (Frame.CheckMaterialDebug(MATERIAL_FLAG_DISABLE_NORMAL)) {} = {};\n", shadeNormal, worldNormal);
        }

        {
            compiler.appendf("PBRPixel pbr;\n");
            compiler.appendf("pbr.shading_position = {}.xyz;\n", worldPosition);
            compiler.appendf("pbr.shading_normal = {}.xyz;\n", shadeNormal);
            compiler.appendf("pbr.shading_view = normalize(CameraPosition - {});\n", worldPosition);
            compiler.appendf("pbr.shading_NoV = max(MIN_N_DOT_V, dot(pbr.shading_normal, pbr.shading_view));\n");
            compiler.appendf("pbr.shading_reflected = reflect(-pbr.shading_view, pbr.shading_normal);\n");
            compiler.appendf("pbr.diffuseColor = {} * (1.0 - {});\n", color, metallic);
            compiler.appendf("float reflectance = ComputeDielectricF0({});\n", specular);
            compiler.appendf("pbr.f0 = ComputeF0({}, {}, reflectance);\n", color, metallic);
            compiler.appendf("pbr.perceptualRoughness = {};\n", rougness);
            compiler.appendf("pbr.roughness = pbr.perceptualRoughness * pbr.perceptualRoughness;\n");
            compiler.appendf("pbr.energyCompensation = vec3(1, 1, 1);\n");
        }

        auto result = compiler.var(base::Vector3(0, 0, 0));

        if (m_applyGlobalDirectionalLighting)
        {
            compiler.appendf("{\n");
            compiler.appendf("Light light;\n");
            compiler.appendf("light.colorIntensity = GlobalLighting.LightColor.xyz1;\n");
            compiler.appendf("light.l = GlobalLighting.LightDirection;\n");
            compiler.appendf("light.attenuation = 1.0f;\n");
            compiler.appendf("light.NoL = saturate(dot(GlobalLighting.LightDirection, {}));\n", shadeNormal);
            compiler.appendf("{} += max(vec3(0, 0, 0), PBR.surfaceShading(pbr, light, {}));\n", result, ambientOcclusion);
            compiler.appendf("}\n");
        }

        if (m_applyGlobalDirectionalLighting)
        {
            compiler.appendf("{} += {} * (lerp(GlobalLighting.AmbientColorHorizon, GlobalLighting.AmbientColorZenith, saturate({}.z)) * {});\n", result, color, worldNormal, ambientOcclusion);
        }

        if (compiler.debugCode())
            compiler.appendf("if (Frame.CheckMaterialDebug(MATERIAL_FLAG_DISABLE_LIGHTING)) {} = {};\n", result, color);

        if (hasConnectionOnSocket("Emissive"_id))
        {
            const auto emissive = compiler.evalInput(this, "Emissive"_id, CodeChunk(base::Vector3(0, 0, 0))).conform(3);
            compiler.appendf("{} += {};\n", result, emissive);
        }

        return result;
    }

    ///---

} // rendering

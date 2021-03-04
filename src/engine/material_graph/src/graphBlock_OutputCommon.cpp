/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material\output #]
***/

#include "build.h"
#include "graph.h"
#include "graphBlock_OutputCommon.h"
#include "code.h"

#include "engine/material/include/runtimeTechnique.h"
#include "engine/material/include/materialTemplate.h"

BEGIN_BOOMER_NAMESPACE()

///---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(MaterialGraphBlockOutputCommon);
    RTTI_CATEGORY("Render state");
    RTTI_PROPERTY(m_twoSided).editable("Material is two sided");
    RTTI_PROPERTY(m_twoSidedLighting).editable("Material is lit from two sides");
    RTTI_PROPERTY(m_depthWrite).editable("Material write to depth channel");
    RTTI_PROPERTY(m_maskThreshold).editable("Default masking threshold");
    RTTI_PROPERTY(m_transparent).editable("Enable blending");
    RTTI_PROPERTY(m_premultiplyAlpha).editable("In alpha blending mode premultiply output color by alpha");
    RTTI_CATEGORY("Masking");
    RTTI_PROPERTY(m_maskThreshold).editable("Default masking threshold");
    RTTI_PROPERTY(m_maskAlphaToCoverage).editable("Use alpha to coverage for masking (if MSAA enabled)");
    RTTI_CATEGORY("Output");
    RTTI_PROPERTY(m_applyFog).editable("Apply fog to the output of the material");
RTTI_END_TYPE();

MaterialGraphBlockOutputCommon::MaterialGraphBlockOutputCommon()
{}

void MaterialGraphBlockOutputCommon::buildLayout(graph::BlockLayoutBuilder& builder) const
{
}
    
void MaterialGraphBlockOutputCommon::evalRenderStates(const IMaterial& params, MaterialRenderState& outMetadata) const
{
    {
        bool transparent = false;
        if (params.readParameterTyped<bool>(PARAM_USE_TRANSPARENCY, transparent))
            outMetadata.hasTransparency = transparent;
        else
            outMetadata.hasTransparency = m_transparent;
    }

    {
        bool mask = false;
        if (params.readParameterTyped<bool>(PARAM_USE_MASK, mask))
            outMetadata.hasPixelDiscard = mask;
        else
            outMetadata.hasPixelDiscard = hasConnectionOnSocket("Mask"_id);
    }
}

bool MaterialGraphBlockOutputCommon::evalTwoSidedFlag(const MaterialStageCompiler& compiler) const
{
    if (compiler.hasStaticSwitch(PARAM_USE_TWOSIDED))
        return compiler.evalStaticSwitch(PARAM_USE_TWOSIDED);
    else
        return m_twoSided;
}

bool MaterialGraphBlockOutputCommon::evalTwoSidedLightingFlag(const MaterialStageCompiler& compiler) const
{
    if (compiler.hasStaticSwitch(PARAM_USE_TWOSIDED_LIGHTING))
        return compiler.evalStaticSwitch(PARAM_USE_TWOSIDED_LIGHTING);
    else
        return false;
}

bool MaterialGraphBlockOutputCommon::evalTransparencyFlag(const MaterialStageCompiler& compiler) const
{
    if (compiler.hasStaticSwitch(PARAM_USE_TRANSPARENCY))
        return compiler.evalStaticSwitch(PARAM_USE_TRANSPARENCY);
    else
        return m_transparent;
}

bool MaterialGraphBlockOutputCommon::evalMaskingFlag(const MaterialStageCompiler& compiler) const
{
    if (compiler.hasStaticSwitch(PARAM_USE_MASK))
        return compiler.evalStaticSwitch(PARAM_USE_MASK);
    else
        return hasConnectionOnSocket("Mask"_id);
}

void MaterialGraphBlockOutputCommon::compilePixelFunction(MaterialStageCompiler& compiler, MaterialTechniqueRenderStates& outRenderStates) const
{
    // common states that are always set
    outRenderStates.twoSided = evalTwoSidedFlag(compiler);

    // do we allow masking ? some modes have masking disabled
    const auto allowMasking = (compiler.context().pass != MaterialPass::MaterialDebug) 
        && (compiler.context().pass != MaterialPass::WireframeSolid)
        && (compiler.context().pass != MaterialPass::WireframePassThrough);

    // simple case masking
    // TODO: integrate the "msaa" flag so alpha-to-coverage is not emitted when in non-mass output
    auto mask = CodeChunk(1.0f);
    const auto flagMask = evalMaskingFlag(compiler);
    if (allowMasking && flagMask)
    {
        mask = compiler.evalInput(this, "Mask"_id, 1.0f).conform(1);
        if (!outRenderStates.alphaToCoverage)
        {
            if (compiler.debugCode())
                compiler.appendf("if (!Frame.CheckMaterialDebug(MATERIAL_FLAG_DISABLE_MASKING)) {\n");
                
            compiler.appendf("if ({} < {}) discard;\n", mask, m_maskThreshold);

            if (compiler.debugCode())
                compiler.appendf("}\n");

            // TODO: per-sample masking (ie. font outline ?)
        }
    }

    // compile general outputs
    switch (compiler.context().pass)
    {
        case MaterialPass::DepthPrepass:
        {
            compiler.appendf("gl_Target0 = vec4(1,0,1,1);\n");

            // TODO: velocities
            break;
        }

        case MaterialPass::SelectionFragments:
        {
            outRenderStates.earlyPixelTests = true;

            const auto objectID = compiler.vertexData(MaterialVertexDataType::ObjectIndex);
            const auto worldPos = compiler.vertexData(MaterialVertexDataType::WorldPosition);
            compiler.appendf("EmitSelection({},{});\n", objectID, worldPos);
            compiler.appendf("gl_Target0 = vec4(0.2,0.5,1,1);\n");
            break;
        }

        case MaterialPass::WireframePassThrough:
        {
            outRenderStates.fillLines = true;
            outRenderStates.twoSided = true;
            // PASS THROUGH
        }

        case MaterialPass::ConstantColor:
        {
            const auto objectID = compiler.vertexData(MaterialVertexDataType::ObjectIndex);
            const auto constantColor = CodeChunk(CodeChunkType::Numerical4, TempString("UnpackColorRGBA4(ObjectData[{}].Color)", objectID));
            compiler.appendf("gl_Target0 = vec4(0,1,1,1);\n", constantColor);
            //compiler.appendf("gl_Target0 = {};\n", constantColor);
            break;
        }

        case MaterialPass::WireframeSolid:
        {
            compiler.includeHeader("material/wireframe.h");
            const auto objectID = compiler.vertexData(MaterialVertexDataType::ObjectIndex);
            const auto constantColor = CodeChunk(CodeChunkType::Numerical4, TempString("UnpackColorRGBA4(ObjectData[{}].Color)", objectID));
            compiler.appendf("gl_Target0 = ({}.xyz * CalcEdgeFactor()).xyz1;\n", constantColor);
            break;
        }

        case MaterialPass::Forward:
        {
            outRenderStates.earlyPixelTests = true;

            // compile the color output of the material
            const auto color = compileMainColor(compiler, outRenderStates).conform(3);

            // selection effect

            // write combined output
            if (outRenderStates.alphaToCoverage)
                compiler.appendf("gl_Target0 = vec4({}, {});\n", color, mask);
            else
                compiler.appendf("gl_Target0 = {}.xyz1;\n", color);

            if (compiler.debugCode())
                compiler.appendf("if (Frame.CheckMaterialDebug(MATERIAL_FLAG_DISABLE_MASKING)) gl_Target0.w = 1;\n");

            break;
        }

        case MaterialPass::ForwardTransparent:
        {
            outRenderStates.alphaBlend = true;
            outRenderStates.alphaToCoverage = false;
            outRenderStates.depthWrite = false;
            outRenderStates.earlyPixelTests = true;

            const auto color = compileMainColor(compiler, outRenderStates).conform(3);
            const auto alpha = compiler.evalInput(this, "Opacity"_id, 1.0f).conform(1); // single scalar

            if (m_premultiplyAlpha)
                compiler.appendf("gl_Target0 = vec4({} * {}, {});\n", color, alpha, alpha);
            else
                compiler.appendf("gl_Target0 = vec4({}, {});\n", color, alpha);

            break;
        }
    }
}

void MaterialGraphBlockOutputCommon::compileVertexFunction(MaterialStageCompiler& compiler, MaterialTechniqueRenderStates& outRenderStates) const
{
    if (hasConnectionOnSocket("World Position Offset"_id))
    {
        auto offset = compiler.evalInput(this, "World Position Offset"_id, Vector3(0, 0, 0)).conform(3);
        if (!offset.constant())
            outRenderStates.hasVertexOffset = true;

        if (compiler.debugCode())
            compiler.appendf("if (!Frame.CheckMaterialDebug(MATERIAL_FLAG_DISABLE_VERTEX_MOTION)) WorldVertexOffset += {};\n", offset);
        else
            compiler.appendf("WorldVertexOffset += {};\n", offset);
    }
}

///---

END_BOOMER_NAMESPACE()

/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material\output #]
***/

#include "build.h"
#include "renderingMaterialGraph.h"
#include "renderingMaterialGraphBlock_OutputCommon.h"
#include "renderingMaterialCode.h"

#include "rendering/material/include/renderingMaterialRuntimeTechnique.h"
#include "rendering/material/include/renderingMaterialTemplate.h"

namespace rendering
{
    ///---

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(MaterialGraphBlockOutputCommon);
        RTTI_CATEGORY("Render state");
        RTTI_PROPERTY(m_twoSided).editable("Material is two sided");
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

    void MaterialGraphBlockOutputCommon::buildLayout(base::graph::BlockLayoutBuilder& builder) const
    {
    }
    
    void MaterialGraphBlockOutputCommon::resolveMetadata(MaterialTemplateMetadata& outMetadata) const
    {
        if (m_transparent)
            outMetadata.hasTransparency = true;

        if (hasConnectionOnSocket("Mask"_id))
            outMetadata.hasPixelDiscard = true;
        else
            outMetadata.hasPixelDiscard = false;
    }

    void MaterialGraphBlockOutputCommon::compilePixelFunction(MaterialStageCompiler& compiler, MaterialTechniqueRenderStates& outRenderStates) const
    {
        // common states that are always set
        outRenderStates.twoSided = m_twoSided;

        // do we allow masking ? some modes have masking disabled
        const auto allowMasking = (compiler.context().pass != MaterialPass::MaterialDebug) && (compiler.context().pass != MaterialPass::Wireframe);

        // simple case masking
        // TODO: integrate the "msaa" flag so alpha-to-coverage is not emitted when in non-mass output
        auto mask = CodeChunk(1.0f);
        if (allowMasking && hasConnectionOnSocket("Mask"_id))
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
                // TODO: this seems fishy
                if (!outRenderStates.alphaToCoverage || !outRenderStates.depthWrite)
                    outRenderStates.earlyPixelTests = true;

                //const auto objectID = compiler.vertexData(MaterialVertexDataType::ObjectID);
                //const auto subObjectID = compiler.vertexData(MaterialVertexDataType::SubObjectID);
                //compiler.appendf("EmitSelection({},{});\n", objectID, subObjectID);
                compiler.appendf("gl_Target0 = vec4(0,0,0,0);\n");
                break;
            }

            case MaterialPass::ConstantColor:
            {
                const auto objectID = compiler.vertexData(MaterialVertexDataType::ObjectIndex);
                const auto constantColor = CodeChunk(CodeChunkType::Numerical4, base::TempString("UnpackColorRGBA4(ObjectData[{}].Color)", objectID));
                compiler.appendf("gl_Target0 = vec4(0,1,1,1);\n", constantColor);
                //compiler.appendf("gl_Target0 = {};\n", constantColor);
                break;
            }

            case MaterialPass::Wireframe:
            {
                compiler.includeHeader("material/wireframe.h");
                const auto objectID = compiler.vertexData(MaterialVertexDataType::ObjectIndex);
                const auto constantColor = CodeChunk(CodeChunkType::Numerical4, base::TempString("UnpackColorRGBA4(ObjectData[{}].Color)", objectID));
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
            auto offset = compiler.evalInput(this, "World Position Offset"_id, base::Vector3(0, 0, 0)).conform(3);
            if (!offset.constant())
                outRenderStates.hasVertexOffset = true;

            if (compiler.debugCode())
                compiler.appendf("if (!Frame.CheckMaterialDebug(MATERIAL_FLAG_DISABLE_VERTEX_MOTION)) WorldVertexOffset += {};\n", offset);
            else
                compiler.appendf("WorldVertexOffset += {};\n", offset);
        }
    }

    ///---

} // rendering

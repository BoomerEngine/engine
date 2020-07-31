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

namespace rendering
{
    ///---

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(MaterialGraphBlockOutputCommon);
        RTTI_CATEGORY("Render state");
        RTTI_PROPERTY(m_twoSided).editable("Material is two sided");
        RTTI_PROPERTY(m_depthWrite).editable("Material write to depth channel");
        RTTI_PROPERTY(m_maskThreshold).editable("Default masking threshold");
        RTTI_PROPERTY(m_blendMode).editable("Blending mode");
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
    
    MaterialSortGroup MaterialGraphBlockOutputCommon::resolveSortGroup() const
    {
        if (m_blendMode == MaterialBlendMode::Opaque)
        {
            if (hasConnectionOnSocket("Mask"_id))
                return MaterialSortGroup::OpaqueMasked;
            else
                return MaterialSortGroup::Opaque;
        }
        else
        {
            return MaterialSortGroup::Transparent;
        }
    }

    void MaterialGraphBlockOutputCommon::compilePixelFunction(MaterialStageCompiler& compiler, MaterialTechniqueRenderStates& outRenderStates) const
    {
        outRenderStates.twoSided = m_twoSided;
        outRenderStates.depthTest = true;
        outRenderStates.depthWrite = (m_blendMode == MaterialBlendMode::Opaque);
        outRenderStates.blendMode = m_blendMode;
        outRenderStates.alphaToCoverage = (m_blendMode == MaterialBlendMode::Opaque) && hasConnectionOnSocket("Mask"_id) && m_maskAlphaToCoverage;

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

                const auto objectID = compiler.vertexData(MaterialVertexDataType::ObjectID);
                const auto subObjectID = compiler.vertexData(MaterialVertexDataType::SubObjectID);
                compiler.appendf("EmitSelection({},{});\n", objectID, subObjectID);
                compiler.appendf("gl_Target0 = vec4(0,0,0,0);\n");
                break;
            }

            case MaterialPass::ConstantColor:
            {
                const auto objectID = compiler.vertexData(MaterialVertexDataType::ObjectID);
                const auto constantColor = CodeChunk(CodeChunkType::Numerical4, base::TempString("UnpackColorRGBA4(ObjectData[{}].Color)", objectID));
                compiler.appendf("gl_Target0 = vec4(0,1,1,1);\n", constantColor);
                //compiler.appendf("gl_Target0 = {};\n", constantColor);
                break;
            }

            case MaterialPass::Wireframe:
            {
                compiler.includeHeader("material/wireframe.h");
                const auto objectID = compiler.vertexData(MaterialVertexDataType::ObjectID);
                const auto constantColor = CodeChunk(CodeChunkType::Numerical4, base::TempString("UnpackColorRGBA4(ObjectData[{}].Color)", objectID));
                compiler.appendf("gl_Target0 = ({}.xyz * CalcEdgeFactor()).xyz1;\n", constantColor);
                break;
            }

            case MaterialPass::Forward:
            {
                // TODO: this seems fishy
                if (!outRenderStates.alphaToCoverage || !outRenderStates.depthWrite)
                    outRenderStates.earlyPixelTests = true;

                // compile the color output of the material
                const auto color = compileMainColor(compiler, outRenderStates).conform(3);

                // selection effect

                // write combined output
                switch (outRenderStates.blendMode)
                {
                    case MaterialBlendMode::Opaque:
                    {
                        if (outRenderStates.alphaToCoverage)
                            compiler.appendf("gl_Target0 = vec4({}, {});\n", color, mask);
                        else
                            compiler.appendf("gl_Target0 = {}.xyz1;\n", color);

                        if (compiler.debugCode())
                            compiler.appendf("if (Frame.CheckMaterialDebug(MATERIAL_FLAG_DISABLE_MASKING)) gl_Target0.w = 1;\n");
                        break;
                    }

                    case MaterialBlendMode::Addtive:
                    {
                        DEBUG_CHECK(!outRenderStates.alphaToCoverage); // won't be supported
                        compiler.appendf("gl_Target0 = {}.xyz1;\n", color); // no premultiplied shit
                        break;
                    }

                    case MaterialBlendMode::AlphaBlend:
                    {
                        DEBUG_CHECK(!outRenderStates.alphaToCoverage); // won't be supported

                        const auto alpha = compiler.evalInput(this, "Opacity"_id, 1.0f).conform(1); // single scalar

                        if (m_premultiplyAlpha)
                            compiler.appendf("gl_Target0 = vec4({} * {}, {});\n", color, alpha, alpha);
                        else 
                            compiler.appendf("gl_Target0 = vec4({}, {});\n", color, alpha);
                        break;
                    }

                    case MaterialBlendMode::Refractive:
                    {
                        DEBUG_CHECK(!outRenderStates.alphaToCoverage); // won't be supported
                        compiler.appendf("gl_Target0 = {}.xyz1;\n", color); // no premultiplied shit
                        break;
                    }
                }

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

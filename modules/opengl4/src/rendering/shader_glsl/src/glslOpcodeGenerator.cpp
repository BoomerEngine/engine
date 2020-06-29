/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shaders #]
***/

#include "build.h"
#include "glslOpcodeGenerator.h"

#include "rendering/compiler/include/renderingShaderTypeLibrary.h"
#include "rendering/compiler/include/renderingShaderOpcodeGenerator.h"
#include "rendering/compiler/include/renderingShaderFunction.h"
#include "rendering/compiler/include/renderingShaderProgram.h"
#include "rendering/compiler/include/renderingShaderProgramInstance.h"
#include "rendering/compiler/include/renderingShaderNativeFunction.h"

#include "base/containers/include/stringBuilder.h"
#include "base/containers/include/bitSet.h"

#include "base/io/include/absolutePathBuilder.h"
#include "base/io/include/utils.h"
#include "base/io/include/ioSystem.h"

#include <functional>

namespace rendering
{
    namespace glsl
    {

        //--

        base::ConfigProperty<bool> cvExportGeneratedShaders("GLSL", "ExportGeneratedShaders", false);

        //--

        RTTI_BEGIN_TYPE_CLASS(GLSLOpcodeGenerator);
            RTTI_OLD_NAME("GLSLOpcodeGenerator");
        RTTI_END_TYPE();

        GLSLOpcodeGenerator::GLSLOpcodeGenerator()
            : m_emitSymbolNames(false)
            , m_emitLineNumbers(false)
            , m_dumpInputShaders(false)
            , m_dumpOutputShaders(false)
        {
            m_dumpDirectory = IO::GetInstance().systemPath(base::io::PathCategory::TempDir).addDir("glsl_out");
        }

        GLSLOpcodeGenerator::~GLSLOpcodeGenerator()
        {
        }

        bool GLSLOpcodeGenerator::buildResourceBinding(const base::Array<const compiler::ResourceTable*>& resourceTables,
            compiler::opcodes::ShaderResourceBindings& outBinding, base::parser::IErrorReporter& err) const
        {
            uint8_t numUniformBuffers = 0;
            uint8_t numStorageBuffers = 0;
            uint8_t numImages = 0;
            uint8_t numTextures = 0;
            uint8_t numSamplers = 0;
            for (auto resourceTable : resourceTables)
            {
                for (auto& entry : resourceTable->members())
                {
                    DEBUG_CHECK_EX(entry.m_type.isResource(), "Expected descriptor member to be a resource");

                    auto& newEntry = outBinding.emplaceBack();
                    newEntry.table = resourceTable;
                    newEntry.tableEntry = &entry;
                    newEntry.set = 0; // in GLSL we don't have descriptors

                    // assign placement
                    const auto& res = entry.m_type.resource();
                    if (res.texture)
                    {
                        if (res.uav)
                            newEntry.position = numImages++;
                        else
                            newEntry.position = numTextures++;
                    }
                    else if (res.buffer)
                    {
                        if (res.resolvedLayout)
                            newEntry.position = numStorageBuffers++;
                        else
                            newEntry.position = numImages++;
                    }
                    else if (res.constants)
                    {
                        newEntry.position = numUniformBuffers++;
                    }
                }
            }

            return true;
        }

        bool GLSLOpcodeGenerator::generateOpcodes(const compiler::opcodes::ShaderOpcodeGenerationContext& context,
            base::Buffer& outData, compiler::opcodes::ShaderStageDependencies& outDependencies, base::parser::IErrorReporter& err) const
        {
            // no entry function
            DEBUG_CHECK_EX(context.entryFunction, "No entry function specified for opcode generaiton");
            if (!context.entryFunction)
                return false;

            // generate code for entry function
            GLSLCodeGenerator generator(context, err);
            if (!generator.generateShader(outData, outDependencies))
                return false;

            // export generated code
            if (cvExportGeneratedShaders.get())
            {
                auto filePath = m_dumpDirectory.addFile(base::TempString("{}.dump.txt", context.shaderName).c_str());
                base::io::SaveFileFromBuffer(filePath, outData.data(), outData.size(), false);
            }

            return true;
        }

        //--

        GLSLCodeGenerator::FunctionKey::FunctionKey(const compiler::Function* func)//, const shader::Program* program, const shader::ProgramConstants* constants)
            : m_func(func)
        {
            calcHash();
        }

        uint64_t GLSLCodeGenerator::FunctionKey::calcHash() const
        {
            base::CRC64 hash;

            hash << m_func->name();

            if (m_func->program())
                hash << m_func->program()->name();

            hash << m_func->foldedKey();

            hash << m_constantArgs.size();
            for (auto& arg : m_constantArgs)
            {
                hash << arg.m_param->name;
                arg.m_value.calcCRC(hash);
            }
            
            return hash.crc();
        }

        base::StringBuf GLSLCodeGenerator::FunctionKey::buildUniqueName() const
        {
            if (entryFunction)
                return "main";

            base::StringBuilder ret;

            if (m_func->program())
            {
                ret << m_func->program()->name();
                ret << "_";
            }

            ret << m_func->name();

            ret << "_";
            ret << Hex(calcHash());

            return ret.toString();
        }

        //--

        GLSLCodeGenerator::GLSLCodeGenerator(const compiler::opcodes::ShaderOpcodeGenerationContext& context, base::parser::IErrorReporter& err)
            : m_context(context)
            , m_hasErrors(false)
            , m_err(err)
        {
            //m_nativeFunctionMapping["__mod"_id] = "mod";
            /*m_nativeFunctionMapping["__castToFloat"_id] = "float";
            m_nativeFunctionMapping["__castToInt"_id] = "int";
            m_nativeFunctionMapping["__castToUint"_id] = "uint";
            m_nativeFunctionMapping["__castToBool"_id] = "bool";*/

            m_nativeFunctionMapping["__create_vec2"_id] = "vec2";
            m_nativeFunctionMapping["__create_vec3"_id] = "vec3";
            m_nativeFunctionMapping["__create_vec4"_id] = "vec4";
            m_nativeFunctionMapping["__create_ivec2"_id] = "ivec2";
            m_nativeFunctionMapping["__create_ivec3"_id] = "ivec3";
            m_nativeFunctionMapping["__create_ivec4"_id] = "ivec4";
            m_nativeFunctionMapping["__create_uvec2"_id] = "uvec2";
            m_nativeFunctionMapping["__create_uvec3"_id] = "uvec3";
            m_nativeFunctionMapping["__create_uvec4"_id] = "uvec4";
            m_nativeFunctionMapping["__create_bvec2"_id] = "bvec2";
            m_nativeFunctionMapping["__create_bvec3"_id] = "bvec3";
            m_nativeFunctionMapping["__create_bvec4"_id] = "bvec4";
            m_nativeFunctionMapping["__create_mat2"_id] = "mat2";
            m_nativeFunctionMapping["__create_mat3"_id] = "mat3";
            m_nativeFunctionMapping["__create_mat4"_id] = "mat4";

            m_nativeFunctionMapping["ddx"_id] = "dFdx";
            m_nativeFunctionMapping["ddx_coarse"_id] = "dFdxCoarse";
            m_nativeFunctionMapping["ddx_fine"_id] = "dFdxFine";
            m_nativeFunctionMapping["ddy"_id] = "dFdy";
            m_nativeFunctionMapping["ddy_coarse"_id] = "dFdyCoarse";
            m_nativeFunctionMapping["ddy_fine"_id] = "dFdyFine";
            m_nativeFunctionMapping["fwidth"_id] = "fwidth";

            m_nativeFunctionMapping["frac"_id] = "fract";
            m_nativeFunctionMapping["lerp"_id] = "mix";
            m_nativeFunctionMapping["mad"_id] = "fma";

            m_nativeFunctionMapping["EmitVertex"_id] = "EmitVertex";
            m_nativeFunctionMapping["EndPrimitive"_id] = "EndPrimitive";
            m_nativeFunctionMapping["intBitsToFloat"_id] = "intBitsToFloat";
            m_nativeFunctionMapping["uintBitsToFloat"_id] = "uintBitsToFloat";
            m_nativeFunctionMapping["floatBitsToInt"_id] = "floatBitsToInt";
            m_nativeFunctionMapping["floatBitsToUint"_id] = "floatBitsToUint";

            m_nativeFunctionMapping["unpackHalf2x16"_id] = "unpackHalf2x16";
            m_nativeFunctionMapping["packHalf2x16"_id] = "packHalf2x16";
            m_nativeFunctionMapping["packUnorm2x16"_id] = "packUnorm2x16";
            m_nativeFunctionMapping["packSnorm2x16"_id] = "packSnorm2x16";
            m_nativeFunctionMapping["packUnorm4x8"_id] = "packUnorm4x8";
            m_nativeFunctionMapping["packSnorm4x8"_id] = "packSnorm4x8";
            m_nativeFunctionMapping["unpackUnorm2x16"_id] = "unpackUnorm2x16";
            m_nativeFunctionMapping["unpackSnorm2x16"_id] = "unpackSnorm2x16";
            m_nativeFunctionMapping["unpackUnorm4x8"_id] = "unpackUnorm4x8";
            m_nativeFunctionMapping["unpackSnorm4x8"_id] = "unpackSnorm4x8";

            //saturate

            m_binaryFunctionMapping["__mul"_id] = "*";
            m_binaryFunctionMapping["__vsmul"_id] = "*";
            m_binaryFunctionMapping["__svmul"_id] = "*";
            m_binaryFunctionMapping["__msmul"_id] = "*";
            m_binaryFunctionMapping["__smmul"_id] = "*";
            //m_binaryFunctionMapping["__mvmul"_id] = "*";
            //m_binaryFunctionMapping["__vmmul"_id] = "*";
            m_customFunctionPrinters["__mvmul"_id] = &GLSLCodeGenerator::PrintFuncMatrixVectorMul;
            m_customFunctionPrinters["__vmmul"_id] = &GLSLCodeGenerator::PrintFuncVectorMatrimMul;
            m_binaryFunctionMapping["__mmmul"_id] = "*";
            m_binaryFunctionMapping["__add"_id] = "+";
            m_binaryFunctionMapping["__sub"_id] = "-";
            m_binaryFunctionMapping["__div"_id] = "/";
            m_binaryFunctionMapping["__logicAnd"_id] = "&&";
            m_binaryFunctionMapping["__logicOr"_id] = "||";
            m_binaryFunctionMapping["__xor"_id] = "^";
            m_binaryFunctionMapping["__or"_id] = "|";
            m_binaryFunctionMapping["__and"_id] = "&";
            //m_binaryFunctionMapping["__neq"_id] = "!=";
            //m_binaryFunctionMapping["__eq"_id] = "==";
            //m_binaryFunctionMapping["__ge"_id] = ">=";
            //m_binaryFunctionMapping["__gt"_id] = ">";
            //m_binaryFunctionMapping["__le"_id] = "<=";
            //m_binaryFunctionMapping["__lt"_id] = "<";
            m_binaryFunctionMapping["__shl"_id] = "<<";
            m_binaryFunctionMapping["__shr"_id] = ">>";

            m_unaryFunctionMapping["__neg"_id] = "-";
            m_unaryFunctionMapping["__logicalNot"_id] = "!";
            m_unaryFunctionMapping["__not"_id] = "~";

            m_customFunctionPrinters.reserve(128);
            m_customFunctionPrinters["saturate"_id] = &GLSLCodeGenerator::PrintFuncSaturate;
            m_customFunctionPrinters["atan2"_id] = &GLSLCodeGenerator::PrintFuncAtan2;
            m_customFunctionPrinters["__neq"_id] = &GLSLCodeGenerator::PrintFuncNotEqual;
            m_customFunctionPrinters["__eq"_id] = &GLSLCodeGenerator::PrintFuncEqual;
            m_customFunctionPrinters["__ge"_id] = &GLSLCodeGenerator::PrintFuncGreaterEqual;
            m_customFunctionPrinters["__gt"_id] = &GLSLCodeGenerator::PrintFuncGreaterThen;
            m_customFunctionPrinters["__le"_id] = &GLSLCodeGenerator::PrintFuncLessEqual;
            m_customFunctionPrinters["__lt"_id] = &GLSLCodeGenerator::PrintFuncLessThen;

            m_customFunctionPrinters["texelLoad"_id] = &GLSLCodeGenerator::PrintFuncTexelLoad;
            m_customFunctionPrinters["texelLoadSample"_id] = &GLSLCodeGenerator::PrintFuncTexelLoadSample;
            m_customFunctionPrinters["texelStore"_id] = &GLSLCodeGenerator::PrintFuncTexelStore;
            m_customFunctionPrinters["texelStoreSample"_id] = &GLSLCodeGenerator::PrintFuncTexelStoreSample;
            m_customFunctionPrinters["texelSize"_id] = &GLSLCodeGenerator::PrintFuncTexelSize;

            /*m_customFunctionPrinters["bufferLoad"_id] = &GLSLCodeGenerator::PrintFuncBufferLoad;
            m_customFunctionPrinters["bufferStore"_id] = &GLSLCodeGenerator::PrintFuncBufferStore;
            m_customFunctionPrinters["bufferAtomicIncrement"_id] = &GLSLCodeGenerator::PrintFuncBufferAtomicIncrement;
            m_customFunctionPrinters["bufferAtomicDecrement"_id] = &GLSLCodeGenerator::PrintFuncBufferAtomicDecrement;
            m_nativeFunctionMapping["bufferAtomicAdd"_id] = "imageAtomicAdd";
            m_customFunctionPrinters["bufferAtomicSubtract"_id] = &GLSLCodeGenerator::PrintFuncBufferAtomicSubtract;
            m_nativeFunctionMapping["bufferAtomicMin"_id] = "imageAtomicMin";
            m_nativeFunctionMapping["bufferAtomicMax"_id] = "imageAtomicMax";
            m_nativeFunctionMapping["bufferAtomicOr"_id] = "imageAtomicOr";
            m_nativeFunctionMapping["bufferAtomicAnd"_id] = "imageAtomicAnd";
            m_nativeFunctionMapping["bufferAtomicXor"_id] = "imageAtomicXor";
            m_nativeFunctionMapping["bufferAtomicExchange"_id] = " imageAtomicExchange";
            m_nativeFunctionMapping["bufferAtomicCompareSwap"_id] = " imageAtomicCompSwap";*/

            m_customFunctionPrinters["atomicIncrement"_id] = &GLSLCodeGenerator::PrintFuncAtomicIncrement;
            m_customFunctionPrinters["atomicDecrement"_id] = &GLSLCodeGenerator::PrintFuncAtomicDecrement;
            m_customFunctionPrinters["atomicAdd"_id] = &GLSLCodeGenerator::PrintFuncAtomicAdd;
            m_customFunctionPrinters["atomicSubtract"_id] = &GLSLCodeGenerator::PrintFuncAtomicSubtract;
            m_customFunctionPrinters["atomicMin"_id] = &GLSLCodeGenerator::PrintFuncAtomicMin;
            m_customFunctionPrinters["atomicMax"_id] = &GLSLCodeGenerator::PrintFuncAtomicMax;
            m_customFunctionPrinters["atomicOr"_id] = &GLSLCodeGenerator::PrintFuncAtomicOr;
            m_customFunctionPrinters["atomicAnd"_id] = &GLSLCodeGenerator::PrintFuncAtomicAnd;
            m_customFunctionPrinters["atomicXor"_id] = &GLSLCodeGenerator::PrintFuncAtomicXor;
            m_customFunctionPrinters["atomicExchange"_id] = &GLSLCodeGenerator::PrintFuncAtomicExchange;
            m_customFunctionPrinters["atomicCompSwap"_id] = &GLSLCodeGenerator::PrintFuncAtomicCompareSwap;

            m_customFunctionPrinters["texture"_id] = &GLSLCodeGenerator::PrintFuncTexture;
            m_customFunctionPrinters["textureGatherOffset"_id] = &GLSLCodeGenerator::PrintFuncTextureGatherOffset;
            m_customFunctionPrinters["textureGather"_id] = &GLSLCodeGenerator::PrintFuncTextureGather;
            m_customFunctionPrinters["textureLod"_id] = &GLSLCodeGenerator::PrintFuncTextureLod;
            m_customFunctionPrinters["textureLodOffset"_id] = &GLSLCodeGenerator::PrintFuncTextureLodOffset;
            m_customFunctionPrinters["textureBias"_id] = &GLSLCodeGenerator::PrintFuncTextureBias;
            m_customFunctionPrinters["textureBiasOffset"_id] = &GLSLCodeGenerator::PrintFuncTextureBiasOffset;
            m_customFunctionPrinters["textureSizeLod"_id] = &GLSLCodeGenerator::PrintFuncTextureSizeLod;
            m_customFunctionPrinters["textureLoadLod"_id] = &GLSLCodeGenerator::PrintFuncTextureLoadLod;
            m_customFunctionPrinters["textureLoadLodOffset"_id] = &GLSLCodeGenerator::PrintFuncTextureLoadLodOffset;
            m_customFunctionPrinters["textureLoadSample"_id] = &GLSLCodeGenerator::PrintFuncTextureLoadLodSample;
            m_customFunctionPrinters["textureLoadSampleOffset"_id] = &GLSLCodeGenerator::PrintFuncTextureLoadLodOffsetSample;
            m_customFunctionPrinters["textureDepthCompare"_id] = &GLSLCodeGenerator::PrintFuncTextureDepthCompare;
            m_customFunctionPrinters["textureDepthCompareOffset"_id] = &GLSLCodeGenerator::PrintFuncTextureDepthCompareOffset;

            if (m_context.stage == ShaderType::Vertex)
            {
                m_allowedBuiltInInputs.insert("gl_VertexID"_id);
                m_allowedBuiltInInputs.insert("gl_InstanceID"_id);
                m_allowedBuiltInInputs.insert("gl_DrawID"_id);
                m_allowedBuiltInInputs.insert("gl_BaseVertex"_id);
                m_allowedBuiltInInputs.insert("gl_BaseInstance"_id);

                m_allowedBuiltInOutputs.insert("gl_Position"_id);
                m_allowedBuiltInOutputs.insert("gl_PointSize"_id);
                m_allowedBuiltInOutputs.insert("gl_ClipDistance"_id);
            }
            else if (m_context.stage == ShaderType::Geometry)
            {
                m_allowedBuiltInInputs.insert("gl_PositionIn"_id);
                m_allowedBuiltInInputs.insert("gl_PointSizeIn"_id);
                m_allowedBuiltInInputs.insert("gl_PrimitiveIDIn"_id);
                m_allowedBuiltInInputs.insert("gl_InvocationID"_id);

                m_allowedBuiltInOutputs.insert("gl_Position"_id);
                m_allowedBuiltInOutputs.insert("gl_PointSize"_id);
                m_allowedBuiltInOutputs.insert("gl_ClipDistance"_id);
            }
            else if (m_context.stage == ShaderType::Pixel)
            {
                m_allowedBuiltInInputs.insert("gl_FragCoord"_id);
                m_allowedBuiltInInputs.insert("gl_FrontFacing"_id);
                m_allowedBuiltInInputs.insert("gl_PointCoord"_id);
                m_allowedBuiltInInputs.insert("gl_SampleID"_id);
                m_allowedBuiltInInputs.insert("gl_SamplePosition"_id);
                m_allowedBuiltInInputs.insert("gl_SampleMaskIn"_id);
                m_allowedBuiltInInputs.insert("gl_ClipDistance"_id);
                m_allowedBuiltInInputs.insert("gl_PrimitiveID"_id);
                m_allowedBuiltInInputs.insert("gl_Layer"_id);
                m_allowedBuiltInInputs.insert("gl_ViewportIndex"_id);

                m_allowedBuiltInOutputs.insert("gl_FragDepth"_id);
                m_allowedBuiltInOutputs.insert("gl_SampleMask"_id);
                m_allowedBuiltInOutputs.insert("gl_Target0"_id);
                m_allowedBuiltInOutputs.insert("gl_Target1"_id);
                m_allowedBuiltInOutputs.insert("gl_Target2"_id);
                m_allowedBuiltInOutputs.insert("gl_Target3"_id);
                m_allowedBuiltInOutputs.insert("gl_Target4"_id);
                m_allowedBuiltInOutputs.insert("gl_Target5"_id);
                m_allowedBuiltInOutputs.insert("gl_Target6"_id);
                m_allowedBuiltInOutputs.insert("gl_Target7"_id);
            }
            else if (m_context.stage == ShaderType::Compute)
            {
                m_allowedBuiltInInputs.insert("gl_NumWorkGroups"_id);
                m_allowedBuiltInInputs.insert("gl_WorkGroupID"_id);
                m_allowedBuiltInInputs.insert("gl_LocalInvocationID"_id);
                m_allowedBuiltInInputs.insert("gl_GlobalInvocationID"_id);
                m_allowedBuiltInInputs.insert("gl_LocalInvocationIndex"_id);
            }            
        }

        GLSLCodeGenerator::~GLSLCodeGenerator()
        {
            m_exportedFunctions.clearPtr();
        }

        void GLSLCodeGenerator::reportError(const base::parser::Location& loc, base::StringView<char> message)
        {
            m_err.reportError(loc, message);
            m_hasErrors = true;
        }

        void GLSLCodeGenerator::reportWarning(const base::parser::Location& loc, base::StringView<char> message)
        {
            m_err.reportWarning(loc, message);
            m_hasErrors = true;
        }

        bool GLSLCodeGenerator::generateShader(base::Buffer& outData, compiler::opcodes::ShaderStageDependencies& outDependencies)
        {
            auto mainKey = FunctionKey(m_context.entryFunction);
            mainKey.entryFunction = true;

            // header
            m_blockPreamble << "#version 450\n";

            // compute layout
            if (m_context.stage == ShaderType::Compute)
            {
                int sizeX = m_context.entryFunction->attributes().valueAsIntOrDefault("local_size_x"_id, 8);
                int sizeY = m_context.entryFunction->attributes().valueAsIntOrDefault("local_size_y"_id, 8);
                int sizeZ = m_context.entryFunction->attributes().valueAsIntOrDefault("local_size_z"_id, 1);

                // warn if we did not define those attributes - usually a bad idea for a CS
                if (!m_context.entryFunction->attributes().has("local_size_x"_id) || !m_context.entryFunction->attributes().has("local_size_y"_id))
                {
                    m_err.reportWarning(m_context.entryFunction->location(), "Compute shader 'main' function expected to have the work group size attributes, e.g: attribute(local_size_x=8, local_size_y=8)");
                }

                m_blockPreamble.appendf("layout(local_size_x={}, local_size_y={}, local_size_z={}) in;\n", sizeX, sizeY, sizeZ);
            }
            else if (m_context.stage == ShaderType::Geometry)
            {
                auto input = m_context.entryFunction->attributes().valueOrDefault("input"_id, "");
                auto output = m_context.entryFunction->attributes().valueOrDefault("output"_id, "");
                auto maxVertices = m_context.entryFunction->attributes().valueAsIntOrDefault("max_vertices"_id, -1);

                if (input.empty())
                {
                    m_err.reportError(m_context.entryFunction->location(), "Geometry shader 'main' requires 'input' attribute, e.g: attribute(input=points)");
                    return false;
                }

                if (output.empty())
                {
                    m_err.reportError(m_context.entryFunction->location(), "Geometry shader 'main' requires 'output' attribute, e.g: attribute(output=triangle_strip)");
                    return false;
                }

                if (maxVertices < 0)
                {
                    m_err.reportError(m_context.entryFunction->location(), "Geometry shader 'main' requires 'max_vertices' attribute, e.g: attribute(max_vertices=3)");
                    return false;
                }

                if (input == "points")
                    m_blockPreamble.append("layout(points) in;\n");
                else if (input == "lines")
                    m_blockPreamble.append("layout(lines) in;\n");
                else if (input == "lines_adjacency")
                    m_blockPreamble.append("layout(lines_adjacency) in;\n");
                else if (input == "triangles")
                    m_blockPreamble.append("layout(triangles) in;\n");
                else if (input == "triangles_adjacency")
                    m_blockPreamble.append("layout(triangles_adjacency) in;\n");
                else
                {
                    m_err.reportError(m_context.entryFunction->location(), base::TempString("Geometry shader 'main' uses invalid input layout '{}'", input));
                    return false;
                }

                if (output == "points")
                    m_blockPreamble.appendf("layout(points, max_vertices={}) out;\n", maxVertices);
                else if (output == "line_strip")
                    m_blockPreamble.appendf("layout(line_strip, max_vertices={}) out;\n", maxVertices);
                else if (output == "triangle_strip")
                    m_blockPreamble.appendf("layout(triangle_strip, max_vertices={}) out;\n", maxVertices);
                else
                {
                    m_err.reportError(m_context.entryFunction->location(), base::TempString("Geometry shader 'main' uses invalid output layout '{}'", output));
                    return false;
                }
            }
            else if (m_context.stage == ShaderType::Pixel)
            {
                if (m_context.entryFunction->attributes().has("early_fragment_tests"_id))
                    m_blockPreamble.appendf("layout(early_fragment_tests) in;\n");
            }

            // export starting from main function
            auto mainFunction  = generateFunction(mainKey);
            if (!mainFunction || m_hasErrors)
                return false;

    /*        // HACK: GLSL expects main function to be named "main"
            if (m_context.stage == ShaderType::Vertex)
                mainFunction->m_name = "__innerMain";
            else
                mainFunction->m_name = "main";*/

            // process functions as we discover them
            while (!m_exportedFunctionToProcess.empty())
            {
                auto func  = m_exportedFunctionToProcess.top();
                m_exportedFunctionToProcess.pop();

                // generate code for function
                printFunctionStatement(*func, 1, &func->m_key.m_func->code(), func->m_code);                

                // export variables and other declarations
                printFunctionPreamble(*func, func->m_preamble); 

                // hack vertex shader
                if (m_context.stage == ShaderType::Vertex && func->m_key.entryFunction)
                {
                    func->m_code << "  gl_Position.z = 2.0 * gl_Position.z - gl_Position.w;\n";
                    //func->m_code << "  gl_Position.y = -gl_Position.y;\n";
                }

                // mark function as having code generated, any further references should assume function will be defined at the time of the call
                func->m_finishedCodeGeneration = true;
            }

            // copy all results to output
            exportResults(outData, outDependencies);
            return !m_hasErrors;
        }

        void GLSLCodeGenerator::exportVertexInputs(base::IFormatStream& f)
        {
            if (!m_usedVertexInputs.empty())
            {
                DEBUG_CHECK(m_context.stage == ShaderType::Vertex);

                f << "// Vertex inputs\n";

                for (auto& var : m_usedVertexInputs)
                {
                    DEBUG_CHECK(var.m_assignedLayoutIndex != -1);
                    f.appendf("in layout(location = {}) ", var.m_assignedLayoutIndex);
                    printDataType(var.m_param->loc, var.m_memberType, var.m_name.view(), f);
                    f << ";\n";
                }

                f << "\n";
            }
        }      

        void GLSLCodeGenerator::exportInputs(base::IFormatStream& f)
        {
            if (!m_usedInputs.empty())
            {
                f << "// Program inputs\n";

                // pre-assigned
                uint32_t freeIndices = 0;
                for (auto& var : m_usedInputs)
                    if (var.m_assignedLayoutIndex != -1)
                        base::SetBit(&freeIndices, var.m_assignedLayoutIndex);

                // dynamic assignment
                for (auto& var : m_usedInputs)
                {
                    if (!var.m_buildIn && var.m_assignedLayoutIndex == -1)
                    {
                        // assign first free bit just to suppress compilation issues
                        var.m_assignedLayoutIndex = base::FindNextBitCleared(&freeIndices, 32, 0);
                        base::SetBit(&freeIndices, var.m_assignedLayoutIndex);
                    }
                }

                // interface blocks
                for (auto& block : m_inputInterfaceBlocks)
                {
                    f.appendf("in {} {\n", block.m_blockName);

                    for (auto varIndex : block.m_members)
                    {
                        const auto& var = m_usedInputs[varIndex];

                        f << "  ";
                        printDataType(var.m_param->loc, var.m_exportType, var.m_name.view(), f);
                        f << ";\n";
                    }
                    
                    f << "}";

                    if (block.m_instanceName)
                        f << block.m_instanceName;
                    if (block.m_array)
                        f << "[]";
                    f << ";\n\n";
                }

                // free inputs (not in interface blocks)
                for (auto& var : m_usedInputs)
                {
                    if (!var.m_declare || var.m_interfaceBlockIndex != -1)
                        continue;

                    if (m_context.stage == ShaderType::Pixel && !var.m_buildIn)
                    {
                        if (var.m_param->attributes.has("flat"_id))
                            f << "flat ";
                    }

                    /*if (var.m_name == "gl_FragCoord")
                        f << "layout(origin_upper_left) ";*/

                    f << "in ";

                    if (var.m_assignedLayoutIndex != -1)
                        f.appendf("layout(location = {}) ", var.m_assignedLayoutIndex);

                    printDataType(var.m_param->loc, var.m_exportType, var.m_name.view(), f);
                    f << ";\n";
                }

                f << "\n";
            }
        }

        void GLSLCodeGenerator::exportOutputs(base::IFormatStream& f)
        {
            uint32_t freeIndices = 0;

            if (!m_usedOutputs.empty())
                f << "// Program outputs\n";

            for (auto& var : m_usedOutputs)
                if (var.m_assignedLayoutIndex != -1)
                    base::SetBit(&freeIndices, var.m_assignedLayoutIndex);

            // assign required inputs to actual slots
            for (auto& var : m_usedOutputs)
            {
                if (!var.m_buildIn && var.m_assignedLayoutIndex == -1)
                {
                    // do we have defined placement from the previous stage ?
                    if (m_context.requiredShaderOutputs)
                    {
                        for (auto& entry : *m_context.requiredShaderOutputs)
                        {
                            if (entry.name == var.m_name)
                            {
                                var.m_assignedLayoutIndex = entry.assignedLayoutIndex;
                                base::SetBit(&freeIndices, var.m_assignedLayoutIndex);
                            }
                        }
                    }
                }
            }

            // auto assign the rest of the outputs to some IDs that we don't use
            for (auto& var : m_usedOutputs)
            {
                if (!var.m_buildIn && var.m_assignedLayoutIndex == -1)
                {
                    var.m_assignedLayoutIndex = base::FindNextBitCleared(&freeIndices, 32, 0);
                    base::SetBit(&freeIndices, var.m_assignedLayoutIndex);
                }
            }

            // interface blocks
            for (auto& block : m_outputInterfaceBlocks)
            {
                f.appendf("out {} {\n", block.m_blockName);

                for (auto varIndex : block.m_members)
                {
                    const auto& var = m_usedOutputs[varIndex];

                    f << "  ";
                    printDataType(var.m_param->loc, var.m_exportType, var.m_name.view(), f);
                    f << ";\n";
                }

                f << "}";

                if (block.m_instanceName)
                    f << block.m_instanceName;
                if (block.m_array)
                    f << "[]";
                f << ";\n\n";
            }

            // free inputs (not in interface blocks)
            for (auto& var : m_usedOutputs)
            {
                if (!var.m_declare || var.m_interfaceBlockIndex != -1)
                    continue;

                f << "out ";

                if (var.m_assignedLayoutIndex != -1)
                    f.appendf("layout(location = {}) ", var.m_assignedLayoutIndex);

                printDataType(var.m_param->loc, var.m_param->dataType, var.m_name.view(), f);
                f << ";\n";
            }            

            if (!m_usedOutputs.empty())
                f << "\n";
        }

        void GLSLCodeGenerator::exportGroupShared(base::IFormatStream& f)
        {
            if (!m_usedGroupShared.empty())
            {
                f << "// Group shared memory\n";
            }

            for (auto& var : m_usedGroupShared)
            {
                if (!var.m_print)
                    continue;

                f << "shared ";
                printDataType(var.m_param->loc, var.m_param->dataType, var.m_name.view(), f);
                f << ";\n";
            }

            if (!m_usedGroupShared.empty())
                f << "\n";
        }

        void GLSLCodeGenerator::printTextureDeclaration(const compiler::ResourceTableEntry& entry, base::IFormatStream& f)
        {
            const auto& type = entry.m_type.resource();

            f << "sampler";

            switch (type.resolvedViewType)
            {
                case ImageViewType::View1D:
                case ImageViewType::View1DArray:
                    f << "1D"; break;

                case ImageViewType::View2D:
                case ImageViewType::View2DArray:
                    f << "2D"; break;

                case ImageViewType::View3D:
                    f << "3D"; break;

                case ImageViewType::ViewCube:
                case ImageViewType::ViewCubeArray:
                    f << "Cube"; break;
            }

            if (type.multisampled)
                f << "MS";

            switch (type.resolvedViewType)
            {
            case ImageViewType::View1DArray:
            case ImageViewType::View2DArray:
            case ImageViewType::ViewCubeArray:
                f << "Array";
            }

            if (type.depth)
                f << "Shadow";

            f << " ";
        }

        void GLSLCodeGenerator::printImageDeclaration(const compiler::ResourceTableEntry& entry, base::IFormatStream& f)
        {
            const auto& type = entry.m_type.resource();

            auto formatClass = GetImageFormatInfo(type.resolvedFormat).formatClass;
            if (formatClass == ImageFormatClass::UINT)
                f << "u";
            else if (formatClass == ImageFormatClass::INT)
                f << "i";

            f << "image";

            switch (type.resolvedViewType)
            {
            case ImageViewType::View1D:
            case ImageViewType::View1DArray:
                f << "1D"; break;

            case ImageViewType::View2D:
            case ImageViewType::View2DArray:
                f << "2D"; break;

            case ImageViewType::View3D:
                f << "3D"; break;

            case ImageViewType::ViewCube:
            case ImageViewType::ViewCubeArray:
                f << "Cube"; break;
            }

            if (type.multisampled)
                f << "MS";

            switch (type.resolvedViewType)
            {
            case ImageViewType::View1DArray:
            case ImageViewType::View2DArray:
            case ImageViewType::ViewCubeArray:
                f << "Array";
            }

            if (type.depth)
                f << "Shadow";

            f << " ";
        }

        static void PrintImageFormat(ImageFormat format, base::IFormatStream& f)
        {
            auto& info = GetImageFormatInfo(format);
            f << info.shaderName;
        }

        void GLSLCodeGenerator::exportDescriptors(base::IFormatStream& f)
        {
            if (m_context.bindingSetup)
            {
                for (auto& name : m_usedDescriptorResources)
                    f.appendf("// Used descriptor resource: {}\n", name);
                for (auto& name : m_usedDescriptorConstants)
                    f.appendf("// Used descirptor constant: {}\n", name);

                for (auto& binding : *m_context.bindingSetup)
                {
                    // was this used ?
                    if (!m_usedDescriptorResources.contains(binding.tableEntry->m_mergedName))
                    {
                        TRACE_SPAM("Descriptor resource '{}' not used by shader and will not be emited", binding.tableEntry->m_mergedName);
                        continue;
                    }

                    // declare type
                    const auto& resourceEnty = binding.tableEntry->m_type.resource();
                    if (resourceEnty.type == "ConstantBuffer"_id)
                    {
                        f.appendf("layout(binding = {}, std140) ", binding.position);
                        f.appendf("uniform {}", binding.tableEntry->m_mergedName);
                        f << "\n{\n";

                        for (auto& member : resourceEnty.resolvedLayout->members())
                        {
                            base::StringID mergedName(base::TempString("{}_{}", binding.tableEntry->m_mergedName, member.name));
                            if (m_usedDescriptorConstants.contains(mergedName))
                            {
                                f.appendf("  layout(offset = {}) ", member.layout.linearOffset);
                                printDataType(binding.tableEntry->m_location, member.type, mergedName.view(), f);
                                f.appendf("; // align={}, size={}", member.layout.linearAlignment, member.layout.linearSize);

                                if (member.type.isArray())
                                    f.appendf(" arrayCount={}, arrayStride={}", member.layout.linearArrayCount, member.layout.linearArrayStride);

                                f << "\n";
                            }
                            else
                            {
                                TRACE_SPAM("Uniform '{}' not used by shader and will not be emited", mergedName);
                            }
                        }

                        f << "};\n\n";
                    }
                    else if (resourceEnty.type == "Texture"_id)
                    {
                        f.appendf("layout(binding = {}", binding.position);

                        if (resourceEnty.uav)
                        {
                            f.appendf(", ");
                            PrintImageFormat(resourceEnty.resolvedFormat, f);
                            f.appendf(") uniform ");
                            printImageDeclaration(*binding.tableEntry, f);
                        }
                        else
                        {
                            f.appendf(") uniform ");
                            printTextureDeclaration(*binding.tableEntry, f);
                        }
                        
                        f.appendf("{} ", binding.tableEntry->m_mergedName);
                        f << ";\n\n";
                    }
                    else if (resourceEnty.type == "Buffer"_id)
                    {
                        auto writeonly = resourceEnty.attributes.has("writeonly"_id);
                        auto readonly = (!resourceEnty.uav | resourceEnty.attributes.has("readonly"_id));

                        auto modeText = readonly ? "readonly " : (writeonly ? "writeonly " : "");

                        if (resourceEnty.resolvedLayout != nullptr)
                        {
                            f.appendf("layout(binding = {}, std430) {} buffer {}_BUFFER\n{\n", binding.position, modeText, binding.tableEntry->m_name);
                            f.appendf("  {} data[24];\n", resourceEnty.resolvedLayout->name());
                            f.appendf("} {};\n\n", binding.tableEntry->m_mergedName);
                        }
                        else if (resourceEnty.resolvedFormat != ImageFormat::UNKNOWN)
                        {
                            f.appendf("layout(binding = {}, ", binding.position);
                            PrintImageFormat(resourceEnty.resolvedFormat, f);

                            auto formatClass = GetImageFormatInfo(resourceEnty.resolvedFormat).formatClass;
                            if (formatClass == ImageFormatClass::INT)
                                f.appendf(") uniform {} iimageBuffer ", modeText);
                            else if (formatClass == ImageFormatClass::UINT)
                                f.appendf(") uniform {} uimageBuffer ", modeText);
                            else
                                f.appendf(") uniform {} imageBuffer ", modeText);

                            f << binding.tableEntry->m_mergedName;
                            f << ";\n\n";
                        }
                    }
                }

                f << "\n";
            }
        }

        void GLSLCodeGenerator::exportResults(base::Buffer& outCode, compiler::opcodes::ShaderStageDependencies& outDependencies)
        {
            // glue final code
            base::StringBuilder finalCode;
            finalCode << "// Boomer Engine GLSL Shader Generator v2.0\n";
            finalCode << "// Automatic file, do not modify\n";
            finalCode << "\n";

            base::StringBuilder ioBlock;
            exportDescriptors(ioBlock);
            exportVertexInputs(ioBlock);
            exportInputs(ioBlock);
            exportOutputs(ioBlock);
            exportGroupShared(ioBlock);

            if (!m_blockPreamble.empty())
            {
                finalCode << m_blockPreamble;
                finalCode << "\n";
            }

            if (!m_blockTypes.empty())
            {
                finalCode << "// Type declarations\n";
                finalCode << m_blockTypes;
                finalCode << "\n";
            }

            if (!m_blockConstants.empty())
            {
                finalCode << "// Global constants\n";
                finalCode << m_blockConstants;
                finalCode << "\n";
            }

            if (!ioBlock.empty())
                finalCode << ioBlock;

            bool hasForwardDeclarations = false;
            for (auto func  : m_exportedFunctions)
            {
                //if (func->m_needsForwardDeclaration)
                {
                    if (!hasForwardDeclarations)
                    {
                        finalCode << "\n";
                        finalCode << "// Forward declarations\n";
                        hasForwardDeclarations = true;
                    }

                    printFunctionSignature(*func, finalCode);
                    finalCode << ";\n";
                }
            }

            if (!m_exportedFunctions.empty())
            {
                finalCode << "\n";
            }

            for (int i= m_exportedFunctions.lastValidIndex(); i >= 0; --i)
            {
                auto func  = m_exportedFunctions[i];

                printFunctionSignature(*func, finalCode);

                finalCode << "\n{\n";
                finalCode << func->m_preamble;
                finalCode << func->m_code;
                finalCode << "}\n\n";
            }

            for (auto& var : m_usedInputs)
            {
                if (var.m_assignedLayoutIndex != -1)
                {
                    auto& entry = outDependencies.emplaceBack();
                    entry.name = var.m_param->name;
                    entry.type = var.m_param->dataType;
                    entry.assignedLayoutIndex = var.m_assignedLayoutIndex;
                }
            }            

            outCode.init(POOL_TEMP, finalCode.length() + 1, 1, finalCode.c_str());
        }

        GLSLCodeGenerator::Function* GLSLCodeGenerator::generateFunction(const FunctionKey& key)
        {
            auto keyHash = key.calcHash();

            // if function is exported 
            Function* entry = nullptr;
            if (m_exportedFunctionsMap.find(keyHash, entry))
            {
                // if function is used again before all it's code is generated we may need to forward declare it
                if (!entry->m_finishedCodeGeneration)
                    entry->m_needsForwardDeclaration = true;
            }

            // create new entry
            if (!entry)
            {
                entry = MemNew(Function, key);
                m_exportedFunctionToProcess.push(entry); // we need to process this function
                m_exportedFunctions.pushBack(entry);
                m_exportedFunctionsMap[keyHash] = entry;
            }

            // use the unique function name as identifier, it encodes all the stuff that went into collapsing the function template
            return entry;
        }
        
        //--


        void GLSLCodeGenerator::printArrayCounts(const compiler::ArrayCounts& counts, base::IFormatStream& f)
        {
            if (counts.empty())
                return;

            auto outerCount = counts.arraySize(0);
            if (outerCount >= 0)
                f.appendf("[{}]", outerCount);
            else
                f.appendf("[1]");

            printArrayCounts(counts.innerCounts(), f);
        }

        void GLSLCodeGenerator::printCompoundMembers(const compiler::CompositeType& compound, base::IFormatStream& f)
        {
            for (auto& member : compound.members())
            {
                //f.appendf("layout(offset = {}) ", member.layout.linearOffset);
                f.append("  ");
                printDataType(member.location, member.type, member.name.view(), f);
                f.append(";\n");
            }
        }

        void GLSLCodeGenerator::printCompoundDeclaration(const compiler::CompositeType& compound, base::IFormatStream& f)
        {
            base::TempString temp;

            temp << "struct " << compound.name() << "\n{\n";
            printCompoundMembers(compound, temp);
            temp << "};\n\n";

            f << temp.c_str();
        }

        void GLSLCodeGenerator::printCompoundName(const compiler::CompositeType& compound, base::IFormatStream& f)
        {
            // special cases
            if (compound.hint() == compiler::CompositeTypeHint::VectorType)
            {
                auto baseType = compound.members()[0].type.baseType();
                auto componentCount = compound.scalarComponentCount();
                switch (baseType)
                {
                    case compiler::BaseType::Boolean:
                        f.appendf("bvec{}", componentCount);
                        break;

                    case compiler::BaseType::Int:
                        f.appendf("ivec{}", componentCount);
                        break;

                    case compiler::BaseType::Uint:
                        f.appendf("uvec{}", componentCount);
                        break;

                    case compiler::BaseType::Float:
                        f.appendf("vec{}", componentCount);
                        break;

                    default:
                        return;
                }
            }
            // matrix
            else if (compound.hint() == compiler::CompositeTypeHint::MatrixType)
            {
                auto rowCount = compound.members().size(); // rows
                auto colCount = compound.members()[0].type.composite().scalarComponentCount(); // cols

                if (rowCount == colCount)
                    f.appendf("mat{}", rowCount);
                else
                    f.appendf("mat{}x{}", colCount, rowCount);
            }

            // export compound declaration
            else
            {
                // export type declaration on first use
                if (m_exportedCompositeTypes.insert(&compound))
                {
                    base::StringBuilder structType;
                    printCompoundDeclaration(compound, structType);
                    m_blockTypes.append(structType.c_str());
                }

                f << compound.name();
            }
        }

        void GLSLCodeGenerator::printDataType(const base::parser::Location& location, const compiler::DataType& dataType, base::StringView<char> varName, base::IFormatStream& f)
        {
            if (!dataType.valid())
            {
                reportError(location, "Using unknown type in code");
                return;
            }

            switch (dataType.baseType())
            {
                // simple types
                case compiler::BaseType::Void:
                {
                    f << "void";
                    break;
                }

                case compiler::BaseType::Float:
                {
                    f << "float";
                    break;
                }

                case compiler::BaseType::Int:
                {
                    f << "int";
                    break;
                }

                case compiler::BaseType::Boolean:
                {
                    f << "bool";
                    break;
                }

                case compiler::BaseType::Uint:
                case compiler::BaseType::Name:
                {
                    f << "uint";
                    break;
                }

                case compiler::BaseType::Struct:
                {
                    printCompoundName(dataType.composite(), f);
                    break;
                }

                default:
                {
                    reportError(location, base::TempString("Using unsupported type '{}'", dataType));
                    return;
                }
            }

            if (varName)
            {
                f.append(" ");
                f << varName;
            }

            if (dataType.isArray())
                printArrayCounts(dataType.arrayCounts(), f);
        }

        void GLSLCodeGenerator::printDataConstant(const base::parser::Location& location, const compiler::DataValue& param, const compiler::DataType& type, base::IFormatStream& f)
        {
            if (type.isResource())
            {
                // value must be a name
                if (!param.isWholeValueDefined() || !param.isScalar() && !param.component(0).isName())
                {
                    reportError(location, base::TempString("Resource reference is not a valid folded name but {} instead", param));
                }
                else
                {
                    // find the resource
                    auto resourceName = param.component(0).name();
                    m_usedDescriptorResources.insert(resourceName);
                    m_usedDescriptors.insert(base::StringID(resourceName.view().beforeFirst("_")));

                    // oh well, just use the name
                    f << resourceName;

                    // structured buffers have internal struct inside
                    if (nullptr != type.resource().resolvedLayout)
                    {
                        if (m_exportedCompositeTypes.insert(type.resource().resolvedLayout))
                            printCompoundDeclaration(*type.resource().resolvedLayout, m_blockTypes);
                    }
                }
            }
            else if (type.isArray())
            {
                if (type.arrayCounts().dimensionCount() != 1)
                {
                    reportError(location, base::TempString("Only one dimensional constant arrays can be used"));
                }
                else
                {
                    base::CRC64 crc;
                    param.calcCRC(crc);

                    base::StringBuf constName;
                    if (!m_exportedConstants.find(crc.crc(), constName))
                    {
                        // register for further use
                        constName = base::TempString("Const{}", m_exportedConstants.size());
                        m_exportedConstants[crc.crc()] = constName;

                        m_blockConstants << "const ";

                        printDataType(location, type, constName, m_blockConstants);

                        m_blockConstants << " = ";

                        printDataType(location, type, "", m_blockConstants);

                        auto elementCount = type.arrayCounts().outermostCount();

                        auto innerType = type.removeArrayCounts();
                        auto innerComponentCount = innerType.computeScalarComponentCount();

                        m_blockConstants << "(";

                        for (uint32_t i = 0; i < elementCount; ++i)
                        {
                            if (i > 0) m_blockConstants << ", ";

                            compiler::DataValue innerValue(innerType);
                            for (uint32_t j = 0; j < innerComponentCount; ++j)
                                innerValue.component(j, param.component(i * innerComponentCount + j));

                            printDataConstant(location, innerValue, innerType, m_blockConstants);
                        }

                        m_blockConstants << ");\n";
                    }

                    f << constName;
                }
            }
            else if (type.isScalar())
            {
                if (type.baseType() == compiler::BaseType::Float)
                    f << param.component(0).valueFloat();
                else if (type.baseType() == compiler::BaseType::Boolean)
                    f << param.component(0).valueBool();
                else if (type.baseType() == compiler::BaseType::Int)
                    f << param.component(0).valueInt();
                else if (type.baseType() == compiler::BaseType::Uint)
                    f << param.component(0).valueUint();
                else if (type.baseType() == compiler::BaseType::Name)
                    f.appendf("0x{}", Hex(base::CompileTimeCRC32(param.component(0).name().c_str()).crc()));
                else
                    reportError(location, base::TempString("Scalar constant of type '{}' cannot be represented in GLSL", type));
            }
            else if (type.isComposite())
            {
                auto& compositeType = type.composite();
                if (compositeType.hint() == compiler::CompositeTypeHint::VectorType)
                {
                    auto compType = compositeType.members()[0].type.baseType();
                    auto compCount = compositeType.scalarComponentCount();
                    ASSERT(compCount >= 2 && compCount <= 4);
                    if (compType == compiler::BaseType::Float)
                        f.appendf("vec{}(", compCount);
                    else if (compType == compiler::BaseType::Int)
                        f.appendf("ivec{}(", compCount);
                    else if (compType == compiler::BaseType::Uint)
                        f.appendf("uvec{}(", compCount);
                    else if (compType == compiler::BaseType::Boolean)
                        f.appendf("bvec{}(", compCount);
                    else
                    {
                        reportError(location, base::TempString("Vector constant of type '{}' cannot be represented in GLSL", type));
                        return;
                    }

                    for (uint32_t i = 0; i < param.size(); ++i)
                    {
                        if (i > 0) f << ",";

                        if (compType == compiler::BaseType::Float)
                            f << param.component(i).valueFloat();
                        else if (compType == compiler::BaseType::Int)
                            f << param.component(i).valueInt();
                        else if (compType == compiler::BaseType::Uint)
                            f << param.component(i).valueUint();
                        else if (compType == compiler::BaseType::Boolean)
                            f << param.component(i).valueBool();
                    }

                    f << ")";
                }
                else if (compositeType.hint() == compiler::CompositeTypeHint::MatrixType)
                {
                    auto compType = compositeType.members()[0].type.composite().members()[0].type.baseType();
                    if (compType == compiler::BaseType::Float)
                    {
                        auto rowCount = compositeType.members().size(); // rows
                        auto colCount = compositeType.members()[0].type.composite().scalarComponentCount(); // cols

                        if (rowCount == colCount)
                            f.appendf("mat{}(", rowCount);
                        else
                            f.appendf("mat{}x{}(", colCount, rowCount);

                        for (uint32_t i = 0; i < param.size(); ++i)
                        {
                            if (i > 0) f << ",";
                            f << param.component(i).valueFloat();
                        }


                        f << ")";
                    }
                    else
                    {
                        reportError(location, base::TempString("Matrix constant of type '{}' cannot be represented in GLSL", type));
                    }
                }
                else
                {
                    // TODO: we may need this
                    reportError(location, base::TempString("Composite type '{}' cannot be represented in GLSL", type));
                }
            }
            else            
            {
                reportError(location, base::TempString("Unable to represent constant value of type '{}' in GLSL", type));
            }
        }
       
        const GLSLCodeGenerator::UsedGroupShared* GLSLCodeGenerator::mapGroupShared(const compiler::DataParameter* param)
        {
            // look for exact match
            for (auto& var : m_usedGroupShared)
                if (var.m_param == param)
                    return &var;

            base::HashSet<base::StringID> usedNames;
            for (auto& var : m_usedGroupShared)
            {
                if (var.m_param->name == param->name)
                {
                    if (compiler::DataType::MatchNoFlags(var.m_param->dataType, param->dataType))
                    {
                        auto& used = m_usedGroupShared.emplaceBack();
                        used.m_param = param;
                        used.m_name = var.m_name;
                        used.m_print = false;
                        return &used;
                    }

                    usedNames.insert(var.m_name);
                }
            }

            if (usedNames.empty())
            {
                auto& used = m_usedGroupShared.emplaceBack();
                used.m_param = param;
                used.m_name = param->name;
                used.m_print = true;
                return &used;
            }
            else
            {
                uint32_t counter = 1;
                for (;;)
                {
                    base::StringBuf testName(base::TempString("{}_{}", param->name, counter++));
                    if (!usedNames.contains(testName))
                    {
                        auto& used = m_usedGroupShared.emplaceBack();
                        used.m_param = param;
                        used.m_name = base::StringID(testName);
                        used.m_print = true;
                        return &used;
                    }
                }
            }
        }

        static base::StringID GetInputInterfaceBlockName(base::StringID varName, rendering::ShaderType stage)
        {
            if (varName == "gl_Position"_id || varName == "gl_PointSize"_id || varName == "gl_ClipDistance"_id || varName == "gl_CullDistance"_id)
                return "gl_PerVertex"_id;

            return base::StringID();
        }

        static base::StringID GetOutputInterfaceBlockName(base::StringID varName, rendering::ShaderType stage)
        {
            if (varName == "gl_Position"_id || varName == "gl_PointSize"_id || varName == "gl_ClipDistance"_id || varName == "gl_CullDistance"_id)
                return "gl_PerVertex"_id;

            return base::StringID();
        }

        int GLSLCodeGenerator::mapInputInterfaceBlock(base::StringID name)
        {
            for (uint32_t i = 0; i < m_inputInterfaceBlocks.size(); i++)
                if (m_inputInterfaceBlocks[i].m_blockName == name)
                    return i;

            auto blockIndex = m_inputInterfaceBlocks.size();
            auto& newBlock = m_inputInterfaceBlocks.emplaceBack();
            newBlock.m_blockName = name;

            if (name == "gl_PerVertex"_id && m_context.stage == ShaderType::Geometry)
            {
                newBlock.m_instanceName = "gl_in"_id;
                newBlock.m_array = true;
            }

            return blockIndex;
        }

        int GLSLCodeGenerator::mapOutputInterfaceBlock(base::StringID name)
        {
            for (uint32_t i = 0; i < m_outputInterfaceBlocks.size(); i++)
                if (m_outputInterfaceBlocks[i].m_blockName == name)
                    return i;

            auto blockIndex = m_outputInterfaceBlocks.size();
            auto& newBlock = m_outputInterfaceBlocks.emplaceBack();
            newBlock.m_blockName = name;

            /*if (name == "gl_PerVertex"_id && m_context.stage == ShaderType::Geometry)
            {
                newBlock.m_instanceName = "gl_in"_id;
                newBlock.m_array = true;
            }*/

            return blockIndex;
        }

        const GLSLCodeGenerator::UsedInputOutput* GLSLCodeGenerator::mapBuiltin(const compiler::DataParameter* param)
        {
            if (m_allowedBuiltInInputs.contains(param->name))
            {
                for (auto& var : m_usedInputs)
                    if (var.m_param == param)
                        return &var;

                auto varIndex = m_usedInputs.size();
                auto& used = m_usedInputs.emplaceBack();
                used.m_buildIn = true;
                used.m_param = param;
                used.m_name = param->name;
                used.m_exportType = param->dataType;
                //used.m_printName = param->name;

                if (m_context.stage == ShaderType::Geometry)
                {
                    if (param->name == "gl_PositionIn"_id)
                    {
                        DEBUG_CHECK(used.m_exportType.arrayCounts().dimensionCount() == 1);
                        used.m_name = "gl_Position"_id;
                        used.m_exportType = used.m_exportType.removeArrayCounts();
                    }
                    else if (param->name == "gl_PointSizeIn"_id)
                    {
                        DEBUG_CHECK(used.m_exportType.arrayCounts().dimensionCount() == 1);
                        used.m_name = "gl_PointSize"_id;
                        used.m_exportType = used.m_exportType.removeArrayCounts();
                    }
                }
                else if (m_context.stage == ShaderType::Compute)
                {
                    // do not print those inputs in the shader
                    if (used.m_name == "gl_NumWorkGroups"_id || used.m_name == "gl_WorkGroupID"_id ||
                        used.m_name == "gl_LocalInvocationID"_id || used.m_name == "gl_GlobalInvocationID"_id ||
                        used.m_name == "gl_LocalInvocationIndex"_id)
                        used.m_declare = false;
                }

                if (auto interfaceBlockName = GetInputInterfaceBlockName(used.m_name, m_context.stage))
                {
                    used.m_interfaceBlockIndex = mapInputInterfaceBlock(interfaceBlockName);
                    m_inputInterfaceBlocks[used.m_interfaceBlockIndex].m_members.pushBack(varIndex);
                }
                
                return &used;
            }
            else if (m_allowedBuiltInOutputs.contains(param->name))
            {
                for (auto& var : m_usedOutputs)
                    if (var.m_param == param)
                        return &var;

                auto varIndex = m_usedOutputs.size();
                auto& used = m_usedOutputs.emplaceBack();
                used.m_param = param;
                used.m_name = param->name;
                used.m_buildIn = true;
                used.m_exportType = param->dataType;
                used.m_name = param->name;

                if (used.m_name.view().beginsWith("gl_Target"))
                {
                    if (used.m_name == "gl_Target0") used.m_assignedLayoutIndex = 0;
                    else if (used.m_name == "gl_Target1") used.m_assignedLayoutIndex = 1;
                    else if (used.m_name == "gl_Target2") used.m_assignedLayoutIndex = 2;
                    else if (used.m_name == "gl_Target3") used.m_assignedLayoutIndex = 3;
                    else if (used.m_name == "gl_Target4") used.m_assignedLayoutIndex = 4;
                    else if (used.m_name == "gl_Target5") used.m_assignedLayoutIndex = 5;
                    else if (used.m_name == "gl_Target6") used.m_assignedLayoutIndex = 6;
                    else if (used.m_name == "gl_Target7") used.m_assignedLayoutIndex = 7;

                    used.m_name = base::StringID(base::TempString("Output{}", used.m_name.view().afterFirst("gl_")));
                }
                else
                {
                    if (auto interfaceBlockName = GetOutputInterfaceBlockName(used.m_name, m_context.stage))
                    {
                        used.m_interfaceBlockIndex = mapOutputInterfaceBlock(interfaceBlockName);
                        m_outputInterfaceBlocks[used.m_interfaceBlockIndex].m_members.pushBack(varIndex);
                    }

                }

                return &used;
            }
            else
            {
                return nullptr;
            }
        }

        static base::StringID BuildMergedVertexAttributeName(base::StringView<char> bindingName, base::StringView<char> memberName)
        {
            return base::StringID(base::TempString("_{}_{}", bindingName, memberName));
        }
        
        const GLSLCodeGenerator::UsedVertexInput* GLSLCodeGenerator::mapVertexInput(const compiler::DataParameter* param, base::StringID memberName)
        {
            DEBUG_CHECK(param->scope == compiler::DataParameterScope::VertexInput);
            DEBUG_CHECK(param->dataType.isComposite());
            DEBUG_CHECK(param->dataType.composite().packingRules() == compiler::CompositePackingRules::Vertex);
            DEBUG_CHECK(param->dataType.composite().hint() == compiler::CompositeTypeHint::User);

            for (auto& var : m_usedVertexInputs)
                if (var.m_param == param && var.m_memberName == memberName)
                    return &var;

            const auto bindingName = param->attributes.valueOrDefault("binding"_id, param->dataType.composite().name().view());

            const compiler::opcodes::ShaderVertexInputEntry* bindingPlacement = nullptr;
            if (nullptr != m_context.vertexInput)
            {
                for (const auto& elem : *m_context.vertexInput)
                {
                    //TRACE_INFO("'{}' '{}'", elem.bindingName, elem.memberName);
                    if (elem.bindingName == bindingName && elem.memberName == memberName)
                    {
                        bindingPlacement = &elem;
                        break;
                    }
                }
            }

            DEBUG_CHECK_EX(nullptr != bindingPlacement, base::TempString("No binding found for '{}' in '{}'", bindingName, memberName));

            const auto memberIndex = param->dataType.composite().memberIndex(memberName);
            DEBUG_CHECK(memberIndex != INDEX_NONE);

            const auto& memberType = param->dataType.composite().memberType(memberIndex);

            const auto mergedName = BuildMergedVertexAttributeName(bindingName, memberName.view());

            DEBUG_CHECK(bindingPlacement)
            auto& used = m_usedVertexInputs.emplaceBack();
            used.m_param = param;
            used.m_memberName = memberName;
            used.m_name = mergedName;
            used.m_memberType = memberType;
            used.m_assignedLayoutIndex = bindingPlacement->layoutIndex;
            return &used;
        }

        static bool StageRequiresArrayInput(const rendering::ShaderType stage)
        {
            return stage == ShaderType::Geometry;
        }

        static bool StageRequiresArrayOutputs(const rendering::ShaderType stage)
        {
            return false;// stage == ShaderType::Geometry;
        }

        const GLSLCodeGenerator::UsedInputOutput* GLSLCodeGenerator::mapInput(const compiler::DataParameter* param)
        {
            DEBUG_CHECK(param->scope == compiler::DataParameterScope::StageInput);

            for (auto& var : m_usedInputs)
                if (var.m_param == param)
                    return &var;

            if (param->dataType.isArray() != StageRequiresArrayInput(m_context.stage))
            {
                if (param->dataType.isArray())
                    reportError(param->loc, base::TempString("Current shader stage does not support input '{}' to be an array", param->name));
                else
                    reportError(param->loc, base::TempString("Current shader stage expects input '{}' to be an array", param->name));

                return nullptr;
            }

            auto& used = m_usedInputs.emplaceBack();
            used.m_param = param;
            used.m_name = param->name;
            //used.m_printName = param->name;
            used.m_exportType = param->dataType;

            return &used;
        }

        const GLSLCodeGenerator::UsedInputOutput* GLSLCodeGenerator::mapOutput(const compiler::DataParameter* param)
        {
            DEBUG_CHECK(param->scope == compiler::DataParameterScope::StageOutput);

            for (auto& var : m_usedOutputs)
                if (var.m_param == param)
                    return &var;

            if (param->dataType.isArray() != StageRequiresArrayOutputs(m_context.stage))
            {
                if (param->dataType.isArray())
                    reportError(param->loc, base::TempString("Current shader stage does not support output '{}' to be an array", param->name));
                else
                    reportError(param->loc, base::TempString("Current shader stage expects output '{}' to be an array", param->name));

                return nullptr;
            }

            auto& used = m_usedOutputs.emplaceBack();
            used.m_param = param;
            used.m_name = param->name.view();
            used.m_exportType = param->dataType;
            //used.m_printName = used.m_name;
            return &used;
        }
        
        //--

        void GLSLCodeGenerator::printDataParam(const base::parser::Location& location, Function &func, const compiler::DataParameter* param, base::IFormatStream& f)
        {
            // it may be part of collapsed function args
            if (param->scope == compiler::DataParameterScope::FunctionInput)
            {
                // check if the function argument was a compile time constant
                bool folded = false;
                for (auto& constArg : func.m_key.m_constantArgs)
                {
                    if (constArg.m_param == param)
                    {
                        printDataConstant(location, constArg.m_value, param->dataType, f);
                        folded = true;
                        break;
                    }
                }

                // use the argument itself
                if (!folded)
                    f << param->name;
            }
            else if (param->scope == compiler::DataParameterScope::StageInput)
            {
                auto used  = mapInput(param);
                if (used)
                    f << used->m_name;
                return;
            }
            else if (param->scope == compiler::DataParameterScope::VertexInput)
            {
                m_err.reportError(param->loc, base::TempString("Usage of vertex input '{}' directly is not allowed yet, you can only read members", param->name));
                return;
            }
            else if (param->scope == compiler::DataParameterScope::StageOutput)
            {
                auto used  = mapOutput(param);
                if (used)
                    f << used->m_name;
                return;
            }
            else if (param->scope == compiler::DataParameterScope::GlobalBuiltin)
            {
                auto used = mapBuiltin(param);
                if (used)
                    f << used->m_name;
                return;
            }
            else if (param->scope == compiler::DataParameterScope::GroupShared)
            {
                auto used  = mapGroupShared(param);
                if (used)
                    f << used->m_name;
                return;
            }
            else if (param->scope == compiler::DataParameterScope::GlobalConst)
            {
                /*if (func.key.constants)
                {
                    if (auto value  = func.key.constants->findConstValue(param))
                    {
                        printDataConstant(location, *value, param->dataType, f);
                        return;
                    }
                }*/

                m_err.reportError(param->loc, base::TempString("Usage of constant '{}' was not folded properly to a compile-time value, make sure constant is well defined in all paths", param->name));
            }
            else if (param->scope == compiler::DataParameterScope::GlobalParameter)
            {
                if (param->dataType.isResource())
                {
                    m_err.reportError(param->loc, base::TempString("Direct usage of resource param '{}' should not happen, it should have been folded to a static resource reference sooner", param->name));
                }
                else
                {
                    m_usedDescriptorConstants.insert(param->name);

                    auto constantBufferName = param->name.view().beforeLast("_");
                    m_usedDescriptorResources.insert(constantBufferName);

                    auto descriptorName = param->name.view().beforeFirst("_");
                    m_usedDescriptors.insert(descriptorName);

                    // use as param name
                    f << param->name;
                }
            }
            else if (param->scope == compiler::DataParameterScope::ScopeLocal)
            {
                base::StringID paramName;
                if (!func.m_localVariablesParamMap.find(param, paramName))
                {
                    // track the local variable
                    auto localVar  = func.m_localVariables.find(param->name);
                    if (!localVar)
                    {
                        FunctionLocalVar entry;
                        entry.m_name = param->name;
                        func.m_localVariables[param->name] = entry;
                        localVar = func.m_localVariables.find(param->name);
                    }
                    
                    // find matching entry (with type)
                    for (auto& entry : localVar->m_names)
                    {
                        if (compiler::DataType::MatchNoFlags(entry.first, param->dataType))
                        {
                            paramName = entry.second;
                            break;
                        }
                    }

                    // map variable to different name in case we have a name conflict
                    if (paramName.empty())
                    {
                        if (localVar->m_names.empty())
                            paramName = localVar->m_name;
                        else
                            paramName = base::StringID(base::TempString("{}__copy_{}", localVar->m_name, localVar->m_names.size()));
                        localVar->m_names.emplaceBack(param->dataType, paramName);
                    }

                    // store for future reference
                    func.m_localVariablesParamMap[param] = paramName;                    
                }

                f << paramName;
            }
            else
            {
                m_err.reportError(param->loc, base::TempString("Unable to recognized way '{}' should be used in this shader", param->name));
            }
        }

        void GLSLCodeGenerator::printFunctionSignature(Function& func, base::IFormatStream& f)
        {
            auto programFunc  = func.m_key.m_func;

            printDataType(programFunc->location(), programFunc->returnType(), "", f);
            f << " ";
            f << func.m_name;
            f << "(";

            // generate parameters
            bool hasParams = false;
            for (auto param  : programFunc->inputParameters())
            {
                // do we have a constant value defined for this parameter ?
                bool hasConst = false;
                for (auto& constParam : func.m_key.m_constantArgs)
                {
                    if (constParam.m_param->name == param->name)
                    {
                        hasConst = true;
                        break;
                    }
                }

                // resources can't be passes as normal parameters, the MUST be folded to a constant reference
                if (param->dataType.isResource())
                    continue;

                // skip parameters that have constant values passed
                if (hasConst)
                    continue;

                if (hasParams)
                    f << ", ";

                if (param->attributes.has("out"_id))
                    f << "inout ";

                printDataType(param->loc, param->dataType, param->name.view(), f);
                hasParams = true;
            }

            f << ")";
        }

        static bool IsIdentityMask(const compiler::ComponentMask& mask, uint32_t inputCount)
        {
            if (inputCount > 4)
                return false;
            if (inputCount != mask.numberOfComponentsProduced())
                return false;

            for (uint32_t i = 0; i < inputCount; ++i)
            {
                if ((uint32_t)mask.componentSwizzle(i) != i)
                    return false;
            }

            return true;
        }

        static bool IsKosherMask(const compiler::ComponentMask& mask)
        {
            auto inputCount = mask.numComponents();
            for (uint32_t i = 0; i < inputCount; ++i)
            {
                auto field = (uint32_t)mask.componentSwizzle(i);
                if (field >= 3)
                    return false;
            }

            return true;
        }

        static bool IsSingleBlockMask(const compiler::ComponentMask& mask)
        {
            uint32_t type = 0;
            uint32_t numBlocks = 0;

            auto inputCount = mask.numComponents();
            for (uint32_t i = 0; i < inputCount; ++i)
            {
                auto field = (uint32_t)mask.componentSwizzle(i);
                if (field <= 3)
                {
                    if (type != 1) numBlocks += 1;
                    type = 1;
                }
                else
                {
                    if (type != 2) numBlocks += 1;
                    type = 2;
                }
            }

            return numBlocks <= 2;
        }

        static void PrintTypeConstructor(const compiler::DataType& type, base::IFormatStream& f)
        {
            if (type.isComposite())
            {
                auto& compositeType = type.composite();
                if (compositeType.hint() == compiler::CompositeTypeHint::VectorType)
                {
                    auto compType = compositeType.members()[0].type.baseType();
                    auto compCount = compositeType.scalarComponentCount();
                    ASSERT(compCount >= 2 && compCount <= 4);
                    if (compType == compiler::BaseType::Float)
                        f.appendf("vec{}(", compCount);
                    else if (compType == compiler::BaseType::Int)
                        f.appendf("ivec{}(", compCount);
                    else if (compType == compiler::BaseType::Uint)
                        f.appendf("uvec{}(", compCount);
                    else if (compType == compiler::BaseType::Boolean)
                        f.appendf("bvec{}(", compCount);
                }
            }
        }

        void GLSLCodeGenerator::printFunctionPreamble(const Function& function, base::IFormatStream& f)
        {
            for (auto& localVar : function.m_localVariables.values())
            {
                for (auto& varEntry : localVar.m_names)
                {
                    f << "  ";
                    printDataType(base::parser::Location(), varEntry.first, varEntry.second.view(), f);
                    f << ";\n";
                }
            }
        }

        void GLSLCodeGenerator::printFunctionStatement(Function& function, uint32_t depth, const compiler::CodeNode* node, base::IFormatStream& f)
        {
            if (!node)
                return;

            switch (node->opCode())
            {
                case compiler::OpCode::Nop:
                    return; // no new line!

                case compiler::OpCode::Scope:
                {
                    //f.appendPadding(' ', depth * 2);
                    //f << "{\n";

                    for (auto child  : node->children())
                        printFunctionStatement(function, depth, child, f);

                    //f.appendPadding(' ', depth * 2);
                    //f << "}\n";

                    break;
                }

                case compiler::OpCode::Break:
                {
                    f.appendPadding(' ', depth * 2);
                    f << "break;";
                    break;
                }

                case compiler::OpCode::Continue:
                {
                    f.appendPadding(' ', depth * 2);
                    f << "continue;";
                    break;
                }

                case compiler::OpCode::Loop:
                {
                    f.appendPadding(' ', depth * 2);
                    f << "for (;";
                    printFunctionCode(function, node->children()[0], f); // condition
                    f << ";";
                    printFunctionCode(function, node->children()[1], f); // increment
                    f << ")\n";

                    f.appendPadding(' ', depth * 2);
                    f << "{\n";

                    printFunctionStatement(function, depth+1, node->children()[2], f); // code

                    f.appendPadding(' ', depth * 2);
                    f << "}\n";

                    break;
                }

                case compiler::OpCode::Exit:
                {
                    f.appendPadding(' ', depth * 2);
                    f << "discard;";
                    break;
                }

                case compiler::OpCode::Return:
                {
                    f.appendPadding(' ', depth * 2);
                    f << "return";

                    if (!node->children().empty())
                    {
                        f << " ";
                        printFunctionCode(function, node->children()[0], f);
                    }
                    f << ";";
                    break;
                }

                case compiler::OpCode::IfElse:
                {
                    auto numChildren = node->children().size();

                    // generate the if/else ladder
                    uint32_t i = 0;
                    while ((i + 1) < numChildren)
                    {
                        if (i == 0)
                        {
                            f.appendPadding(' ', depth * 2);
                            f << "if (";
                        }
                        else
                        {
                            f << "else if (";
                        }

                        printFunctionCode(function, node->children()[i], f); // condition
                        f << ") {\n";
                        printFunctionStatement(function, depth+1, node->children()[i + 1], f); // code

                        f.appendPadding(' ', depth * 2);
                        f << "}";

                        i += 2;
                    }

                    // if we have the last bit it's the "else"
                    if (i < numChildren)
                    {
                        f << " else {\n";
                        printFunctionStatement(function, depth+1, node->children()[i], f); // code

                        f.appendPadding(' ', depth * 2);
                        f << "}";
                    }

                    break;
                }

                default:
                {
                    f.appendPadding(' ', depth * 2);
                    printFunctionCode(function, node, f);
                    f.append(";");
                }
            }

            f.append("\n");
        }

        void GLSLCodeGenerator::printFunctionCode(Function& function, const compiler::CodeNode* node, base::IFormatStream& f)
        {
            if (!node)
                return;

            switch (node->opCode())
            {
                case compiler::OpCode::Nop:
                    break;

                case compiler::OpCode::Load:
                {
                    printFunctionCode(function, node->children()[0], f);
                    break;
                }

                case compiler::OpCode::Store:
                {
                    printStore(function, node, f);
                    break;
                }

                case compiler::OpCode::NativeCall:
                {
                    printNativeCall(function, node, node->extraData().m_nativeFunctionRef, f);
                    break;
                }

                case compiler::OpCode::Const:
                {
                    printDataConstant(node->location(), node->dataValue(), node->dataType(), f);
                    break;
                }

                case compiler::OpCode::ParamRef:
                {
                    printDataParam(node->location(), function, node->extraData().m_paramRef, f);
                    break;
                }

                case compiler::OpCode::ReadSwizzle:
                {
                    auto mask = node->extraData().m_mask;
                    auto& inputDataType = node->children()[0]->dataType();
                    if (IsIdentityMask(mask, inputDataType.computeScalarComponentCount()))
                    {
                        printFunctionCode(function, node->children()[0], f);
                    }
                    else if (IsKosherMask(mask))
                    {
                        printFunctionCode(function, node->children()[0], f);
                        f << ".";

                        const char components[4] = { 'x','y','z','w' };

                        auto inputCount = mask.numComponents();
                        for (uint32_t i = 0; i < inputCount; ++i)
                        {
                            auto field = (uint32_t)mask.componentSwizzle(i);
                            f.appendch(components[field]);
                        }
                    }
                    else if (IsSingleBlockMask(mask))
                    {
                        PrintTypeConstructor(node->dataType(), f); // vec3

                        const char components[4] = { 'x','y','z','w' };

                        bool hasBlock = false;
                        bool needsComma = false;
                        auto inputCount = mask.numComponents();
                        for (uint32_t i = 0; i < inputCount; ++i)
                        {
                            auto field = (uint32_t)mask.componentSwizzle(i);
                            if (field <= 3)
                            {
                                if (!hasBlock)
                                {
                                    if (needsComma) f << ",";
                                    hasBlock = true;
                                    printFunctionCode(function, node->children()[0], f);
                                    f << ".";
                                }

                                f.appendch(components[field]);
                                needsComma = true;
                            }
                            else
                            {
                                if (needsComma) f << ",";

                                if (field == 4) f << "0";
                                if (field == 5) f << "1";

                                needsComma = true;
                            }
                        }       

                        if (node->dataType().isComposite())
                            f << ")";
                    }
                    else
                    {
                        // TODO!
                        reportError(node->location(), base::TempString("Unsupported swizzle '{}'", mask));
                    }

                    break;
                }

                case compiler::OpCode::AccessArray:
                {
                    printArrayAcccess(function, node, f);
                    break;
                }

                case compiler::OpCode::AccessMember:
                {
                    printMemberAccess(function, node, f);
                    break;
                }

                case compiler::OpCode::Call:
                {
                    FunctionKey callKey;
                    callKey.m_func = node->extraData().m_finalFunctionRef;
                    //callKey.m_program = node->extraData().m_finalProgramRef;
                    //callKey.m_constants = node->extraData().m_finalProgramConsts;
                    ASSERT(callKey.m_func);

                    //TRACE_INFO("Calling GLSL function '{}' from program '{}' with '{}'", callKey.targetFunction->name(), callKey.program ? callKey.program->name() : base::StringID(), IndirectPrint(callKey.constants));

                    // look at inputs, extract all the ones that are constants into a separate list that we will use to specialize the function
                    auto numFunctionArgs = callKey.m_func->inputParameters().size();
                    ASSERT(node->children().size() == (numFunctionArgs + 1));
                    base::InplaceArray<uint32_t, 16> argumentsToPass;
                    for (uint32_t i = 0; i<numFunctionArgs; ++i)
                    {
                        auto argNode  = node->children()[i + 1];
                        auto arg  = callKey.m_func->inputParameters()[i];

                        if (argNode->dataValue().isWholeValueDefined())
                        {
                            auto& entry = callKey.m_constantArgs.emplaceBack();
                            entry.m_param = arg;
                            entry.m_value = argNode->dataValue();
                        }
                        else if (argNode->dataType().isResource())
                        {
                            m_err.reportError(node->location(), base::TempString("Cannot pass resource-type parameter '{}' directly to function '{}'. It should have been folded to a static reference first", arg->name, callKey.m_func->name()));
                        }
                        else
                        {
                            argumentsToPass.pushBack(i);
                        }
                    }

                    // generate a function declaration with given key
                    auto childFunction  = this->generateFunction(callKey);

                    // generate call
                    f << childFunction->m_name;
                    f << "(";

                    // pass parameters that are NOT constants
                    for (uint32_t i = 0; i < argumentsToPass.size(); i++)
                    {
                        if (i > 0) f << ", ";

                        auto argIndex = argumentsToPass[i];
                        auto argNode  = node->children()[argIndex + 1];
                        printFunctionCode(function, argNode, f);
                    }

                    f << ")";
                    break;
                }

                // rest of the opcodes are not translatable as expressions
                default:
                    reportError(node->location(), base::TempString("Unable to generate GLSL code for this node '{}'", node->opCode()));
                    break;
            }            
        }

        static bool IsArrayBuiltin(const compiler::DataParameter* param)
        {
            if (!param)
                return false;
            if (param->scope != compiler::DataParameterScope::GlobalBuiltin)
                return false;
            return param->dataType.arrayDimensionCount() != 0;
        }
        
        void GLSLCodeGenerator::printArrayAcccess(Function& function, const compiler::CodeNode* node, base::IFormatStream& f)
        {
            DEBUG_CHECK(node->children().size() == 2);

            // some resources may redefine the array access
            const auto& childType = node->children()[0]->dataType();
            if (childType.isResource())
            {
                const auto& res = childType.resource();

                if (res.buffer)
                {
                    if (res.resolvedLayout == nullptr)
                    {
                        // use load function
                        f << "(imageLoad(  ";
                        printFunctionCode(function, node->children()[0], f);
                        f << ", int(";
                        printFunctionCode(function, node->children()[1], f);
                        f << ")))";

                        // do we emit less than 4 components ?
                        const auto formatInfo = GetImageFormatInfo(res.resolvedFormat);
                        if (formatInfo.numComponents == 3)
                            f << ".xyz";
                        else if (formatInfo.numComponents == 2)
                            f << ".xy";
                        else if (formatInfo.numComponents == 1)
                            f << ".x";
                    }
                    else
                    {
                        // print as generic array access
                        printFunctionCode(function, node->children()[0], f);
                        f << ".data[";
                        printFunctionCode(function, node->children()[1], f);
                        f << "]";
                    }
                }
                else if (res.texture)
                {
                    // cast type
                    auto castType = "int";
                    const auto numComps = res.addressComponentCount();
                    if (numComps == 2)
                        castType = "ivec2";
                    else if (numComps == 3)
                        castType = "ivec3";
                    else if (numComps == 4)
                        castType = "ivec4";

                    if (!res.uav)
                    {
                        // use fetch function
                        f << "texelFetch(  ";
                        printFunctionCode(function, node->children()[0], f);
                        f.appendf(", {}(", castType);
                        printFunctionCode(function, node->children()[1], f);
                        f << "), 0)";
                    }
                    else
                    {
                        // use load function
                        f << "imageLoad(  ";
                        printFunctionCode(function, node->children()[0], f);
                        f.appendf(", {}(", castType);
                        printFunctionCode(function, node->children()[1], f);
                        f << "))";
                    }

                    // do we emit less than 4 components ?
                    const auto formatInfo = GetImageFormatInfo(res.resolvedFormat);
                    if (formatInfo.numComponents == 3)
                        f << ".xyz";
                    else if (formatInfo.numComponents == 2)
                        f << ".xy";
                    else if (formatInfo.numComponents == 1)
                        f << ".x";
                }
                else
                {
                    printGenericArrayAcces(function, node, f);
                }
            }
            else if (node->children()[0]->opCode() == compiler::OpCode::ParamRef && IsArrayBuiltin(node->children()[0]->extraData().m_paramRef))
            {
                // TODO: get the instance name
                f << "gl_in[";
                printFunctionCode(function, node->children()[1], f); // index goes first
                f << "].";
                printFunctionCode(function, node->children()[0], f); // then the param
            }
            else
            {
                printGenericArrayAcces(function, node, f);
            }
        }

        void GLSLCodeGenerator::printStore(Function& function, const compiler::CodeNode* node, base::IFormatStream& f)
        {
            // special case for a store to an resource
            const auto* rValue = node->children()[0];
            const auto* lValue = node->children()[1];
            if (rValue->opCode() == compiler::OpCode::AccessArray)
            {
                const auto* dataValue = rValue->children()[0];
                if (dataValue->dataType().isResource())
                {
                    const auto* indexValue = rValue->children()[1];
                    printResourceStore(function, node, dataValue, indexValue, lValue, f);
                    return;
                }
            }

            // handle as generic store
            printGenericStore(function, node, f);
        }

        void GLSLCodeGenerator::printResourceStore(Function& function, const compiler::CodeNode* node, const compiler::CodeNode* resNode, const compiler::CodeNode* indexNode, const compiler::CodeNode* dataNode, base::IFormatStream& f)
        {
            const auto& childType = resNode->dataType();
            DEBUG_CHECK(childType.isResource());

            const auto& res = childType.resource();

            if (res.buffer)
            {
                if (res.resolvedLayout == nullptr)
                {
                    // use load function
                    f << "imageStore(  ";
                    printFunctionCode(function, resNode, f);
                    f << ", int(";
                    printFunctionCode(function, indexNode, f);
                    f << "), (";
                    printFunctionCode(function, dataNode, f);

                    // do we emit less than 4 components ?
                    const auto formatInfo = GetImageFormatInfo(res.resolvedFormat);
                    if (formatInfo.numComponents == 4)
                        f << ")";
                    else if (formatInfo.numComponents == 3)
                        f << ").xyzz";
                    else if (formatInfo.numComponents == 2)
                        f << ").xyyy";
                    else if (formatInfo.numComponents == 1)
                        f << ").xxxx";

                    f << ")";
                }
                else
                {
                    printGenericStore(function, node, f);
                }
            }
            else if (res.texture)
            {
                DEBUG_CHECK(res.resolvedLayout == nullptr);

                // cast type
                auto castType = "int";
                const auto numComps = res.addressComponentCount();
                if (numComps == 2)
                    castType = "ivec2";
                else if (numComps == 3)
                    castType = "ivec3";
                else if (numComps == 4)
                    castType = "ivec4";

                // use load function
                f << "imageStore(  ";
                printFunctionCode(function, resNode, f);
                f.appendf(", {}(", castType);
                printFunctionCode(function, indexNode, f);
                f << "), (";
                printFunctionCode(function, dataNode, f);

                // do we emit less than 4 components ?
                const auto formatInfo = GetImageFormatInfo(res.resolvedFormat);
                if (formatInfo.numComponents == 4)
                    f << ")";
                else if (formatInfo.numComponents == 3)
                    f << ").xyzz";
                else if (formatInfo.numComponents == 2)
                    f << ").xyyy";
                else if (formatInfo.numComponents == 1)
                    f << ").xxxx";

                f << ")";
            }
            else
            {
                printGenericStore(function, node, f);
            }
        }

        void GLSLCodeGenerator::printGenericStore(Function& function, const compiler::CodeNode* node, base::IFormatStream& f)
        {
            printFunctionCode(function, node->children()[0], f);

            auto& mask = node->extraData().m_mask;
            if (mask.valid())
            {
                if (!mask.isMask())
                    reportError(node->location(), "Invalid writing mask");

                f << ".";

                const char components[4] = { 'x','y','z','w' };

                auto inputCount = mask.numComponents();
                for (uint32_t i = 0; i < inputCount; ++i)
                {
                    auto field = (uint32_t)mask.componentSwizzle(i);
                    f.appendch(components[field]);
                }
            }

            f << " = ";
            printFunctionCode(function, node->children()[1], f);
        }

        void GLSLCodeGenerator::printGenericArrayAcces(Function& function, const compiler::CodeNode* node, base::IFormatStream& f)
        {
            // print as generic array access
            printFunctionCode(function, node->children()[0], f);
            f << "[";
            printFunctionCode(function, node->children()[1], f);
            f << "]";
        }

        void GLSLCodeGenerator::printMemberAccess(Function& function, const compiler::CodeNode* node, base::IFormatStream& f)
        {
            DEBUG_CHECK(node->children().size() == 1);
            DEBUG_CHECK(node->extraData().m_name);

            const auto* structNode = node->children()[0];
            const auto memberName = node->extraData().m_name;
            if (structNode->opCode() == compiler::OpCode::ParamRef)
            {
                const auto* structParam = structNode->extraData().m_paramRef;
                DEBUG_CHECK(nullptr != structParam);

                if (structParam->scope == compiler::DataParameterScope::VertexInput && m_context.stage == ShaderType::Vertex)
                {
                    const auto* mappedVertexParam = mapVertexInput(structParam, memberName);
                    if (mappedVertexParam)
                        f << mappedVertexParam->m_name;
                    return;
                }
            }

            printGenericMemberAccess(function, node, f);
        }

        void GLSLCodeGenerator::printGenericMemberAccess(Function& function, const compiler::CodeNode* node, base::IFormatStream& f)
        {
            printFunctionCode(function, node->children()[0], f);
            f << "." << node->extraData().m_name;
        }

        void GLSLCodeGenerator::printNativeCall(Function& function, const compiler::CodeNode* node, const compiler::INativeFunction* nativeFunc, base::IFormatStream& f)
        {
            auto name = base::StringID(nativeFunc->cls()->findMetadataRef<compiler::FunctionNameMetadata>().name());
            auto printName = name.view();

            // specialized shit
            if (node->children().size() == 2)
            {
                if (m_binaryFunctionMapping.find(name, printName))
                {
                    f << "(";
                    printFunctionCode(function, node->children()[0], f);
                    f << " " << printName << " ";
                    printFunctionCode(function, node->children()[1], f);
                    f << ")";

                    return;
                }
            }
            else if (node->children().size() == 1)
            {
                if (m_unaryFunctionMapping.find(name, printName))
                {
                    f << "(";
                    f << " " << printName << " ";
                    printFunctionCode(function, node->children()[0], f);
                    f << ")";

                    return;
                }
            }

            // select
            if (name == "__select"_id)
            {
                f << "(";
                printFunctionCode(function, node->children()[0], f);
                f << " ? ";
                printFunctionCode(function, node->children()[1], f);
                f << " : ";
                printFunctionCode(function, node->children()[2], f);
                f << ")";
                return;
            }
            // casting
            else if (name == "__castToFloat"_id || name == "__castToInt"_id || name == "__castToUint"_id || name == "__castToBool"_id)
            {
                if (node->dataType().isComposite())
                {
                    printCompoundName(node->dataType().composite(), f);
                    f << "(";
                    printFunctionCode(function, node->children()[0], f);
                    f << ")";
                }
                else if (name == "__castToFloat"_id)
                {
                    f << "float(";
                    printFunctionCode(function, node->children()[0], f);
                    f << ")";
                }
                else if (name == "__castToInt"_id)
                {
                    f << "int(";
                    printFunctionCode(function, node->children()[0], f);
                    f << ")";
                }
                else if (name == "__castToUint"_id)
                {
                    f << "uint(";
                    printFunctionCode(function, node->children()[0], f);
                    f << ")";
                }
                else if (name == "__castToBool"_id)
                {
                    f << "bool(";
                    printFunctionCode(function, node->children()[0], f);
                    f << ")";
                }
                return;
            }
            // modulo
            else if (name == "__mod"_id)
            {
                if (node->dataType().isScalar() && (node->dataType().baseType() == compiler::BaseType::Int || node->dataType().baseType() == compiler::BaseType::Uint))
                {
                    f << "(";
                    printFunctionCode(function, node->children()[0], f);
                    f << " % ";
                    printFunctionCode(function, node->children()[1], f);
                    f << ")";
                    return;
                }
                else
                {
                    f << "mod(";
                    printFunctionCode(function, node->children()[0], f);
                    f << ", ";
                    printFunctionCode(function, node->children()[1], f);
                    f << ")";
                }                    
                return;
            }

            // use custom printer
            if (auto customFunc  = m_customFunctionPrinters.find(name))
            {
                (*customFunc)(*this, function, node, f);
                return;
            }

            // use normal function call
            {
                // we may want to change our name
                m_nativeFunctionMapping.find(name, printName);
                f << printName << "(";

                // print arguments
                bool firstArg = true;
                for (auto child  : node->children())
                {
                    if (!firstArg) f << ", ";
                    firstArg = false;

                    printFunctionCode(function, child, f);
                }
                f << ")";
            }
        }

        //--

        void GLSLCodeGenerator::printGenericFunctionArgs(uint32_t firstArgIndex, Function& function, const compiler::CodeNode* node, base::IFormatStream& f)
        {
            // print arguments
            bool firstArg = true;
            for (uint32_t i= firstArgIndex; i < node->children().size(); ++i)
            {
                auto child  = node->children()[i];

                if (!firstArg) f << ", ";
                firstArg = false;

                printFunctionCode(function, child, f);
            }
        }

        void GLSLCodeGenerator::PrintFuncSaturate(GLSL_FUNCTION)
        {
            auto& dataType = node->children()[0]->dataType();
            f << "clamp(";
            gen.printGenericFunctionArgs(0, function, node, f);
            f << ", 0.0, 1.1)";
        }

        void GLSLCodeGenerator::PrintFuncAtan2(GLSL_FUNCTION)
        {
            auto& dataType = node->children()[0]->dataType();
            f << "atan(";
            gen.printFunctionCode(function, node->children()[0], f); // Y
            f << ",";
            gen.printFunctionCode(function, node->children()[1], f); // X
            f << ")";
        }

        void GLSLCodeGenerator::PrintFuncMatrixVectorMul(GLSL_FUNCTION)
        {
            auto& dataType = node->children()[0]->dataType();
            f << "(";
            gen.printFunctionCode(function, node->children()[0], f); // M
            f << "*";
            gen.printFunctionCode(function, node->children()[1], f); // V
            f << ")";
        }

        void GLSLCodeGenerator::PrintFuncVectorMatrimMul(GLSL_FUNCTION)
        {
            auto& dataType = node->children()[0]->dataType();
            f << "(";
            gen.printFunctionCode(function, node->children()[0], f); // V
            f << "*";
            gen.printFunctionCode(function, node->children()[1], f); // M
            f << ")";
        }

        void GLSLCodeGenerator::PrintFuncLessThen(GLSL_FUNCTION)
        {
            auto& dataType = node->children()[0]->dataType();
            if (dataType.isScalar())
            {
                f << "(";
                gen.printFunctionCode(function, node->children()[0], f);
                f << " < ";
                gen.printFunctionCode(function, node->children()[1], f);
                f << ")";
            }
            else
            {
                f << "lessThan(";
                gen.printFunctionCode(function, node->children()[0], f);
                f << ", ";
                gen.printFunctionCode(function, node->children()[1], f);
                f << ")";
            }
        }

        void GLSLCodeGenerator::PrintFuncLessEqual(GLSL_FUNCTION)
        {
            auto& dataType = node->children()[0]->dataType();
            if (dataType.isScalar())
            {
                f << "(";
                gen.printFunctionCode(function, node->children()[0], f);
                f << " <= ";
                gen.printFunctionCode(function, node->children()[1], f);
                f << ")";
            }
            else
            {
                f << "lessThanEqual(";
                gen.printFunctionCode(function, node->children()[0], f);
                f << ", ";
                gen.printFunctionCode(function, node->children()[1], f);
                f << ")";
            }
        }

        void GLSLCodeGenerator::PrintFuncEqual(GLSL_FUNCTION)
        {
            auto& dataType = node->children()[0]->dataType();
            if (dataType.isScalar())
            {
                f << "(";
                gen.printFunctionCode(function, node->children()[0], f);
                f << " == ";
                gen.printFunctionCode(function, node->children()[1], f);
                f << ")";
            }
            else
            {
                f << "equal(";
                gen.printFunctionCode(function, node->children()[0], f);
                f << ", ";
                gen.printFunctionCode(function, node->children()[1], f);
                f << ")";
            }
        }

        void GLSLCodeGenerator::PrintFuncNotEqual(GLSL_FUNCTION)
        {
            auto& dataType = node->children()[0]->dataType();
            if (dataType.isScalar())
            {
                f << "(";
                gen.printFunctionCode(function, node->children()[0], f);
                f << " != ";
                gen.printFunctionCode(function, node->children()[1], f);
                f << ")";
            }
            else
            {
                f << "notEqual(";
                gen.printFunctionCode(function, node->children()[0], f);
                f << ", ";
                gen.printFunctionCode(function, node->children()[1], f);
                f << ")";
            }
        }

        void GLSLCodeGenerator::PrintFuncGreaterEqual(GLSL_FUNCTION)
        {
            auto& dataType = node->children()[0]->dataType();
            if (dataType.isScalar())
            {
                f << "(";
                gen.printFunctionCode(function, node->children()[0], f);
                f << " >= ";
                gen.printFunctionCode(function, node->children()[1], f);
                f << ")";
            }
            else
            {
                f << "greaterThanEqual(";
                gen.printFunctionCode(function, node->children()[0], f);
                f << ", ";
                gen.printFunctionCode(function, node->children()[1], f);
                f << ")";
            }
        }

        void GLSLCodeGenerator::PrintFuncGreaterThen(GLSL_FUNCTION)
        {
            auto& dataType = node->children()[0]->dataType();
            if (dataType.isScalar())
            {
                f << "(";
                gen.printFunctionCode(function, node->children()[0], f);
                f << " > ";
                gen.printFunctionCode(function, node->children()[1], f);
                f << ")";
            }
            else
            {
                f << "greaterThan(";
                gen.printFunctionCode(function, node->children()[0], f);
                f << ", ";
                gen.printFunctionCode(function, node->children()[1], f);
                f << ")";
            }
        }

        //--

        void GLSLCodeGenerator::printAtomicFunctionCore(Function& function, base::StringView<char> baseFunctionStem, const compiler::CodeNode* node, base::IFormatStream& f)
        {
            // printable stem
            base::StringView<char> functionStem = baseFunctionStem;
            if (functionStem == "tomicIncrement")
                functionStem = "tomicAdd";
            else if (functionStem == "tomicDecrement")
                functionStem = "tomicAdd";
            else if (functionStem == "tomicSubtract")
                functionStem = "tomicAdd";

            // allow resources to inject special versions
            bool usedResourceFunction = false;
            const auto* memoryNode = node->children()[0];
            if (memoryNode->opCode() == compiler::OpCode::AccessArray)
            {
                const auto* resourceNode = memoryNode->children()[0];
                if (resourceNode->dataType().isResource())
                {
                    const auto& childType = resourceNode->dataType();
                    const auto* indexNode = memoryNode->children()[1];
                    DEBUG_CHECK(childType.isResource());

                    const auto& res = childType.resource();
                    if (res.buffer)
                    {
                        if (res.resolvedLayout == nullptr)
                        {
                            usedResourceFunction = true;

                            // use load function
                            f.appendf("imageA{}(", functionStem);
                            printFunctionCode(function, resourceNode, f);
                            f << ", int(";
                            printFunctionCode(function, indexNode, f);
                            f << ") ";
                        }                        
                    }
                    else if (res.texture)
                    {
                        DEBUG_CHECK(res.resolvedLayout == nullptr);
                        usedResourceFunction = true;

                        // cast type
                        auto castType = "int";
                        const auto numComps = res.addressComponentCount();
                        if (numComps == 2)
                            castType = "ivec2";
                        else if (numComps == 3)
                            castType = "ivec3";
                        else if (numComps == 4)
                            castType = "ivec4";

                        // use load function
                        f.appendf("imageA{}(  ", functionStem);
                        printFunctionCode(function, resourceNode, f);
                        f.appendf(", {}(", castType);
                        printFunctionCode(function, indexNode, f);
                        f << ") ";
                    }
                }
            }

            // add generic header
            if (!usedResourceFunction)
            {
                f.appendf("a{}(", functionStem);
                printFunctionCode(function, memoryNode, f);
            }

            // add rest of arguments
            if (baseFunctionStem == "tomicIncrement")
            {
                f.appendf(", 1)");
            }
            else if (baseFunctionStem == "tomicDecrement")
            {
                f.appendf(", -1)");
            }
            else if (baseFunctionStem == "tomicSubtract")
            {
                f << ", -(";
                printFunctionCode(function, node->children()[1], f);
                f << "))";
            }
            else
            {
                for (uint32_t i = 1; i < node->children().size(); ++i)
                {
                    f << ", ";
                    printFunctionCode(function, node->children()[i], f);
                }
                f << ")";
            }
        }

        void GLSLCodeGenerator::PrintFuncAtomicIncrement(GLSL_FUNCTION)
        {
            gen.printAtomicFunctionCore(function, "tomicIncrement", node, f);
        }

        void GLSLCodeGenerator::PrintFuncAtomicDecrement(GLSL_FUNCTION)
        {
            gen.printAtomicFunctionCore(function, "tomicDecrement", node, f);
        }

        void GLSLCodeGenerator::PrintFuncAtomicAdd(GLSL_FUNCTION)
        {
            gen.printAtomicFunctionCore(function, "tomicAdd", node, f);
        }

        void GLSLCodeGenerator::PrintFuncAtomicSubtract(GLSL_FUNCTION)
        {
            gen.printAtomicFunctionCore(function, "tomicSubtract", node, f);
        }

        void GLSLCodeGenerator::PrintFuncAtomicMin(GLSL_FUNCTION)
        {
            gen.printAtomicFunctionCore(function, "tomicMin", node, f);
        }

        void GLSLCodeGenerator::PrintFuncAtomicMax(GLSL_FUNCTION)
        {
            gen.printAtomicFunctionCore(function, "tomicMax", node, f);
        }

        void GLSLCodeGenerator::PrintFuncAtomicAnd(GLSL_FUNCTION)
        {
            gen.printAtomicFunctionCore(function, "tomicAnd", node, f);
        }

        void GLSLCodeGenerator::PrintFuncAtomicOr(GLSL_FUNCTION)
        {
            gen.printAtomicFunctionCore(function, "tomicOr", node, f);
        }

        void GLSLCodeGenerator::PrintFuncAtomicXor(GLSL_FUNCTION)
        {
            gen.printAtomicFunctionCore(function, "tomicXor", node, f);
        }

        void GLSLCodeGenerator::PrintFuncAtomicExchange(GLSL_FUNCTION)
        {
            gen.printAtomicFunctionCore(function, "tomicExchange", node, f);
        }

        void GLSLCodeGenerator::PrintFuncAtomicCompareSwap(GLSL_FUNCTION)
        {
            gen.printAtomicFunctionCore(function, "tomicCompSwap", node, f);
        }

        void GLSLCodeGenerator::PrintFuncTexture(GLSL_FUNCTION)
        {
            DEBUG_CHECK(node->children().size() == 2);

            f << "texture(";
            gen.printFunctionCode(function, node->children()[0], f); // texture
            f << ",";
            gen.printFunctionCode(function, node->children()[1], f); // coords
            f << ")";
        }

        void GLSLCodeGenerator::PrintFuncTextureGatherOffset(GLSL_FUNCTION)
        {
            DEBUG_CHECK(node->children().size() == 3);

            f << "textureGatherOffset(  ";
            gen.printFunctionCode(function, node->children()[0], f); // texture
            f << ",";
            gen.printFunctionCode(function, node->children()[1], f); // coords
            f << ",";
            gen.printFunctionCode(function, node->children()[2], f); // offset
            f << ",";
            f << "0"; // red component only (HLSL compatibility)
            f << ")";
        }

        void GLSLCodeGenerator::PrintFuncTextureGather(GLSL_FUNCTION)
        {
            DEBUG_CHECK(node->children().size() == 2);

            f << "textureGather(";
            gen.printFunctionCode(function, node->children()[0], f); // texture
            f << ",";
            gen.printFunctionCode(function, node->children()[1], f); // coords
            f << ",";
            f << "0"; // red component only (HLSL compatibility)
            f << ")";
        }

        void GLSLCodeGenerator::PrintFuncTextureLod(GLSL_FUNCTION)
        {
            f << "textureLod(  ";
            gen.printGenericFunctionArgs(0, function, node, f);
            f << "  )";
        }

        void GLSLCodeGenerator::PrintFuncTextureLodOffset(GLSL_FUNCTION)
        {
            f << "textureLodOffset(  ";
            gen.printGenericFunctionArgs(0, function, node, f);
            f << "  )";
        }

        void GLSLCodeGenerator::PrintFuncTextureBias(GLSL_FUNCTION)
        {
            DEBUG_CHECK(node->children().size() == 3);

            f << "texture(";
            gen.printFunctionCode(function, node->children()[0], f); // texture
            f << ",";
            gen.printFunctionCode(function, node->children()[1], f); // coords
            f << ",";
            gen.printFunctionCode(function, node->children()[2], f); // bias
            f << "  )";
        }

        void GLSLCodeGenerator::PrintFuncTextureBiasOffset(GLSL_FUNCTION)
        {
            DEBUG_CHECK(node->children().size() == 4);

            f << "textureOffset(";
            gen.printFunctionCode(function, node->children()[0], f); // texture
            f << ",";
            gen.printFunctionCode(function, node->children()[1], f); // coords
            f << ",";
            gen.printFunctionCode(function, node->children()[2], f); // offset
            f << ",";
            gen.printFunctionCode(function, node->children()[3], f); // bias
            f << "  )";
        }

        void GLSLCodeGenerator::PrintFuncTextureSizeLod(GLSL_FUNCTION)
        {
            f << "textureSize(  ";
            gen.printGenericFunctionArgs(0, function, node, f);
            f << "  )";
        }

        void GLSLCodeGenerator::PrintFuncTextureLoadLod(GLSL_FUNCTION)
        {
            f << "texelFetch(  ";
            gen.printGenericFunctionArgs(0, function, node, f);
            f << "  )";
        }

        void GLSLCodeGenerator::PrintFuncTextureLoadLodOffset(GLSL_FUNCTION)
        {
            f << "texelFetchOffset(  ";
            gen.printGenericFunctionArgs(0, function, node, f);
            f << "  )";
        }

        void GLSLCodeGenerator::PrintFuncTextureLoadLodSample(GLSL_FUNCTION)
        {
            f << "texelFetch(  ";
            gen.printGenericFunctionArgs(0, function, node, f);
            f << "  )";
        }

        void GLSLCodeGenerator::PrintFuncTextureLoadLodOffsetSample(GLSL_FUNCTION)
        {
            f << "texelFetchOffset(  ";
            gen.printGenericFunctionArgs(0, function, node, f);
            f << "  )";
        }

        void GLSLCodeGenerator::PrintFuncTextureDepthCompare(GLSL_FUNCTION)
        {
            DEBUG_CHECK(node->children().size() == 3);

            f << "textureLod(  ";
            gen.printFunctionCode(function, node->children()[0], f);
            f << ", vec3(";
            gen.printFunctionCode(function, node->children()[1], f);
            f << ",";
            gen.printFunctionCode(function, node->children()[2], f);
            f << "), 0.0)";
        }

        void GLSLCodeGenerator::PrintFuncTextureDepthCompareOffset(GLSL_FUNCTION)
        {
            DEBUG_CHECK(node->children().size() == 4);

            f << "textureLodOffset(  ";
            gen.printFunctionCode(function, node->children()[0], f);
            f << ", vec3(";
            gen.printFunctionCode(function, node->children()[1], f);
            f << ",";
            gen.printFunctionCode(function, node->children()[2], f);
            f << "), 0.0, ";
            gen.printFunctionCode(function, node->children()[3], f);
            f << ")";
        }

        void GLSLCodeGenerator::PrintFuncTexelLoad(GLSL_FUNCTION)
        {
            DEBUG_CHECK(node->children().size() == 2);

            f << "imageLoad(  ";
            gen.printFunctionCode(function, node->children()[0], f);
            f << ", ";
            gen.printFunctionCode(function, node->children()[1], f);
            f << ")";
        }

        void GLSLCodeGenerator::PrintFuncTexelLoadSample(GLSL_FUNCTION)
        {
            DEBUG_CHECK(node->children().size() == 3);

            f << "imageLoad(  ";
            gen.printFunctionCode(function, node->children()[0], f);
            f << ", ";
            gen.printFunctionCode(function, node->children()[1], f);
            f << ", ";
            gen.printFunctionCode(function, node->children()[2], f);
            f << ")";
        }

        void GLSLCodeGenerator::PrintFuncTexelStore(GLSL_FUNCTION)
        {
            DEBUG_CHECK(node->children().size() == 3);

            f << "imageStore(  ";
            gen.printFunctionCode(function, node->children()[0], f);
            f << ", ";
            gen.printFunctionCode(function, node->children()[1], f);
            f << ", ";
            gen.printFunctionCode(function, node->children()[2], f);
            f << ")";
        }

        void GLSLCodeGenerator::PrintFuncTexelStoreSample(GLSL_FUNCTION)
        {
            DEBUG_CHECK(node->children().size() == 4);

            f << "imageStore(  ";
            gen.printFunctionCode(function, node->children()[0], f);
            f << ", ";
            gen.printFunctionCode(function, node->children()[1], f);
            f << ", ";
            gen.printFunctionCode(function, node->children()[2], f);
            f << ", ";
            gen.printFunctionCode(function, node->children()[3], f);
            f << ")";
        }

        void GLSLCodeGenerator::PrintFuncTexelSize(GLSL_FUNCTION)
        {
            DEBUG_CHECK(node->children().size() == 1);

            f << "imageSize(  ";
            gen.printFunctionCode(function, node->children()[0], f);
            f << ")";
        }

        //--

    } // glsl
} // rendering

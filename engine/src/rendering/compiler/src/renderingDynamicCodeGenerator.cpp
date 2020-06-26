/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\dynamic #]
*/

#include "build.h"
#include "renderingDynamicCodeGenerator.h"
#include "base/containers/include/stringBuilder.h"

namespace rendering
{
    namespace compiler
    {

        ///---

        RTTI_BEGIN_TYPE_ENUM(DynamicCodeChunkDataType);
            RTTI_ENUM_OPTION(None);
            RTTI_ENUM_OPTION(Float);
            RTTI_ENUM_OPTION(Float2);
            RTTI_ENUM_OPTION(Float3);
            RTTI_ENUM_OPTION(Float4);
            RTTI_ENUM_OPTION(Bool);
            RTTI_ENUM_OPTION(Texture2D);
            RTTI_ENUM_OPTION(Texture2DArray);
            RTTI_ENUM_OPTION(TextureCube);
            RTTI_ENUM_OPTION(TextureCubeArray);
        RTTI_END_TYPE()

        ///---

        RTTI_BEGIN_TYPE_ENUM(DynamicCodeNodeOp);
            RTTI_ENUM_OPTION(Invalid);
            RTTI_ENUM_OPTION(OpExternalParam);
            RTTI_ENUM_OPTION(OpInputFunctionCall);
            RTTI_ENUM_OPTION(OpGenericFunctionCall);
            RTTI_ENUM_OPTION(OpSampleTexture);
            RTTI_ENUM_OPTION(OpConstant);
            RTTI_ENUM_OPTION(OpMakeFloat);
            RTTI_ENUM_OPTION(OpAdd);
            RTTI_ENUM_OPTION(OpMul);
            RTTI_ENUM_OPTION(OpSub);
            RTTI_ENUM_OPTION(OpMin);
            RTTI_ENUM_OPTION(OpMax);
            RTTI_ENUM_OPTION(OpDot);
            RTTI_ENUM_OPTION(OpSwizzle);
            RTTI_ENUM_OPTION(OpSaturate);
            RTTI_ENUM_OPTION(OpClamp);
            RTTI_ENUM_OPTION(OpNeg);
            RTTI_ENUM_OPTION(OpSwizzle);
            RTTI_ENUM_OPTION(OpFrac);
            RTTI_ENUM_OPTION(OpCeil);
            RTTI_ENUM_OPTION(OpFloor);
            RTTI_ENUM_OPTION(OpRound);
            RTTI_ENUM_OPTION(OpFmod);
            RTTI_ENUM_OPTION(OpSin);
            RTTI_ENUM_OPTION(OpCos);
            RTTI_ENUM_OPTION(OpTan);
            RTTI_ENUM_OPTION(OpAtan);
            RTTI_ENUM_OPTION(OpPow);
            RTTI_ENUM_OPTION(OpExp);
            RTTI_ENUM_OPTION(OpLn);
            RTTI_ENUM_OPTION(OpInv);
            RTTI_ENUM_OPTION(OpOneMinus);
            RTTI_ENUM_OPTION(OpLerp);
            RTTI_ENUM_OPTION(OpCmpEq);
            RTTI_ENUM_OPTION(OpCmpNeq);
            RTTI_ENUM_OPTION(OpCmpLt);
            RTTI_ENUM_OPTION(OpCmpLe);
            RTTI_ENUM_OPTION(OpCmpGt);
            RTTI_ENUM_OPTION(OpCmpGe);
            RTTI_ENUM_OPTION(OpSelect);
            RTTI_ENUM_OPTION(OpLogicAnd);
            RTTI_ENUM_OPTION(OpLogicOr);
            RTTI_ENUM_OPTION(OpLogicXor);
            RTTI_ENUM_OPTION(OpLogicNot);
            RTTI_ENUM_OPTION(OpLogicTrue);
            RTTI_ENUM_OPTION(OpLogicFalse);
        };

        ///---

        DynamicCompiler::DynamicCompiler()
            : m_mem(POOL_TEMP)
            , m_hasError(false)
            , m_varIdAllocator(1)
        {
            // create root block
            m_block = m_mem.create<DynamicCodeBlock>();

            // set node0 to be null
            m_nodes.pushBack(nullptr);
        }

        DynamicCompiler::~DynamicCompiler()
        {
        }

        DynamicCodeChunk DynamicCompiler::printError(DynamicCodeChunk id, const char* txt) const
        {
            TRACE_ERROR("error: {}", txt);
            m_hasError = true;
            return 0;
        }

        DynamicCodeChunkDataType DynamicCompiler::type(DynamicCodeChunk id) const
        {
            auto node  = this->node(id);
            return node ? node->m_type : DynamicCodeChunkDataType::None;
        }

        DynamicCodeNodeOp DynamicCompiler::op(DynamicCodeChunk id) const
        {
            auto node  = this->node(id);
            return node ? node->m_op : DynamicCodeNodeOp::Invalid;
        }

        bool DynamicCompiler::validateType(DynamicCodeChunk id, DynamicCodeChunkDataType type) const
        {
            if (!id)
            {
                printError(id, base::TempString("Found unexpected NULL node, expected type {}", base::reflection::GetEnumValueName(type)));
                return false;
            }

            auto currentType = this->type(id);
            if (currentType != type)
            {
                printError(id, base::TempString("Unexpected type {} found at {}, expected {}", base::reflection::GetEnumValueName(currentType), base::reflection::GetEnumValueName(op(id)), base::reflection::GetEnumValueName(type)));
                return false;
            }

            return true;
        }

        DynamicCodeChunk DynamicCompiler::requestParam(const char* name, DynamicCodeChunkDataType dataType)
        {
            for (auto& param : m_params)
            {
                if (param.m_name == name)
                {
                    if (param.m_type != dataType)
                        printError(0, base::TempString("Parameter '{}' previously requested as type {} but now as type {}", name, base::reflection::GetEnumValueName(param.m_type), base::reflection::GetEnumValueName(dataType)));
                    return param.m_node->m_id;
                }
            }

            auto exportNode  = allocNode(DynamicCodeNodeOp::OpExternalParam, dataType);
            exportNode->m_value.m_name = name;

            auto& newParam = m_params.emplaceBack();
            newParam.m_type = dataType;
            newParam.m_name = name;
            newParam.m_node = exportNode;

            return exportNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::runInputFunction(const char* name, DynamicCodeChunkDataType retType)
        {
            for (auto& param : m_inputs)
            {
                if (param.m_paramName == name)
                {
                    if (param.m_type != retType)
                        printError(0, base::TempString("Input '{}' previously requested as type {} but now as type {}",
                            name, base::reflection::GetEnumValueName(param.m_type), base::reflection::GetEnumValueName(retType)));
                    return param.m_node->m_id;
                }
            }

            auto exportNode  = allocNode(DynamicCodeNodeOp::OpInputFunctionCall, retType);
            exportNode->m_value.m_name = base::TempString("Input{}", name);

            auto& inputInfo = m_inputs.emplaceBack();
            inputInfo.m_type = retType;
            inputInfo.m_paramName = exportNode->m_value.m_name;
            inputInfo.m_funcName = name;
            inputInfo.m_node = exportNode;

            return exportNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::runFunction(const char* name, DynamicCodeChunkDataType retType, std::initializer_list<DynamicCodeChunk> params)
        {
            auto exportNode  = allocNode(DynamicCodeNodeOp::OpGenericFunctionCall, retType);
            exportNode->m_value.m_name = name;

            for (auto& params : params)
                exportNode->m_children.pushBack(node(params));

            return exportNode->m_id;
        }

        void DynamicCompiler::emitOutput(const char* name, DynamicCodeChunk id)
        {
            auto node  = this->node(id);
            if (node)
            {
                for (auto& param : m_outputs)
                {
                    if (param.m_name == name)
                    {
                        param.m_node = node;
                        return;
                    }
                }

                auto& info = m_outputs.emplaceBack();
                info.m_name = name;
                info.m_node = node;
            }
        }

        DynamicCodeChunk DynamicCompiler::emitLocal(DynamicCodeChunk a)
        {
            auto node  = this->node(a);
            if (!node)
                return printError(a, "Unable to create a local variable based on invalid node");

            auto type = this->type(a);
            if (type != DynamicCodeChunkDataType::Float && type != DynamicCodeChunkDataType::Float2
            && type != DynamicCodeChunkDataType::Float3 && type != DynamicCodeChunkDataType::Float4
            && type != DynamicCodeChunkDataType::Bool)
                return printError(a, base::TempString("Unable to allocate variable of non numerical type {}", base::reflection::GetEnumValueName(type)));

            auto& var = m_vars.emplaceBack();
            var.m_type = type;
            var.m_name = base::TempString("v{}", m_varIdAllocator++);

            auto newNode  = allocNode(DynamicCodeNodeOp::OpLocalVarAssignment, type);
            newNode->m_value.m_name = var.m_name;
            newNode->m_children.pushBack(node);

            m_block->m_code.pushBack(newNode);

            return newNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::sampleTexture(DynamicCodeChunk tex, DynamicCodeChunk uv)
        {
            auto texType = this->type(tex);
            if (texType == DynamicCodeChunkDataType::Texture2D)
            {
                if (!validateType(uv, DynamicCodeChunkDataType::Float2))
                    return 0;

                auto node = allocNode(DynamicCodeNodeOp::OpSampleTexture, DynamicCodeChunkDataType::Float4);
                node->m_children.pushBack(this->node(tex));
                node->m_children.pushBack(this->node(uv));
                return node->m_id;
            }

            // unknown texture format
            return printError(tex, base::TempString("Unknown texture format {}", base::reflection::GetEnumValueName(type(tex))));
        }

        DynamicCodeChunk DynamicCompiler::constant(float val)
        {
            auto node = allocNode(DynamicCodeNodeOp::OpConstant, DynamicCodeChunkDataType::Float);
            node->m_value.m_const.x = val;
            return node->m_id;
        }

        DynamicCodeChunk DynamicCompiler::constant(const base::Vector2& val)
        {
            auto node = allocNode(DynamicCodeNodeOp::OpConstant, DynamicCodeChunkDataType::Float2);
            node->m_value.m_const.x = val.x;
            node->m_value.m_const.y = val.y;
            return node->m_id;
        }

        DynamicCodeChunk DynamicCompiler::constant(const base::Vector3& val)
        {
            auto node = allocNode(DynamicCodeNodeOp::OpConstant, DynamicCodeChunkDataType::Float3);
            node->m_value.m_const.x = val.x;
            node->m_value.m_const.y = val.y;
            node->m_value.m_const.z = val.z;
            return node->m_id;
        }

        DynamicCodeChunk DynamicCompiler::constant(const base::Vector4& val)
        {
            auto node = allocNode(DynamicCodeNodeOp::OpConstant, DynamicCodeChunkDataType::Float4);
            node->m_value.m_const.x = val.x;
            node->m_value.m_const.y = val.y;
            node->m_value.m_const.z = val.z;
            node->m_value.m_const.w = val.w;
            return node->m_id;
        }

        DynamicCodeChunk DynamicCompiler::float2(DynamicCodeChunk x, DynamicCodeChunk y)
        {
            if (!validateType(x, DynamicCodeChunkDataType::Float) || !validateType(y, DynamicCodeChunkDataType::Float))
                return 0;

            auto node = allocNode(DynamicCodeNodeOp::OpMakeFloat, DynamicCodeChunkDataType::Float2);
            node->m_children.pushBack(this->node(x));
            node->m_children.pushBack(this->node(y));
            return node->m_id;
        }

        DynamicCodeChunk DynamicCompiler::float3(DynamicCodeChunk x, DynamicCodeChunk y, DynamicCodeChunk z)
        {
            if (!validateType(x, DynamicCodeChunkDataType::Float) || !validateType(y, DynamicCodeChunkDataType::Float) || !validateType(z, DynamicCodeChunkDataType::Float))
                return 0;

            auto node = allocNode(DynamicCodeNodeOp::OpMakeFloat, DynamicCodeChunkDataType::Float3);
            node->m_children.pushBack(this->node(x));
            node->m_children.pushBack(this->node(y));
            node->m_children.pushBack(this->node(z));
            return node->m_id;
        }

        DynamicCodeChunk DynamicCompiler::float4(DynamicCodeChunk x, DynamicCodeChunk y, DynamicCodeChunk z, DynamicCodeChunk w)
        {
            if (!validateType(x, DynamicCodeChunkDataType::Float) || !validateType(y, DynamicCodeChunkDataType::Float)
            || !validateType(z, DynamicCodeChunkDataType::Float) || !validateType(w, DynamicCodeChunkDataType::Float))
                return 0;

            auto node = allocNode(DynamicCodeNodeOp::OpMakeFloat, DynamicCodeChunkDataType::Float4);
            node->m_children.pushBack(this->node(x));
            node->m_children.pushBack(this->node(y));
            node->m_children.pushBack(this->node(z));
            node->m_children.pushBack(this->node(w));
            return node->m_id;
        }

        DynamicCodeChunk DynamicCompiler::swizzle(DynamicCodeChunk val, const char* swz)
        {
            ASSERT(swz && *swz);

            uint32_t numComponents = 0;
            DynamicCodeSwizzle components[4];

            for (uint32_t i=0; i<4; ++i)
            {
                switch (swz[i])
                {
                    case '0': components[numComponents++] = DynamicCodeSwizzle::Zero; break;
                    case '1': components[numComponents++] = DynamicCodeSwizzle::One; break;
                    case 'x': components[numComponents++] = DynamicCodeSwizzle::X; break;
                    case 'y': components[numComponents++] = DynamicCodeSwizzle::Y; break;
                    case 'z': components[numComponents++] = DynamicCodeSwizzle::Z; break;
                    case 'w': components[numComponents++] = DynamicCodeSwizzle::W; break;

                    default:
                    {
                        printError(val, base::TempString("Invalid swizzle string '{}'", swz));
                        return 0;
                    }
                }
            }

            return swizzle(val, components, numComponents);
        }

        DynamicCodeChunk DynamicCompiler::swizzle(DynamicCodeChunk val, const DynamicCodeSwizzle* swz, uint32_t numComponents)
        {
            ASSERT(numComponents >= 1 && numComponents <= 4);

            uint32_t numRequiredComponents = 0;
            for (uint32_t i=0; i<numComponents; ++i)
            {
                switch (swz[i])
                {
                    case DynamicCodeSwizzle::X:
                        numRequiredComponents = std::max<uint32_t>(numRequiredComponents, 1);
                        break;
                    case DynamicCodeSwizzle::Y:
                        numRequiredComponents = std::max<uint32_t>(numRequiredComponents, 2);
                        break;
                    case DynamicCodeSwizzle::Z:
                        numRequiredComponents = std::max<uint32_t>(numRequiredComponents, 3);
                        break;
                    case DynamicCodeSwizzle::W:
                        numRequiredComponents = std::max<uint32_t>(numRequiredComponents, 4);
                        break;
                }
            }

            if (numRequiredComponents == 1)
            {
                if (!validateType(val, DynamicCodeChunkDataType::Float) && !validateType(val, DynamicCodeChunkDataType::Float2)
                && !validateType(val, DynamicCodeChunkDataType::Float3) && !validateType(val, DynamicCodeChunkDataType::Float4))
                    return 0;
            }
            else if (numRequiredComponents == 2)
            {
                if (!validateType(val, DynamicCodeChunkDataType::Float2) && !validateType(val, DynamicCodeChunkDataType::Float3)
                && !validateType(val, DynamicCodeChunkDataType::Float4))
                    return 0;
            }
            else if (numRequiredComponents == 3)
            {
                if (!validateType(val, DynamicCodeChunkDataType::Float3) && !validateType(val, DynamicCodeChunkDataType::Float4))
                    return 0;
            }
            else if (numRequiredComponents == 4)
            {
                if (!validateType(val, DynamicCodeChunkDataType::Float4))
                    return 0;
            }

            auto outputType = DynamicCodeChunkDataType::Float4;
            if (numComponents == 1)
                outputType = DynamicCodeChunkDataType::Float;
            else if (numComponents == 2)
                outputType = DynamicCodeChunkDataType::Float2;
            else if (numComponents == 3)
                outputType = DynamicCodeChunkDataType::Float3;

            auto node = allocNode(DynamicCodeNodeOp::OpSwizzle, outputType);
            node->m_children.pushBack(this->node(val));

            for (uint32_t i=0; i<numComponents; ++i)
                node->m_swizzle.pushBack(swz[i]);

            return node->m_id;
        }

        bool DynamicCompiler::changeTypeInplace(DynamicCodeChunk& node, DynamicCodeChunkDataType type)
        {
            auto a = convertType(node, type);
            if (!a)
                return false;

            node = a;
            return true;
        }

        DynamicCodeChunk DynamicCompiler::convertType(DynamicCodeChunk node, DynamicCodeChunkDataType type)
        {
            auto currentType = this->type(node);
            if (currentType == type)
                return node;

            if (currentType == DynamicCodeChunkDataType::Float)
            {
                if (type == DynamicCodeChunkDataType::Float)
                    return swizzle(node, "x");
                else if (type == DynamicCodeChunkDataType::Float2)
                    return swizzle(node, "xx");
                else if (type == DynamicCodeChunkDataType::Float3)
                    return swizzle(node, "xxx");
                else if (type == DynamicCodeChunkDataType::Float4)
                    return swizzle(node, "xxxx");
            }
            else if (currentType == DynamicCodeChunkDataType::Float2)
            {
                if (type == DynamicCodeChunkDataType::Float)
                    return swizzle(node, "x");
                else if (type == DynamicCodeChunkDataType::Float2)
                    return swizzle(node, "xy");
                else if (type == DynamicCodeChunkDataType::Float3)
                    return swizzle(node, "xyy");
                else if (type == DynamicCodeChunkDataType::Float4)
                    return swizzle(node, "xyyy");
            }
            else if (currentType == DynamicCodeChunkDataType::Float3)
            {
                if (type == DynamicCodeChunkDataType::Float)
                    return swizzle(node, "x");
                else if (type == DynamicCodeChunkDataType::Float2)
                    return swizzle(node, "xy");
                else if (type == DynamicCodeChunkDataType::Float3)
                    return swizzle(node, "xyz");
                else if (type == DynamicCodeChunkDataType::Float4)
                    return swizzle(node, "xyzz");
            }
            else if (currentType == DynamicCodeChunkDataType::Float4)
            {
                if (type == DynamicCodeChunkDataType::Float)
                    return swizzle(node, "x");
                else if (type == DynamicCodeChunkDataType::Float2)
                    return swizzle(node, "xy");
                else if (type == DynamicCodeChunkDataType::Float3)
                    return swizzle(node, "xyz");
                else if (type == DynamicCodeChunkDataType::Float4)
                    return swizzle(node, "xyzw");
            }

            return printError(node, base::TempString("Invalid type conversion {}->{}", base::reflection::GetEnumValueName(currentType), base::reflection::GetEnumValueName(type)));
        }

        static int GetTypeComponentCount(DynamicCodeChunkDataType type)
        {
            switch (type)
            {
                case DynamicCodeChunkDataType::Float:
                    return 1;
                case DynamicCodeChunkDataType::Float2:
                    return 2;
                case DynamicCodeChunkDataType::Float3:
                    return 3;
                case DynamicCodeChunkDataType::Float4:
                    return 4;
            }

            return 0;
        }

        static DynamicCodeChunkDataType GetTypeForComponentCount(int index)
        {
            switch (index)
            {
                case 1: return DynamicCodeChunkDataType::Float;
                case 2: return DynamicCodeChunkDataType::Float2;
                case 3: return DynamicCodeChunkDataType::Float3;
                case 4: return DynamicCodeChunkDataType::Float4;
            }

            return DynamicCodeChunkDataType::None;
        }

        static int Reconsile(int a, int b)
        {
            if (!a || !b)
                return 0;

            return std::max<int>(a, b);
        }

        static int Reconsile(int a, int b, int c)
        {
            if (!a || !b || !c)
                return 0;

            return std::max<int>(a, std::max<int>(b, c));
        }

        bool DynamicCompiler::matchNumericalType(DynamicCodeChunk& a, DynamicCodeChunk& b)
        {
            auto typeA = type(a);
            auto typeB = type(b);

            auto bestCount = Reconsile(GetTypeComponentCount(typeA), GetTypeComponentCount(typeB));
			if (bestCount == 0)
			{
				printError(a, base::TempString("Unable to match operand to types {}, {}", base::reflection::GetEnumValueName(typeA), base::reflection::GetEnumValueName(typeB)));
				return false;
			}

            auto bestType = GetTypeForComponentCount(bestCount);
            return changeTypeInplace(a, bestType) && changeTypeInplace(b, bestType);
        }

        bool DynamicCompiler::matchNumericalType(DynamicCodeChunk& a, DynamicCodeChunk& b, DynamicCodeChunk& c)
        {
            auto typeA = type(a);
            auto typeB = type(b);
            auto typeC = type(c);

            auto bestCount = Reconsile(GetTypeComponentCount(typeA), GetTypeComponentCount(typeB), GetTypeComponentCount(typeC));
			if (bestCount == 0)
			{
				printError(a, base::TempString("Unable to match operand to types {}, {}, {}", base::reflection::GetEnumValueName(typeA), base::reflection::GetEnumValueName(typeB), base::reflection::GetEnumValueName(typeC)));
				return false;
			}

            auto bestType = GetTypeForComponentCount(bestCount);
            return changeTypeInplace(a, bestType) && changeTypeInplace(b, bestType) && changeTypeInplace(c, bestType);
        }

        DynamicCodeChunkDataType DynamicCompiler::returnNumericalType(DynamicCodeChunk a, DynamicCodeChunk b)
        {
            auto typeA = type(a);
            auto typeB = type(b);

            auto bestCount = Reconsile(GetTypeComponentCount(typeA), GetTypeComponentCount(typeB));
            if (bestCount == 0)
            {
                printError(a, base::TempString("Unable to match operand to types {}, {}", base::reflection::GetEnumValueName(typeA), base::reflection::GetEnumValueName(typeB)));
                return DynamicCodeChunkDataType::None;
            }

            return GetTypeForComponentCount(bestCount);
        }

        DynamicCodeChunkDataType DynamicCompiler::returnNumericalType(DynamicCodeChunk a, DynamicCodeChunk b, DynamicCodeChunk c)
        {
            auto typeA = type(a);
            auto typeB = type(b);
            auto typeC = type(c);

            auto bestCount = Reconsile(GetTypeComponentCount(typeA), GetTypeComponentCount(typeB), GetTypeComponentCount(typeC));
            if (bestCount == 0)
            {
                printError(a, base::TempString("Unable to match operand to types {}, {}, {}", base::reflection::GetEnumValueName(typeA), base::reflection::GetEnumValueName(typeB), base::reflection::GetEnumValueName(typeC)));
                return DynamicCodeChunkDataType::None;
            }

            return GetTypeForComponentCount(bestCount);
        }

        DynamicCodeChunk DynamicCompiler::add(DynamicCodeChunk a, DynamicCodeChunk b)
        {
            auto retType = returnNumericalType(a,b);
            if (retType == DynamicCodeChunkDataType::None)
                return 0;

            auto nodeNode  = allocNode(DynamicCodeNodeOp::OpAdd, retType);
            nodeNode->m_children.pushBack(node(a));
            nodeNode->m_children.pushBack(node(b));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::mul(DynamicCodeChunk a, DynamicCodeChunk b)
        {
            auto retType = returnNumericalType(a,b);
            if (retType == DynamicCodeChunkDataType::None)
                return 0;

            auto nodeNode  = allocNode(DynamicCodeNodeOp::OpMul, retType);
            nodeNode->m_children.pushBack(node(a));
            nodeNode->m_children.pushBack(node(b));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::sub(DynamicCodeChunk a, DynamicCodeChunk b)
        {
            auto retType = returnNumericalType(a,b);
            if (retType == DynamicCodeChunkDataType::None)
                return 0;

            auto nodeNode  = allocNode(DynamicCodeNodeOp::OpSub, retType);
            nodeNode->m_children.pushBack(node(a));
            nodeNode->m_children.pushBack(node(b));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::min(DynamicCodeChunk a, DynamicCodeChunk b)
        {
            auto retType = returnNumericalType(a, b);
            if (retType == DynamicCodeChunkDataType::None)
                return 0;

            auto nodeNode = allocNode(DynamicCodeNodeOp::OpMin, retType);
            nodeNode->m_children.pushBack(node(a));
            nodeNode->m_children.pushBack(node(b));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::max(DynamicCodeChunk a, DynamicCodeChunk b)
        {
            auto retType = returnNumericalType(a, b);
            if (retType == DynamicCodeChunkDataType::None)
                return 0;

            auto nodeNode = allocNode(DynamicCodeNodeOp::OpMax, retType);
            nodeNode->m_children.pushBack(node(a));
            nodeNode->m_children.pushBack(node(b));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::dot4(DynamicCodeChunk a, DynamicCodeChunk b)
        {
            if (!changeTypeInplace(a, DynamicCodeChunkDataType::Float4) || !changeTypeInplace(b, DynamicCodeChunkDataType::Float4))
                return 0;

            auto nodeNode = allocNode(DynamicCodeNodeOp::OpDot, DynamicCodeChunkDataType::Float);
            nodeNode->m_children.pushBack(node(a));
            nodeNode->m_children.pushBack(node(b));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::dot3(DynamicCodeChunk a, DynamicCodeChunk b)
        {
            if (!changeTypeInplace(a, DynamicCodeChunkDataType::Float3) || !changeTypeInplace(b, DynamicCodeChunkDataType::Float3))
                return 0;

            auto nodeNode = allocNode(DynamicCodeNodeOp::OpDot, DynamicCodeChunkDataType::Float);
            nodeNode->m_children.pushBack(node(a));
            nodeNode->m_children.pushBack(node(b));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::dot2(DynamicCodeChunk a, DynamicCodeChunk b)
        {
            if (!changeTypeInplace(a, DynamicCodeChunkDataType::Float2) || !changeTypeInplace(b, DynamicCodeChunkDataType::Float2))
                return 0;

            auto nodeNode = allocNode(DynamicCodeNodeOp::OpDot, DynamicCodeChunkDataType::Float);
            nodeNode->m_children.pushBack(node(a));
            nodeNode->m_children.pushBack(node(b));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::saturate(DynamicCodeChunk a)
        {
            auto nodeNode = allocNode(DynamicCodeNodeOp::OpSaturate, type(a));
            nodeNode->m_children.pushBack(node(a));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::clamp(DynamicCodeChunk a, DynamicCodeChunk vmin, DynamicCodeChunk vmax)
        {
            auto retType = returnNumericalType(a, vmin, vmax);
            if (retType == DynamicCodeChunkDataType::None)
                return 0;


            if (!changeTypeInplace(a, DynamicCodeChunkDataType::Float4)
                || !changeTypeInplace(vmin, DynamicCodeChunkDataType::Float4)
                || !changeTypeInplace(vmax, DynamicCodeChunkDataType::Float4))
                return 0;

            auto nodeNode = allocNode(DynamicCodeNodeOp::OpClamp, type(a));
            nodeNode->m_children.pushBack(node(a));
            nodeNode->m_children.pushBack(node(vmin));
            nodeNode->m_children.pushBack(node(vmax));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::neg(DynamicCodeChunk a)
        {
            auto nodeNode = allocNode(DynamicCodeNodeOp::OpNeg, type(a));
            nodeNode->m_children.pushBack(node(a));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::frac(DynamicCodeChunk a)
        {
            auto nodeNode = allocNode(DynamicCodeNodeOp::OpFrac, type(a));
            nodeNode->m_children.pushBack(node(a));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::floor(DynamicCodeChunk a)
        {
            auto nodeNode = allocNode(DynamicCodeNodeOp::OpFloor, type(a));
            nodeNode->m_children.pushBack(node(a));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::ceil(DynamicCodeChunk a)
        {
            auto nodeNode = allocNode(DynamicCodeNodeOp::OpFmod, type(a));
            nodeNode->m_children.pushBack(node(a));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::fmod(DynamicCodeChunk a, DynamicCodeChunk b)
        {
            auto nodeNode = allocNode(DynamicCodeNodeOp::OpFmod, type(a));
            nodeNode->m_children.pushBack(node(a));
            nodeNode->m_children.pushBack(node(b));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::sin(DynamicCodeChunk a)
        {
            auto nodeNode = allocNode(DynamicCodeNodeOp::OpSin, type(a));
            nodeNode->m_children.pushBack(node(a));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::cos(DynamicCodeChunk a)
        {
            auto nodeNode = allocNode(DynamicCodeNodeOp::OpCos, type(a));
            nodeNode->m_children.pushBack(node(a));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::tan(DynamicCodeChunk a)
        {
            auto nodeNode = allocNode(DynamicCodeNodeOp::OpTan, type(a));
            nodeNode->m_children.pushBack(node(a));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::atan(DynamicCodeChunk a, DynamicCodeChunk b)
        {
            auto nodeNode = allocNode(DynamicCodeNodeOp::OpAtan, type(a));
            nodeNode->m_children.pushBack(node(a));
            nodeNode->m_children.pushBack(node(b));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::pow(DynamicCodeChunk a, DynamicCodeChunk b)
        {
            auto nodeNode = allocNode(DynamicCodeNodeOp::OpPow, type(a));
            nodeNode->m_children.pushBack(node(a));
            nodeNode->m_children.pushBack(node(b));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::exp(DynamicCodeChunk a)
        {
            auto nodeNode = allocNode(DynamicCodeNodeOp::OpExp, type(a));
            nodeNode->m_children.pushBack(node(a));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::ln(DynamicCodeChunk a)
        {
            auto nodeNode = allocNode(DynamicCodeNodeOp::OpLn, type(a));
            nodeNode->m_children.pushBack(node(a));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::inv(DynamicCodeChunk a)
        {
            auto nodeNode = allocNode(DynamicCodeNodeOp::OpInv, type(a));
            nodeNode->m_children.pushBack(node(a));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::oneMinus(DynamicCodeChunk a)
        {
            auto nodeNode = allocNode(DynamicCodeNodeOp::OpOneMinus, type(a));
            nodeNode->m_children.pushBack(node(a));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::lerp(DynamicCodeChunk a, DynamicCodeChunk b, DynamicCodeChunk s)
        {
            auto nodeNode = allocNode(DynamicCodeNodeOp::OpLerp, type(a));
            nodeNode->m_children.pushBack(node(a));
            nodeNode->m_children.pushBack(node(b));
            nodeNode->m_children.pushBack(node(s));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::cmpEq(DynamicCodeChunk a, DynamicCodeChunk b)
        {
            auto nodeNode = allocNode(DynamicCodeNodeOp::OpCmpEq, DynamicCodeChunkDataType::Bool);
            nodeNode->m_children.pushBack(node(a));
            nodeNode->m_children.pushBack(node(b));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::cmpNeq(DynamicCodeChunk a, DynamicCodeChunk b)
        {
            auto nodeNode = allocNode(DynamicCodeNodeOp::OpCmpNeq, DynamicCodeChunkDataType::Bool);
            nodeNode->m_children.pushBack(node(a));
            nodeNode->m_children.pushBack(node(b));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::cmpLt(DynamicCodeChunk a, DynamicCodeChunk b)
        {
            auto nodeNode = allocNode(DynamicCodeNodeOp::OpCmpLt, DynamicCodeChunkDataType::Bool);
            nodeNode->m_children.pushBack(node(a));
            nodeNode->m_children.pushBack(node(b));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::cmpLe(DynamicCodeChunk a, DynamicCodeChunk b)
        {
            auto nodeNode = allocNode(DynamicCodeNodeOp::OpCmpLe, DynamicCodeChunkDataType::Bool);
            nodeNode->m_children.pushBack(node(a));
            nodeNode->m_children.pushBack(node(b));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::cmpGt(DynamicCodeChunk a, DynamicCodeChunk b)
        {
            auto nodeNode = allocNode(DynamicCodeNodeOp::OpCmpGt, DynamicCodeChunkDataType::Bool);
            nodeNode->m_children.pushBack(node(a));
            nodeNode->m_children.pushBack(node(b));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::cmpGe(DynamicCodeChunk a, DynamicCodeChunk b)
        {
            auto nodeNode = allocNode(DynamicCodeNodeOp::OpCmpGe, DynamicCodeChunkDataType::Bool);
            nodeNode->m_children.pushBack(node(a));
            nodeNode->m_children.pushBack(node(b));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::select(DynamicCodeChunk a, DynamicCodeChunk b, DynamicCodeChunk mask)
        {
            auto retType = returnNumericalType(a, b);
            if (retType == DynamicCodeChunkDataType::None)
                return 0;

            auto nodeNode = allocNode(DynamicCodeNodeOp::OpSelect, retType);
            nodeNode->m_children.pushBack(node(a));
            nodeNode->m_children.pushBack(node(b));
            nodeNode->m_children.pushBack(node(mask));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::logicAnd(DynamicCodeChunk a, DynamicCodeChunk b)
        {
            auto nodeNode = allocNode(DynamicCodeNodeOp::OpLogicAnd, DynamicCodeChunkDataType::Bool);
            nodeNode->m_children.pushBack(node(a));
            nodeNode->m_children.pushBack(node(b));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::logicOr(DynamicCodeChunk a, DynamicCodeChunk b)
        {
            auto nodeNode = allocNode(DynamicCodeNodeOp::OpLogicOr, DynamicCodeChunkDataType::Bool);
            nodeNode->m_children.pushBack(node(a));
            nodeNode->m_children.pushBack(node(b));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::logicXor(DynamicCodeChunk a, DynamicCodeChunk b)
        {
            auto nodeNode = allocNode(DynamicCodeNodeOp::OpLogicXor, DynamicCodeChunkDataType::Bool);
            nodeNode->m_children.pushBack(node(a));
            nodeNode->m_children.pushBack(node(b));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::logicNot(DynamicCodeChunk a)
        {
            auto nodeNode = allocNode(DynamicCodeNodeOp::OpLogicNot, DynamicCodeChunkDataType::Bool);
            nodeNode->m_children.pushBack(node(a));
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::logicTrue()
        {
            auto nodeNode = allocNode(DynamicCodeNodeOp::OpLogicTrue, DynamicCodeChunkDataType::Bool);
            return nodeNode->m_id;
        }

        DynamicCodeChunk DynamicCompiler::logicFalse()
        {
            auto nodeNode = allocNode(DynamicCodeNodeOp::OpLogicFalse, DynamicCodeChunkDataType::Bool);
            return nodeNode->m_id;
        }

        ///---

        static bool IsConstantParam(DynamicCodeChunkDataType type)
        {
            switch (type)
            {
                case DynamicCodeChunkDataType::Float:
                case DynamicCodeChunkDataType::Float2:
                case DynamicCodeChunkDataType::Float3:
                case DynamicCodeChunkDataType::Float4:
                case DynamicCodeChunkDataType::Bool:
                    return true;
            }

            return false;
        }

        static void WriteUniformType(DynamicCodeChunkDataType type, base::StringBuilder& ret)
        {
            switch (type)
            {
                case DynamicCodeChunkDataType::Float: ret.append("float"); break;
                case DynamicCodeChunkDataType::Float2: ret.append("float2"); break;
                case DynamicCodeChunkDataType::Float3: ret.append("float3"); break;
                case DynamicCodeChunkDataType::Float4: ret.append("float4"); break;
                case DynamicCodeChunkDataType::Bool: ret.append("bool"); break;
                case DynamicCodeChunkDataType::Texture2D: ret.append("Texture2D"); break;
                case DynamicCodeChunkDataType::Texture2DArray: ret.append("Texture2DArray"); break;
                case DynamicCodeChunkDataType::TextureCube: ret.append("TextureCube"); break;
                case DynamicCodeChunkDataType::TextureCubeArray: ret.append("TextureCubeArray"); break;
            }
        }

        static void WriteDescriptorType(DynamicCodeChunkDataType type, base::StringBuilder& ret)
        {
            switch (type)
            {
                case DynamicCodeChunkDataType::Float: ret.append("float"); break;
                case DynamicCodeChunkDataType::Float2: ret.append("float2"); break;
                case DynamicCodeChunkDataType::Float3: ret.append("float3"); break;
                case DynamicCodeChunkDataType::Float4: ret.append("float4"); break;
                case DynamicCodeChunkDataType::Bool: ret.append("bool"); break;
                case DynamicCodeChunkDataType::Texture2D: ret.append("Texture2D"); break;
                case DynamicCodeChunkDataType::Texture2DArray: ret.append("Texture2DArray"); break;
                case DynamicCodeChunkDataType::TextureCube: ret.append("TextureCube"); break;
                case DynamicCodeChunkDataType::TextureCubeArray: ret.append("TextureCubeArray"); break;
            }
        }

        void DynamicCompiler::GenerateCodeForchildren(base::StringBuilder& ret, const DynamicCodeNode* node)
        {
            ret.append("(");

            for (uint32_t i=0; i<node->m_children.size(); ++i)
            {
                if (i > 0)
                    ret.append(", ");
                GenerateCodeNode(ret, node->m_children[i]);
            }

            ret.append(")");
        }

        void DynamicCompiler::GenerateCodeNode(base::StringBuilder& ret, const DynamicCodeNode* node)
        {
            switch (node->m_op)
            {
                case DynamicCodeNodeOp::OpExternalParam:
                {
                    ret.append(node->m_value.m_name.c_str());
                    break;
                }

                case DynamicCodeNodeOp::OpInputFunctionCall:
                {
                    ret.append(node->m_value.m_name.c_str());
                    break;
                }

                case DynamicCodeNodeOp::OpLocalVarAssignment:
                {
                    ret.append(node->m_value.m_name.c_str());
                    break;
                }

                case DynamicCodeNodeOp::OpGenericFunctionCall:
                {
                    ret.append(node->m_value.m_name.c_str());
                    GenerateCodeForchildren(ret, node);
                    break;
                }

                case DynamicCodeNodeOp::OpSampleTexture:
                {
                    ret.append("texture");
                    GenerateCodeForchildren(ret, node);
                    break;
                }

                case DynamicCodeNodeOp::OpConstant:
                {
                    if (node->m_type == DynamicCodeChunkDataType::Float)
                        ret.appendf("{}", node->m_value.m_const.x);
                    if (node->m_type == DynamicCodeChunkDataType::Float2)
                        ret.appendf("float2({}, {})", node->m_value.m_const.x, node->m_value.m_const.y);
                    else if (node->m_type == DynamicCodeChunkDataType::Float3)
                        ret.appendf("float3({}, {}, {})", node->m_value.m_const.x, node->m_value.m_const.y, node->m_value.m_const.z);
                    else if (node->m_type == DynamicCodeChunkDataType::Float4)
                        ret.appendf("float4({}, {}, {}, {})", node->m_value.m_const.x, node->m_value.m_const.y, node->m_value.m_const.z, node->m_value.m_const.w);

                    break;
                }

                case DynamicCodeNodeOp::OpMakeFloat:
                {
                    if (node->m_type == DynamicCodeChunkDataType::Float2)
                        ret.append("float2");
                    else if (node->m_type == DynamicCodeChunkDataType::Float3)
                        ret.append("float3");
                    else if (node->m_type == DynamicCodeChunkDataType::Float4)
                        ret.append("float4");

                    GenerateCodeForchildren(ret, node);
                    break;
                }

                case DynamicCodeNodeOp::OpAdd:
                {
                    ret.append("(");
                    GenerateCodeNode(ret, node->m_children[0]);
                    ret.append("+");
                    GenerateCodeNode(ret, node->m_children[1]);
                    ret.append(")");
                    break;
                }

                case DynamicCodeNodeOp::OpMul:
                {
                    ret.append("(");
                    GenerateCodeNode(ret, node->m_children[0]);
                    ret.append("*");
                    GenerateCodeNode(ret, node->m_children[1]);
                    ret.append(")");
                    break;
                }

                case DynamicCodeNodeOp::OpSub:
                {
                    ret.append("(");
                    GenerateCodeNode(ret, node->m_children[0]);
                    ret.append("-");
                    GenerateCodeNode(ret, node->m_children[1]);
                    ret.append(")");
                    break;
                }

                case DynamicCodeNodeOp::OpMin:
                {
                    ret.append("min");
                    GenerateCodeForchildren(ret, node);
                    break;
                }

                case DynamicCodeNodeOp::OpMax:
                {
                    ret.append("max");
                    GenerateCodeForchildren(ret, node);
                    break;
                }

                case DynamicCodeNodeOp::OpDot:
                {
                    ret.append("dot");
                    GenerateCodeForchildren(ret, node);
                    break;
                }

                case DynamicCodeNodeOp::OpSwizzle:
                {
                    GenerateCodeForchildren(ret, node);
                    if (!node->m_swizzle.empty())
                    {
                        ret.append(".");
                        for (auto swz : node->m_swizzle)
                        {
                            switch (swz)
                            {
                                case DynamicCodeSwizzle::X:
                                    ret.append("x");
                                    break;
                                case DynamicCodeSwizzle::Y:
                                    ret.append("y");
                                    break;
                                case DynamicCodeSwizzle::Z:
                                    ret.append("z");
                                    break;
                                case DynamicCodeSwizzle::W:
                                    ret.append("w");
                                    break;
                                case DynamicCodeSwizzle::One:
                                    ret.append("1");
                                    break;
                                case DynamicCodeSwizzle::Zero:
                                    ret.append("0");
                                    break;
                            }
                        }
                    }
                    break;
                }

                case DynamicCodeNodeOp::OpSaturate:
                {
                    ret.append("saturate");
                    GenerateCodeForchildren(ret, node);
                    break;
                }

                case DynamicCodeNodeOp::OpClamp:
                {
                    ret.append("clamp");
                    GenerateCodeForchildren(ret, node);
                    break;
                }

                case DynamicCodeNodeOp::OpNeg:
                {
                    ret.append("-(");
                    GenerateCodeNode(ret, node->m_children[0]);
                    ret.append(")");
                    break;
                }

                case DynamicCodeNodeOp::OpFrac:
                {
                    ret.append("swizzle");
                    GenerateCodeForchildren(ret, node);
                    break;
                }

                case DynamicCodeNodeOp::OpCeil:
                {
                    ret.append("ceil");
                    GenerateCodeForchildren(ret, node);
                    break;
                }

                case DynamicCodeNodeOp::OpFloor:
                {
                    ret.append("floor");
                    GenerateCodeForchildren(ret, node);
                    break;
                }

                case DynamicCodeNodeOp::OpRound:
                {
                    ret.append("round");
                    GenerateCodeForchildren(ret, node);
                    break;
                }

                case DynamicCodeNodeOp::OpFmod:
                {
                    ret.append("fmod");
                    GenerateCodeForchildren(ret, node);
                    break;
                }

                case DynamicCodeNodeOp::OpSin:
                {
                    ret.append("sin");
                    GenerateCodeForchildren(ret, node);
                    break;
                }

                case DynamicCodeNodeOp::OpCos:
                {
                    ret.append("cos");
                    GenerateCodeForchildren(ret, node);
                    break;
                }

                case DynamicCodeNodeOp::OpTan:
                {
                    ret.append("tan");
                    GenerateCodeForchildren(ret, node);
                    break;
                }

                case DynamicCodeNodeOp::OpAtan:
                {
                    ret.append("atan2");
                    GenerateCodeForchildren(ret, node);
                    break;
                }

                case DynamicCodeNodeOp::OpPow:
                {
                    ret.append("pow");
                    GenerateCodeForchildren(ret, node);
                    break;
                }

                case DynamicCodeNodeOp::OpExp:
                {
                    ret.append("exp");
                    GenerateCodeForchildren(ret, node);
                    break;
                }

                case DynamicCodeNodeOp::OpLn:
                {
                    ret.append("ln");
                    GenerateCodeForchildren(ret, node);
                    break;
                }

                case DynamicCodeNodeOp::OpInv:
                {
                    ret.append("(1.0f /");
                    GenerateCodeNode(ret, node->m_children[0]);
                    ret.append(")");
                    break;
                }

                case DynamicCodeNodeOp::OpOneMinus:
                {
                    ret.append("(1.0f - ");
                    GenerateCodeNode(ret, node->m_children[0]);
                    ret.append(")");
                    break;
                }

                case DynamicCodeNodeOp::OpLerp:
                {
                    ret.append("lerp");
                    GenerateCodeForchildren(ret, node);
                    break;
                }

                case DynamicCodeNodeOp::OpCmpEq:
                {
                    ret.append("(");
                    GenerateCodeNode(ret, node->m_children[0]);
                    ret.append("==");
                    GenerateCodeNode(ret, node->m_children[1]);
                    ret.append(")");
                    break;
                }

                case DynamicCodeNodeOp::OpCmpNeq:
                {
                    ret.append("(");
                    GenerateCodeNode(ret, node->m_children[0]);
                    ret.append("!=");
                    GenerateCodeNode(ret, node->m_children[1]);
                    ret.append(")");
                    break;
                }

                case DynamicCodeNodeOp::OpCmpLt:
                {
                    ret.append("(");
                    GenerateCodeNode(ret, node->m_children[0]);
                    ret.append("<");
                    GenerateCodeNode(ret, node->m_children[1]);
                    ret.append(")");
                    break;
                }

                case DynamicCodeNodeOp::OpCmpLe:
                {
                    ret.append("(");
                    GenerateCodeNode(ret, node->m_children[0]);
                    ret.append("<=");
                    GenerateCodeNode(ret, node->m_children[1]);
                    ret.append(")");
                    break;
                }

                case DynamicCodeNodeOp::OpCmpGt:
                {
                    ret.append("(");
                    GenerateCodeNode(ret, node->m_children[0]);
                    ret.append(">");
                    GenerateCodeNode(ret, node->m_children[1]);
                    ret.append(")");
                    break;
                }

                case DynamicCodeNodeOp::OpCmpGe:
                {
                    ret.append("(");
                    GenerateCodeNode(ret, node->m_children[0]);
                    ret.append(">=");
                    GenerateCodeNode(ret, node->m_children[1]);
                    ret.append(")");
                    break;
                }

                case DynamicCodeNodeOp::OpSelect:
                {
                    ret.append("(");
                    GenerateCodeNode(ret, node->m_children[0]);
                    ret.append("?");
                    GenerateCodeNode(ret, node->m_children[1]);
                    ret.append(":");
                    GenerateCodeNode(ret, node->m_children[2]);
                    ret.append(")");
                    break;
                }

                case DynamicCodeNodeOp::OpLogicAnd:
                {
                    ret.append("(");
                    GenerateCodeNode(ret, node->m_children[0]);
                    ret.append("&&");
                    GenerateCodeNode(ret, node->m_children[1]);
                    ret.append(")");
                    break;
                }

                case DynamicCodeNodeOp::OpLogicOr:
                {
                    ret.append("(");
                    GenerateCodeNode(ret, node->m_children[0]);
                    ret.append("||");
                    GenerateCodeNode(ret, node->m_children[1]);
                    ret.append(")");
                    break;
                }

                case DynamicCodeNodeOp::OpLogicXor:
                {
                    ret.append("(");
                    GenerateCodeNode(ret, node->m_children[0]);
                    ret.append("^");
                    GenerateCodeNode(ret, node->m_children[1]);
                    ret.append(")");
                    break;
                }

                case DynamicCodeNodeOp::OpLogicNot:
                {
                    ret.append("!");
                    GenerateCodeForchildren(ret, node);
                    break;
                }

                case DynamicCodeNodeOp::OpLogicTrue:
                {
                    ret.append("true");
                    break;
                }

                case DynamicCodeNodeOp::OpLogicFalse:
                {
                    ret.append("false");
                    break;
                }

                default:
                {
                    TRACE_ERROR("Trying to generate code from invalid expression opcode {}", (uint32_t)node->m_op);
                    break;
                }
            }
        }

        void DynamicCompiler::GenerateCodeBlock(base::StringBuilder& ret, const DynamicCodeBlock* block)
        {
            for (auto node  : block->m_code)
            {
                switch (node->m_op)
                {
                    case DynamicCodeNodeOp::OpLocalVarAssignment:
                    {
                        ret.append(node->m_value.m_name.c_str());
                        ret.append(" = ");
                        GenerateCodeNode(ret, node->m_children[0]);
                        ret.append(";\n");
                        break;
                    }

                    default:
                    {
                        TRACE_ERROR("Trying to generate code from invalid statement opcode {}", (uint32_t) node->m_op);
                        break;
                    }
                }
            }
        }

        void DynamicCompiler::generateCode(const base::StringBuf& templateText, base::StringBuilder& ret) const
        {
            //base::StringGluer gluer;
/*
            /// generate the parameter layout descriptor
            if (!params.empty())
            {
                auto descriptorBuilder  = gluer.insertionPoint("DynamicParameterLayout");

                // export constants
                descriptorBuilder->append("material descriptor DynamicShaderParameters\n");
                descriptorBuilder->append("{\n");

                // if we have constants emit the constant block
                bool hasConstants = false;
                for (auto& p : params)
                    if (IsConstantParam(p.type))
                        hasConstants = true;

                // emit constant block
                if (hasConstants)
                {
                    descriptorBuilder->append("ConstantBuffer {\n");
                    for (auto& p : params)
                    {
                        if (IsConstantParam(p.type))
                        {
                            WriteDescriptorType(p.type, *descriptorBuilder);
                            descriptorBuilder->append(" ");
                            descriptorBuilder->append(p.name.c_str());
                            descriptorBuilder->append(";\n");
                        }
                    }
                    descriptorBuilder->append("}\n");
                }

                // emit other resources
                for (auto& p : params)
                {
                    if (!IsConstantParam(p.type))
                    {
                        WriteDescriptorType(p.type, *descriptorBuilder);
                        descriptorBuilder->append(" ");
                        descriptorBuilder->append(p.name.c_str());
                        descriptorBuilder->append(";\n");
                    }
                }

                // close the block
                descriptorBuilder->append("}\n");
            }

            // generate the uniform block
            if (!params.empty())
            {
                auto codeBuilder = gluer.insertionPoint("DynamicUniformBlock");

                for (auto& p : params)
                {
                    //if (IsConstantParam(p.type))
                    {
                        codeBuilder->append("uniform ");
                        WriteUniformType(p.type, *codeBuilder);
                        codeBuilder->append(" ");
                        codeBuilder->append(p.name.c_str());
                        codeBuilder->append(";\n");
                    }
                }
            }

            // declare param use
            if (!params.empty())
            {
                auto codeBuilder = gluer.insertionPoint("DynamicParameterLayoutUsages");
                codeBuilder->append("state.params.type = 'DynamicShaderParameters';\n");
            }

            // emit inputs
            {
                auto codeBuilder = gluer.insertionPoint("DynamicInputBlock");
                for (auto& p : m_inputs)
                {
                    WriteUniformType(p.type, *codeBuilder);
                    codeBuilder->append(" ");
                    codeBuilder->append(p.m_paramName.c_str());
                    codeBuilder->append(" = ");
                    codeBuilder->append(p.m_funcName.c_str());
                    codeBuilder->append("();\n");
                }
            }

            // emit variable declarations
            for (auto& v : m_vars)
            {
                auto codeBuilder = gluer.insertionPoint("DynamicTempVars");
                WriteUniformType(v.type, *codeBuilder);
                codeBuilder->append(" ");
                codeBuilder->append(v.name.c_str());
                codeBuilder->append(";");
            }

            // emit code from blocks
            {
                auto codeBuilder = gluer.insertionPoint("DynamicCodeBlock");
                GenerateCodeBlock(*codeBuilder, block);
            }

            // emit outputs
            {
                auto codeBuilder = gluer.insertionPoint("DynamicOutputBlock");
                for (auto& p : m_outputs)
                {
                    auto node  = p.node;
                    if (node)
                    {
                        codeBuilder->append(p.name.c_str());
                        codeBuilder->append(" = ");
                        GenerateCodeNode(*codeBuilder, p.node);
                        codeBuilder->append(";\n");
                    }
                }
            }

            // assemble using the template code
            //gluer.generateOutput(templateText, ret);*/
        }

        ///----

    } // pipeline
} // rendering


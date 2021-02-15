/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\program #]
***/

#include "build.h"
#include "renderingShaderFunction.h"
#include "renderingShaderProgram.h"
#include "renderingShaderNativeFunction.h"
#include "base/containers/include/stringBuilder.h"
#include "base/reflection/include/reflectionTypeName.h"
#include "renderingShaderFileParserHelper.h"

namespace rendering
{
    namespace compiler
    {
		//---

		RTTI_BEGIN_TYPE_ENUM(DataParameterScope)
			RTTI_ENUM_OPTION(Unknown)
			RTTI_ENUM_OPTION(StaticConstant)
			RTTI_ENUM_OPTION(GlobalConst)
			RTTI_ENUM_OPTION(GlobalParameter)
			RTTI_ENUM_OPTION(GlobalBuiltin)
			RTTI_ENUM_OPTION(VertexInput)
			RTTI_ENUM_OPTION(StageInput)
			RTTI_ENUM_OPTION(StageOutput)
			RTTI_ENUM_OPTION(GroupShared)
			RTTI_ENUM_OPTION(FunctionInput)
			RTTI_ENUM_OPTION(ScopeLocal)
			RTTI_ENUM_OPTION(Export)
		RTTI_END_TYPE();

        //---

        void DataParameter::print(base::IFormatStream& ret) const
        {
            switch (scope)
            {
                case DataParameterScope::Unknown: ret.append("unknown "); break;
                case DataParameterScope::StageInput: ret.append("input "); break;
                case DataParameterScope::StageOutput: ret.append("output "); break;
                case DataParameterScope::GlobalBuiltin: ret.append("builtin "); break;
                case DataParameterScope::GlobalConst: ret.append("const "); break;
                case DataParameterScope::GlobalParameter: ret.append("global "); break;
                case DataParameterScope::GroupShared: ret.append("group "); break;
                case DataParameterScope::FunctionInput: ret.append("arg "); break;
                case DataParameterScope::ScopeLocal: ret.append("local "); break;
            }

            ret << dataType;
            ret.append(" ");
            ret << name.view();

            if (!initalizationTokens.empty())
            {
                ret.append(", InitTokens:");
                for (auto token  : initalizationTokens)
                    ret << " " << token->view();
            }
        }

        //---

        ComponentMask::ComponentMask()
        {
            for (uint32_t i = 0; i < MAX_COMPONENTS; ++i)
                m_bits[i] = ComponentMaskBit::NotDefined;
        }

        ComponentMask::ComponentMask(ComponentMaskBit x, ComponentMaskBit y, ComponentMaskBit z, ComponentMaskBit w)
        {           
            for (uint32_t i = 0; i < MAX_COMPONENTS; ++i)
                m_bits[i] = ComponentMaskBit::NotDefined;

            if (x != ComponentMaskBit::NotDefined)
            {
                m_bits[0] = x;
                if (y != ComponentMaskBit::NotDefined)
                {
                    m_bits[1] = y;
                    if (z != ComponentMaskBit::NotDefined)
                    {
                        m_bits[2] = z;
                        if (w != ComponentMaskBit::NotDefined)
                        {
                            m_bits[3] = w;
                        }
                    }
                }
            }
        }

        void ComponentMask::print(base::IFormatStream& f) const
        {
            for (uint32_t i = 0; i < MAX_COMPONENTS; ++i)
            {
                switch (m_bits[i])
                {
                    case ComponentMaskBit::NotDefined: f.append("?"); break;
                    case ComponentMaskBit::X: f.append("x"); break;
                    case ComponentMaskBit::Y: f.append("y"); break;
                    case ComponentMaskBit::Z: f.append("z"); break;
                    case ComponentMaskBit::W: f.append("w"); break;
                    case ComponentMaskBit::Zero: f.append("0"); break;
                    case ComponentMaskBit::One: f.append("1"); break;
                }
            }
        }

        uint8_t ComponentMask::writeMask() const
        {
            uint8_t mask = 0;
            uint8_t prevValue = 0;

            for (uint32_t i = 0; i < MAX_COMPONENTS; ++i)
            {
                uint8_t bitValue = 0;

                switch (m_bits[i])
                {
                    case ComponentMaskBit::X: bitValue = 1; break;
                    case ComponentMaskBit::Y: bitValue = 2; break;
                    case ComponentMaskBit::Z: bitValue = 4; break;
                    case ComponentMaskBit::W: bitValue = 8; break;
                    default: return 0;
                }

                if (mask & bitValue)
                    return 0;
                if (bitValue < prevValue)
                    return 0;
                mask |= bitValue;
                prevValue = bitValue;
            }

            return mask;
        }

        uint16_t ComponentMask::numericalValue() const
        {
            uint16_t ret = 0;

            for (uint32_t i = 0; i < MAX_COMPONENTS; ++i)
            {
                ret *= 7;
                ret += ((int)m_bits[i]) + 1;
            }

            return ret;
        }

        uint8_t ComponentMask::numberOfComponentsNeeded() const
        {
            uint8_t ret = 0;

            for (uint32_t i = 0; i < MAX_COMPONENTS; ++i)
            {
                auto val = (char)m_bits[i];
                if (val >= 0 && val <= 3)
                    ret = std::max<uint8_t>(ret, (uint8_t)(val+1));
            }

            return ret;
        }

        uint8_t ComponentMask::numberOfComponentsProduced() const
        {
            uint32_t i = 0;
            for (i = 0; i < MAX_COMPONENTS; ++i)
                if (m_bits[i] == ComponentMaskBit::NotDefined)
                    break;

            return range_cast<uint8_t>(i);
        }

        bool ComponentMask::isMask() const
        {
            uint8_t components[4] = { 0,0,0,0 };
            for (uint32_t i = 0; i < MAX_COMPONENTS; ++i)
            {
                if (m_bits[i] == ComponentMaskBit::NotDefined)
                    break;

                if (m_bits[i] == ComponentMaskBit::Zero || m_bits[i] == ComponentMaskBit::One)
                    return false;

                auto val = (uint8_t)m_bits[i];
                if (components[val])
                    return false; // already set

                components[val] = 1;
            }

            return true;
        }

        void ComponentMask::componentsToWrite(base:: InplaceArray<uint8_t, 4>& outComponents) const
        {
            for (uint32_t i = 0; i < MAX_COMPONENTS; ++i)
            {
                if (m_bits[i] == ComponentMaskBit::NotDefined)
                    break;

                if (m_bits[i] == ComponentMaskBit::Zero || m_bits[i] == ComponentMaskBit::One)
                    continue;

                auto val = (uint8_t)m_bits[i];
                outComponents.pushBack(val);
            }
        }

        bool ComponentMask::Parse(const char* txt, ComponentMask& outSwizzle)
        {
            ComponentMask ret;

            // parse general pattern
            uint32_t pos = 0;
            while (*txt)
            {
                // swizzle is to long
                if (pos >= MAX_COMPONENTS)
                    return false;

                // get the char
                auto ch = *txt++;

                // match
                auto& bit = ret.m_bits[pos++];
                switch (ch)
                {
                    case '0': bit = ComponentMaskBit::Zero; break;
                    case '1': bit = ComponentMaskBit::One; break;
                    case 'x': bit = ComponentMaskBit::X; break;
                    case 'y': bit = ComponentMaskBit::Y; break;
                    case 'z': bit = ComponentMaskBit::Z; break;
                    case 'w': bit = ComponentMaskBit::W; break;
                    case 'r': bit = ComponentMaskBit::X; break;
                    case 'g': bit = ComponentMaskBit::Y; break;
                    case 'b': bit = ComponentMaskBit::Z; break;
                    case 'a': bit = ComponentMaskBit::W; break;

                    default: return false; // invalid char
                }
            }

            // done
            outSwizzle = ret;
            return true;
        }
        
        //---

        Function::Function(const CodeLibrary& library, const Program* program, const base::parser::Location& loc, /*const FunctionFlags flags, */const base::StringID name, const DataType& retType, const base::Array<DataParameter*>& params, const base::Array<base::parser::Token*>& tokens, AttributeList&& attributes)
            : m_loc(loc)
            , m_name(name)
            //, flags(flags)
            , m_returnType(retType)
            , m_attributes(attributes)
            , m_inputParameters(params)
            , m_code(nullptr)
            , m_codeTokens(tokens)
            , m_library(&library)
            , m_program(program)
        {
        }

        Function::Function(const Function& func, const ProgramConstants* localConstantArgs)
            : m_loc(func.m_loc)
            , m_name(func.m_name)
            , m_returnType(func.m_returnType)
            , m_inputParameters(func.m_inputParameters)
            , m_attributes(func.m_attributes)
            , m_code(nullptr)
            , m_library(func.m_library)
            , m_program(func.m_program)
        {
			if (localConstantArgs)
				for (const auto* param : m_inputParameters)
					if (localConstantArgs->findConstValue(param))
						m_staticParameters.pushBack(param->name);
		}

        Function::~Function()
        {
        }            

        //---

        void Function::print(base::IFormatStream& f) const
        {
            /*if (flags.test(FunctionFlag::Override))
                f << "override ";*/

            f.appendf("function {} ({} params)\n", m_name, m_inputParameters.size());

            for (uint32_t i=0; i<m_inputParameters.size(); ++i)
            {
                f.appendf("         input[{}]: ", i);
                m_inputParameters[i]->print(f);
                f.appendf("\n");
            }

            /*if (!m_codeTokens.empty())
            {
                f.append("         Code:");
                f << m_codeTokens;
                f.appendf("\n");
            }*/
        }

        void Function::bindCode(CodeNode* code)
        {
            m_code = code;
        }

        void Function::rename(base::StringID name)
        {
            m_name = name;
        }

        //---

    } // shader
} // rendering


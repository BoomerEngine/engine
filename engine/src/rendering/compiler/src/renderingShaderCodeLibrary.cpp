/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\nodes #]
***/

#include "build.h"
#include "renderingShaderFunction.h"
#include "renderingShaderProgram.h"
#include "renderingShaderProgram.h"
#include "renderingShaderProgramInstance.h"
#include "renderingShaderStaticExecution.h"
#include "renderingShaderTypeLibrary.h"
#include "renderingShaderCodeLibrary.h"

#include "base/io/include/ioSystem.h"
#include "base/memory/include/linearAllocator.h"
#include "base/containers/include/sortedArray.h"
#include "base/containers/include/stringBuilder.h"
#include "base/containers/include/crc.h"

namespace rendering
{
    namespace compiler
    {
        //--

        CodeLibrary::CodeLibrary(base::mem::LinearAllocator& allocator, TypeLibrary& typeLibrary)
            : m_allocator(allocator)
            , m_typeLibrary(&typeLibrary)
        {
        }

        CodeLibrary::~CodeLibrary()
        {
        }

        static void CollectBasePrograms(const Program* program, base::Array<const Program*>& basePrograms)
        {
            for (auto parentProgram : program->parentPrograms())
            {
                if (!basePrograms.contains(parentProgram))
                {
                    basePrograms.pushBack(parentProgram);
                    CollectBasePrograms(parentProgram, basePrograms);
                }
            }
        }

        bool CodeLibrary::parseContent(base::parser::TokenList& tokens, base::parser::IErrorReporter &err)
        {
            // load file content
            if (!loadFile(tokens, err))
                return false;

            /*// update CRC all everything
            for (;;)
            {
                bool valid = true;
                for (auto program  : m_programs.values())
                    valid &= const_cast<Program*>(program)->refreshCRC();
                for (auto func  : m_globalFunctions.values())
                    valid &= const_cast<Function*>(func)->refreshCRC();

                if (valid)
                    break;
            }*/

            // stats
            TRACE_INFO("Found {} programs in loaded files", m_programs.size());
            return true;
        }

        const Program* CodeLibrary::createProgram(const base::StringID programName, const base::parser::Location& loc, AttributeList&& attributes)
        {
            ASSERT(!m_programs.contains(programName));

            // return existing one
            auto program = m_allocator.create<Program>(*this, programName, std::move(attributes));
            m_programs.set(programName, program);
            m_programList.pushBack(program);
            return program;
        }

        const Program* CodeLibrary::findProgram(const base::StringID programName) const
        {
            Program* program = nullptr;
            m_programs.find(programName, program);
            return program;
        }

        const Function* CodeLibrary::findGlobalFunction(const base::StringID name) const
        {
            Function* func = nullptr;
            m_globalFunctions.find(name, func);
            return func;
        }

        const DataParameter* CodeLibrary::findGlobalConstant(const base::StringID name) const
        {
            DataParameter* param = nullptr;
            m_globalConstants.find(name, param);
            return param;
        }

        //--

        static uint64_t CalcProgramInstanceKey(const Program* program, const ProgramConstants& params)
        {
            base::CRC64 crc;
            crc << program->name();
            //crc << program->crc();
            params.calcCRC(crc);
            return crc;
        }

        ProgramInstance::ProgramInstance(const Program* program, ProgramConstants&& params, uint64_t key)
            : m_program(program)
            , m_params(std::move(params))
            , m_uniqueKey(key)
        {}

        static void CollectConstants(const Program* program, base::Array<const DataParameter*>& outParams)
        {
            for (const auto* parent : program->parentPrograms())
                CollectConstants(parent, outParams);

            for (const auto* param : program->parameters())
                if (param->scope == DataParameterScope::GlobalConst && param->initializerCode)
                    outParams.pushBackUnique(param);
        }

        const ProgramInstance* CodeLibrary::createProgramInstance(const base::parser::Location& loc, const Program* program, const ProgramConstants& sourceParams, base::parser::IErrorReporter& err)
        {
            auto key = CalcProgramInstanceKey(program, sourceParams);

            auto lock = CreateLock(m_programInstanceLock);

            // find existing
            ProgramInstance* ret = nullptr;
            if (m_programInstanceMap.find(key, ret))
            {
                DEBUG_CHECK_EX(ret->program() == program, "Program instance key collision");
                //DEBUG_CHECK_EX(ret->params() == params, "Program instance key collision");
                return ret;
            }

            // find all constants in the programs
            base::Array<const DataParameter*> constParameters;
            CollectConstants(program, constParameters);
            TRACE_DEEP("Instancing '{}' with '{}', {} params, key {}", program->name(), sourceParams, constParameters.size(), Hex(key));

            // create a "blank" parametrization where all params are undefined - this is the initial state
            // inject provided values
            ProgramConstants params;
            base::InplaceArray<const DataParameter*, 32> uninitializedParams;
            base::InplaceArray<const DataParameter*, 32> uninitializedPrograms;
            for (const auto* param : constParameters)
            {
                if (const auto* definedValue = sourceParams.findConstValue(param))
                {
                    params.constValue(param, *definedValue);
                    TRACE_DEEP("Value defined for '{}' as '{}'", param->name, *definedValue);
                }
                else
                {
                    DataValue val(param->dataType); // undefined value of matching type
                    params.constValue(param, val);
                    TRACE_DEEP("No value defined for '{}'", param->name);

                    if (param->dataType.isProgram())
                        uninitializedPrograms.pushBack(param);
                    else
                        uninitializedParams.pushBack(param);
                }
            }

            // initialize program after simple params
            uninitializedParams.pushBack(uninitializedPrograms.typedData(), uninitializedPrograms.size());
            uninitializedPrograms.reset();

            // create program instance with this blank parametrization
            ret = m_allocator.create<ProgramInstance>(program, std::move(params), key);

            // evaluate values for all remaining uninitialized parameters
            for (const auto* param : uninitializedParams)
            {
                // fold the initial value
                ExecutionValue value;
                ExecutionStack stack(m_allocator, *this, err, nullptr, ret); // all nulls -> evaluate in global context
                if (ExecutionResult::Finished != ExecuteProgramCode(stack, param->initializerCode, value))
                {
                    err.reportError(loc, base::TempString("Unable to evaluate value for parameter '{}' when instancing program '{}'", param->name, program->name()));
                    return false;
                }

                // value must be fully defined
                DataValue constValue;
                if (!value.isWholeValueDefined() || !value.readValue(constValue))
                {
                    err.reportError(loc, base::TempString("Unable to evaluate compile-time value for parameter '{}' when instancing program '{}'", param->name, program->name()));
                    err.reportError(param->loc, base::TempString("Parameter '{}' was defined here", param->name, param->loc));
                    return false;
                }

                // store as parameter value
                TRACE_DEEP("Evaluated '{}' in '{}' as '{}'", param->name, program->name(), constValue);
                ret->params().constValue(param, constValue);
            }

            // store final instance
            m_programInstanceMap[key] = ret;
            return ret;
        }

        //--

    } // shader
} // rendering

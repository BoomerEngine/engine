/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler #]
***/

#include "build.h"
#include "renderingShaderFunction.h"
#include "renderingShaderProgram.h"
#include "renderingShaderTypeLibrary.h"
#include "renderingShaderCodeLibrary.h"

namespace rendering
{
    namespace compiler
    {

        Program::Program(const CodeLibrary& library, base::StringID name, AttributeList&& attributes)
            : m_name(name)
            , m_library(&library)
            , m_attributes(attributes)
        {}

        Program::~Program()
        {}

        /*bool Program::refreshCRC()
        {
            base::CRC64 crc;

            // crc is already computed
            if (m_crc != 0)
                return true;

            // basic stuff
            crc << m_name;
            m_attributes.calcTypeHash(crc);

            // the following CRC computation may fail if the partial CRCs for the relevant stuff are not yet known
            bool validCRC = true;

            // include the CRC of programs we are based on
            for (auto prog  : m_parentPrograms)
            {
                if (!const_cast<Program*>(prog)->refreshCRC())
                {
                    validCRC = false;
                    continue;
                }

                crc << prog->crc();
            }

            // include crc of all the functions
            for (auto func  : m_functions)
            {
                crc << func->name();
                if (!func->refreshCRC())
                {
                    validCRC = false;
                    continue;
                }

                crc << func->crc();
            }

            //TRACE_INFO("CRC after funcs: {}", Hex(crc.crc()));
            // include all parameters
            for (auto param  : m_parameters)
            {
                crc << param->name.c_str();
                validCRC &= CodeNode::CalcCRC(crc, param->initializerCode);
            }

            if (validCRC)
            {
                TRACE_INFO("Static CRC of program '{}' computed to be 0x{}", name(), Hex(crc.crc()));
                m_crc = crc.crc();
            }

            return validCRC;
        }*/

        void Program::addParentProgram(const Program* parentProgram)
        {
            ASSERT(!m_parentPrograms.contains(parentProgram));
            m_parentPrograms.pushBackUnique(parentProgram);
        }

        void Program::addParameter(DataParameter* param)
        {
            ASSERT(!m_parameters.contains(param));
            ASSERT(!findParameter(param->name));
            m_parameters.pushBack(param);
        }

        void Program::addFunction(Function* func)
        {
            ASSERT(!m_functions.contains(func));

            auto existing  = findFunction(func->name(), false);
            if (existing)
                m_functions.remove((Function*)existing);

            m_functions.pushBack(func);
        }

        bool Program::isBasedOnProgram(const Program* program) const
        {
            if (this == program)
                return true;

            for (auto baseProgram  : m_parentPrograms)
                if (baseProgram->isBasedOnProgram(program))
                    return true;

            return false;
        }

        //---

        const DataParameter* Program::createBuildinParameterReference(base::StringID name) const
        {
            for (auto param : m_parameters)
                if (param->name == name)
                    return param;

            DataType type;
            DataParameterScope scope = DataParameterScope::GlobalBuiltin;
            if (name == "gl_Position"_id)
                type = m_library->typeLibrary().floatType(4);
            else if (name == "gl_PositionIn"_id)
                type = m_library->typeLibrary().floatType(4).applyArrayCounts(ArrayCounts().appendUndefinedArray());
            else if (name == "gl_PointSize"_id)
                type = m_library->typeLibrary().floatType();
            else if (name == "gl_PointSize"_id)
                type = m_library->typeLibrary().floatType();
            else if (name == "gl_ClipDistance"_id)
                type = m_library->typeLibrary().floatType().applyArrayCounts(6);
            else if (name == "gl_VertexID"_id)
                type = m_library->typeLibrary().integerType(1);
            else if (name == "gl_InstanceID"_id)
                type = m_library->typeLibrary().integerType(1);
            else if (name == "gl_DrawID"_id)
                type = m_library->typeLibrary().integerType(1);
            else if (name == "gl_BaseVertex"_id)
                type = m_library->typeLibrary().integerType(1);
            else if (name == "gl_BaseInstance"_id)
                type = m_library->typeLibrary().integerType(1);
            else if (name == "gl_PatchVerticesIn"_id)
                type = m_library->typeLibrary().integerType(1);
            else if (name == "gl_PrimitiveID"_id)
                type = m_library->typeLibrary().integerType(1);
            else if (name == "gl_InvocationID"_id)
                type = m_library->typeLibrary().integerType(1);
            else if (name == "gl_TessLevelOuter"_id)
                type = m_library->typeLibrary().floatType(1).applyArrayCounts(4);
            else if (name == "gl_TessLevelInner"_id)
                type = m_library->typeLibrary().floatType(1).applyArrayCounts(2);
            else if (name == "gl_TessCoord"_id)
                type = m_library->typeLibrary().floatType(3);
            else if (name == "gl_PatchVerticesIn"_id)
                type = m_library->typeLibrary().integerType();
            else if (name == "gl_FragCoord"_id)
                type = m_library->typeLibrary().floatType(4);
            else if (name == "gl_FrontFacing"_id)
                type = m_library->typeLibrary().booleanType();
            else if (name == "gl_PointCoord"_id)
                type = m_library->typeLibrary().floatType(2);
            else if (name == "gl_SampleID"_id)
                type = m_library->typeLibrary().integerType(1);
            else if (name == "gl_SamplePosition"_id)
                type = m_library->typeLibrary().floatType(2);
            else if (name == "gl_SampleMaskIn"_id)
                type = m_library->typeLibrary().integerType();
            else if (name == "gl_SampleMask"_id)
                type = m_library->typeLibrary().integerType();
            else if (name == "gl_Target0"_id)
                type = m_library->typeLibrary().floatType(4);
            else if (name == "gl_Target1"_id)
                type = m_library->typeLibrary().floatType(4);
            else if (name == "gl_Target2"_id)
                type = m_library->typeLibrary().floatType(4);
            else if (name == "gl_Target3"_id)
                type = m_library->typeLibrary().floatType(4);
            else if (name == "gl_Target4"_id)
                type = m_library->typeLibrary().floatType(4);
            else if (name == "gl_Target5"_id)
                type = m_library->typeLibrary().floatType(4);
            else if (name == "gl_Target6"_id)
                type = m_library->typeLibrary().floatType(4);
            else if (name == "gl_Target7"_id)
                type = m_library->typeLibrary().floatType(4);
            else if (name == "gl_NumWorkGroups"_id)
                type = m_library->typeLibrary().unsignedType(3);
            else if (name == "gl_GlobalInvocationID"_id)
                type = m_library->typeLibrary().unsignedType(3);
            else if (name == "gl_LocalInvocationID"_id)
                type = m_library->typeLibrary().unsignedType(3);
            else if (name == "gl_WorkGroupID"_id)
                type = m_library->typeLibrary().unsignedType(3);
            else if (name == "gl_LocalInvocationIndex"_id)
                type = m_library->typeLibrary().unsignedType(1);
            

            if (!type.valid())
                return nullptr;

            DataParameter* param = m_library->allocator().create<DataParameter>();
            param->name = name;
            param->scope = scope;
            param->dataType = type;
            param->attributes.add("builtin"_id);
            param->loc = m_loc;

            m_parameters.pushBack(param);
            return param;
        }
        
        const DataParameter* Program::createDescriptorElementReference(const ResolvedDescriptorEntry& entry) const
        {
            DEBUG_CHECK(entry.entry != nullptr);
            DEBUG_CHECK(entry.table != nullptr);

            DataParameter* param = nullptr;
            if (entry.entry->m_type.resource().constants)
            {
                DEBUG_CHECK(entry.member != nullptr);

                // format name and find existing param
                auto mergedName = base::StringID(base::TempString("{}_{}_{}", entry.table->name(), entry.entry->m_name, entry.member->name));
                if (auto existingParam = findParameter(mergedName, true))
                    return existingParam;

                // create param
                param = m_library->allocator().create<DataParameter>();
                param->name = mergedName;
                param->scope = DataParameterScope::GlobalParameter;
                param->dataType = entry.member->type;
                param->attributes = entry.member->attributes;
                param->loc = entry.member->location;
            }
            else
            {
                // format name and find existing param
                auto mergedName = base::StringID(base::TempString("{}_{}", entry.table->name(), entry.entry->m_name));
                if (auto existingParam = findParameter(mergedName, true))
                    return existingParam;

                // create param
                param = m_library->allocator().create<DataParameter>();
                param->name = mergedName;
                param->scope = DataParameterScope::GlobalParameter;
                param->dataType = entry.entry->m_type;
                param->attributes = entry.entry->m_attributes;
                param->loc = entry.entry->m_location;
            }

            m_parameters.pushBack(param);
            m_descriptors.pushBackUnique(entry.table);
            return param;
        }

        const DataParameter* Program::findParameter(const base::StringID name, bool recurseToParent /*= true*/) const
        {
            // local search
            for (auto param  : m_parameters)
                if (param->name == name)
                    return param;

            // recurse
            if (recurseToParent)
            {
                for (auto parentProgram  : m_parentPrograms)
                {
                    auto param  = parentProgram->findParameter(name, recurseToParent);
                    if (param != nullptr)
                        return param;
                }
            }

            // not found
            return nullptr;
        }

        const Function* Program::findFunction(const base::StringID functionName, bool recurseToParent /*= true*/) const
        {
            // local search
            for (auto func  : m_functions)
                if (func->name() == functionName)
                    return func;

            // recurse
            if (recurseToParent)
            {
                for (auto parentProgram  : m_parentPrograms)
                {
                    auto func  = parentProgram->findFunction(functionName, recurseToParent);
                    if (func != nullptr)
                        return func;
                }
            }

            // not found
            return nullptr;
        }

        //---

        void Program::print(base::IFormatStream& f) const
        {
            if (m_attributes)
                f << m_attributes << " ";

            f.appendf("program {}\n", m_name);

            if (!m_parentPrograms.empty())
            {
                f.appendf("    {} parent programs\n", m_parentPrograms.size());
                for (uint32_t i=0; i<m_parentPrograms.size(); ++i)
                {
                    f.appendf("       [{}]: {}\n", i, m_parentPrograms[i]->name());
                }
            }

            if (!m_parameters.empty())
            {
                f.appendf("    {} parameters\n", m_parameters.size());
                for (uint32_t i=0; i<m_parameters.size(); ++i)
                {
                    f.appendf("       [{}]: ", i);
                    m_parameters[i]->print(f);
                    f.append("\n");
                }
            }

            if (!m_functions.empty())
            {
                f.appendf("    {} functions\n", m_functions.size());
                for (uint32_t i=0; i<m_functions.size(); ++i)
                {
                    f.appendf("       [{}]: ", i);
                    m_functions[i]->print(f);
                }
            }
        }

    } // shader
} // rendering


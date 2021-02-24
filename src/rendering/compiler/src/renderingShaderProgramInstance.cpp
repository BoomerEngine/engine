/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\program #]
***/

#include "build.h"
#include "renderingShaderProgram.h"
#include "renderingShaderProgramInstance.h"
#include "renderingShaderFunction.h"
#include "renderingShaderCodeLibrary.h"

BEGIN_BOOMER_NAMESPACE(rendering::shadercompiler)

//---

ProgramConstants::ProgramConstants() = default;
ProgramConstants::ProgramConstants(const ProgramConstants & other) = default;
ProgramConstants::ProgramConstants(ProgramConstants&& other) = default;
ProgramConstants& ProgramConstants::operator=(const ProgramConstants& other) = default;
ProgramConstants& ProgramConstants::operator=(ProgramConstants&& other) = default;
ProgramConstants::~ProgramConstants() = default;

const DataValue* ProgramConstants::findConstValue(base::StringID name) const
{
    for (auto& entry : m_constants)
        if (entry.parameter->name == name)
            return &entry.value;

    return nullptr;
}

const DataValue* ProgramConstants::findConstValue(const DataParameter* param) const
{
    for (auto& entry : m_constants)
        if (entry.parameter == param)
            return &entry.value;

    return nullptr;
}

void ProgramConstants::constValue(const DataParameter* param, const DataValue& value)
{
    //ASSERT(value.isWholeValueDefined());//&& value.isScalar());

    for (auto& entry : m_constants)
    {
        if (entry.parameter == param)
        {
            entry.value = value;
            return;
        }
    }

    auto& entry = m_constants.emplaceBack();
    entry.parameter = param;
    entry.value = value;

    std::sort(m_constants.begin(), m_constants.end(), [](const ProgramInstanceParam& a, const ProgramInstanceParam& b) { return a.parameter->name < b.parameter->name; });

    updateKey();
}

void ProgramConstants::calcCRC(base::CRC64& crc) const
{
    crc << m_constants.size();
    for (auto& param : m_constants)
    {
        crc << param.parameter->name.c_str();
        param.value.calcCRC(crc);
    }
}

void ProgramConstants::updateKey()
{
    base::CRC64 crc;
    calcCRC(crc);
    m_key = crc;
}

bool ProgramConstants::operator==(const ProgramConstants& other) const
{
    if (m_constants.size() != other.m_constants.size())
        return false;

    for (uint32_t i = 0; i < m_constants.size(); ++i)
        if (m_constants[i] != other.m_constants[i])
            return false;

    return true;
}

bool ProgramConstants::operator!=(const ProgramConstants& other) const
{
    return !operator==(other);
}

void ProgramConstants::print(base::IFormatStream& f) const
{
    bool addComa = false;
    for (auto& perm : m_constants)
    {
        if (addComa) f.append(", ");
        addComa = true;

        f << perm.parameter->name;
        f << "=";
        f << perm.value;
    }
}

//---

END_BOOMER_NAMESPACE(rendering::shadercompiler)
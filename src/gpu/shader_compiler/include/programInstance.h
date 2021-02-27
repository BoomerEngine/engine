/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\program #]
***/

#pragma once

#include "codeNode.h"
#include "core/memory/include/linearAllocator.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::compiler)

//---

/// parametrization argument for program instance
struct ProgramInstanceParam
{
    // parametrized parameter in the program
    // can be used to determine the type
    const DataParameter* parameter = nullptr;

    // data value for the parametrization
    // type (num of components) matches the type of the parameter
    DataValue value;

    //--

    INLINE bool operator==(const ProgramInstanceParam& other) const { return (parameter == other.parameter) && (value == other.value); }
    INLINE bool operator!=(const ProgramInstanceParam& other) const { return !operator==(other); }
};

//---

/// constants definitions, used during evaluation/constant folding/permutation tracing of a shader program
class GPU_SHADER_COMPILER_API ProgramConstants
{
public:
    ProgramConstants();
    ProgramConstants(const ProgramConstants& other);
    ProgramConstants(ProgramConstants&& other);
    ProgramConstants& operator=(const ProgramConstants& other);
    ProgramConstants& operator=(ProgramConstants&& other);
    ~ProgramConstants();

    //--
            
    INLINE const Array<ProgramInstanceParam>& constants() const { return m_constants; }

    INLINE bool empty() const { return m_constants.empty(); }

    INLINE uint64_t key() const { return m_key; }

    //--

    /// find value for given parameter
    const DataValue* findConstValue(const DataParameter* param) const;
            
    /// find value for given name
    const DataValue* findConstValue(StringID param) const;

    /// set value
    void constValue(const DataParameter* param, const DataValue& value);

    //---

    void print(IFormatStream& f) const;

    void calcCRC(CRC64& crc) const;

    //--

    bool operator==(const ProgramConstants& other) const;
    bool operator!=(const ProgramConstants& other) const;

private:
    Array<ProgramInstanceParam> m_constants;
    uint64_t m_key = 0;

    void updateKey();
}; 
        
//---

/// program instance - program + parameters
class GPU_SHADER_COMPILER_API ProgramInstance
{
public:
    ProgramInstance(const Program* program, ProgramConstants&& params, uint64_t key);

    INLINE const Program* program() const { return m_program; }
    INLINE const ProgramConstants& params() const { return m_params; }
    INLINE ProgramConstants& params() { return m_params; }

    INLINE uint64_t key() const { return m_uniqueKey; }

private:
    const Program* m_program = nullptr;
    ProgramConstants m_params;

    uint64_t m_uniqueKey = 0;
};

//---

END_BOOMER_NAMESPACE_EX(gpu::compiler)

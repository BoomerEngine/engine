/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "gpu_shader_compiler_glue.inl"

BEGIN_BOOMER_NAMESPACE_EX(gpu::compiler)

//--

struct DataType;
struct DataValue;
struct DataParameter;
class Function;
class INativeFunction;
class CodeNode;
class CodeLibrary;
class Program;
class ProgramConstants;
class Function;
class ExecutionValue;
class ParametrizedFragment;
class CompositeType;
class Permutation;
class TypeLibrary;
class ResourceTable;
struct ResourceTableEntry;
class CompositeType;
struct ProgramInstanceParam;

// TODO: move
struct GPU_SHADER_COMPILER_API AttributeList
{
    HashMap<StringID, StringBuf> attributes;

    INLINE bool empty() const { return attributes.empty(); }
    INLINE operator bool() const { return !attributes.empty(); }

    AttributeList& clear();
    AttributeList& add(StringID key, StringView txt = "");
    AttributeList& merge(const AttributeList& attr);

    bool has(StringID key) const;
    StringView value(StringID key) const;
    StringView valueOrDefault(StringID key, StringView defaultValue = "") const;
    int valueAsIntOrDefault(StringID key, int defaultValue = 0) const;

    void print(IFormatStream& f) const;
    void calcTypeHash(CRC64& crc) const;
};

//--

struct ExtraAttribute
{
	StringView key;
	StringView value;
};

//--

END_BOOMER_NAMESPACE_EX(gpu::compiler)

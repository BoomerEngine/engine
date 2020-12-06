/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "rendering_compiler_glue.inl"

namespace rendering
{
    namespace compiler
    {
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
        struct RENDERING_COMPILER_API AttributeList
        {
            base::HashMap<base::StringID, base::StringBuf> attributes;

            INLINE bool empty() const { return attributes.empty(); }
            INLINE operator bool() const { return !attributes.empty(); }

            AttributeList& clear();
            AttributeList& add(base::StringID key, base::StringView txt = "");

            bool has(base::StringID key) const;
            base::StringView value(base::StringID key) const;
            base::StringView valueOrDefault(base::StringID key, base::StringView defaultValue = "") const;
            int valueAsIntOrDefault(base::StringID key, int defaultValue = 0) const;

            void print(base::IFormatStream& f) const;
            void calcTypeHash(base::CRC64& crc) const;
        };

        //--

		struct ExtraAttribute
		{
			base::StringView key;
			base::StringView value;
		};

		//--

    } // shader
} // rendering
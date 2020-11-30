/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\program #]
***/

#pragma once

#include "renderingShaderTypeLibrary.h"
#include "renderingShaderCodeNode.h"
#include "base/containers/include/hashSet.h"

namespace rendering
{
    namespace compiler
    {

        //---

        class PermutationTable;
        class ResourceTable;
        struct ResourceTableEntry;
        struct ResolvedDescriptorEntry;

        //---

        /// a shader program definition
        class RENDERING_COMPILER_API Program : public base::NoCopy
        {
        public:
            Program(const CodeLibrary& library, base::StringID name, AttributeList&& attributes);
            ~Program();

            // get shader library this function is coming from
            INLINE const CodeLibrary& library() const { return *m_library; }

            // get location of function in the file
            INLINE const base::parser::Location& location() const { return m_loc; }

            // get general attributes (from parser)
            INLINE const AttributeList& attributes() const { return m_attributes; }

            // get program parameters
            typedef base::Array<DataParameter*> TParameters;
            INLINE const TParameters& parameters() const { return m_parameters; }

            // get program functions
            typedef base::Array<Function*> TFunctions;
            INLINE const TFunctions& functions() const { return m_functions; }

            // get parent programs
            typedef base::Array<const Program*> TParentPrograms;
            INLINE const TParentPrograms& parentPrograms() const { return m_parentPrograms; }

			// get render states used by the program
			typedef base::Array<const StaticRenderStates*> TStaticRenderStates;
			INLINE const TStaticRenderStates& staticRenderStates() const { return m_staticRenderStates; }

            // get list of referenced descriptors 
            // NOTE: given descriptor may be NOT used in specific permutation, this is upper bound
            typedef base::Array<const ResourceTable*> TDescriptors;
            INLINE const TDescriptors& descriptors() const { return m_descriptors; }

            // get the name of the function
            INLINE base::StringID name() const { return m_name; }

            //---

            // add parent program (we will suck functions and constants from it)
            void addParentProgram(const Program* parentProgram);

            // add program parameter
            void addParameter(DataParameter* param);

            // add program function definition
            void addFunction(Function* func);

			// add render states to use with this program
			void addRenderStates(const StaticRenderStates* states);

            //---

            // check if this program depends on given other program
            bool isBasedOnProgram(const Program* program) const;

            //---

            // find attribute in the program by name
            // NOTE: may recurse to the parent programs
            const DataParameter* findParameter(const base::StringID name, bool recurseToParent = true) const;

            // find function in the program by name
            // NOTE: may recurse to the parent programs
            const Function* findFunction(const base::StringID function, bool recurseToParent = true) const;

            //---

            /*// refresh the CRC of the program, returns false the CRC cannot be computed
            bool refreshCRC();*/

            //---

            // build data param from a descriptor constant buffer field or resource
            const DataParameter* createDescriptorElementReference(const ResolvedDescriptorEntry& entry) const;

            // resolve a built-in parameter
            const DataParameter* createBuildinParameterReference(base::StringID name) const;

            //---

            // print to debug text
            void print(base::IFormatStream& f) const;

        private:
            base::StringID m_name; // a user-given program name
            mutable TParameters m_parameters; // all parameters of the program
            mutable TDescriptors m_descriptors; // all referenced descriptors
            TFunctions m_functions; // all program functions
            TParentPrograms m_parentPrograms; // parent programs we extend/implement
			TStaticRenderStates m_staticRenderStates; // static graphics pipeline configuration
            AttributeList m_attributes; // as defined in the shader code

            base::parser::Location m_loc; // location in source file

            const CodeLibrary* m_library; // source library this function was compiled as part of

            base::Array<base::parser::Location> m_refLocs;

			mutable base::HashMap<base::StringBuf, DataParameter*> m_descriptorConstantBufferEntriesMap;
			mutable base::HashMap<base::StringBuf, DataParameter*> m_descriptorResourceMap;

            friend class ParsingContext;
            friend class FunctionContext;
        };

        //---

    } // shader
} // rendering
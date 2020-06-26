/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\functions #]
***/

#include "build.h"
#include "renderingShaderFunction.h"
#include "renderingShaderNativeFunction.h"
#include "renderingShaderTypeUtils.h"
#include "renderingShaderTypeLibrary.h"

namespace rendering
{
    namespace compiler
    {

        //---

        static DataType PrepareGenericConstructorArgs(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err, BaseType baseTypeOverride, uint32_t expectedComponentCount = 0)
        {
            // we need at least one argument
            if (numArgs == 0)
            {
                err.reportError(loc, "Vector constructor requires at least one argument");
                return DataType();
            }

            // get the base type
            auto baseType = (baseTypeOverride != BaseType::Invalid) ? baseTypeOverride : ExtractBaseType(argTypes[0]);
            if (baseType != BaseType::Float && baseType != BaseType::Int && baseType != BaseType::Uint && baseType != BaseType::Boolean)
            {
                err.reportError(loc, "Vector constructor requires a valid numerical type as a base");
                return DataType();
            }

            // construction from single scalar
            if (numArgs == 1 && argTypes[0].isScalar() && expectedComponentCount)
                return typeLibrary.simpleCompositeType(baseType, expectedComponentCount);
            
            // collect components
            uint32_t numComponentsCollected = 0;
            for (uint32_t i = 0; i < numArgs; ++i)
            {
                auto type = argTypes[i];

                // we must be a conforming vector :)
                if (!argTypes[i].isNumericalVectorLikeOperand(true))
                {
                    err.reportError(loc, base::TempString("Vector constructor cannot accept '{}' as input type", type));
                    return DataType();
                }

                // get the number of components in the argument
                auto numComponents = ExtractComponentCount(type);
                if (!numComponents)
                {
                    err.reportError(loc, base::TempString("Vector constructor cannot accept type '{}' as argument becaues the component count cannot be determined", type));
                    return DataType();
                }

                // make sure we get casted to the base type (for conversion)
                argTypes[i] = GetCastedType(typeLibrary, type, baseType);
                numComponentsCollected += numComponents;
            }

            // we support only 1-4 components
            // TODO: we can make other counts work as well
            if (numComponentsCollected > 4)
            {
                err.reportError(loc, base::TempString("Vector constructor can accept up to 4 components, {} provided",  numComponentsCollected));
                return DataType();
            }

            // validate component count
            if (expectedComponentCount && numComponentsCollected < expectedComponentCount)
            {
                err.reportError(loc, base::TempString("Vector constructor expects {} components, {} provided", expectedComponentCount, numComponentsCollected));
                return DataType();
            }

            // create the output type
            return typeLibrary.simpleCompositeType(baseType, numComponentsCollected);
        }

        static void EvaluateGenericConstruction(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues, BaseType baseTypeOverride)
        {
            auto maxComponents = ExtractComponentCount(retData.type());
            ASSERT(maxComponents >= 1 && maxComponents <= 4);

            if (numArgs == 1)
            {
                auto numIncomingComponents = ExtractComponentCount(argValues[0].type());
                if (numIncomingComponents == 1)
                {
                    for (uint32_t j = 0; j < maxComponents; ++j)
                        retData.component(j, argValues[0].component(0));
                    return;
                }
            }

            uint32_t writeIndex = 0;
            for (uint32_t i = 0; i < numArgs; ++i)
            {
                auto numIncomingComponents = ExtractComponentCount(argValues[i].type());
                ASSERT(numIncomingComponents >= 1 && numIncomingComponents <= 4);

                for (uint32_t j = 0; j < numIncomingComponents; ++j)
                {
                    if (writeIndex < maxComponents)
                    {
                        retData.component(writeIndex, argValues[i].component(j));
                        writeIndex += 1;
                    }
                }
            }
        }

        //--

        static DataType PrepareMatrixConstructorArgs(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err, BaseType baseTypeOverride, uint32_t expectedComponentCount, uint32_t expectedRowCount)
        {
            // we need at least one argument
            if (numArgs == 0)
            {
                err.reportError(loc, "Vector constructor requires at least one argument");
                return DataType();
            }

            // get the base type
            auto baseType = (baseTypeOverride != BaseType::Invalid) ? baseTypeOverride : ExtractBaseType(argTypes[0]);
            if (baseType != BaseType::Float && baseType != BaseType::Int && baseType != BaseType::Uint && baseType != BaseType::Boolean)
            {
                err.reportError(loc, "Vector constructor requires a valid numerical type as a base");
                return DataType();
            }

            // construction from single scalar creates a trace matrix
            if (numArgs == 1 && argTypes[0].isScalar() && expectedComponentCount)
                return typeLibrary.simpleCompositeType(baseType, expectedComponentCount, expectedRowCount);

            // collect components
            uint32_t numComponentsCollected = 0;
            for (uint32_t i = 0; i < numArgs; ++i)
            {
                auto type = argTypes[i];

                // we must be a conforming vector :)
                if (!argTypes[i].isNumericalVectorLikeOperand(true))
                {
                    err.reportError(loc, base::TempString("Vector constructor cannot accept '{}' as input type", type));
                    return DataType();
                }

                // get the number of components in the argument
                auto numComponents = ExtractComponentCount(type);
                if (!numComponents)
                {
                    err.reportError(loc, base::TempString("Vector constructor cannot accept type '{}' as argument becaues the component count cannot be determined", type));
                    return DataType();
                }

                // make sure we get casted to the base type (for conversion)
                argTypes[i] = GetCastedType(typeLibrary, type, baseType);
                numComponentsCollected += numComponents;
            }

            // we support only 1-4 components
            // TODO: we can make other counts work as well
            const auto maxComponents = expectedRowCount * expectedComponentCount;
            if (numComponentsCollected > maxComponents)
            {
                err.reportError(loc, base::TempString("Matrix constructor of this type can accept up to {} components, {} provided", maxComponents, numComponentsCollected));
                return DataType();
            }

            // create the output type
            return typeLibrary.simpleCompositeType(baseType, expectedComponentCount, expectedRowCount);
        }

        //--

        // generic vector creation with arbitrary number of parameters
        class FunctionCreateVec : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionCreateVec, INativeFunction);

        public:
            FunctionCreateVec(BaseType baseTypeOverride = BaseType::Invalid, uint32_t numExpectedComponents = 0)
                : m_baseTypeOverride(baseTypeOverride)
                , m_numExpectedComponents(numExpectedComponents)
            {}

            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return PrepareGenericConstructorArgs(typeLibrary, numArgs, argTypes, loc, err, m_baseTypeOverride, m_numExpectedComponents);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                EvaluateGenericConstruction(retData, numArgs, argValues, m_baseTypeOverride);
            }

        private:
            BaseType m_baseTypeOverride;
            uint32_t m_numExpectedComponents;
        };

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(FunctionCreateVec);
        RTTI_END_TYPE();

        //--

        // generic matrix creation with arbitrary number of parameters
        class FunctionCreateMatrix : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionCreateMatrix, INativeFunction);

        public:
            FunctionCreateMatrix(BaseType baseTypeOverride = BaseType::Invalid, uint32_t numExpectedComponents = 0, uint32_t numExpectedRows = 0)
                : m_baseTypeOverride(baseTypeOverride)
                , m_numExpectedComponents(numExpectedComponents)
                , m_numExpectedRows(numExpectedRows)
            {}

            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return PrepareMatrixConstructorArgs(typeLibrary, numArgs, argTypes, loc, err, m_baseTypeOverride, m_numExpectedComponents, m_numExpectedRows);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                EvaluateGenericConstruction(retData, numArgs, argValues, m_baseTypeOverride);
            }

        private:
            BaseType m_baseTypeOverride;
            uint32_t m_numExpectedComponents;
            uint32_t m_numExpectedRows;
        };

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(FunctionCreateMatrix);
        RTTI_END_TYPE();

        //--

        // generic vector creation with arbitrary number of parameters
        class FunctionCreateFloat2 : public FunctionCreateVec
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionCreateFloat2, FunctionCreateVec);

        public:
            FunctionCreateFloat2()
                : FunctionCreateVec(BaseType::Float, 2)
            {}
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionCreateFloat2);
            RTTI_METADATA(FunctionNameMetadata).name("__create_vec2");
        RTTI_END_TYPE();

        //--

        // generic vector creation with arbitrary number of parameters
        class FunctionCreateFloat3 : public FunctionCreateVec
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionCreateFloat3, FunctionCreateVec);

        public:
            FunctionCreateFloat3()
                : FunctionCreateVec(BaseType::Float, 3)
            {}
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionCreateFloat3);
            RTTI_METADATA(FunctionNameMetadata).name("__create_vec3");
        RTTI_END_TYPE();

        //--

        // generic vector creation with arbitrary number of parameters
        class FunctionCreateFloat4 : public FunctionCreateVec
        {
         RTTI_DECLARE_VIRTUAL_CLASS(FunctionCreateFloat4, FunctionCreateVec);

        public:
            FunctionCreateFloat4()
                : FunctionCreateVec(BaseType::Float, 4)
            {}
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionCreateFloat4);
            RTTI_METADATA(FunctionNameMetadata).name("__create_vec4");
        RTTI_END_TYPE();

        //--

        // generic vector creation with arbitrary number of parameters
        class FunctionCreateInt2 : public FunctionCreateVec
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionCreateInt2, FunctionCreateVec);

        public:
            FunctionCreateInt2()
                : FunctionCreateVec(BaseType::Int, 2)
            {}
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionCreateInt2);
            RTTI_METADATA(FunctionNameMetadata).name("__create_ivec2");
        RTTI_END_TYPE();

        //--

        // generic vector creation with arbitrary number of parameters
        class FunctionCreateInt3 : public FunctionCreateVec
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionCreateInt3, FunctionCreateVec);

        public:
            FunctionCreateInt3()
                : FunctionCreateVec(BaseType::Int, 3)
            {}
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionCreateInt3);
            RTTI_METADATA(FunctionNameMetadata).name("__create_ivec3");
        RTTI_END_TYPE();

        //--

        // generic vector creation with arbitrary number of parameters
        class FunctionCreateInt4 : public FunctionCreateVec
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionCreateInt4, FunctionCreateVec);

        public:
            FunctionCreateInt4()
                : FunctionCreateVec(BaseType::Int, 4)
            {}
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionCreateInt4);
            RTTI_METADATA(FunctionNameMetadata).name("__create_ivec4");
        RTTI_END_TYPE();

        //--

        // generic vector creation with arbitrary number of parameters
        class FunctionCreateUint2 : public FunctionCreateVec
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionCreateUint2, FunctionCreateVec);

        public:
            FunctionCreateUint2()
                : FunctionCreateVec(BaseType::Uint, 2)
            {}
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionCreateUint2);
            RTTI_METADATA(FunctionNameMetadata).name("__create_uvec2");
        RTTI_END_TYPE();

        //--

        // generic vector creation with arbitrary number of parameters
        class FunctionCreateUint3 : public FunctionCreateVec
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionCreateUint3, FunctionCreateVec);

        public:
            FunctionCreateUint3()
                : FunctionCreateVec(BaseType::Uint, 3)
            {}
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionCreateUint3);
            RTTI_METADATA(FunctionNameMetadata).name("__create_uvec3");
        RTTI_END_TYPE();

        //--

        // generic vector creation with arbitrary number of parameters
        class FunctionCreateUint4 : public FunctionCreateVec
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionCreateUint4, FunctionCreateVec);

        public:
            FunctionCreateUint4()
                : FunctionCreateVec(BaseType::Uint, 4)
            {}
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionCreateUint4);
            RTTI_METADATA(FunctionNameMetadata).name("__create_uvec4");
        RTTI_END_TYPE();

        //--

        // generic matrix creation with arbitrary number of parameters
        class FunctionCreateMat2 : public FunctionCreateMatrix
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionCreateMat2, FunctionCreateMatrix);

        public:
            FunctionCreateMat2()
                : FunctionCreateMatrix(BaseType::Float, 2, 2)
            {}
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionCreateMat2);
            RTTI_METADATA(FunctionNameMetadata).name("__create_mat2");
        RTTI_END_TYPE();

        //--

        // generic matrix creation with arbitrary number of parameters
        class FunctionCreateMat3 : public FunctionCreateMatrix
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionCreateMat3, FunctionCreateMatrix);

        public:
            FunctionCreateMat3()
                : FunctionCreateMatrix(BaseType::Float, 3, 3)
            {}
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionCreateMat3);
        RTTI_METADATA(FunctionNameMetadata).name("__create_mat3");
        RTTI_END_TYPE();

        //--

        // generic matrix creation with arbitrary number of parameters
        class FunctionCreateMat4 : public FunctionCreateMatrix
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionCreateMat4, FunctionCreateMatrix);

        public:
            FunctionCreateMat4()
                : FunctionCreateMatrix(BaseType::Float, 4, 4)
            {}
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionCreateMat4);
        RTTI_METADATA(FunctionNameMetadata).name("__create_mat4");
        RTTI_END_TYPE();

        //--

        // vector creation with override for float
        class FunctionCreateVecFloat : public FunctionCreateVec
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionCreateVecFloat, FunctionCreateVec);

        public:
            FunctionCreateVecFloat()
                : FunctionCreateVec(BaseType::Float)
            {}
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionCreateVecFloat);
            RTTI_METADATA(FunctionNameMetadata).name("vec__float");
        RTTI_END_TYPE();

        //--

        // vector creation with override for int
        class FunctionCreateVecInt : public FunctionCreateVec
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionCreateVecInt, FunctionCreateVec);

        public:
            FunctionCreateVecInt()
                : FunctionCreateVec(BaseType::Int)
            {}
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionCreateVecInt);
        RTTI_METADATA(FunctionNameMetadata).name("vec__int");
        RTTI_END_TYPE();

        //--

        // vector creation with override for uint
        class FunctionCreateVecUint : public FunctionCreateVec
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionCreateVecUint, FunctionCreateVec);

        public:
            FunctionCreateVecUint()
                : FunctionCreateVec(BaseType::Uint)
            {}
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionCreateVecUint);
        RTTI_METADATA(FunctionNameMetadata).name("vec__uint");
        RTTI_END_TYPE();

        //--

        // vector creation with override for uint
        class FunctionCreateVecBool : public FunctionCreateVec
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionCreateVecBool, FunctionCreateVec);

        public:
            FunctionCreateVecBool()
                : FunctionCreateVec(BaseType::Boolean)
            {}
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionCreateVecBool);
        RTTI_METADATA(FunctionNameMetadata).name("vec__bool");
        RTTI_END_TYPE();

        //---

        static DataType PrepareArrayConstructorArgs(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err)
        {
            // we need at least one argument
            if (numArgs == 0)
            {
                err.reportError(loc, "Array constructor requires at least one argument");
                return DataType();
            }

            // get the type of element in the array
            auto elementType = argTypes[0].unmakePointer();

            // setup elements to be compatible with the first type
            for (uint32_t i = 1; i < numArgs; ++i)
                argTypes[i] = elementType;

            // create the output type
            return elementType.applyArrayCounts(ArrayCounts(numArgs));
        }

        static void EvaluateArrayConstruction(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues)
        {
            auto numComponentsPerElement = ExtractComponentCount(retData.type().removeArrayCounts());
            auto maxComponents = numComponentsPerElement * numArgs;

            uint32_t writeIndex = 0;
            for (uint32_t i = 0; i < numArgs; ++i)
            {
                auto numIncomingComponents = ExtractComponentCount(argValues[i].type());
                ASSERT(numIncomingComponents == numComponentsPerElement);

                for (uint32_t j = 0; j < numIncomingComponents; ++j)
                {
                    retData.component(writeIndex, argValues[i].component(j));
                    writeIndex += 1;
                }
            }
        }

        // generic vector creation with arbitrary number of parameters
        class FunctionCreateArray : public INativeFunction
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FunctionCreateArray, INativeFunction);

        public:
            FunctionCreateArray()
            {}

            virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const override final
            {
                return PrepareArrayConstructorArgs(typeLibrary, numArgs, argTypes, loc, err);
            }

            virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
            {
                EvaluateArrayConstruction(retData, numArgs, argValues);
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FunctionCreateArray);
            RTTI_METADATA(FunctionNameMetadata).name("array");
        RTTI_END_TYPE();

    } // shader
} // rendering


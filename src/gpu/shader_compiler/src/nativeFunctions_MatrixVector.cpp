/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\functions #]
***/

#include "build.h"
#include "function.h"
#include "nativeFunction.h"
#include "typeUtils.h"
#include "typeLibrary.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::compiler)

//---

/// stub function for multiplication that gets mutated into different functions
class FunctionMulStub : public INativeFunction
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionMulStub, INativeFunction);

public:
    virtual const INativeFunction* mutateFunction(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const parser::Location& loc, parser::IErrorReporter& err) const override final
    {
        if (numArgs == 2)
        {
            auto& leftType = argTypes[0];
            auto& rightType = argTypes[1];

            if (leftType.isScalar() && rightType.isMatrix())
            {
                return FindFunctionByName("__smmul");
            }
            else if (leftType.isScalar() && rightType.isVector())
            {
                return FindFunctionByName("__svmul");
            }
            else if (leftType.isVector() && rightType.isScalar())
            {
                return FindFunctionByName("__vsmul");
            }
            else if (leftType.isMatrix() && rightType.isScalar())
            {
                return FindFunctionByName("__msmul");
            }
            else if (leftType.isMatrix() && rightType.isVector())
            {
                return FindFunctionByName("__mvmul");
            }
            else if (leftType.isVector() && rightType.isMatrix())
            {
                return FindFunctionByName("__vmmul");
            }
            else if (leftType.isMatrix() && rightType.isMatrix())
            {
                return FindFunctionByName("__mmmul");
            }

            err.reportError(loc, TempString("Unable to determine the nature of multiplication with types '{}' and '{}'", leftType, rightType));
            return nullptr;
        }
        else
        {
            err.reportError(loc, "Multiplication requires 2 arguments");
            return nullptr;
        }
    }

    virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const parser::Location& loc, parser::IErrorReporter& err) const override final
    {
        err.reportError(loc, "Unable to determine the nature of multiplication in this expression");
        return DataType();
    }

    virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
    {
    }
};

RTTI_BEGIN_TYPE_CLASS(FunctionMulStub);
RTTI_METADATA(FunctionNameMetadata).name("mul");
RTTI_END_TYPE();

//---

/// matrix * matrix
/// matNM = N rows M columns
/// matPQ = P rows Q columns
/// matNM * matPQ = matNQ, N rows, Q columns, M and P must equal (left columns and right rows)
class FunctionMatrixMulMatrix : public INativeFunction
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionMatrixMulMatrix, INativeFunction);

public:
    virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const parser::Location& loc, parser::IErrorReporter& err) const override final
    {
        // math uses const values
        argTypes[0] = argTypes[0].unmakePointer();
        argTypes[1] = argTypes[1].unmakePointer();

        // we must have 2 arguments here
        if (numArgs != 2)
        {
            err.reportError(loc, "Binary function requires 2 arguments");
            return DataType();
        }

        // validate base type of operands
        if (!argTypes[0].isNumericalMatrixLikeOperand())
        {
            err.reportError(loc, TempString("Matrix/Vector multiplication works only with numerical types, type '{}' is not compatible", argTypes[0]));
            return DataType();
        }
        else  if (!argTypes[1].isNumericalMatrixLikeOperand())
        {
            err.reportError(loc, TempString("Matrix/Vector multiplication works only with numerical types, type '{}' is not compatible", argTypes[1]));
            return DataType();
        }

        // get the left
        auto leftCols = ExtractComponentCount(argTypes[0]);
        auto leftRows = ExtractRowCount(argTypes[0]);
        auto rightCols = ExtractComponentCount(argTypes[1]);
        auto rightRows = ExtractRowCount(argTypes[1]);

        // this function should not be called with vectors
        if (leftRows == 1 || leftCols == 1 || rightRows == 1 || leftRows == 1)
            err.reportWarning(loc, TempString("Matrix multiplication function called with semi-vectors: {}x{} and {}x{}", leftRows, leftCols, rightRows, rightCols));

        // well, check the math rules
        if (leftCols != rightRows)
        {
            err.reportError(loc, TempString("Matrix {}x{} cannot be multiplied with matrix {}x{} (blame math, not me)", leftRows, leftCols, rightRows, rightCols));
            return DataType();
        }

        // establish the output type AxB * BxC -> AxC
        auto outCols = leftRows;
        auto outRows = rightCols;
        argTypes[0] = typeLibrary.simpleCompositeType(BaseType::Float, leftCols, leftRows);
        argTypes[1] = typeLibrary.simpleCompositeType(BaseType::Float, rightCols, rightRows);
        return typeLibrary.simpleCompositeType(BaseType::Float, outCols, outRows);
    }

    virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
    {
        auto baseType = ExtractBaseType(retData.type());
        ASSERT(baseType == BaseType::Float);

        // extract argument sizes
        auto outCols = ExtractComponentCount(retData.type());
        auto outRows = ExtractRowCount(retData.type());
        auto rightRows = ExtractRowCount(argValues[1].type());
        auto rightCols = ExtractComponentCount(argValues[1].type());
        auto leftRows = ExtractRowCount(argValues[0].type());
        auto leftCols = ExtractComponentCount(argValues[0].type());

        // get the inner dot product sizes
        ASSERT(leftCols == rightRows);
        auto dotProductSize = leftCols; // number of elements contracted

        // do the matrix multiplication (works for vectors to)
        // rows of left with columns of right
        ASSERT(outCols > 0 && outRows > 0 && outCols <= 4 && outRows <= 4);
        ASSERT(outRows == leftRows);
        ASSERT(outCols == rightCols);
        for (uint32_t row = 0; row < outRows; ++row)
        {
            for (uint32_t col = 0; col < outCols; ++col)
            {
                DataValueComponent sum(0.0f);

                for (uint32_t i = 0; i < dotProductSize; ++i)
                {
                    auto leftValue = argValues[0].matrixComponent(leftCols, leftRows, i, row);
                    auto rightValue = argValues[1].matrixComponent(rightCols, rightRows, col, i);
                    sum = valop::FAdd(sum, valop::FMul(leftValue, rightValue));
                    if (!sum.isDefined())
                        break; // optimization
                }

                retData.matrixComponent(outCols, outRows, col, row, sum);
            }
        }
    }
};

RTTI_BEGIN_TYPE_CLASS(FunctionMatrixMulMatrix);
    RTTI_METADATA(FunctionNameMetadata).name("__mmmul");
RTTI_END_TYPE();

//---

/// vector * matrix
/// vec4 * mat4x4 => mat1x4 * mat4x4 => mat1x4 => vec4
/// vec4 * mat4x3 => mat1x4 * mat4x3 => mat1x3 => vec3
/// vec3 * mat3x4 => mat1x3 * mat3x4 => mat1x4 => vec4
class FunctionVectorMulMatrix : public INativeFunction
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionVectorMulMatrix, INativeFunction);

public:
    virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const parser::Location& loc, parser::IErrorReporter& err) const override final
    {
        // math uses const values
        argTypes[0] = argTypes[0].unmakePointer();
        argTypes[1] = argTypes[1].unmakePointer();

        // we must have 2 arguments here
        if (numArgs != 2)
        {
            err.reportError(loc, "Binary function requires 2 arguments");
            return DataType();
        }

        // validate base type of operands
        if (!argTypes[0].isNumericalVectorLikeOperand())
        {
            err.reportError(loc, TempString("Matrix/Vector multiplication works only with numerical types, type '{}' is not compatible", argTypes[0]));
            return DataType();
        }
        else  if (!argTypes[1].isNumericalMatrixLikeOperand())
        {
            err.reportError(loc, TempString("Matrix/Vector multiplication works only with numerical types, type '{}' is not compatible", argTypes[1]));
            return DataType();
        }

        // get the left
        if (1 != ExtractRowCount(argTypes[0]))
        {
            err.reportError(loc, "Left hand is expected to be a vector");
            return DataType();
        }

        auto vectorSize = ExtractComponentCount(argTypes[0]);
        auto rightCols = ExtractComponentCount(argTypes[1]);
        auto rightRows = ExtractRowCount(argTypes[1]);

        // well, check the math rules
        if (vectorSize != rightCols)
        {
            err.reportError(loc, TempString("Vector with {} components cannot be multiplied with matrix {}x{} (blame math, not me)", vectorSize, rightRows, rightCols));
            return DataType();
        }

        // establish the output type AxB * BxC -> AxC
        argTypes[0] = typeLibrary.simpleCompositeType(BaseType::Float, rightCols);
        argTypes[1] = typeLibrary.simpleCompositeType(BaseType::Float, rightCols, rightRows);
        return typeLibrary.simpleCompositeType(BaseType::Float, rightRows); // note: we get vec4 * mat43 => vec3 (math works)
    }

    virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
    {
        auto baseType = ExtractBaseType(retData.type());
        ASSERT(baseType == BaseType::Float);

        // extract argument sizes
        auto outComponents = ExtractComponentCount(retData.type());
        ASSERT(ExtractRowCount(retData.type()) == 1);

        auto rightRows = ExtractRowCount(argValues[1].type());
        auto rightCols = ExtractComponentCount(argValues[1].type());

        auto leftComponents = ExtractComponentCount(argValues[0].type());
        ASSERT(ExtractRowCount(argValues[0].type()) == 1);

        for (uint32_t comp = 0; comp < outComponents; ++comp)
        {
            DataValueComponent sum(0.0f);

            for (uint32_t i = 0; i < leftComponents; ++i)
            {
                auto leftValue = argValues[0].component(i);
                auto rightValue = argValues[1].matrixComponent(rightCols, rightRows, comp, i); // dot the vector with the row of the matrix, the result is the value for the component
                sum = valop::FAdd(sum, valop::FMul(leftValue, rightValue));
                if (!sum.isDefined())
                    break; // optimization
            }

            retData.component(comp, sum);
        }
    }
};

RTTI_BEGIN_TYPE_CLASS(FunctionVectorMulMatrix);
RTTI_METADATA(FunctionNameMetadata).name("__vmmul");
RTTI_END_TYPE();

//---

/// matrix * vector (vector is row vector)
/// mat4x4 * vec4 => mat4x4 * mat4x1 => mat4x1 => vec4
/// mat4x3 * vec3 => mat4x3 * mat3x1 => mat4x1 => vec4
/// mat3x4 * vec4 => mat3x4 * mat4x1 => mat3x1 => vec3
class FunctionMatrixMulVector : public INativeFunction
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionMatrixMulVector, INativeFunction);

public:
    virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const parser::Location& loc, parser::IErrorReporter& err) const override final
    {
        // math uses const values
        argTypes[0] = argTypes[0].unmakePointer();
        argTypes[1] = argTypes[1].unmakePointer();

        // we must have 2 arguments here
        if (numArgs != 2)
        {
            err.reportError(loc, "Binary function requires 2 arguments");
            return DataType();
        }

        // validate base type of operands
        if (!argTypes[0].isNumericalMatrixLikeOperand())
        {
            err.reportError(loc, TempString("Matrix/Vector multiplication works only with numerical types, type '{}' is not compatible", argTypes[0]));
            return DataType();
        }
        else  if (!argTypes[1].isNumericalVectorLikeOperand())
        {
            err.reportError(loc, TempString("Matrix/Vector multiplication works only with numerical types, type '{}' is not compatible", argTypes[1]));
            return DataType();
        }

        // get the left
        if (1 != ExtractRowCount(argTypes[1]))
        {
            err.reportError(loc, "Right hand is expected to be a vector");
            return DataType();
        }

        auto vectorSize = ExtractComponentCount(argTypes[1]);
        auto leftCols = ExtractComponentCount(argTypes[0]);
        auto leftRows = ExtractRowCount(argTypes[0]);

        // well, check the math rules
        if (vectorSize != leftCols)
        {
            err.reportError(loc, TempString("Matrix {}x{} cannot be multiplied with vector with {} components (blame math, not me)", leftRows, leftCols, vectorSize));
            return DataType();
        }

        // establish the output type AxB * BxC -> AxC
        argTypes[0] = typeLibrary.simpleCompositeType(BaseType::Float, leftCols, leftRows);
        argTypes[1] = typeLibrary.simpleCompositeType(BaseType::Float, leftCols);
        return typeLibrary.simpleCompositeType(BaseType::Float, leftRows); // note: we get mat43 * vec3 => vec4
    }

    virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
    {
        auto baseType = ExtractBaseType(retData.type());
        ASSERT(baseType == BaseType::Float);

        // extract argument sizes
        auto outComponents = ExtractComponentCount(retData.type());
        ASSERT(ExtractRowCount(retData.type()) == 1);

        auto leftRows = ExtractRowCount(argValues[0].type());
        auto leftCols = ExtractComponentCount(argValues[0].type());

        auto rightComponents = ExtractComponentCount(argValues[1].type());
        ASSERT(ExtractRowCount(argValues[1].type()) == 1);

        for (uint32_t comp = 0; comp < outComponents; ++comp)
        {
            DataValueComponent sum(0.0f);

            for (uint32_t i = 0; i < rightComponents; ++i)
            {
                auto leftValue = argValues[0].matrixComponent(leftCols, leftRows, comp, i);
                auto rightValue = argValues[1].component(i);
                sum = valop::FAdd(sum, valop::FMul(leftValue, rightValue));
                if (!sum.isDefined())
                    break; // optimization
            }

            retData.component(comp, sum);
        }
    }
};

RTTI_BEGIN_TYPE_CLASS(FunctionMatrixMulVector);
RTTI_METADATA(FunctionNameMetadata).name("__mvmul");
RTTI_END_TYPE();

//---

/// vector * scalar, special (hopefuly optimized) function
class FunctionVectorMulScalar : public INativeFunction
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionVectorMulScalar, INativeFunction);

public:
    virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const parser::Location& loc, parser::IErrorReporter& err) const override final
    {
        // math uses const values
        argTypes[0] = argTypes[0].unmakePointer();
        argTypes[1] = argTypes[1].unmakePointer();

        // we must have 2 arguments here
        if (numArgs != 2)
        {
            err.reportError(loc, "Binary function requires 2 arguments");
            return DataType();
        }

        // get the base type of the operands
        if (!argTypes[0].isNumericalVectorLikeOperand())
        {
            err.reportError(loc, TempString("Matrix/Vector multiplication works only with numerical types, type '{}' is not compatible", argTypes[0]));
            return DataType();
        }
        else  if (!argTypes[1].isNumericalScalar())
        {
            err.reportError(loc, TempString("Matrix/Vector multiplication works only with numerical types, type '{}' is not compatible", argTypes[1]));
            return DataType();
        }

        // right is expected to be a vector
        if (1 != ExtractRowCount(argTypes[1]))
        {
            err.reportError(loc, "Right hand is expected to be a vector");
            return DataType();
        }

        // left hand is expected to be a scalar
        if (!argTypes[1].isScalar())
        {
            err.reportError(loc, "Left hand is expected to be a vector");
            return DataType();
        }

        // scalar multiplication does not change the type size
        auto vectorSize = ExtractComponentCount(argTypes[0]);
        argTypes[0] = typeLibrary.simpleCompositeType(BaseType::Float, vectorSize);
        argTypes[1] = DataType::FloatScalarType();
        return typeLibrary.simpleCompositeType(BaseType::Float, vectorSize);
    }

    virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
    {
        auto baseType = ExtractBaseType(retData.type());
        ASSERT(baseType == BaseType::Float);

        // extract argument sizes
        auto outComponents = ExtractComponentCount(retData.type());
        ASSERT(ExtractRowCount(retData.type()) == 1);
        ASSERT(ExtractRowCount(argValues[0].type()) == 1);
        ASSERT(ExtractComponentCount(argValues[0].type()) == outComponents);
        ASSERT(ExtractRowCount(argValues[1].type()) == 1);
        ASSERT(ExtractComponentCount(argValues[1].type()) == 1);

        for (uint32_t comp = 0; comp < outComponents; ++comp)
        {
            auto leftValue = argValues[0].component(comp);
            auto rightValue = argValues[1].component(0);
            retData.component(comp, valop::FMul(leftValue, rightValue));
        }
    }
};

RTTI_BEGIN_TYPE_CLASS(FunctionVectorMulScalar);
    RTTI_METADATA(FunctionNameMetadata).name("__vsmul");
RTTI_END_TYPE();

//---

///  scalar * vector, special (hopefuly optimized) function
class FunctionScalarMulVector : public INativeFunction
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionScalarMulVector, INativeFunction);

public:
    virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const parser::Location& loc, parser::IErrorReporter& err) const override final
    {
        // math uses const values
        argTypes[0] = argTypes[0].unmakePointer();
        argTypes[1] = argTypes[1].unmakePointer();

        // we must have 2 arguments here
        if (numArgs != 2)
        {
            err.reportError(loc, "Binary function requires 2 arguments");
            return DataType();
        }

        // get the base type of the operands
        if (!argTypes[0].isNumericalScalar())
        {
            err.reportError(loc, TempString("Matrix/Vector multiplication works only with numerical types, type '{}' is not compatible", argTypes[0]));
            return DataType();
        }
        else  if (!argTypes[1].isNumericalVectorLikeOperand())
        {
            err.reportError(loc, TempString("Matrix/Vector multiplication works only with numerical types, type '{}' is not compatible", argTypes[1]));
            return DataType();
        }

        // right is expected to be a vector
        if (1 != ExtractRowCount(argTypes[0]))
        {
            err.reportError(loc, "Left hand is expected to be a vector");
            return DataType();
        }

        // left hand is expected to be a scalar
        if (!argTypes[0].isScalar())
        {
            err.reportError(loc, "Right hand is expected to be a vector");
            return DataType();
        }

        // scalar multiplication does not change the type size
        auto vectorSize = ExtractComponentCount(argTypes[1]);
        argTypes[0] = DataType::FloatScalarType();
        argTypes[1] = typeLibrary.simpleCompositeType(BaseType::Float, vectorSize);
        return typeLibrary.simpleCompositeType(BaseType::Float, vectorSize);
    }

    virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
    {
        auto baseType = ExtractBaseType(retData.type());
        ASSERT(baseType == BaseType::Float);

        // extract argument sizes
        auto outComponents = ExtractComponentCount(retData.type());
        ASSERT(ExtractRowCount(retData.type()) == 1);
        ASSERT(ExtractRowCount(argValues[0].type()) == 1);
        ASSERT(ExtractComponentCount(argValues[0].type()) == 1);
        ASSERT(ExtractRowCount(argValues[1].type()) == 1);
        ASSERT(ExtractComponentCount(argValues[1].type()) == outComponents);

        for (uint32_t comp = 0; comp < outComponents; ++comp)
        {
            auto leftValue = argValues[0].component(0);
            auto rightValue = argValues[1].component(comp);
            retData.component(comp, valop::FMul(leftValue, rightValue));
        }
    }
};

RTTI_BEGIN_TYPE_CLASS(FunctionScalarMulVector);
RTTI_METADATA(FunctionNameMetadata).name("__svmul");
RTTI_END_TYPE();

//---

/// matrix * scalar, special (hopefuly optimized) function
class FunctionMatrixMulScalar : public INativeFunction
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionMatrixMulScalar, INativeFunction);

public:
    virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const parser::Location& loc, parser::IErrorReporter& err) const override final
    {
        // math uses const values
        argTypes[0] = argTypes[0].unmakePointer();
        argTypes[1] = argTypes[1].unmakePointer();

        // we must have 2 arguments here
        if (numArgs != 2)
        {
            err.reportError(loc, "Binary function requires 2 arguments");
            return DataType();
        }

        // get the base type of the operands
        if (!argTypes[0].isNumericalMatrixLikeOperand())
        {
            err.reportError(loc, TempString("Matrix/Vector multiplication works only with numerical types, type '{}' is not compatible", argTypes[0]));
            return DataType();
        }
        else  if (!argTypes[1].isNumericalScalar())
        {
            err.reportError(loc, TempString("Matrix/Vector multiplication works only with numerical types, type '{}' is not compatible", argTypes[1]));
            return DataType();
        }

        // left hand is expected to be a scalar
        if (!argTypes[1].isScalar())
        {
            err.reportError(loc, "Left hand is expected to be a vector");
            return DataType();
        }

        // scalar multiplication does not change the type size
        auto outRows = ExtractRowCount(argTypes[0]);
        auto outCols = ExtractComponentCount(argTypes[0]);
        argTypes[1] = DataType::FloatScalarType();
        argTypes[0] = typeLibrary.simpleCompositeType(BaseType::Float, outCols, outRows);
        return typeLibrary.simpleCompositeType(BaseType::Float, outCols, outRows);
    }

    virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
    {
        auto baseType = ExtractBaseType(retData.type());
        ASSERT(baseType == BaseType::Float);

        // extract argument sizes
        auto outRows = ExtractRowCount(retData.type());
        auto outCols = ExtractComponentCount(retData.type());
        ASSERT(ExtractRowCount(argValues[0].type()) == outRows);
        ASSERT(ExtractComponentCount(argValues[0].type()) == outCols);
        ASSERT(ExtractRowCount(argValues[1].type()) == 1);
        ASSERT(ExtractComponentCount(argValues[1].type()) == 1);

        for (uint32_t row = 0; row < outRows; ++row)
        {
            for (uint32_t col = 0; col < outCols; ++col)
            {
                auto leftValue = argValues[0].matrixComponent(outCols, outRows, col, row);
                auto rightValue = argValues[1].component(0);
                retData.matrixComponent(outCols, outRows, col, row, valop::FMul(leftValue, rightValue));
            }
        }
    }
};

RTTI_BEGIN_TYPE_CLASS(FunctionMatrixMulScalar);
    RTTI_METADATA(FunctionNameMetadata).name("__msmul");
RTTI_END_TYPE();

//---

/// scalar * matrix, special (hopefuly optimized) function
class FunctionScalarMulMatrix : public INativeFunction
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionScalarMulMatrix, INativeFunction);

public:
    virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const parser::Location& loc, parser::IErrorReporter& err) const override final
    {
        // math uses const values
        argTypes[0] = argTypes[0].unmakePointer();
        argTypes[1] = argTypes[1].unmakePointer();

        // we must have 2 arguments here
        if (numArgs != 2)
        {
            err.reportError(loc, "Binary function requires 2 arguments");
            return DataType();
        }

        // get the base type of the operands
        if (!argTypes[0].isNumericalScalar())
        {
            err.reportError(loc, TempString("Matrix/Vector multiplication works only with numerical types, type '{}' is not compatible", argTypes[0]));
            return DataType();
        }
        else  if (!argTypes[1].isNumericalMatrixLikeOperand())
        {
            err.reportError(loc, TempString("Matrix/Vector multiplication works only with numerical types, type '{}' is not compatible", argTypes[1]));
            return DataType();
        }

        // left hand is expected to be a scalar
        if (!argTypes[0].isScalar())
        {
            err.reportError(loc, "Left hand is expected to be a vector");
            return DataType();
        }

        // scalar multiplication does not change the type size
        auto outRows = ExtractRowCount(argTypes[1]);
        auto outCols = ExtractComponentCount(argTypes[1]);
        argTypes[0] = DataType::FloatScalarType();
        argTypes[1] = typeLibrary.simpleCompositeType(BaseType::Float, outCols, outRows);
        return typeLibrary.simpleCompositeType(BaseType::Float, outCols, outRows);
    }

    virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
    {
        auto baseType = ExtractBaseType(retData.type());
        ASSERT(baseType == BaseType::Float);

        // extract argument sizes
        auto outRows = ExtractRowCount(retData.type());
        auto outCols = ExtractComponentCount(retData.type());
        ASSERT(ExtractRowCount(argValues[1].type()) == outRows);
        ASSERT(ExtractComponentCount(argValues[1].type()) == outCols);
        ASSERT(ExtractRowCount(argValues[0].type()) == 1);
        ASSERT(ExtractComponentCount(argValues[0].type()) == 1);

        for (uint32_t row = 0; row < outRows; ++row)
        {
            for (uint32_t col = 0; col < outCols; ++col)
            {
                auto leftValue = argValues[0].component(0);
                auto rightValue = argValues[1].matrixComponent(outCols, outRows, col, row);
                retData.matrixComponent(outCols, outRows, col, row, valop::FMul(leftValue, rightValue));
            }
        }
    }
};

RTTI_BEGIN_TYPE_CLASS(FunctionScalarMulMatrix);
RTTI_METADATA(FunctionNameMetadata).name("__smmul");
RTTI_END_TYPE();

//---

/// dot product
class FunctionVectorDot : public INativeFunction
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionVectorDot, INativeFunction);

public:
    virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const parser::Location& loc, parser::IErrorReporter& err) const override final
    {
        // math uses const values
        argTypes[0] = argTypes[0].unmakePointer();
        argTypes[1] = argTypes[1].unmakePointer();

        // we must have 2 arguments here
        if (numArgs != 2)
        {
            err.reportError(loc, "Binary function requires 2 arguments");
            return DataType();
        }

        // get the base type of the operands
        auto baseType0 = ExtractBaseType(argTypes[0]);
        auto baseType1 = ExtractBaseType(argTypes[1]);
        if (!argTypes[0].isNumericalVectorLikeOperand() || !argTypes[1].isNumericalVectorLikeOperand())
        {
            err.reportError(loc, "Vector functions work only with numerical types");
            return DataType();
        }

        // only vectors
        if (ExtractRowCount(argTypes[0]) > 1 || ExtractRowCount(argTypes[1]) > 1)
        {
            err.reportError(loc, "Dot product is legal only between vectors");
            return DataType();
        }

        // normalize component count for both sides
        auto componentCount = ExtractComponentCount(argTypes[0]);
        argTypes[0] = typeLibrary.simpleCompositeType(BaseType::Float, componentCount).unmakePointer();
        argTypes[1] = typeLibrary.simpleCompositeType(BaseType::Float, componentCount).unmakePointer();
        return DataType::FloatScalarType();
    }

    virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
    {
        auto baseType = ExtractBaseType(retData.type());
        ASSERT(baseType == BaseType::Float);

        // extract argument sizes
        auto numComponents = ExtractComponentCount(argValues[0].type());
        ASSERT(ExtractRowCount(retData.type()) == 1);
        ASSERT(ExtractComponentCount(retData.type()) == 1);
        ASSERT(ExtractRowCount(argValues[1].type()) == 1);
        ASSERT(ExtractComponentCount(argValues[1].type()) == numComponents);
        ASSERT(ExtractRowCount(argValues[0].type()) == 1);
        ASSERT(ExtractComponentCount(argValues[0].type()) == numComponents);

        DataValueComponent sum(0.0f);
        for (uint32_t comp = 0; comp < numComponents; ++comp)
        {
            auto leftValue = argValues[0].component(comp);
            auto rightValue = argValues[1].component(comp);
            sum = valop::FAdd(sum, valop::FMul(leftValue, rightValue));
            if (!sum.isDefined())
                break; // optimization
        }
        retData.component(0,sum);
    }
};

RTTI_BEGIN_TYPE_CLASS(FunctionVectorDot);
    RTTI_METADATA(FunctionNameMetadata).name("dot");
RTTI_END_TYPE();

//---

/// cross product
class FunctionVectorCross : public INativeFunction
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionVectorCross, INativeFunction);

public:
    virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const parser::Location& loc, parser::IErrorReporter& err) const override final
    {
        // math uses const values
        argTypes[0] = argTypes[0].unmakePointer();
        argTypes[1] = argTypes[1].unmakePointer();

        // we must have 2 arguments here
        if (numArgs != 2)
        {
            err.reportError(loc, "Binary function requires 2 arguments");
            return DataType();
        }

        // get the base type of the operands
        auto baseType0 = ExtractBaseType(argTypes[0]);
        auto baseType1 = ExtractBaseType(argTypes[1]);
        if (!argTypes[0].isNumericalVectorLikeOperand() || !argTypes[1].isNumericalVectorLikeOperand())
        {
            err.reportError(loc, "Vector functions work only with numerical types");
            return DataType();
        }

        // only vectors
        if (ExtractRowCount(argTypes[0]) > 1 || ExtractRowCount(argTypes[1]) > 1)
        {
            err.reportError(loc, "Cross product is legal only between vectors");
            return DataType();
        }

        // normalize component count for both sides
        auto vectorType = typeLibrary.simpleCompositeType(BaseType::Float, 3).unmakePointer();
        argTypes[0] = vectorType;
        argTypes[1] = vectorType;
        return vectorType;
    }

    virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
    {
        // TODO
    }
};

RTTI_BEGIN_TYPE_CLASS(FunctionVectorCross);
    RTTI_METADATA(FunctionNameMetadata).name("cross");
RTTI_END_TYPE();

//---

/// length of a vector
class FunctionVectorLength : public INativeFunction
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionVectorLength, INativeFunction);

public:
    virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const parser::Location& loc, parser::IErrorReporter& err) const override final
    {
        // math uses const values
        argTypes[0] = argTypes[0].unmakePointer();

        // we must have 2 arguments here
        if (numArgs != 1)
        {
            err.reportError(loc, "Function requires 1 argument");
            return DataType();
        }

        if (!argTypes[0].isNumericalVectorLikeOperand())
        {
            err.reportError(loc, "Vector functions work only with numerical types");
            return DataType();
        }

        // matrices are not supported
        if (ExtractRowCount(argTypes[0]) != 1)
        {
            err.reportError(loc, "Math operation on matrices are not supported");
            return DataType();
        }

        // exact component count is expected
        auto numComponents = ExtractComponentCount(argTypes[0]);
        argTypes[0] = typeLibrary.simpleCompositeType(BaseType::Float, numComponents).unmakePointer();
        return DataType::FloatScalarType();
    }

    virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
    {
        auto baseType = ExtractBaseType(retData.type());
        ASSERT(baseType == BaseType::Float);

        // extract argument sizes
        auto numComponents = ExtractComponentCount(argValues[0].type());
        ASSERT(ExtractRowCount(retData.type()) == 1);
        ASSERT(ExtractComponentCount(retData.type()) == 1);
        ASSERT(ExtractRowCount(argValues[0].type()) == 1);

        DataValueComponent sumSquares(0.0f);
        for (uint32_t comp = 0; comp < numComponents; ++comp)
        {
            auto leftValue = argValues[0].component(comp);
            sumSquares = valop::FAdd(sumSquares, valop::FMul(leftValue, leftValue));
            if (!sumSquares.isDefined())
                break; // optimization
        }
        retData.component(0,valop::Sqrt(sumSquares));
    }
};

RTTI_BEGIN_TYPE_CLASS(FunctionVectorLength);
RTTI_METADATA(FunctionNameMetadata).name("length");
RTTI_END_TYPE();

//---

/// normalized vector
class FunctionVectorNormalize : public INativeFunction
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionVectorNormalize, INativeFunction);

public:
    virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const parser::Location& loc, parser::IErrorReporter& err) const override final
    {
        // math uses const values
        argTypes[0] = argTypes[0].unmakePointer();

        // we must have 2 arguments here
        if (numArgs != 1)
        {
            err.reportError(loc, "Function requires 1 argument");
            return DataType();
        }

        // get the base type of the operands
        auto baseType0 = ExtractBaseType(argTypes[0]);
        if (!argTypes[0].isNumericalVectorLikeOperand())
        {
            err.reportError(loc, "Vector functions work only with floating point types");
            return DataType();
        }

        // normalization works only for vectors >= 2 components
        auto numComponents = ExtractComponentCount(argTypes[0]);
        if (numComponents < 2)
        {
            err.reportError(loc, "Normalization operation requires vector with at least vector 2 components");
            return DataType();
        }

        // matrices are not supported
        if (ExtractRowCount(argTypes[0]) != 1)
        {
            err.reportError(loc, "Math operation on matrices are not supported");
            return DataType();
        }

        argTypes[0] = typeLibrary.simpleCompositeType(BaseType::Float, numComponents).unmakePointer();
        return argTypes[0];
    }

    virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
    {
        auto baseType = ExtractBaseType(retData.type());
        ASSERT(baseType == BaseType::Float);

        // extract argument sizes
        auto numComponents = ExtractComponentCount(argValues[0].type());
        ASSERT(ExtractRowCount(retData.type()) == 1);
        ASSERT(ExtractComponentCount(retData.type()) == numComponents);
        ASSERT(ExtractRowCount(argValues[0].type()) == 1);

        DataValueComponent sumSquares(0.0f);
        for (uint32_t comp = 0; comp < numComponents; ++comp)
        {
            auto leftValue = argValues[0].component(comp);
            sumSquares = valop::FAdd(sumSquares, valop::FMul(leftValue, leftValue));
            if (!sumSquares.isDefined())
                break; // optimization
        }
        DataValueComponent invLen = valop::Rsqrt(sumSquares);
        for (uint32_t comp = 0; comp < numComponents; ++comp)
        {
            auto leftValue = argValues[0].component(comp);
            retData.component(0, valop::FMul(leftValue, invLen));
        }
    }
};

RTTI_BEGIN_TYPE_CLASS(FunctionVectorNormalize);
RTTI_METADATA(FunctionNameMetadata).name("normalize");
RTTI_END_TYPE();

//---

/// distance between points
class FunctionVectorDistance : public INativeFunction
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionVectorDistance, INativeFunction);

public:
    virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const parser::Location& loc, parser::IErrorReporter& err) const override final
    {
        // math uses const values
        argTypes[0] = argTypes[0].unmakePointer();

        // we must have 2 arguments here
        if (numArgs != 2)
        {
            err.reportError(loc, "Function requires 2 arguments");
            return DataType();
        }

        // get the base type of the operands
        if (!argTypes[0].isNumericalVectorLikeOperand())
        {
            err.reportError(loc, "Vector functions work only with numerical types");
            return DataType();
        }

        // matrices are not supported
        if (ExtractRowCount(argTypes[0]) != 1)
        {
            err.reportError(loc, "Math operation on matrices are not supported");
            return DataType();
        }

        auto numComponents = ExtractComponentCount(argTypes[0]);
        argTypes[0] = typeLibrary.simpleCompositeType(BaseType::Float, numComponents).unmakePointer();
        argTypes[1] = typeLibrary.simpleCompositeType(BaseType::Float, numComponents).unmakePointer();
        return DataType::FloatScalarType();
    }

    virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
    {
        auto baseType = ExtractBaseType(retData.type());
        ASSERT(baseType == BaseType::Float);

        // extract argument sizes
        auto numComponents = ExtractComponentCount(argValues[0].type());
        ASSERT(ExtractRowCount(retData.type()) == 1);
        ASSERT(ExtractComponentCount(retData.type()) == 1);
        ASSERT(ExtractRowCount(argValues[0].type()) == 1);

        DataValueComponent sumSquares(0.0f);
        for (uint32_t comp = 0; comp < numComponents; ++comp)
        {
            auto leftValue = argValues[0].component(comp);
            auto rightValue = argValues[1].component(comp);
            auto diff = valop::FSub(rightValue, leftValue);
            sumSquares = valop::FAdd(sumSquares, valop::FMul(diff, diff));
            if (!sumSquares.isDefined())
                break; // optimization
        }

        retData.component(0, valop::Rsqrt(sumSquares));
    }
};

RTTI_BEGIN_TYPE_CLASS(FunctionVectorDistance);
RTTI_METADATA(FunctionNameMetadata).name("distance");
RTTI_END_TYPE();

//--

/// transpose matrix
class FunctionVectorTranspose : public INativeFunction
{
    RTTI_DECLARE_VIRTUAL_CLASS(FunctionVectorTranspose, INativeFunction);

public:
    virtual DataType determineReturnType(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const parser::Location& loc, parser::IErrorReporter& err) const override final
    {
        // math uses const values
        argTypes[0] = argTypes[0].unmakePointer();

        // we must have 1 arguments here
        if (numArgs != 1)
        {
            err.reportError(loc, "Function requires 1 argument");
            return DataType();
        }

        // get the base type of the operands
        auto baseType0 = ExtractBaseType(argTypes[0]);
        if (!argTypes[0].isNumericalMatrixLikeOperand())
        {
            err.reportError(loc, "Matrix functions work only with floating point types");
            return DataType();
        }

        // normalization works only for vectors >= 2 components
        auto numComponents = ExtractComponentCount(argTypes[0]);
        auto numRows = ExtractRowCount(argTypes[0]);
        if (numComponents < 2 || numComponents > 4 || numRows < 2 || numRows > 5)
        {
            err.reportError(loc, "Matrix transposition requies matrix between 2x2 to 4x4 components in size");
            return DataType();
        }

        argTypes[0] = typeLibrary.simpleCompositeType(BaseType::Float, numRows, numComponents).unmakePointer(); // swap
        return argTypes[0];
    }

    virtual void evaluate(ExecutionValue& retData, uint32_t numArgs, const ExecutionValue* argValues) const override final
    {
        // TODO: actual transposed value
        for (uint32_t comp = 0; comp < retData.numComponents(); ++comp)
            retData.component(comp, DataValueComponent());
    }
};

RTTI_BEGIN_TYPE_CLASS(FunctionVectorTranspose);
RTTI_METADATA(FunctionNameMetadata).name("transpose");
RTTI_END_TYPE();

//--

END_BOOMER_NAMESPACE_EX(gpu::compiler)

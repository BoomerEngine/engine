/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\optimizer #]
***/

#include "build.h"

#include "renderingShaderStaticExecution.h"
#include "renderingShaderProgram.h"
#include "renderingShaderProgramInstance.h"
#include "renderingShaderFunction.h"
#include "renderingShaderTypeLibrary.h"
#include "renderingShaderNativeFunction.h"
#include "renderingShaderCodeNode.h"
#include "renderingShaderCodeLibrary.h"

BEGIN_BOOMER_NAMESPACE(rendering::shadercompiler)

//---

ExecutionValue::ExecutionValue()
{}

ExecutionValue::ExecutionValue(const DataType& type)
    : m_type(type)
    , m_val(type)
{
}

ExecutionValue::ExecutionValue(const DataType& type, const DataValue& value)
    : m_type(type)
    , m_val(value)
{
}

ExecutionValue::ExecutionValue(const DataValueRef& refx)
    : m_type(refx.dataType())
    , m_ref(refx)
{
    refx.readValue(m_val);
}

ExecutionValue ExecutionValue::arrayElement(uint32_t index) const
{
    // we are not an array
    if (!m_type.isArray())
        return ExecutionValue();

    // get the inner array size
    auto& arrayCounts = m_type.arrayCounts();
    auto innerArrayType  = m_type.applyArrayCounts(arrayCounts.innerCounts());

    // validate the array index
    if (index >= arrayCounts.outermostCount())
        return ExecutionValue(innerArrayType);

    // compute the element placement
    auto elementSize  = innerArrayType.computeScalarComponentCount();
    auto placementIndex  = elementSize * index;
    if (m_ref.valid())
    {
        auto firstComponent  = m_ref.component(placementIndex);
        return ExecutionValue(DataValueRef(innerArrayType, firstComponent, elementSize));
    }
    else if (m_val.valid())
    {
        return ExecutionValue(innerArrayType, DataValue(m_val, placementIndex, elementSize));
    }
    else
    {
        return ExecutionValue(innerArrayType);
    }
}

ExecutionValue ExecutionValue::compositeElement(const base::StringID name) const
{
    // we are not a structure
    if (!m_type.isComposite())
        return ExecutionValue();

    // find the structure element
    auto& compositeType = m_type.composite();
    auto memberIndex  = compositeType.memberIndex(name);
    if (memberIndex == -1)
        return DataValueRef();

    // compute the offset to member
    uint32_t offsetToMember = 0;
    for (uint32_t i=0; i<(uint32_t)memberIndex; ++i)
    {
        auto memberType  = compositeType.memberType(i);
        offsetToMember += memberType.computeScalarComponentCount();
    }

    // create member reference
    auto memberType  = compositeType.memberType(memberIndex);
    if (m_ref.valid())
    {
        auto firstComponent  = m_ref.component(offsetToMember);
        return ExecutionValue(DataValueRef(memberType, firstComponent, memberType.computeScalarComponentCount()));
    }
    else if (m_val.valid())
    {
        return ExecutionValue(memberType, DataValue(m_val, offsetToMember, memberType.computeScalarComponentCount()));
    }
    else
    {
        return ExecutionValue(memberType);
    }
}

uint32_t ExecutionValue::numComponents() const
{
    return isReference() ? m_ref.numComponents() : m_val.size();
}

DataValueComponent ExecutionValue::component(uint32_t index) const
{
    if (isReference())
        return *m_ref.component(index);

    return m_val.component(index);
}

DataValueComponent ExecutionValue::matrixComponent(uint32_t numCols, uint32_t numRows, uint32_t col, uint32_t row) const
{
    if (col > numCols || row > numRows)
        return DataValueComponent();

    auto index  = col + row * numCols; // matrix is ALWAYS row-major in the memory
    return component(index);
}

void ExecutionValue::matrixComponent(uint32_t numCols, uint32_t numRows, uint32_t col, uint32_t row, const DataValueComponent& val)
{
    if (col > numCols || row > numRows)
        return;

    auto index  = col + row * numCols; // matrix is ALWAYS row-major in the memory
    component(index, val);
}

void ExecutionValue::component(uint32_t index, const DataValueComponent& newValue)
{
    /*if (isReference())
        stub.component(index, newValue);
    else*/
        m_val.component(index, newValue);
}

void ExecutionValue::print(base::IFormatStream& f) const
{
    if (isReference())
        m_ref.print(f);
    else
        m_val.print(f);
}

bool ExecutionValue::isWholeValueDefined() const
{
    if (isReference())
        return m_ref.isWholeValueDefined();

    return m_val.isWholeValueDefined();
}

bool ExecutionValue::readValue(DataValue& outValue) const
{
    if (isReference())
        return m_ref.readValue(outValue);

    outValue = m_val;
    return true;
}

bool ExecutionValue::writeValue(const DataValue& newValue) const
{
    if (isReference())
        return m_ref.writeValue(newValue);
    return false;
}

bool ExecutionValue::writeValueWithMask(const DataValue& newValue, const ComponentMask& mask) const
{
    if (isReference())
        return m_ref.writeValueWithMask(newValue, mask);
    return false;
}

bool ExecutionValue::writeValue(const ExecutionValue& newValue) const
{
    DataValue newValueData;
    if (newValue.readValue(newValueData))
        return writeValue(newValueData);
    else
        return false;            
}

bool ExecutionValue::writeValueWithMask(const ExecutionValue& newValue, const ComponentMask& mask) const
{
    DataValue newValueData;
    if (newValue.readValue(newValueData))
        return writeValueWithMask(newValueData, mask);
    else
        return false;
}

//---

ExecutionStack::ParamMap::Param::Param(const DataParameter* param)
    : m_param(param)
    , m_storage(param->dataType)
    , m_valueSet(false)
{
    m_ref = ExecutionValue(DataValueRef(param->dataType, &m_storage));
}

ExecutionStack::ParamMap::ParamMap()
{
    m_paramMap.reserve(16);
}

ExecutionStack::ParamMap::~ParamMap()
{
}

/*DataValueRef ExecutionStack::ParamMap::Param::ref()
{
    // use the existing reference is provided
    if (value.isReference())
        return value.reference();

    // create new reference
    auto& value = value.value();
    auto valueComponentCount  = m_param->dataType.computeScalarComponentCount();
    ASSERT(value.size() == valueComponentCount);
    auto firstComponent  = value.m_components.typedData();
    return DataValueRef(m_param->dataType, firstComponent, valueComponentCount);
}*/

bool ExecutionStack::ParamMap::paramReference(base::mem::LinearAllocator& mem, const DataParameter* param, ExecutionValue& outValue)
{
    // local ?
    if (param->scope == DataParameterScope::ScopeLocal)
    {
        Param* ret = nullptr;
        if (m_localMap.find(param, ret))
        {
            outValue = ret->m_ref;
            return true;
        }

        TRACE_DEEP("{}: creating entry for local var '{}'", param->loc, param->name);
        auto stackVal  = mem.create<ParamMap::Param>(param);
        m_localMap[param] = stackVal;
        outValue = stackVal->m_ref;
        return true;
    }

    // find parameter
    Param* ret = nullptr;
    if (m_paramMap.find(param->name, ret))
    {
        outValue = ret->m_ref;
        return true;
    }

    // nothing found
    return false;
}

void ExecutionStack::ParamMap::storeConstantValue(base::mem::LinearAllocator& mem, const DataParameter* param, const ExecutionValue& value)
{
    Param* ret = nullptr;
    if (!m_localMap.find(param, ret))
    {
        TRACE_DEEP("{}: creating entry for local var '{}'", param->loc, param->name);
        ret = mem.create<ParamMap::Param>(param);
        m_localMap[param] = ret;
    }

    value.readValue(ret->m_storage);
    ret->m_valueSet = true;
}

//---

ExecutionStack::ExecutionStack(base::mem::LinearAllocator& mem, CodeLibrary& code, base::parser::IErrorReporter& err, const Function* function, const ProgramInstance* thisVars)
    : m_mem(mem)
    , m_code(code)
    , m_err(err)
    , m_contextFunction(function)
    , m_contextThisVars(thisVars)
{
    // map accessible function params
    if (m_contextFunction)
        buildFunctionParams();

    // map accessible program params
    if (m_contextThisVars)
        buildProgramParams();
}

ExecutionStack::~ExecutionStack()
{
}

ExecutionResult ExecutionStack::returnCode(const base::parser::Location& loc, ExecutionResult error)
{
    if (error == ExecutionResult::Error)
    {
        TRACE_DEEP("{}: returning error code {}", loc, (int)error);
    }

    return error;
}

static void CollectBasePrograms(const Program* program, base::Array<const Program*>& basePrograms)
{
    for (auto parentProgram  : program->parentPrograms())
    {
        if (!basePrograms.contains(parentProgram))
        {
            basePrograms.pushBack(parentProgram);
            CollectBasePrograms(parentProgram, basePrograms);
        }
    }
}

void ExecutionStack::buildFunctionParams()
{
    PC_SCOPE_LVL3(BuildFunctionParams);
    m_functionParams.m_paramMap.clear();

    // setup slots for input parameters
    for (auto param  : m_contextFunction->inputParameters())
    {
        auto stackVal  = m_mem.create<ParamMap::Param>(param);
        m_functionParams.m_paramMap.set(param->name, stackVal);
    }
}

void ExecutionStack::buildProgramParams()
{
    PC_SCOPE_LVL3(BuildProgramParams);

    // get all base programs
    base::InplaceArray<const Program*, 8> basePrograms;
    basePrograms.pushBack(m_contextThisVars->program());
    CollectBasePrograms(m_contextThisVars->program(), basePrograms);

    // extract shit
    m_programParams.m_paramMap.clear();
    for (auto baseProgram  : basePrograms)
    {
        // setup parameters
        for (auto param  : baseProgram->parameters())
        {
            auto stackVal  = m_mem.create<ParamMap::Param>(param);

            if (param->scope == DataParameterScope::GlobalConst)
            {
                if (auto constValue = m_contextThisVars->params().findConstValue(param))
                {
                    TRACE_DEEP("{}: constant value {} set to {}", param->loc, param->name, *constValue);
                    stackVal->m_storage = *constValue;
                    stackVal->m_ref = ExecutionValue(DataValueRef(param->dataType, &stackVal->m_storage));
                    stackVal->m_valueSet = true;
                }
                else
                {
                    TRACE_DEEP("{}: constant value {} not found in param table", param->loc, param->name);
                }
            }
            else if (param->scope == DataParameterScope::GroupShared)
            {
                stackVal->m_storage = DataValue(param->dataType);
                stackVal->m_ref = ExecutionValue(DataValueRef(param->dataType, &stackVal->m_storage));
                stackVal->m_valueSet = true;
            }

            //TRACE_DEEP("{}: added param {} to program execution context", param->loc, param->name);
            m_programParams.m_paramMap.set(param->name, stackVal);
        }
    }

    // evaluate initialization for other program parameters
    for (auto paramInfo  : m_programParams.m_paramMap.values())
    {
        if (!paramInfo->m_valueSet)
        {
            auto param  = paramInfo->m_param;
            if (param->initializerCode)
            {
                ExecuteProgramCode(*this, param->initializerCode, paramInfo->m_ref);
                paramInfo->m_valueSet = true;
            }
        }
    }
}


ExecutionResult ExecutionStack::callFunction(const base::Array<ExecutionValue>& args, ExecutionValue& result)
{
    PC_SCOPE_LVL2(CallFunction);

    // copy constant values into the function arg list
    for (uint32_t argIndex=0; argIndex<args.size(); ++argIndex)
    {
        // invalid param, ?
        if (argIndex < m_contextFunction->inputParameters().size())
        {
            // get input param
            auto param  = m_contextFunction->inputParameters()[argIndex];

            // find param
            ParamMap::Param* paramIndex = nullptr;
            if (m_functionParams.m_paramMap.find(param->name, paramIndex))
            {
                paramIndex->m_ref = args[argIndex];
                paramIndex->m_valueSet = true;
            }
        }
    }

    /*// evaluate initialization for other program parameters
    for (auto param  : fi->inputParameters())
    {
        if (param->m_initializer)
        {
            ParamMap::Param* paramIndex = nullptr;
            frame.m_paramMap.find(param, paramIndex);
            ASSERT(paramIndex != nullptr);
            if (!paramIndex->m_valueSet)
            {
                auto ret  = ExecuteProgramCode(*this, pi, param->m_initializer, err, paramIndex->value);
                if (ret != ExecutionResult::Finished)
                    return ret;

                paramIndex->m_valueSet = true;
            }
        }
    }*/

    // run function code
    return ExecuteProgramCode(*this, &m_contextFunction->code(), result);
}
      
void ExecutionStack::storeConstantValue(const DataParameter* param, const ExecutionValue& value)
{
    m_functionParams.storeConstantValue(m_mem, param, value);            
}

DataValueRef ExecutionStack::resolveGlobalConstantValue(const DataParameter* param, base::parser::IErrorReporter& err)
{
    PC_SCOPE_LVL3(ResolveGlobalConstantValue);
    ASSERT(param->scope == DataParameterScope::StaticConstant);

    // invalid constant
    if (!param->initializerCode)
    {
        err.reportError(param->loc, base::TempString("Constant {} has no initialization and cannot be used", param->name));
        return DataValueRef();
    }

    // lookup in the map
    GlobalConstant gc;
    if (m_globalConstantsValues.find(param, gc))
        return DataValueRef(gc.m_type, gc.m_value);

    // create a execution context for the global scope
    ExecutionStack stack(m_mem, m_code, err, nullptr, nullptr); // no function, program or locals as context

    // evaluate the constant expression
    ExecutionValue result;
    auto ret  = ExecuteProgramCode(stack, param->initializerCode, result);
    if (ret != ExecutionResult::Finished)
    {
        err.reportError(param->loc, base::TempString("Failed to evaluate constant {}", param->name));
        return DataValueRef();
    }

    // make sure the value is defined
    if (!result.isWholeValueDefined())
    {
        err.reportError(param->loc, base::TempString("Evaluation of constant {} did not produce a defined value, code may not be unequivocal", param->name));
        return DataValueRef();
    }

    // store
    TRACE_DEEP("Evaluated constant {} to {}", param->name, result);
    gc.m_type = result.type();
    gc.m_value = m_mem.create<DataValue>();
    if (!result.readValue(*gc.m_value))
    {
        err.reportError(param->loc, base::TempString("Evaluation of constant {} did not produce a loadable value, code may not be unequivocal", param->name));
        return DataValueRef();
    }

    m_globalConstantsValues[param] = gc;
    return DataValueRef(gc.m_type, gc.m_value);
}

bool ExecutionStack::paramReference(const DataParameter* param, ExecutionValue& outValue)
{
    PC_SCOPE_LVL3(ResolveParamReference);

    // constant
    if (param->scope == DataParameterScope::StaticConstant)
    {
        outValue = resolveGlobalConstantValue(param, m_err);
        return true;
    }

    // check the function context
    if (m_functionParams.paramReference(m_mem, param, outValue))
        return true;

    // check the program context
    if (m_programParams.paramReference(m_mem, param, outValue))
        return true;

    // not found
    return false;
}

//---

ExecutionResult ExecuteProgramCode(ExecutionStack& stack, const CodeNode* code, ExecutionValue& result)
{
    DEBUG_CHECK_EX(code->dataTypeResolved(), base::TempString("Code at '{}' was not properly type-checked before execution", code->location()));
    PC_SCOPE_LVL1(EvalCode);

    auto op  = code->opCode();
    switch (op)
    {
        case OpCode::Nop:
        {
            return ExecutionResult::Finished;
        }

        case OpCode::Return:
        {
            if (code->children().size() == 1)
            {
                auto ret = ExecuteProgramCode(stack, code->children()[0], result);
                if (ret != ExecutionResult::Finished)
                    return ret;
            }
            return ExecutionResult::Return;
        }

        case OpCode::Scope:
        {
            for (auto child : code->children())
            {
                ExecutionValue value;
                auto ret = ExecuteProgramCode(stack, child, value);
                if (ret != ExecutionResult::Finished)
                {
                    if (ret == ExecutionResult::Return)
                        result = value;
                    return ret;
                }
                else
                {
                    result = value;
                }
            }
            return ExecutionResult::Finished;
        }

        case OpCode::ParamRef:
        {
            auto param = code->extraData().m_paramRef;
            ASSERT(param != nullptr);

            TRACE_DEEP("{}: Accessing param {}, {}", code->location(), param->name, param->dataType);

            ExecutionValue value;
            if (!stack.paramReference(param, value))
            {
                stack.errorReporter().reportWarning(code->location(), base::TempString("Unable to resolve reference to parameter '{}'", param->name));
                return stack.returnCode(code->location(), ExecutionResult::Error);
            }

            result = value;
            return ExecutionResult::Finished;
        }

        case OpCode::AccessArray:
        {
            PC_SCOPE_LVL3(AccessArray);
            ASSERT(code->children().size() == 2);
            auto arrayNode = code->children()[0];
            auto indexNode = code->children()[1];

            // resolve array
            ExecutionValue arrayValue;
            {
                auto ret = ExecuteProgramCode(stack, arrayNode, arrayValue);
                if (ret != ExecutionResult::Finished)
                    return ret;
            }

            // resources
            if (arrayValue.type().isReference())
            {
                result = ExecutionValue(code->dataType());
                return ExecutionResult::Finished;
            }

            // not an array
            if (!arrayValue.type().isArray())
            {
                // array access on resources leads to undefined values
                if (arrayValue.type().isResource())
                {
                    result = ExecutionValue(code->dataType());
                    return ExecutionResult::Finished;
                }

                stack.errorReporter().reportWarning(code->location(), "Argument of [] ios not an array");
                return stack.returnCode(code->location(), ExecutionResult::Error);
            }

            // evaluate the index
            ExecutionValue indexValue;
            {
                auto ret = ExecuteProgramCode(stack, indexNode, indexValue);
                if (ret != ExecutionResult::Finished)
                    return ret;
            }

            // get the index, if the index is not defined we can't evaluate the static array value
            if (!indexValue.component(0).isDefined())
            {
                result = ExecutionValue(code->dataType());
                return ExecutionResult::Finished;
            }

            auto resolvedIndexValue = indexValue.component(0).valueUint();

            // get the element
            result = arrayValue.arrayElement(resolvedIndexValue);
            return ExecutionResult::Finished;
        }

        case OpCode::AccessMember:
        {
            PC_SCOPE_LVL3(AccessMember);
            ASSERT(code->children().size() == 1);
            auto contextNode  = code->children()[0];

            // evaluate context node
            ExecutionValue contextValue;
            {
                auto ret = ExecuteProgramCode(stack, contextNode, contextValue);
                if (ret != ExecutionResult::Finished)
                    return ret;
            }

            // composite param
            auto memberName = code->extraData().m_name;
            if (contextNode->dataType().isComposite())
            {
                result = contextValue.compositeElement(memberName);
                return ExecutionResult::Finished;
            }
            /*else if (contextNode->dataType().isProgram())
            {
                // resolve the parameter name
                auto program  = contextNode->dataType().program();
                auto param  = program->findParameter(memberName);
                ASSERT(param != nullptr);

                // get the actual program instance
                auto programInstance  = contextValue.value().component(0).programInstance();
                if (!programInstance)
                {
                    stack.errorReporter().reportWarning(code->location(), "No program instance specified as context");
                    return stack.returnCode(code->location(), ExecutionResult::Error);
                }

                // create the child parameters
                auto programInstanceParams  = stack.buildChildProgramConstants(pc, &programInstance->parametrization());

                // resolve as parameter
                result = stack.paramReference(programInstance->program(), programInstanceParams, param, err);
                return ExecutionResult::Finished;
            }*/
            else
            {
                stack.errorReporter().reportWarning(code->location(), "Invalid node type for member element evaluation");
                return stack.returnCode(code->location(), ExecutionResult::Error);
            }
        }

        case OpCode::ReadSwizzle:
        {
            PC_SCOPE_LVL3(ReadSwizzle);
            ASSERT(code->children().size() == 1);
            auto contextNode  = code->children()[0];

            // evaluate context node
            ExecutionValue contextValue;
            {
                auto ret = ExecuteProgramCode(stack, contextNode, contextValue);
                if (ret != ExecutionResult::Finished)
                    return ret;
            }

            // we cannot be a reference here
            //ASSERT(!contextValue.isReference());

            // swizzle the result
            auto& mask = code->extraData().m_mask;
            auto numInputComponents = mask.numberOfComponentsNeeded();
            auto numOutputComponents = mask.numberOfComponentsProduced();

            // load the input value to swizzle
            DataValue contextValueData;
            if (!contextValue.readValue(contextValueData))
                return stack.returnCode(code->location(), ExecutionResult::Error);

            // get input data
            auto& inputComponents = contextValueData.m_components;
            ASSERT(inputComponents.size() >= numInputComponents);

            // create the output value
            DataValue outputValue(code->dataType());
            for (uint32_t i=0; i<numOutputComponents; ++i)
            {
                auto componentMask = mask.componentSwizzle(i);
                auto& outputComponent = outputValue.m_components[i];

                switch (componentMask)
                {
                    case ComponentMaskBit::X:
                        outputComponent = inputComponents[0];
                        break;

                    case ComponentMaskBit::Y:
                        outputComponent = inputComponents[1];
                        break;

                    case ComponentMaskBit::Z:
                        outputComponent = inputComponents[2];
                        break;

                    case ComponentMaskBit::W:
                        outputComponent = inputComponents[3];
                        break;

                    case ComponentMaskBit::Zero:
                        outputComponent = DataValueComponent(0);
                        break;

                    case ComponentMaskBit::One:
                        outputComponent = DataValueComponent(1);
                        break;
                }
            }

            result = ExecutionValue(code->dataType(), outputValue);
            return ExecutionResult::Finished;
        }

        case OpCode::Const:
        {
            auto& dataType = code->dataType();
            auto& dataValue = code->dataValue();
            result = ExecutionValue(dataType, dataValue);
            return ExecutionResult::Finished;
        }

        case OpCode::This:
        {
            result = ExecutionValue(code->dataType(), DataValue(stack.contextThis()));
            return ExecutionResult::Finished;
        }

        case OpCode::FuncRef:
        {
            ASSERT(code->children().size() == 1);
            auto child  = code->children()[0];
            return ExecuteProgramCode(stack, child, result);
        }

        case OpCode::Cast:
        {
            ASSERT(code->children().size() == 1);

			auto ret = ExecuteProgramCode(stack, code->children()[0], result);
			if (ret != ExecutionResult::Finished)
				return ret;

			const auto numComponents = result.numComponents();
			switch (code->extraData().m_castMode)
			{
				case TypeMatchTypeConv::ConvToBool:
				{
					for (uint32_t i = 0; i < numComponents; ++i)
						result.component(i, valop::ToBool(result.component(i)));
					break;
				}

				case TypeMatchTypeConv::ConvToFloat:
				{
					for (uint32_t i = 0; i < numComponents; ++i)
						result.component(i, valop::ToFloat(result.component(i)));
					break;
				}

				case TypeMatchTypeConv::ConvToInt:
				{
					for (uint32_t i = 0; i < numComponents; ++i)
						result.component(i, valop::ToInt(result.component(i)));
					break;
				}

				case TypeMatchTypeConv::ConvToUint:
				{
					for (uint32_t i = 0; i < numComponents; ++i)
						result.component(i, valop::ToUint(result.component(i)));
					break;
				}
			}

			return ExecutionResult::Finished;
		}

        case OpCode::NativeCall:
        {
            PC_SCOPE_LVL3(NativeCall);
            auto nativeFunction = code->extraData().m_nativeFunctionRef;
            ASSERT(nativeFunction != nullptr);

            // create return value holder
            DataValue returnValue(code->dataType());
            ASSERT(returnValue.valid());
            result = ExecutionValue(code->dataType(), returnValue);

            // prepare parameters
            base::Array<ExecutionValue> functionArguments;
            functionArguments.reserve(code->children().size());

            // evaluate parameters for the function
            bool evaluated = false;
            for (uint32_t i = 0; i < code->children().size(); ++i)
            {
                // try to run the function with what we have, maybe it will succeed and we won't have to evaluate the rest of the arguments
                // this is the essential part of the "fusing" for some functions, mostly ternary operator and boolean and/or operator
                if (nativeFunction->partialEvaluate(result, functionArguments.size(), functionArguments.typedData()))
                {
                    evaluated = true;
                    break;
                }

                // get the node carrying the function argument
                auto childNode = code->children()[i];
                auto &childNodeType = childNode->dataType();
                ASSERT(childNodeType.valid());

                // reserve the space for the argument in the argument list
                DataValue emptyArgumentValue(childNodeType);
                functionArguments.emplaceBack(childNodeType, emptyArgumentValue);

                // valuate the value of the argument
                auto ret = ExecuteProgramCode(stack, childNode, functionArguments.back());
                if (ret != ExecutionResult::Finished)
                    return ret;
            }

            // execute full function if partial evaluation didn't yield results
            if (!evaluated)
                nativeFunction->evaluate(result, functionArguments.size(), functionArguments.typedData());
                    
            {
                auto funcName  = code->extraData().m_nativeFunctionRef->cls()->findMetadataRef<FunctionNameMetadata>().name();
                TRACE_DEEP("{}: Native call to '{}' returned '{}'", code->location(), funcName, result);
            }

            return ExecutionResult::Finished;
        }

        case OpCode::Call:
        {
            PC_SCOPE_LVL3(Call);
            auto contextNode = code->children()[0];
            ASSERT(contextNode->dataType().isFunction());

            // evaluate execution context
            ExecutionValue callContext;
            auto ret = ExecuteProgramCode(stack, contextNode, callContext);
            if (ret != ExecutionResult::Finished)
                return ret;

            // get the function to call
            auto callFunction  = contextNode->dataType().function();
            ASSERT(callFunction != nullptr);
            auto functionName = callFunction->name();

            // get the program instance context
            const auto* callThisVars = stack.contextThis();
            if (callFunction->program() != nullptr) // function requires a context
            {
                TRACE_DEEP("Call context for {}: {}, type {}", functionName, callContext);
                if (callContext.component(0).isProgramInstance())
                {
                    auto callProgramInstance = callContext.component(0).programInstance();
                    if (!callProgramInstance)
                        return stack.returnCode(code->location(), ExecutionResult::Error);

                    // lookup a program function
                    callFunction = callProgramInstance->program()->findFunction(functionName);
                    if (!callFunction)
                    {
                        stack.errorReporter().reportWarning(code->location(), base::TempString("Missing function '{}' in '{}'", functionName, callProgramInstance->program()->name()));
                        return stack.returnCode(code->location(), ExecutionResult::Error);
                    }

                    // Important: if the context for the function call is from the program that we are based on than keep using it to preserve the type of "this"
                    if (!stack.contextThis()->program()->isBasedOnProgram(callProgramInstance->program()))
                        callThisVars = callProgramInstance;
                }
                else
                {
                    stack.errorReporter().reportWarning(code->location(), base::TempString("Trying to call function '{}' but no program context can be established", functionName));
                    return stack.returnCode(code->location(), ExecutionResult::Error);
                }
            }

            // evaluate arguments
            base::Array<ExecutionValue> functionArguments;
            functionArguments.reserve(callFunction->inputParameters().size());

            // evaluate parameters for the function
            auto numDefinedParams = code->children().size() - 1;
            for (uint32_t i = 0; i < callFunction->inputParameters().size(); ++i)
            {
                auto &childNodeType = callFunction->inputParameters()[i]->dataType;
                ASSERT(childNodeType.valid());

                DataValue emptyArgumentValue(childNodeType);
                functionArguments.emplaceBack(childNodeType, emptyArgumentValue);

                // evaluate param
                if (i < numDefinedParams)
                {
                    auto ret = ExecuteProgramCode(stack, code->children()[1 + i], functionArguments.back());
                    if (ret != ExecutionResult::Finished)
                        return ret;
                }
            }

            // create a new execution context for called function
            ExecutionStack childStack(stack.allocator(), stack.code(), stack.errorReporter(), callFunction, callThisVars);

            // call function
            {
                const char* functionName = callFunction->name().c_str();
                TRACE_DEEP("Calling '{}", callFunction->name());

                auto ret = childStack.callFunction(functionArguments, result);
                if (ret != ExecutionResult::Finished && ret != ExecutionResult::Return)
                {
                    TRACE_DEEP("Calling '{}' returned non OK code {}", callFunction->name(), ret);
                    return ret;
                }
            }

            return ExecutionResult::Finished;
        }

        case OpCode::ProgramInstance:
        {
            PC_SCOPE_LVL3(ProgramInstance);
            // get the program to setup
            ASSERT(code->dataType().isProgram());
            auto program  = code->dataType().program();
            ASSERT(program != nullptr);

            // extract parametrization
            ProgramConstants programParams;
            for (auto childNode  : code->children())
            {
                ASSERT(childNode->opCode() == OpCode::ProgramInstanceParam);
                auto programParamName = childNode->extraData().m_name;

                // evaluate
                ExecutionValue parametrizationValue;
                auto ret = ExecuteProgramCode(stack, childNode->children()[0], parametrizationValue);
                if (ret != ExecutionResult::Finished)
                    return ret;

                // should be compile time known
                if (!parametrizationValue.isWholeValueDefined())
                {
                    stack.errorReporter().reportWarning(code->location(), base::TempString("Program instance variable '{}' should be known at compile time and it isn't", programParamName));
                    return stack.returnCode(code->location(), ExecutionResult::Error);
                }

                // load the value
                DataValue parametrizationValueData;
                if (!parametrizationValue.readValue(parametrizationValueData))
                {
                    stack.errorReporter().reportWarning(code->location(), base::TempString("Loading error", programParamName));
                    return stack.returnCode(code->location(), ExecutionResult::Error);
                }

                // set parametrization
                if (auto param  = program->findParameter(programParamName))
                    programParams.constValue(param, parametrizationValueData);

            }

            // create program instance
            const auto* programWithParams = stack.code().createProgramInstance(code->location(), program, std::move(programParams), stack.errorReporter());
            result = ExecutionValue(code->dataType(), DataValue(programWithParams));
            return ExecutionResult::Finished;
        }

        case OpCode::Store:
        {
            PC_SCOPE_LVL3(Store);
            ASSERT(code->children().size() == 2);
            auto leftSide = code->children()[0];
            auto rightSide = code->children()[1];

            // evaluate the reference
            ExecutionValue leftSideValue;
            {
                auto ret = ExecuteProgramCode(stack, leftSide, leftSideValue);
                if (ret != ExecutionResult::Finished)
                {
                    TRACE_DEEP("{}: failed to eval left side", code->location());
                    return ret;
                }
            }

            // left side should be a reference
            if (!leftSideValue.isReference())
            {
                //stack.errorReporter().reportWarning(code->location(), "Left side of assignment didn't evaluate to a reference");
                //return stack.returnCode(code->location(), ExecutionResult::Error);
                return ExecutionResult::Finished;
            }

            // evaluate the right side
            ExecutionValue rightSideValue;
            {
                auto ret = ExecuteProgramCode(stack, rightSide, rightSideValue);
                if (ret != ExecutionResult::Finished)
                {
                    TRACE_DEEP("{}: failed to eval right side", code->location());
                    return ret;
                }
            } 

            // write value
            if (code->extraData().m_mask.valid())
            {
                TRACE_DEEP("{}: writing '{}', mask '{}' (current value '{}')", code->location(), rightSideValue, code->extraData().m_mask, leftSideValue);
                if (!leftSideValue.writeValueWithMask(rightSideValue, code->extraData().m_mask))
                {
                    stack.errorReporter().reportWarning(code->location(), "Failed to assign value");
                    return stack.returnCode(code->location(), ExecutionResult::Error);
                }
                TRACE_DEEP("{}: post write current value '{}'", code->location(), leftSideValue);
            }
            else
            {
                TRACE_DEEP("{}: writing '{}' (current value '{}')", code->location(), rightSideValue, leftSideValue);
                if (!leftSideValue.writeValue(rightSideValue))
                {
                    stack.errorReporter().reportWarning(code->location(), "Failed to assign value");
                    return stack.returnCode(code->location(), ExecutionResult::Error);
                }
            }

            return ExecutionResult::Finished;
        }

		case OpCode::VariableDecl:
		{
			ASSERT(code->children().size() == 0 || code->children().size() == 1);
			if (code->children().empty())
				return ExecutionResult::Finished;

			// resolve reference
			auto param = code->extraData().m_paramRef;
			ASSERT(param != nullptr);

			TRACE_DEEP("{}: Accessing initialization param {}, {}", code->location(), param->name, param->dataType);

			ExecutionValue leftSideValue;
			if (!stack.paramReference(param, leftSideValue))
			{
				stack.errorReporter().reportWarning(code->location(), base::TempString("Unable to resolve reference to parameter '{}'", param->name));
				return stack.returnCode(code->location(), ExecutionResult::Error);
			}

			// evaluate value
			auto rightSide = code->children()[0];

			// evaluate the right side
			ExecutionValue rightSideValue;
			{
				auto ret = ExecuteProgramCode(stack, rightSide, rightSideValue);
				if (ret != ExecutionResult::Finished)
				{
					TRACE_DEEP("{}: failed to eval right side", code->location());
					return ret;
				}
			}

			// write
			TRACE_DEEP("{}: writing '{}' (current value '{}')", code->location(), rightSideValue, leftSideValue);
			if (!leftSideValue.writeValue(rightSideValue))
			{
				stack.errorReporter().reportWarning(code->location(), "Failed to assign value");
				return stack.returnCode(code->location(), ExecutionResult::Error);
			}
				
			return ExecutionResult::Finished;
		}

		case OpCode::CreateVector:
		{
			const auto numTargetComponents = code->dataType().computeScalarComponentCount();

			// scalar case
			if (code->children().size() == 1)
			{ 
				const auto* scalarChild = code->children()[0];
				if (scalarChild->dataType().isNumericalScalar())
				{
					ExecutionValue elementValue;

					// evaluate the element
					auto ret = ExecuteProgramCode(stack, scalarChild, elementValue);
					if (ret != ExecutionResult::Finished)
					{
						TRACE_DEEP("{}: failed to eval scalar value", code->location());
						return ret;
					}

					result = ExecutionValue(code->dataType()); // wasted effort, 99.999% of matrices won't be static

					// replicate
					for (uint32_t i = 0; i < numTargetComponents; ++i)
						result.component(i, elementValue.component(0));

					return ExecutionResult::Finished;
				}
			}

			// FALL THROUGH!!!
		}

		case OpCode::CreateArray:
		{
			const auto numTargetComponents = code->dataType().computeScalarComponentCount();
			result = ExecutionValue(code->dataType()); // wasted effort, 99.999% of matrices won't be static
			TRACE_DEEP("{}: evaluating type with {} components", code->location(), numTargetComponents);

			uint32_t writeComponentOffset = 0;
			for (auto* child : code->children())
			{
				ExecutionValue elementValue;

				// evaluate the element
				auto ret = ExecuteProgramCode(stack, child, elementValue);
				if (ret != ExecutionResult::Finished)
				{
					TRACE_DEEP("{}: failed to eval type at element {}", code->location(), writeComponentOffset);
					return ret;
				}

				// copy values
				const auto numElementValues = elementValue.size();
				for (uint32_t i = 0; i < numElementValues; ++i)
					result.component(writeComponentOffset++, elementValue.component(i));
			}

			return ExecutionResult::Finished;
		}

		case OpCode::CreateMatrix:
		{
			result = ExecutionValue(code->dataType()); // wasted effort, 99.999% of matrices won't be static
			return ExecutionResult::Finished;
		}

        /*case OpCode::ConstInit:
        {
            // evaluate the right side
            ExecutionValue rightSideValue;
            {
                auto rightSide = code->children()[0];
                auto ret = ExecuteProgramCode(stack, rightSide, rightSideValue);
                if (ret != ExecutionResult::Finished)
                    return ret;
            }

            // value not evaluated
            if (!rightSideValue.isWholeValueDefined())
            {
                stack.errorReporter().reportWarning(code->location(), "Expression evaluation did not produce a defined value, code may not be unequivocal");
                return stack.returnCode(code->location(), ExecutionResult::Error);
            }

            // get the param ref
            auto param = code->extraData().m_paramRef;
            ASSERT(param);
            TRACE_DEEP("Storing const init for {} as {}", param->name, rightSideValue);
            stack.storeConstantValue(param, rightSideValue);
            return ExecutionResult::Finished;
        }*/

        case OpCode::Load:
        {
            PC_SCOPE_LVL3(Load);
            ASSERT(code->children().size() == 1);
            auto leftSide = code->children()[0];

            // evaluate the reference
            ExecutionValue leftSideValue;
            {
                auto ret = ExecuteProgramCode(stack, leftSide, leftSideValue);
                if (ret != ExecutionResult::Finished)
                    return ret;
            }

            // left size value is not defined
            if (leftSideValue.size() == 0)
                return ExecutionResult::Finished;

            // load value
            DataValue value;
            if (!leftSideValue.readValue(value))
            {
                stack.errorReporter().reportWarning(code->location(), "Failed to load value from a reference");
                return stack.returnCode(code->location(), ExecutionResult::Error);
            }

            // return evaluated value
            auto valueType = leftSideValue.type();
            result = ExecutionValue(valueType, value);
            return ExecutionResult::Finished;
        }

        case OpCode::IfElse:
        {
            PC_SCOPE_LVL3(IfElse);
            auto& nodes = code->children();
            auto numCases = nodes.size() / 2;

            // process if cases
            bool hasSpeculativeCondition = false;
            for (uint32_t i=0; i<numCases; ++i)
            {
                auto testNode  = nodes[i*2 + 0];
                auto codeNode  = nodes[i*2 + 1];

                // evaluate the condition
                ExecutionValue testValue;
                auto ret = ExecuteProgramCode(stack, testNode, testValue);
                if (ret != ExecutionResult::Finished)
                    return ret;

                // make sure the evaluation yielded a valid result
                if (!testValue.isWholeValueDefined())
                    return stack.returnCode(code->location(), ExecutionResult::Error);

                // execute the condition
                auto taken = testValue.component(0).valueBool();
                if (taken)
                    return ExecuteProgramCode(stack, codeNode, result);
            }

            // execute the else case
            auto hasDefaultCase = (nodes.size() & 1) == 1;
            if (hasDefaultCase)
            {
                auto elseNode  = nodes.back();
                return ExecuteProgramCode(stack, elseNode, result);
            }

            // empty "if", nothing bad
            return ExecutionResult::Finished;
        }

        case OpCode::Exit:
        {
            return ExecutionResult::Discard;
        }

        case OpCode::Break:
        {
            return ExecutionResult::Break;
        }

        case OpCode::Continue:
        {
            return ExecutionResult::Continue;
        }

        case OpCode::Loop:
        {
            PC_SCOPE_LVL3(Loop);
            ASSERT(code->children().size() == 4);

			auto initNode = code->children()[0];
            auto conditionNode = code->children()[1];
            auto incrementNode = code->children()[2];
            auto codeNode = code->children()[3];

			{
				ExecutionValue testValue;
				auto ret = ExecuteProgramCode(stack, initNode, testValue);
				if (ret != ExecutionResult::Finished)
					return ret;
			}

            {
                // execute the inner code for as long as the condition remain valid
                uint32_t iterationLimit = 10000;
                while (--iterationLimit > 0)
                {
                    // evaluate the iteration condition
                    {
                        ExecutionValue testValue;
                        auto ret = ExecuteProgramCode(stack, conditionNode, testValue);
                        if (ret != ExecutionResult::Finished)
                            return ret;

                        if (!testValue.isWholeValueDefined())
                        {
                            stack.errorReporter().reportWarning(code->location(), "Unable to evaluate deterministic value for the loop condition");
                            return stack.returnCode(code->location(), ExecutionResult::Error);
                        }

                        auto continueLooping = testValue.component(0).valueBool();
                        if (!continueLooping)
                            break;
                    }

                    // evaluate the code
                    {
                        ExecutionValue testValue;
                        auto ret = ExecuteProgramCode(stack, codeNode, testValue);
                        if (ret == ExecutionResult::Break)
                            break;
                        if (ret != ExecutionResult::Finished && ret != ExecutionResult::Continue)
                            return ret;
                    }

                    // increment
                    {
                        ExecutionValue testValue;
                        auto ret = ExecuteProgramCode(stack, incrementNode, testValue);
                        if (ret != ExecutionResult::Finished)
                            return ret;
                    }
                }

                // endless loop
                if (iterationLimit == 0)
                    stack.errorReporter().reportError(code->location(), "Endless loop encountered");

                // evaluated
                return ExecutionResult::Finished;
            }
        }
    }

    stack.errorReporter().reportError(code->location(), base::TempString("Unsupported opcode {}", base::reflection::GetEnumValueName(op)));
    return stack.returnCode(code->location(), ExecutionResult::Error);
}

//---

END_BOOMER_NAMESPACE(rendering::shadercompiler)
/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\optimizer #]
***/

#include "build.h"
#include "renderingShaderProgram.h"
#include "renderingShaderFunction.h"
#include "renderingShaderFunctionFolder.h"
#include "renderingShaderTypeLibrary.h"
#include "renderingShaderCodeNode.h"
#include "renderingShaderCodeLibrary.h"
#include "renderingShaderStaticExecution.h"
#include "renderingShaderNativeFunction.h"
#include "renderingShaderTypeUtils.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::compiler)

///---

static DataValue GetArrayElement(const DataValue& value, const DataType& type, uint32_t index)
{
    if (!type.isArray())
        return DataValue();

    // get the inner array size
    auto& arrayCounts = type.arrayCounts();
    auto innerArrayType = type.applyArrayCounts(arrayCounts.innerCounts());

    // validate the array index
    if (index >= arrayCounts.outermostCount())
        return DataValue();

    // compute the element placement
    auto elementSize = innerArrayType.computeScalarComponentCount();
    auto placementIndex = elementSize * index;
    return DataValue(value, placementIndex, elementSize);
}

static DataValue GetCompositeElement(const DataValue& value, const DataType& type, const StringID name)
{
    // we are not a structure
    if (!type.isComposite())
        return DataValue();

    // find the structure element
    auto& compositeType = type.composite();
    auto memberIndex = compositeType.memberIndex(name);
    if (memberIndex == -1)
        return DataValue();

    // compute the offset to member
    uint32_t offsetToMember = 0;
    for (uint32_t i = 0; i < (uint32_t)memberIndex; ++i)
    {
        auto memberType = compositeType.memberType(i);
        offsetToMember += memberType.computeScalarComponentCount();
    }

    // create member reference
    auto memberType = compositeType.memberType(memberIndex);
    return DataValue(value, offsetToMember, memberType.computeScalarComponentCount());
}

///---

uint32_t FoldedFunctionKey::CalcHash(const FoldedFunctionKey& key)
{
    CRC32 crc;
    crc << key.func->name();
    crc << (key.pi ? key.pi->key() : 0);
    crc << key.localArgsKey;
    return crc;
}

///---

FunctionFolder::FunctionFolder(mem::LinearAllocator& mem, CodeLibrary& code)
    : m_mem(mem)
    , m_code(code)
{}

FunctionFolder::~FunctionFolder()
{}

CodeNode* FunctionFolder::foldCode(Function* func, const CodeNode* node, const ProgramInstance* thisVars, const ProgramConstants& localVars, parser::IErrorReporter& err)
{
    PC_SCOPE_LVL2(FoldCode);

    // invalid node is kept the same
    if (!node)
        return nullptr;

    auto ret = m_mem.create<CodeNode>(node->location(), node->opCode());
    ret->m_dataType = node->m_dataType;
    ret->m_dataValue = node->m_dataValue;
    ret->m_typesResolved = node->m_typesResolved;
    ret->m_extraData = node->m_extraData;
    ret->m_extraData.m_nodeRef = nullptr;
    ret->m_extraData.m_parentScopeRef = nullptr;

    if (node->opCode() != OpCode::IfElse)
    {
        if (node->opCode() == OpCode::Call)
        {
            auto functionNode  = node->children()[0];
            auto functionName = functionNode->dataType().function()->name();
            TRACE_DEEP("{}: info: Folding call to '{}'", node->location(), functionName);
        }

        for (auto child  : node->m_children)
        {
            auto foldedChild  = foldCode(func, child, thisVars, localVars, err);
            ret->m_children.pushBack(foldedChild);
        }
    }

    switch (node->opCode())
    {
		// copy local variables as is in scope
		case OpCode::Scope:
		{
			ret->m_declarations = node->m_declarations; // TODO: we should remove 100% constant parameters
			break;
		}

        // fold parameter into known constant value
        case OpCode::ParamRef:
        {
            PC_SCOPE_LVL3(ParamRef);
            auto param  = node->extraData().m_paramRef;
            if (param->scope == DataParameterScope::GlobalConst)
            {
                if (auto constValue = thisVars->params().findConstValue(param))
                {
                    ASSERT(constValue->isWholeValueDefined());

                    ret->m_dataValue = *constValue;
                    ret->m_op = OpCode::Const;
                    TRACE_DEEP("{}: info: folded '{}' to '{}'", ret->location(), param->name, ret->m_dataValue);
                }
                else
                {
                    TRACE_DEEP("{}: found unintialized constant '{}'", ret->location(), param->name);

                    ret->m_op = OpCode::Const; 
                    ret->m_dataValue = DataValue(ret->m_dataType);
                    for (uint32_t i = 0; i < ret->m_dataValue.size(); ++i)
                        ret->m_dataValue.component(i, DataValueComponent(0));
                            
                    if (!param->initializerCode)
                    {
                        err.reportError(ret->location(), TempString("Unexpected constant '{}' with missing initialization and specified value", param->name));
                    }
                    else if (auto foldedConstant = foldCode(nullptr, param->initializerCode, thisVars, ProgramConstants(), err))
                    {                                
                        if (foldedConstant->dataValue().isWholeValueDefined())
                        {
                            ret->m_dataValue = foldedConstant->dataValue();
                            ret->m_dataType = foldedConstant->dataType();
                        }
                        else
                        {
                            err.reportError(ret->location(), TempString("Unable to fully initialize value of constant '{}' in given context", param->name));
                        }
                    }
                    else
                    {
                        err.reportError(ret->location(), TempString("Unable to initialize constant '{}' in given context", param->name));
                    }
                }
            }
            else if (param->scope == DataParameterScope::FunctionInput)
            {
                if (auto constValue = localVars.findConstValue(param))
                {
                    ASSERT(constValue->isWholeValueDefined());

                    ret->m_dataValue = *constValue;
                    ret->m_op = OpCode::Const;
                    TRACE_DEEP("{}: info: folded '{}' to '{}'", ret->location(), param->name, ret->m_dataValue);
                }
            }                    
            else if (param->scope == DataParameterScope::StaticConstant)
            {
                auto foldedConstant = foldCode(nullptr, param->initializerCode, nullptr, ProgramConstants(), err);
                if (foldedConstant->dataValue().isWholeValueDefined())
                {
                    ret->m_dataValue = foldedConstant->dataValue();
                    ret->m_dataType = foldedConstant->dataType();
                    ret->m_op = OpCode::Const;
                    TRACE_DEEP("{}: info: static const '{}' folded to '{}'", ret->location(), param->name, ret->m_dataValue);
                }
                else if (!param->dataType.isArray())
                {
                    err.reportError(ret->location(), TempString("Unable to fully initialize value of constant '{}' in given context", param->name));
                }
            }
            else if (param->scope == DataParameterScope::GlobalParameter)
            {
                // access to resources must be folded into a constant name that can be propagated through the 
                if (param->dataType.isResource())
                {
                    TRACE_DEEP("{}: info: access to resource '{}' folded into a named constant", ret->location(), param->name);

                    ret->m_dataValue = DataValue(param->name);
                    ret->m_op = OpCode::Const;
                }
            }

            break;
        }

        // constants
        case OpCode::Const:
        {
            ASSERT(ret->dataValue().isWholeValueDefined());
            break;
        }

        // cast
        case OpCode::Cast:
        {
			auto valueNode = ret->children()[0];
			if (valueNode->dataValue().isWholeValueDefined())
			{
				bool castValid = true;

				const auto destCastType = ExtractBaseType(ret->dataType());
				switch (destCastType)
				{
					case BaseType::Boolean:
					{
						ret->m_dataValue = valueNode->m_dataValue;
						for (auto& comp : ret->m_dataValue.m_components)
							comp = DataValueComponent(comp.valueBool());
						break;
					}

					case BaseType::Int:
					{
						ret->m_dataValue = valueNode->m_dataValue;
						for (auto& comp : ret->m_dataValue.m_components)
							comp = DataValueComponent(comp.valueInt());
						break;
					}

					case BaseType::Uint:
					{
						ret->m_dataValue = valueNode->m_dataValue;
						for (auto& comp : ret->m_dataValue.m_components)
							comp = DataValueComponent(comp.valueUint());
						break;
					}

					case BaseType::Float:
					{
						ret->m_dataValue = valueNode->m_dataValue;
						for (auto& comp : ret->m_dataValue.m_components)
							comp = DataValueComponent(comp.valueNumerical());
						break;
					}

					default:
						castValid = false;
				}

				ret->m_dataValue = valueNode->m_dataValue;

				if (castValid)
				{
					ret->m_op = OpCode::Const;
					ret->m_children.clear();
					TRACE_DEEP("{}: info: cast folded into '{}'", ret->location(), ret->m_dataValue);
				}
			}
			break;
        }

        // load
        case OpCode::Load:
        {
            auto valueNode  = ret->children()[0];
            if (valueNode->dataValue().isWholeValueDefined())
            {
                ret->m_dataValue = valueNode->m_dataValue;
                ret->m_dataType = valueNode->m_dataType;
                ret->m_op = OpCode::Const;
                ret->m_children.clear();
                TRACE_DEEP("{}: info: load folded into '{}'", ret->location(), ret->m_dataValue);
            }
            break;
        }

        // fold array access
        case OpCode::AccessArray:
        {
            PC_SCOPE_LVL3(AccessArray);
            ASSERT(ret->children().size() == 2);
            auto arrayNode  = ret->children()[0];
            auto indexNode  = ret->children()[1];

            if (indexNode->dataValue().isWholeValueDefined())
            {
                auto resolvedIndexValue = indexNode->dataValue().component(0).valueUint();

                auto arrayValue = GetArrayElement(arrayNode->dataValue(), arrayNode->dataType(), resolvedIndexValue);
                if (arrayValue.isWholeValueDefined())
                {
                    ret->m_dataValue = arrayValue;
                    ret->m_op = OpCode::Const;
                    ret->m_children.clear();
                    TRACE_DEEP("{}: Array access to element [{}] folded into '{}'", ret->m_loc, resolvedIndexValue, arrayValue);
                }
            }
            break;
        }

        // fold member access
        case OpCode::AccessMember:
        {
            PC_SCOPE_LVL3(AccessMember);
            ASSERT(ret->children().size() == 1);
            auto structNode  = ret->children()[0];

            auto memberName = ret->extraData().m_name;
            if (structNode->dataType().isComposite())
            {
                auto memberValue = GetCompositeElement(structNode->dataValue(), structNode->dataType(), memberName);
                if (memberValue.isWholeValueDefined())
                {
                    ret->m_dataValue = memberValue;
                    ret->m_op = OpCode::Const;
                    ret->m_children.clear();
                    TRACE_DEEP("{}: Member access '{}' folded into '{}'", ret->m_loc, memberName, memberValue);
                }
            }
            break;
        }

        // fold swizzle
        case OpCode::ReadSwizzle:
        {
            PC_SCOPE_LVL3(ReadSwizzle);
            ASSERT(ret->children().size() == 1);
            auto contextNode  = ret->children()[0];

            // evaluate context node
            if (contextNode->dataValue().isWholeValueDefined())
            {
                // swizzle the result
                auto& mask = ret->extraData().m_mask;
                auto numInputComponents = mask.numberOfComponentsNeeded();
                auto numOutputComponents = mask.numberOfComponentsProduced();

                // get input data
                auto& inputComponents = contextNode->dataValue().m_components;
                ASSERT(inputComponents.size() >= numInputComponents);

                // create the output value
                DataValue outputValue(ret->dataType());
                for (uint32_t i = 0; i < numOutputComponents; ++i)
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

                ret->m_dataValue = outputValue;
                ret->m_dataType = node->m_dataType;
                ret->m_op = OpCode::Const;
                ret->m_children.clear();
                TRACE_DEEP("{}: Swizzle '{}' folded into '{}'", ret->m_loc, mask, outputValue);
            }

            break;
        }

        // fold native function calls (math)
        case OpCode::NativeCall:
        {
            PC_SCOPE_LVL3(NativeCall);
            auto nativeFunction  = ret->extraData().m_nativeFunctionRef;
            ASSERT(nativeFunction != nullptr);

            auto funcName  = nativeFunction->cls()->findMetadataRef<FunctionNameMetadata>().name();
            TRACE_DEEP("{}: trying to fold '{}'", ret->m_loc, funcName);

            // prepare parameters
            Array<ExecutionValue> functionArguments;
            functionArguments.reserve(ret->children().size());

            // evaluate parameters for the function
            bool evaluated = false;
            ExecutionValue result(ret->m_dataType, DataValue(ret->m_dataType));
            for (uint32_t i = 0; i < ret->children().size(); ++i)
            {
                // try to run the function with what we have, maybe it will succeed and we won't have to evaluate the rest of the arguemnts
                // this is the essential part of the "fusing" for some functions, mostly ternary operator and boolean and/or operator
                if (nativeFunction->partialEvaluate(result, functionArguments.size(), functionArguments.typedData()))
                {
                    evaluated = true;
                    break;
                }

                // copy arguments
                auto childNode  = ret->children()[i];
                if (childNode->dataValue().isWholeValueDefined())
                    functionArguments.emplaceBack(childNode->dataType(), childNode->dataValue());
                else
                    functionArguments.emplaceBack(childNode->dataType());
            }

            // execute function
            if (!evaluated)
                nativeFunction->evaluate(result, functionArguments.size(), functionArguments.typedData());

            // if the result is valid, replace the whole crap with just the result
            if (result.isWholeValueDefined())
            {
                if (result.readValue(ret->m_dataValue))
                {
                    TRACE_DEEP("{}: Native call to '{}' folded into '{}'", ret->m_loc, funcName, result);
                    ret->m_op = OpCode::Const;
                    ret->m_children.clear();
                }
            }
            break;
        }

        case OpCode::This:
        {
            if (thisVars != nullptr)
            {
                ret->m_dataValue = DataValue(thisVars);
                ret->m_op = OpCode::Const;
                ret->m_children.clear();
            }
            else
            {
                ret->m_op = OpCode::Nop;
                ret->m_typesResolved = true;
                ret->m_children.clear();
            }
            break;
        }

        case OpCode::ProgramInstanceParam:
        {
            ret->m_dataValue = std::move(ret->m_children[0]->m_dataValue);
            ret->m_children.clear();
            break;
        }

        case OpCode::ProgramInstance:
        {
            PC_SCOPE_LVL3(ProgramInstance);
            ProgramConstants builtConstants;

            auto program  = node->dataType().program();
            TRACE_DEEP("Program: '{}'", program->name());

            // eval the program parameter nodes
            for (auto child  : ret->m_children)
            {
                auto programParamName = child->extraData().m_name;
                auto programParam  = program->findParameter(programParamName, true);
                if (programParam)
                {
                    auto& value = child->dataValue();
                    if (value.isWholeValueDefined())
                    {
                        TRACE_DEEP("Passing '{}' as param '{}' when creating instance of program '{}'", value, programParamName, program->name());
                        builtConstants.constValue(programParam, value);
                    }
                    else
                    {
                        err.reportError(ret->location(), TempString("Parameter '{}' for program '{}' has a value that is not constant at compile time. Pass parameter as function argument.", programParamName, program->name()));
                    }
                }
                else
                {
                    err.reportError(ret->location(), TempString("Parameter '{}' is not declared in program '{}' or any parent programs", programParamName, program->name()));
                }
            }

            auto newInstance = m_code.createProgramInstance(node->location(), program, std::move(builtConstants), err);

            ret->m_dataValue = DataValue(newInstance);
            ret->m_op = OpCode::Const;
            ret->m_children.clear();
            break;
        }

        case OpCode::FuncRef:
        {
            break;
        }

        case OpCode::IfElse:
        {
            PC_SCOPE_LVL3(IfElse);
            struct ValidOptions
            {
                const CodeNode* foldedTest;
                const CodeNode* unfoldedCode;
            };

            InplaceArray<ValidOptions, 10> options;

            // process if cases
            uint32_t i = 0;
            bool hasCollapsed = false;
            while ((i+1) < node->children().size())
            {
                auto testNode  = node->children()[i+0];
                auto foldedTestChild  = foldCode(func, testNode, thisVars, localVars, err);

                // look at the condition, if it has a definitive value than use that information
                auto testValue = foldedTestChild->dataValue();
                TRACE_DEEP("{}: info: if else condition evaluated to {}", node->location(), testValue);

                // in the exploration mode we may still end up with determined case
                if (testValue.valid() && testValue.isWholeValueDefined())
                {
                    // if the branch is taken for sure even in the exploration mode than we can return whatever the code returns
                    auto taken = testValue.component(0).valueBool();
                    if (taken)
                    {
                        TRACE_DEEP("{}: folded into single option as this option will always be taken (condition is always true)", testNode->m_loc);
                        hasCollapsed = true;
                        ret = foldCode(func, node->children()[i + 1], thisVars, localVars, err);
                        break;
                    }
                    else
                    {
                        TRACE_DEEP("{}: folded this option away as it cannot be taken (condition is always false)", testNode->m_loc);
                    }
                }
                else
                {
                    TRACE_DEEP("{}: info: if/else branch not decidable at this point, leaving for runtime", node->location());

                    auto& entry = options.emplaceBack();
                    entry.foldedTest = foldedTestChild;
                    entry.unfoldedCode = node->children()[i + 1];
                }

                i += 2;
            }

            // the else case
            if (!hasCollapsed)
            {
                // add the else case
                if (!options.empty())
                {
                    ret->m_op = OpCode::IfElse;

                    for (auto& entry : options)
                    {
                        ret->m_children.pushBack(entry.foldedTest);

                        auto foldedCode = foldCode(func, entry.unfoldedCode, thisVars, localVars, err);
                        ret->m_children.pushBack(foldedCode);
                    }

                    if (i < node->children().size())
                    {
                        auto elseNode  = node->children()[i];
                        auto foldedElse  = foldCode(func, elseNode, thisVars, localVars, err);
                        ret->m_children.pushBack(foldedElse);
                    }
                }
                else
                {                        
                    if (i < node->children().size())
                    {
                        TRACE_DEEP("{}: info: left only with the else branch, removing the if/else", node->location());
                        auto elseNode  = node->children()[i];
                        return foldCode(func, elseNode, thisVars, localVars, err);
                    } 
                    else
                    {
                        TRACE_DEEP("{}: info: none of the if/else structure was matched", node->location());
                        ret->m_op = OpCode::Nop;
                    }
                }
            }                    
                    
            // empty "if", nothing bad
            break;
        }

		case OpCode::CreateVector:
		{
			const auto numTargetComponents = node->dataType().computeScalarComponentCount();
			ret->m_dataValue = DataValue(node->dataType());

			if (ret->children().size() == 1 && ret->children()[0]->dataType().isNumericalScalar())
			{
				const auto singleValue = ret->children()[0]->dataValue().component(0);
				for (uint32_t i = 0; i < numTargetComponents; ++i)
				{
					ret->m_dataValue.component(i, singleValue);
				}
			}
			else
			{
				uint32_t componentOffset = 0;
				for (const auto* child : ret->children())
				{
					const auto numSourceComponents = child->dataValue().size();
					for (uint32_t i = 0; i < numSourceComponents; ++i)
						ret->m_dataValue.component(i + componentOffset, child->dataValue().component(i));
					componentOffset += numSourceComponents;
				}
			}

			if (ret->m_dataValue.isWholeValueDefined())
			{
				ret->m_op = OpCode::Const;
				ret->m_children.clear();
			}

			break;
		}

		case OpCode::CreateArray:
		{
			const auto numTargetComponents = node->dataType().computeScalarComponentCount();
			ret->m_dataValue = DataValue(node->dataType());

			uint32_t componentOffset = 0;
			for (const auto* child : ret->children())
			{
				const auto numSourceComponents = child->dataValue().size();
				for (uint32_t i = 0; i < numSourceComponents; ++i)
					ret->m_dataValue.component(i + componentOffset, child->dataValue().component(i));
				componentOffset += numSourceComponents;
			}

			if (ret->m_dataValue.isWholeValueDefined())
			{
				ret->m_op = OpCode::Const;
				ret->m_children.clear();
			}

			break;
		}

		case OpCode::CreateMatrix:
		{
			break;
		}

        case OpCode::Call:
        { 
            PC_SCOPE_LVL3(Call);

            auto functionNode  = ret->children()[0];
            auto contextNode  = functionNode->children()[0];
            ASSERT(functionNode->dataType().isFunction());

            // get the program instance
            const ProgramInstance* callThisVars = thisVars;
            {
                // get the function to call
                auto callFunction = functionNode->dataType().function();
                if (callFunction->program() != nullptr)
                {
                    if (contextNode->dataValue().component(0).isProgramInstance())
                    {
                        // get the static context to find the function, needed since we may want to call function in different context
                        auto callProgramInstance = contextNode->dataValue().component(0).programInstance();
                        auto callProgram = callProgramInstance->program();

                        // execute functions in local context unless override
                        //auto callParams = &callProgramInstance->params();

                        // lookup a program function (virtual function in all practical way)
                        auto functionName = callFunction->name();
                        callFunction = callProgram->findFunction(functionName);

                        // Important: if the context for the function call is from the program that we are based on than keep using it to preserve the type of "this"
                        if (!thisVars->program()->isBasedOnProgram(callProgram))
                            callThisVars = callProgramInstance;

                        // store resolved program and function for future reference
                        ret->m_extraData.m_finalFunctionRef = callFunction;
                    }
                    else
                    {
                        err.reportError(ret->location(), TempString("Context for a call to '{}' cannot be determined at compile time. Expression is to complex or depends on values not known at compile time.", callFunction->name()));
                        ret->m_op = OpCode::Nop;
                        ret->m_children.clear();
                    }
                }
                else
                {
                    ret->m_extraData.m_finalFunctionRef = functionNode->dataType().function();
                }
            }

            TRACE_DEEP("Calling '{}' from '{}' in context of '{}'", ret->m_extraData.m_finalFunctionRef->name(),
                ret->m_extraData.m_finalFunctionRef->program() ? ret->m_extraData.m_finalFunctionRef->program()->name() : StringID(),
                callThisVars ? callThisVars->program()->name() : StringID());

            // prepare call
            auto numDefinedParams = ret->children().size() - 1;
            if (ret->m_extraData.m_finalFunctionRef && ret->m_extraData.m_finalFunctionRef->inputParameters().size() == numDefinedParams)
            {
                Array<ExecutionValue> functionArguments;
                functionArguments.reserve(numDefinedParams);

                // evaluate parameters for the function
                // NOTE: not all parameters have to evaluate to compile time constants since inside the function they may be filtered by compile time constants (ie. *0 always gives zero, etc)
                for (uint32_t i = 0; i < numDefinedParams; ++i)
                {
                    auto& childNodeType = ret->m_extraData.m_finalFunctionRef->inputParameters()[i]->dataType;
                    ASSERT(childNodeType.valid());

                    auto argNode  = ret->children()[i + 1];
                    auto& argNodeValue = argNode->dataValue();
                    if (argNodeValue.isWholeValueDefined())
                        functionArguments.emplaceBack(childNodeType, argNodeValue); // use actual value
                    else
                        functionArguments.emplaceBack(childNodeType, DataValue(childNodeType)); // use "undefined" value of matching type
                }

                // create a new execution context for called function
                ExecutionStack childStack(m_mem, m_code, err, ret->m_extraData.m_finalFunctionRef, callThisVars);

                // call function
                const char* functionName = ret->m_extraData.m_finalFunctionRef->name().c_str();
                TRACE_DEEP("{}: info: Evaluating '{}'", ret->location(), functionName);
                {
                    ScopeTimer timer;
                    ExecutionValue result;
                    auto evalRet = childStack.callFunction(functionArguments, result);
                    if (evalRet == ExecutionResult::Finished || evalRet == ExecutionResult::Return)
                    {
                        DataValue foldedValue;
                        if (result.readValue(foldedValue))
                        {
                            TRACE_DEEP("{}: call to '{}' folded into '{}' in {}", ret->m_loc, functionName, result, TimeInterval(timer.timeElapsed()));

                            if (foldedValue.isWholeValueDefined())
                            {
                                ret->m_dataValue = foldedValue;
                                ret->m_dataType = result.type();
                                ret->m_op = OpCode::Const;
                                ret->m_children.clear();
                                break;
                            }
                        }
                    }
                    else
                    {
                        TRACE_DEEP("{}: call to '{}' not folded to a single value and will be evaluated at runtime ({})", ret->m_loc, functionName, (int)evalRet);
                    }
                }
            }

            // we were not folded, but still, the function itself may be worth folding
            if (ret->m_op != OpCode::Const)
            {
                ProgramConstants localFunctionConstants;

                auto numDefinedParams = ret->children().size() - 1;
                if (ret->m_extraData.m_finalFunctionRef && ret->m_extraData.m_finalFunctionRef->inputParameters().size() == numDefinedParams)
                {
                    for (uint32_t i = 0; i < numDefinedParams; ++i)
                    {
                        auto argParam  = ret->m_extraData.m_finalFunctionRef->inputParameters()[i];
                        auto& childNodeType = argParam->dataType;
                        ASSERT(childNodeType.valid());

                        auto argNode  = ret->children()[i + 1];
                        auto& argNodeValue = argNode->dataValue();
                        if (argNodeValue.isWholeValueDefined())
                        {
                            TRACE_DEEP("Setting value of param '{}' to '{}' for child call folding", argParam->name, argNodeValue);
                            localFunctionConstants.constValue(argParam, argNodeValue);
                        }
                    }
                }

                auto functionName = ret->m_extraData.m_finalFunctionRef->name();
                TRACE_DEEP("Folding '{}' with '{}'", functionName, localFunctionConstants);
                ret->m_extraData.m_finalFunctionRef = foldFunction(ret->m_extraData.m_finalFunctionRef, callThisVars, localFunctionConstants, err);

                if (!ret->m_extraData.m_finalFunctionRef)
                {
                    err.reportError(ret->location(), TempString("Unable to process call to function '{}', possible parts of the call can't be determined at compile time.", functionName));
                    ret->m_op = OpCode::Nop;
                    ret->m_children.clear();
                }
            }

            break;
        }
    }

    return ret;
}

Function* FunctionFolder::foldFunction(const Function* original, const ProgramInstance* thisArgs, const ProgramConstants& localArgs, parser::IErrorReporter& err)
{
    PC_SCOPE_LVL1(FoldFunction);

    FoldedFunctionKey key;
    key.func = original;
    key.pi = thisArgs;
    key.localArgsKey = localArgs.key();

    Function* ret = nullptr;
    if (m_foldedFunctionsMap.find(key, ret))
        return ret;

    ret = m_mem.create<Function>(*original, &localArgs);
    m_foldedFunctionsMap[key] = ret; // add before folding to support recursion :P

    {
        auto& nameCounter = m_functionNameCounter[ret->name()];
        if (nameCounter++ > 0)
            ret->m_name = StringID(TempString("{}{}", ret->m_name, nameCounter));
    }

    ret->m_code = foldCode(ret, original->m_code, thisArgs, localArgs, err);
    return ret;
}

///---

END_BOOMER_NAMESPACE_EX(gpu::compiler)

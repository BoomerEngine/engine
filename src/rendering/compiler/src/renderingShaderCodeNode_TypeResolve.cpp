/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\nodes #]
***/

#include "build.h"
#include "renderingShaderCodeNode.h"
#include "renderingShaderNativeFunction.h"
#include "renderingShaderFunction.h"
#include "renderingShaderProgram.h"
#include "renderingShaderProgramInstance.h"
#include "renderingShaderTypeLibrary.h"
#include "renderingShaderCodeLibrary.h"
#include "renderingShaderTypeUtils.h"
#include "base/containers/include/inplaceArray.h"
#include "base/memory/include/linearAllocator.h"

namespace rendering
{
    namespace compiler
    {
        //---

        static TypeMatchTypeConv MatchBaseType(BaseType currentType, BaseType requiredType)
        {
            // Base type matches
            if (currentType == requiredType)
                return TypeMatchTypeConv::Matches;

            // Int/Uint -> float
            if (requiredType == BaseType::Float)
                return TypeMatchTypeConv::ConvToFloat;
            else if (requiredType == BaseType::Int)
                return TypeMatchTypeConv::ConvToInt;
            else if (requiredType == BaseType::Uint)
                return TypeMatchTypeConv::ConvToUint;
            else if (requiredType == BaseType::Boolean)
                return TypeMatchTypeConv::ConvToBool;

            // no conversion possible
            return TypeMatchTypeConv::NotMatches;
        }

        static TypeMatchTypeExpand MatchScalarExpand(uint32_t numRequiredComponents)
        {
            if (numRequiredComponents == 2)
                return TypeMatchTypeExpand::ExpandXX;
            else if (numRequiredComponents == 3)
                return TypeMatchTypeExpand::ExpandXXX;
            else if (numRequiredComponents == 4)
                return TypeMatchTypeExpand::ExpandXXXX;

            FATAL_ERROR("You lied!");
            return TypeMatchTypeExpand::DontExpand;
        }

        TypeMatchResult MatchType(const DataType& currentType, const DataType& requiredType)
        {
            // if type matches directly we are home
            if (DataType::MatchNoFlags(requiredType, currentType))
                return TypeMatchResult(TypeMatchTypeConv::Matches);

            // program cast, the source program must implement the target program interface
            if (currentType.isProgram() && requiredType.isProgram())
            {
                //TRACE_INFO("Program match {} to {}", currentType, requiredType);
                if (!requiredType.program() || currentType.program()->isBasedOnProgram(requiredType.program()))
                    return TypeMatchResult(TypeMatchTypeConv::Matches);
                else
                    return TypeMatchResult(TypeMatchTypeConv::NotMatches);
            }

            // scalar types convert easily
            if (currentType.isScalar() && requiredType.isScalar())
                return TypeMatchResult(MatchBaseType(currentType.baseType(), requiredType.baseType()));

            // vector types with numerical components convert as well
            if (currentType.isNumericalVectorLikeOperand(true) && requiredType.isNumericalVectorLikeOperand())
            {
                if (ExtractComponentCount(currentType) == ExtractComponentCount(requiredType))
                {
                    auto currentBaseType = ExtractBaseType(currentType);
                    auto requiredBaseType = ExtractBaseType(requiredType);
                    return TypeMatchResult(MatchBaseType(currentBaseType, requiredBaseType));
                }
            }

            // we can expand scalar types to SOME special composite types
            if (currentType.isScalar() && requiredType.isComposite() && requiredType.composite().hint() != CompositeTypeHint::User)
            {
                auto requiredBaseType = ExtractBaseType(requiredType);
                auto requiredComponentCount = ExtractComponentCount(requiredType);
                return TypeMatchResult(
                    MatchBaseType(currentType.baseType(), requiredBaseType),
                    MatchScalarExpand(requiredComponentCount));
            }

            // no conversion possible for other types
            return TypeMatchResult();
        }

        static bool RequiresExplicitCast(TypeMatchResult conversionType)
        {
            switch (conversionType.m_conv)
            {
            case TypeMatchTypeConv::Matches:
            case TypeMatchTypeConv::ConvToBool:
            case TypeMatchTypeConv::ConvToFloat:
            case TypeMatchTypeConv::ConvToUint:
            case TypeMatchTypeConv::ConvToInt:
                return false;
            }

            return true;
        }

		static bool AllowedVectorConstructionType(BaseType type)
		{
			switch (type)
			{
				case BaseType::Float:
				case BaseType::Int:
				case BaseType::Uint:
				case BaseType::Boolean:
					return true;
			}

			return false;
		}

		static bool AllowedMatrixConstructionType(BaseType type)
		{
			switch (type)
			{
			case BaseType::Float:
				return true;
			}

			return false;
		}
		
        static DataType GetCastedType(CodeLibrary& lib, const DataType& sourceType, TypeMatchTypeConv convType)
        {
            auto componentCount = ExtractComponentCount(sourceType);

            switch (convType)
            {
            case TypeMatchTypeConv::Matches:
                return sourceType;

            case TypeMatchTypeConv::ConvToInt:
                return lib.typeLibrary().simpleCompositeType(BaseType::Int, componentCount);

            case TypeMatchTypeConv::ConvToBool:
                return lib.typeLibrary().simpleCompositeType(BaseType::Boolean, componentCount);

            case TypeMatchTypeConv::ConvToUint:
                return lib.typeLibrary().simpleCompositeType(BaseType::Uint, componentCount);

            case TypeMatchTypeConv::ConvToFloat:
                return lib.typeLibrary().simpleCompositeType(BaseType::Float, componentCount);
            }

            FATAL_ERROR("Invalid type conversion mode");
            return DataType();
        }

        //---

        CodeNode* CodeNode::InsertTypeCastNode(CodeLibrary& lib, CodeNode* nodePtr, TypeMatchTypeConv convType)
        {
            // no conversion needed
            if (convType == TypeMatchTypeConv::Matches)
                return nodePtr;

            // create the call node
            auto ret  = lib.allocator().create<CodeNode>(nodePtr->location(), OpCode::Cast);
			ret->extraData().m_castMode = convType;
            ret->m_dataType = GetCastedType(lib, nodePtr->dataType(), convType);
            ret->m_typesResolved = true;
            ret->m_children.pushBack(nodePtr);
            return ret;
        }

        CodeNode* CodeNode::InsertScalarExtensionNode(CodeLibrary& lib, CodeNode* nodePtr, TypeMatchTypeExpand expandType)
        {
            auto currentType = nodePtr->dataType();

            // no expand required
            if (expandType == TypeMatchTypeExpand::DontExpand)
                return nodePtr;

            ASSERT(currentType.isScalar());

            // create a swizzle node
            ComponentMask readSwizzle;
            if (expandType == TypeMatchTypeExpand::ExpandXX)
                readSwizzle = ComponentMask(ComponentMaskBit::X, ComponentMaskBit::X, ComponentMaskBit::NotDefined, ComponentMaskBit::NotDefined);
            else if (expandType == TypeMatchTypeExpand::ExpandXXX)
                readSwizzle = ComponentMask(ComponentMaskBit::X, ComponentMaskBit::X, ComponentMaskBit::X, ComponentMaskBit::NotDefined);
            if (expandType == TypeMatchTypeExpand::ExpandXXXX)
                readSwizzle = ComponentMask(ComponentMaskBit::X, ComponentMaskBit::X, ComponentMaskBit::X, ComponentMaskBit::X);

            // insert the node
            auto node  = lib.allocator().create<CodeNode>(nodePtr->location(), OpCode::ReadSwizzle);
            node->extraData().m_mask = readSwizzle;
            node->m_children.pushBack(nodePtr);
            node->m_dataType = GetExpandedType(lib.typeLibrary(), nodePtr->dataType(), expandType);
            node->m_typesResolved = true;
            return node;
        }

        CodeNode* CodeNode::Dereferece(CodeLibrary& lib, CodeNode* nodePtr, base::parser::IErrorReporter& err)
        {
            // if the node is a reference make it a value by inserting a Load opcode
            if (nodePtr && nodePtr->m_dataType.isReference())
            {
                auto loadNode  = lib.allocator().create<CodeNode>(nodePtr->location(), OpCode::Load);
                loadNode->m_dataType = nodePtr->m_dataType.unmakePointer();
                loadNode->m_typesResolved = true;
                loadNode->addChild(nodePtr);
                return loadNode;
            }

            // nothing changed
            return nodePtr;
        }

        bool CodeNode::MatchNodeType(CodeLibrary& lib, CodeNode*& nodePtr, DataType requiredType, bool userExplicitCast, base::parser::IErrorReporter& err)
        {
            // source type is 
            auto currentType = nodePtr->dataType();

            // we allow cast of constants
            bool explicitCast = userExplicitCast || (nodePtr->opCode() == OpCode::Const);

            // node has no type
            if (!currentType.valid())
            {
                err.reportError(nodePtr->location(), base::TempString("Undetermined value"));
                return false;
            }
            
            // we can't convert from a value to reference or from const type to non-const type
            if (requiredType.isReference() && !currentType.isReference())
            {
                err.reportError(nodePtr->location(), base::TempString("Unable to convert from non-LValue '{}' to '{}'", currentType, requiredType));
                return false;
            }

            // if the target type is not a reference make sure the input argument is dereferenced
            if (!requiredType.isReference())
                nodePtr = Dereferece(lib, nodePtr, err);

            // normal casting
            auto matchType = MatchType(currentType.unmakePointer().unmakePointer(), requiredType.unmakePointer().unmakePointer());
            if (matchType.m_conv == TypeMatchTypeConv::NotMatches)
            {
                err.reportError(nodePtr->location(), base::TempString("Unable to convert from '{}' to '{}'", nodePtr->dataType(), requiredType));
                return false;
            }
            else if (matchType.m_conv == TypeMatchTypeConv::Matches && matchType.m_expand == TypeMatchTypeExpand::DontExpand)
            {
                return true;
            }

            // cannot convert if the target type requires a reference
            if (requiredType.isReference())
            {
                err.reportError(nodePtr->location(), base::TempString("Unable to convert from '{}' to '{}'", nodePtr->dataType(), requiredType));
                return false;
            }

            // cast requires explicit casting
            if (!explicitCast && RequiresExplicitCast(matchType))
            {
                err.reportError(nodePtr->location(), base::TempString("Casting from '{}' to '{}' requires explicit casting", nodePtr->dataType(), requiredType));
                return false;
            }

            // create casting nodes, this is not supposed to fail so no error checking
            nodePtr = Dereferece(lib, nodePtr, err);
            nodePtr = InsertScalarExtensionNode(lib, nodePtr, matchType.m_expand);
            nodePtr = InsertTypeCastNode(lib, nodePtr, matchType.m_conv);
            ASSERT(!nodePtr->dataType().isReference());
            return true;
        }

        void CodeNode::LinkScopes(CodeNode* node, CodeNode* parentScopeNode)
        {
			/*TRACE_INFO("Node {} at {}, {} children", node->opCode(), node->location(), node->children().size());
			for (uint32_t i : node->children().indexRange())
			{
				const auto* child = node->children()[i];
				TRACE_INFO("  Child[i] {} at {} (0x{})", i, child->opCode(), child->location(), Hex((uint64_t)child));
			}*/

            ASSERT(!node->m_extraData.m_parentScopeRef);
            node->m_extraData.m_parentScopeRef = parentScopeNode;

            if (node->m_op == OpCode::Scope)
                parentScopeNode = node;


            for (auto child  : node->m_children)
                if (child != nullptr)
                    LinkScopes(const_cast<CodeNode *>(child), parentScopeNode);
        }

        CodeNode::ResolveResult::ResolveResult()
            : m_param(nullptr)
            , m_function(nullptr)
            , m_nativeFunction(nullptr)
            , m_program(nullptr)
        {}

        CodeNode::ResolveResult::ResolveResult(const DataParameter* param, const Program* program)
            : m_param(param)
            , m_function(nullptr)
            , m_nativeFunction(nullptr)
            , m_program(program)
        {}

        CodeNode::ResolveResult::ResolveResult(const Function* func)
            : m_param(nullptr)
            , m_function(func)
            , m_nativeFunction(nullptr)
            , m_program(nullptr)
        {}

        CodeNode::ResolveResult::ResolveResult(const INativeFunction* func)
            : m_param(nullptr)
            , m_function(nullptr)
            , m_nativeFunction(func)
            , m_program(nullptr)
        {}

        CodeNode::ResolveResult CodeNode::ResolveIdent(CodeLibrary& lib, const Program* program, const Function* func, CodeNode* contextNode, const base::StringID name, base::parser::IErrorReporter& err)
        {
            // built-in ?
            if (name.view().beginsWith("gl_"))
            {
                auto builtin = program->createBuildinParameterReference(name);
                if (nullptr == builtin)
                {
                    err.reportError(contextNode->location(), base::TempString("Unrecognized built-in parameter '{}'", name));
                    return CodeNode::ResolveResult();
                }

                return CodeNode::ResolveResult(builtin);
            }

            // try as local variable in the scope
            if (contextNode)
            {
                auto scope  = contextNode->extraData().m_parentScopeRef;
                while (scope)
                {
                    // look at the declared variables
                    for (auto declaration  : scope->m_declarations)
                        if (declaration->name == name)
                            return declaration;

                    // go to parent scope
                    scope = scope->m_extraData.m_parentScopeRef;
                }
            }

            // try to resolve as a function input
            if (func)
            {
                for (auto arg  : func->inputParameters())
                {
                    if (arg->name == name)
                    {
                        return arg;
                    }
                }
            }

            // maybe it's a function in program
            if (program)
            {
                if (auto programFunction = program->findFunction(name))
                    return programFunction;

                if (auto programParam = program->findParameter(name))
                    return CodeNode::ResolveResult(programParam, program);

                // maybe it's a part of descriptor
                {
                    base::InplaceArray< ResolvedDescriptorEntry, 10> entries;
                    lib.findGlobalDescriptorEntry(name, entries);
                    lib.findGlobalUniformEntry(name, entries);

                    if (entries.size() > 1)
                    {
                        err.reportError(contextNode->location(), base::TempString("Name '{}' matched multiple elements of descriptors", name));
                        for (const auto& entry : entries)
                        {
                            if (entry.member)
                                err.reportError(entry.member->location, base::TempString("Constant buffer entry in descriptor '{}'", entry.table->name()));
                            else
                                err.reportError(entry.entry->m_location, base::TempString("Resource entry in descriptor '{}'", entry.table->name()));
                        }
                        return CodeNode::ResolveResult();
                    }
                    else if (entries.size() == 1)
                    {
                        return program->createDescriptorElementReference(entries[0]);
                    }
                }
            }

            // look for global param
            if (auto globalConstant  = lib.findGlobalConstant(name))
                return globalConstant;

            // look for global function
            if (auto globalFunction  = lib.findGlobalFunction(name))
                return globalFunction;

            // last resort - built in function
            if (auto nativeFunctionRef  = INativeFunction::FindFunctionByName(name.c_str()))
                return nativeFunctionRef;

            // not resolved
            return ResolveResult();
        }

        bool CodeNode::MutateNode(CodeLibrary& lib, const Program* program, const Function* func, CodeNode*& node, base::Array<CodeNode*>& parentStack, base::parser::IErrorReporter& err)
        {
            // replace null nodes with nodes
            if (!node)
            {
				node = lib.allocator().create<CodeNode>(parentStack.back()->location(), OpCode::Nop);
                return true;
            }

            // mutators that don't care about children
            auto op = node->opCode();
            auto& value = node->extraData();
            switch (op)
            {
                case OpCode::Ident:
                {
                    // resolve as descriptor
                    auto baseDescriptor  = lib.typeLibrary().findResourceTable(value.m_name);
                    if (baseDescriptor)
                    {
                        node->m_op = OpCode::ResourceTable;
                        node->m_extraData.m_resourceTable = baseDescriptor;
                        return true;
                    }

                    // resolve as program name
                    auto baseProgram  = lib.findProgram(value.m_name);
                    if (baseProgram)
                    {
                        node->m_op = OpCode::ProgramInstance; // create empty program instance, it will inherit the permutations but will retain default constants
                        node->m_dataType = DataType(baseProgram);
                        node->m_typesResolved = true;
                        return true;
                    }

                    break;
                }
            }

            // mutate the node children
            parentStack.pushBack(node);
            for (uint32_t i=0; i<node->m_children.size(); ++i)
                if (!MutateNode(lib, program, func, *(CodeNode**)&node->m_children[i], parentStack, err))
                    return false;

            ASSERT(parentStack.back() == node);
            parentStack.popBack();

            switch (op)
            {
                case OpCode::Call:
                {
                    auto callContextNode  = const_cast<CodeNode*>(node->m_children[0]);
                    if (callContextNode->m_op == OpCode::Ident)
                    {
                        // convert the pipeline state settings
                        auto funcName = callContextNode->m_extraData.m_name.view();
                        if (node->m_children.size() == 3 && funcName.beginsWith("__assign"))
                        {
                            auto extraFunctionName = funcName.afterFirst("__assign");
                            if (extraFunctionName.empty())
                            {
                                // create the normal assign node
                                auto storeNode  = lib.allocator().create<CodeNode>(node->location(), OpCode::Store);
                                storeNode->addChild(node->m_children[1]);
                                storeNode->addChild(node->m_children[2]);

                                // use the store node
                                node = storeNode;
                            }
                            else
                            {
                                auto storeNode = lib.allocator().create<CodeNode>(node->location(), OpCode::Store);
                                storeNode->addChild(node->m_children[1]);

                                auto identNode = lib.allocator().create<CodeNode>(node->location(), OpCode::Ident);
                                identNode->extraData().m_name = extraFunctionName;

                                auto opNode = lib.allocator().create<CodeNode>(node->location(), OpCode::Call);
                                opNode->addChild(identNode);
                                opNode->addChild(node->m_children[1]->clone(lib.allocator()));
                                opNode->addChild(node->m_children[2]);
                                storeNode->addChild(opNode);

                                // switch to store node
                                node = storeNode;
                            }
                        }
						else if (node->m_children.size() > 1 && (funcName == "__create_vector" || funcName == "__create_matrix"))
						{
							if (node->m_extraData.m_castType.isComposite())
							{
								const auto hint = node->m_extraData.m_castType.composite().hint();
								if (hint == CompositeTypeHint::VectorType)
								{
									node->m_op = OpCode::CreateVector;
									node->m_children.erase(0);
								}
								else if (hint == CompositeTypeHint::MatrixType)
								{
									node->m_op = OpCode::CreateMatrix;
									node->m_children.erase(0);
								}
							}
						}
						else if (node->m_children.size() >= 1 && (funcName == "__create_array"))
						{
							node->m_op = OpCode::CreateArray;
							node->m_children.erase(0);
						}
					}

                    break;
                }

                case OpCode::AccessMember:
                {
                    // state member
                    if (node->m_children.size() == 1)
                    {
                        if (node->m_children[0]->opCode() == OpCode::ResourceTable)
                        {
                            auto resourceTable = node->m_children[0]->extraData().m_resourceTable;

                            // check what did we mean by "name"
                            base::InplaceArray<ResolvedDescriptorEntry, 10> resolvedEntries;
                            lib.findDescriptorEntry(resourceTable, node->m_extraData.m_name, resolvedEntries);
                            lib.findUniformEntry(resourceTable, node->m_extraData.m_name, resolvedEntries);
                            if (resolvedEntries.empty())
                            {
                                err.reportError(node->location(), base::TempString("Descriptor '{}' does not contain '{}'", resourceTable->name(), value.m_name));
                                return false;
                            }
                            else if (resolvedEntries.size() != 1)
                            {
                                err.reportError(node->location(), base::TempString("Descriptor '{}' contains more than one '{}', name is ambiguous", resourceTable->name(), value.m_name));
                                return false;
                            }

                            // we can't access the constant buffer directly
                            const auto& resolvedEntry = resolvedEntries[0];
                            if (resolvedEntry.entry->m_type.resource().type == DeviceObjectViewType::ConstantBuffer && resolvedEntry.member == nullptr)
                            {
                                err.reportError(node->location(), base::TempString("Constant buffer '{}' in descriptor '{}' can't be accessed directly, only it members can", resolvedEntry.entry->m_name, resourceTable->name()));
                                return false;
                            }

                            // create the parameter reference
                            const auto* paramRef = program->createDescriptorElementReference(resolvedEntry);
                            if (!paramRef)
                            {
                                err.reportError(node->location(), base::TempString("Unable to access '{}' in descriptor '{}'", value.m_name, resourceTable->name()));
                                return false;
                            }

							if (paramRef->dataType.isResource())
							{
								const auto key = base::StringID(base::TempString("res:{}.{}", resolvedEntry.table->name(), resolvedEntry.entry->m_name));

								node->m_op = OpCode::Const;
								node->m_dataType = paramRef->dataType.unmakePointer();
								node->m_dataValue = DataValue(key);
								node->m_typesResolved = true;
							}
							else
							{
								node->m_op = OpCode::ParamRef;
								node->m_extraData.m_paramRef = paramRef;
								node->m_children.clear();
								node->m_dataType = node->m_extraData.m_paramRef->dataType;
								node->m_typesResolved = true;
							}
                            
                            return true;
                        }
                        
                    }

                    break;
                }

                case OpCode::VariableDecl:
                {
                    // we need to have a scope to declare a variable
                    if (value.m_parentScopeRef == nullptr)
                    {
                        err.reportError(node->location(), base::TempString("Unable to declare '{}' without a proper parent scope", value.m_name));
                        return false;
                    }

                    // check if the name does not alias something in the current scope
                    for (auto declaration  : value.m_parentScopeRef->m_declarations)
                    {
                        if (declaration->name == value.m_name)
                        {
                            err.reportError(node->location(), base::TempString("Variable '{}' was already declared at {}", value.m_name, declaration->loc));
                            return false;
                        }
                    }

                    // make sure type is known
                    if (!value.m_castType.valid())
                    {
                        err.reportError(node->location(), base::TempString("Unable to declare '{}' without a proper type", value.m_name));
                        return false;
                    }

                    // if we are compile time we must have a initializer
                    if (node->m_children.empty() && value.m_rawAttribute)
                    {
                        err.reportError(node->location(), base::TempString("Value '{}' was declared as constant and must be initialized right away", value.m_name));
                        return false;
                    }

                    // create the local variable in the scope
                    auto localVar = lib.allocator().create<DataParameter>();
                    localVar->name = value.m_name;
                    localVar->scope = DataParameterScope::ScopeLocal;
					localVar->dataType = value.m_castType;
					localVar->loc = node->location();
					localVar->assignable = (value.m_rawAttribute == 0);

					// add local variable to parent scope
                    value.m_parentScopeRef->m_declarations.pushBack(localVar);

					// link variable 
					node->extraData().m_paramRef = localVar;

					// if we are a composite type initialize the members with the initialization code
					if (node->m_children.empty() && localVar->dataType.isComposite())
                    {
                        uint32_t numMembersWithInitialization = 0;
                        auto& compositeType = localVar->dataType.composite();
                        for (auto& member : compositeType.members())
                        {
                            if (member.initializerCode)
                                numMembersWithInitialization += 1;
                        }

                        // warn if not all members were initialized
                        if (numMembersWithInitialization && numMembersWithInitialization != compositeType.members().size())
                            err.reportWarning(node->location(), base::TempString("Not all members of '{}' are initialized", compositeType.name()));

                        // emit the initialization code
                        if (0 != numMembersWithInitialization)
                        {
							auto* scopeNode = lib.allocator().create<CodeNode>(node->m_loc, OpCode::Scope);
							scopeNode->addChild(node);
							node = scopeNode;

                            for (auto& member : compositeType.members())
                            {
                                if (member.initializerCode)
                                {
                                    auto paramRefNode = lib.allocator().create<CodeNode>(node->m_loc, OpCode::Ident);
                                    paramRefNode->m_extraData.m_name = value.m_name;
                                    paramRefNode->m_extraData.m_parentScopeRef = value.m_parentScopeRef;
                                    paramRefNode->m_dataType = localVar->dataType;

                                    auto memberRefNode = lib.allocator().create<CodeNode>(node->m_loc, OpCode::AccessMember);
                                    memberRefNode->m_extraData.m_name = member.name;
                                    memberRefNode->m_extraData.m_parentScopeRef = value.m_parentScopeRef;
                                    memberRefNode->m_dataType = member.type;
                                    memberRefNode->addChild(paramRefNode);

                                    auto storeNode = lib.allocator().create<CodeNode>(node->m_loc, OpCode::Store);
                                    storeNode->m_op = OpCode::Store;
                                    storeNode->m_dataType = DataType(); // no expected value of the store
                                    storeNode->addChild(memberRefNode);
                                    storeNode->addChild(member.initializerCode);

                                    scopeNode->addChild(storeNode);
                                }
                            }
                        }
                    }

                    break;
                }

				case OpCode::IfElse:
				{
					ASSERT(node->m_children.size() <= 3);
					if (node->m_children.size() == 3) // we have the else
					{
						auto* elseNode = node->m_children[2];
						if (elseNode->opCode() == OpCode::IfElse) // suck in the if inside the else as our own if/else
						{
							node->m_children.popBack(); // remove the else
							node->m_children.pushBack(elseNode->m_children.typedData(), elseNode->m_children.size());
						}
					}

					break;
				}
            }

            return true;
        }

        bool CodeNode::ResolveTypes(CodeLibrary& lib, const Program* program, const Function* func, CodeNode* node, base::parser::IErrorReporter& err)
        {
            ASSERT(node != nullptr);

            // node is resolved already (probably reference from other part of the tree)
            if (node->m_typesResolved)
                return true;

            // mark as resolved
            node->m_typesResolved = true;

            // resolve current types at the child nodes
            // NOTE: the resolving may CHANGE the node pointer
            {
                bool allTypesResolved = true;
                auto numChildren = node->children().size();
                for (uint32_t i = 0; i < numChildren; ++i)
                {
                    auto child  = const_cast<CodeNode*>(node->m_children[i]);

                    // resolve types in the child
                    if (!ResolveTypes(lib, program, func, child, err))
                        allTypesResolved = false;

                    // suck in the cast nodes, hack...
                    if (node->m_children[i] && (node->m_children[i]->m_op == OpCode::Cast))
                    {
                        auto castNode  = node->m_children[i];
                        node->m_children[i] = node->m_children[i]->m_children[0];
                        castNode->~CodeNode();
                    }
                }

                if (!allTypesResolved)
                    return false;
            }

            // some expressions require
            auto op = node->opCode();
            switch (op)
            {
                case OpCode::This:
					return ResolveTypes_This(lib, program, func, node, err);
               
                case OpCode::Load:
					return ResolveTypes_Load(lib, program, func, node, err);
               
                case OpCode::Store:
					return ResolveTypes_Store(lib, program, func, node, err);

				case OpCode::VariableDecl:
					return ResolveTypes_VariableDecalration(lib, program, func, node, err);
                
                case OpCode::Ident:
					return ResolveTypes_Ident(lib, program, func, node, err);
                
                case OpCode::AccessArray:
					return ResolveTypes_AccessArray(lib, program, func, node, err);

                case OpCode::AccessMember:
					return ResolveTypes_AccessMember(lib, program, func, node, err);

                case OpCode::Cast:
					return ResolveTypes_Cast(lib, program, func, node, err);
               
                case OpCode::Call:
					return ResolveTypes_Call(lib, program, func, node, err);

                case OpCode::IfElse:
					return ResolveTypes_IfElse(lib, program, func, node, err);

                case OpCode::Loop:
					return ResolveTypes_Loop(lib, program, func, node, err);

                case OpCode::ProgramInstance:
					return ResolveTypes_ProgramInstance(lib, program, func, node, err);

				case OpCode::CreateVector:
					return ResolveTypes_CreateVector(lib, program, func, node, err);

				case OpCode::CreateMatrix:
					return ResolveTypes_CreateMatrix(lib, program, func, node, err);

				case OpCode::CreateArray:
					return ResolveTypes_CreateArray(lib, program, func, node, err);

                case OpCode::Return:
					return ResolveTypes_Return(lib, program, func, node, err);

				// empty shit
				case OpCode::Nop:
				case OpCode::Const:
				case OpCode::Break:
				case OpCode::Continue:
				case OpCode::Exit:
				case OpCode::Scope:
				case OpCode::ProgramInstanceParam:
					return true;

                // unsupported node
                default:
                {
                    ASSERT(!"Unhandled node type");
                }
            }

            // if we get here there were no errors
            return true;
        }

        bool CodeNode::HandleResourceArrayAccess(CodeLibrary& lib, CodeNode* node, base::parser::IErrorReporter& err)
        {
            auto sourceType = node->m_children[0]->dataType();

            DEBUG_CHECK(sourceType.isResource());
            const auto& res = sourceType.resource();

            if (res.type == DeviceObjectViewType::Image || res.type == DeviceObjectViewType::ImageWritable)
            {
                // we need multi-dimensional index to access 2D/3D textures like that
                const auto numCoords = res.addressComponentCount();

				// texel loading always uses integer coordinates
				const auto arrayIndexType = lib.typeLibrary().integerType(numCoords);
				if (!MatchNodeType(lib, (CodeNode*&)node->m_children[1], arrayIndexType, true, err))
					return false;

                // unknown texture format ?
                if (res.resolvedFormat == ImageFormat::UNKNOWN)
                {
                    err.reportError(node->location(), base::TempString("Texture has no format defined and cannot be accessed as array"));
                    return false;
                }

                // our output depends on the texture format
                node->m_dataType = lib.typeLibrary().packedFormatElementType(res.resolvedFormat);
                if (!node->m_dataType.valid())
                {
                    err.reportError(node->location(), base::TempString("Unable to determine texel type for texture"));
                    return false;
                }

                // for UAV textures the output may be writable
                auto writable = (res.type == DeviceObjectViewType::ImageWritable);
                if (writable)
                    node->m_dataType = node->m_dataType.makePointer().makeAtomic();

                return true;
            }
			else if (res.type == DeviceObjectViewType::SampledImage)
			{
				// TODO: add support for texture tables (unbounded tables of descriptors/bindless texture arrays)

				// we need multi-dimensional index to access 2D/3D textures like that
				const auto numCoords = res.addressComponentCount();

				// texel loading always uses integer coordinates
				const auto arrayIndexType = lib.typeLibrary().integerType(numCoords);
				if (!MatchNodeType(lib, (CodeNode*&)node->m_children[1], arrayIndexType, true, err))
					return false;

				// all textures are always return 4 component value but it can be signed/unsigned
				ASSERT(res.sampledImageFlavor == BaseType::Float || res.sampledImageFlavor == BaseType::Int || res.sampledImageFlavor == BaseType::Uint);
				node->m_dataType = lib.typeLibrary().simpleCompositeType(res.sampledImageFlavor, 4);
				return true;
			}
            else if (res.type == DeviceObjectViewType::BufferStructured || res.type == DeviceObjectViewType::BufferWritable 
				|| res.type == DeviceObjectViewType::BufferStructuredWritable || res.type == DeviceObjectViewType::Buffer)
            {
                // all buffers can be indexed with one coordinate
                //const auto arrayIndexType = lib.typeLibrary().unsignedType();
				const auto arrayIndexType = lib.typeLibrary().integerType(); // addressing functions want int's for some reason
                if (!MatchNodeType(lib, (CodeNode*&)node->m_children[1], arrayIndexType, true, err))
                    return false;

                // if buffer has layout struct than it's a "structured buffer" and output is a struct
                if (res.resolvedLayout != nullptr)
                {
                    node->m_dataType = DataType(res.resolvedLayout);
                }
                else if (res.resolvedFormat != ImageFormat::UNKNOWN)
                {
                    node->m_dataType = lib.typeLibrary().packedFormatElementType(res.resolvedFormat);
                }

                if (!node->m_dataType.valid())
                {
                    err.reportError(node->location(), base::TempString("Unable to determine element type for buffer"));
                    return false;
                }

                // for UAV buffers the output may be writable
				auto writable = (res.type == DeviceObjectViewType::BufferWritable  || res.type == DeviceObjectViewType::BufferStructuredWritable) && !res.attributes.has("readonly"_id);
                if (writable)
                    node->m_dataType = node->m_dataType.makePointer().makeAtomic();
                return true;
            }

            err.reportError(node->location(), base::TempString("This resource can't be accessed like an array"));
            return false;
        }

		//--

		bool CodeNode::ResolveTypes_This(SHADER_RESOLVE_FUNC)
		{
			if (!program)
			{
				err.reportError(node->location(), "'this' can only be used when function is member of a program/shader");
				return false;
			}

			node->m_dataType = DataType(program);
			return true;
		}

		bool CodeNode::ResolveTypes_Load(SHADER_RESOLVE_FUNC)
		{
			// we can only load from reference
			auto loadedType = node->m_children[0]->dataType();
			if (!loadedType.isReference())
			{
				err.reportError(node->location(), "Explicit loading requires a reference");
				return false;
			}

			node->m_dataType = loadedType.unmakePointer();
			return true;
		}

		bool CodeNode::ResolveTypes_Store(SHADER_RESOLVE_FUNC)
		{
			// pull the mask in
			auto targetType = node->m_children[0]->dataType();
			if (node->m_children[0]->m_op == OpCode::ReadSwizzle)
			{
				auto& mask = node->m_children[0]->m_extraData.m_mask;
				if (!mask.isMask())
				{
					err.reportError(node->location(), base::TempString("Swizzle '{}' is not a valid write mask for '{}'", mask, targetType));
					return false;
				}

				// pull in the mask
				node->m_extraData.m_mask = mask;
				node->m_children[0] = node->m_children[0]->m_children[0];

				// un-load (to make a reference again)
				if (node->m_children[0]->m_op == OpCode::Load)
					node->m_children[0] = node->m_children[0]->m_children[0];

				targetType = node->m_children[0]->dataType();
			}

			// we can only store into references
			if (!targetType.isReference())
			{
				err.reportError(node->location(), base::TempString("Cannot write to '{}', type is not a reference", targetType));
				return false;
			}

			// if the store has a mask use it to limit the write
			if (node->m_extraData.m_mask.valid())
				targetType = GetContractedType(lib.typeLibrary(), targetType, node->m_extraData.m_mask.numberOfComponentsProduced());

			// convert the source into the target type, note that the type should be dereferenced (will generate a load)
			auto targetTypeNoRef = targetType.unmakePointer();
			if (!MatchNodeType(lib, (CodeNode*&)node->m_children[1], targetTypeNoRef, false, err))
				return false;

			// the assignment has the same type as the target and is, of course, read only
			node->m_dataType = targetTypeNoRef;
			return true;
		}

		bool CodeNode::ResolveTypes_VariableDecalration(SHADER_RESOLVE_FUNC)
		{
			// variable declaration node has not type on it's own 
			node->m_dataType = DataType();

			// if we have no children we have nothing to do
			if (node->m_children.empty())
				return true;

			// make sure initialization code matches type
			const auto& variableType = node->extraData().m_paramRef->dataType;

			// convert the source into the target type, note that the type should be dereferenced (will generate a load)
			// NOTE: we assume variable initialization can use explicit casting, ie, float a = 5 will cast just fine.
			auto targetTypeNoRef = variableType.unmakePointer();
			return MatchNodeType(lib, (CodeNode*&)node->m_children[0], targetTypeNoRef, true, err);
		}

		bool CodeNode::ResolveTypes_Ident(SHADER_RESOLVE_FUNC)
		{
			// resolve the identifier
			const auto& paramName = node->m_extraData.m_name;
			auto resolvedIdent = ResolveIdent(lib, program, func, node, paramName, err);
			if (!resolvedIdent)
			{
				err.reportError(node->location(), base::TempString("Unknown reference to '{}'", paramName));
				auto resolvedIdent2 = ResolveIdent(lib, program, func, node, paramName, err);
				return false;
			}

			// assign type from param
			if (resolvedIdent.m_param)
			{
				// there's a trick with resource references: they are used as names internally to allow us to pass them through functions etc
				if (resolvedIdent.m_param->dataType.isResource())
				{
					DEBUG_CHECK(resolvedIdent.m_param->scope == DataParameterScope::GlobalParameter);

					// make ourselves a constant
					node->m_op = OpCode::Const;
					node->m_dataType = resolvedIdent.m_param->dataType.unmakePointer();

					// make resource key
					const auto key = base::StringID(base::TempString("res:{}.{}", resolvedIdent.m_param->resourceTable->name(), resolvedIdent.m_param->resourceTableEntry->m_name));
					node->m_dataValue = DataValue(key);
				}
				else
				{
					// the node's return type depends on the parameter type
					node->m_op = OpCode::ParamRef;
					node->m_extraData.m_paramRef = resolvedIdent.m_param;

					if (resolvedIdent.m_param->scope == DataParameterScope::FunctionInput)
					{
						DEBUG_CHECK(resolvedIdent.m_param->assignable);
					}

					if (resolvedIdent.m_param->assignable)
						node->m_dataType = resolvedIdent.m_param->dataType.makePointer();
					else
						node->m_dataType = resolvedIdent.m_param->dataType;
				}
			}
			// assign type from function
			else if (resolvedIdent.m_function)
			{
				// the node's return type depends on the parameter type
				node->m_op = OpCode::FuncRef;
				node->m_dataType = DataType(resolvedIdent.m_function);

				// sub node (call context for the function)
				if (program != nullptr)
				{
					auto thisNode = lib.allocator().create<CodeNode>(node->location(), OpCode::This);
					thisNode->m_dataType = DataType(program);
					thisNode->m_typesResolved = true;
					node->addChild(thisNode);
				}
				else
				{
					auto nopNode = lib.allocator().create<CodeNode>(node->location(), OpCode::Nop);
					nopNode->m_typesResolved = true;
					node->addChild(nopNode);
				}
			}
			// native function call
			else if (resolvedIdent.m_nativeFunction)
			{
				node->m_extraData.m_nativeFunctionRef = resolvedIdent.m_nativeFunction;
			}

			return true;
		}

		bool CodeNode::ResolveTypes_AccessArray(SHADER_RESOLVE_FUNC)
		{
			// structured parameters are implicit arrays
			auto sourceType = node->m_children[0]->dataType();
			if (sourceType.isResource())
				return HandleResourceArrayAccess(lib, node, err);

			// we can only access elements of arrays :)
			if (!sourceType.isArray())
			{
				err.reportError(node->location(), base::TempString("Type '{}' has no operator[] defined", sourceType));
				return false;
			}

			// if we are already an integer number than we don't have to convert it (saves us a cast)
			const auto currentIndexType = node->m_children[1]->dataType();
			if (!currentIndexType.isArrayIndex())
			{
				// the array index should be a single int
				//auto arrayIndexType = DataType::UnsignedScalarType();
				auto arrayIndexType = DataType::IntScalarType();
				if (!MatchNodeType(lib, (CodeNode*&)node->m_children[1], arrayIndexType, true, err))
					return false;
			}

			// return type is the array inner type
			node->m_dataType = GetArrayInnerType(sourceType).applyFlags(sourceType.flags());
			return true;
		}

		bool CodeNode::ResolveTypes_AccessMember(SHADER_RESOLVE_FUNC)
		{
			// we can only access member of structures or types that can be swizzled
			auto sourceType = node->m_children[0]->dataType();
			auto memberName = node->m_extraData.m_name;
			if (sourceType.isComposite())
			{
				if (sourceType.composite().hint() == CompositeTypeHint::VectorType)
					return ResolveTypes_AccessMember_Vector(sourceType, memberName, lib, program, func, node, err);

				else if (sourceType.composite().hint() == CompositeTypeHint::MatrixType)
					return ResolveTypes_AccessMember_Matrix(sourceType, memberName, lib, program, func, node, err);

				else
					return ResolveTypes_AccessMember_Struct(sourceType, memberName, lib, program, func, node, err);

			}
			else if (sourceType.isProgram())
				return ResolveTypes_AccessMember_Program(sourceType, memberName, lib, program, func, node, err);

			else if (sourceType.isNumericalScalar())
				return ResolveTypes_AccessMember_Scalar(sourceType, memberName, lib, program, func, node, err);

			// well, no member
			err.reportError(node->location(), base::TempString("Type '{}' has no member '{}'", sourceType, memberName));
			return false;
		}

		bool CodeNode::ResolveTypes_AccessMember_Vector(const DataType& sourceType, base::StringID memberName, SHADER_RESOLVE_FUNC)
		{
			// use the swizzle if possible
			ComponentMask mask;
			if (!ComponentMask::Parse(memberName.c_str(), mask))
			{
				err.reportError(node->location(), base::TempString("Unable to parse swizzle from '{}'", memberName));
				return false;
			}

			// validate the swizzle
			if (!CanUseComponentMask(sourceType, mask))
			{
				err.reportError(node->location(), base::TempString("Swizzle '{}' is not compatible with type '{}'", memberName, sourceType));
				return false;
			}

			// make sure the type is dereferenced before swizzling
			node->m_children[0] = Dereferece(lib, (CodeNode*)node->m_children[0], err);

			// change the node type
			node->m_op = OpCode::ReadSwizzle;
			node->m_extraData.m_mask = mask;

			// contract the type to the number of components extracted 
			node->m_dataType = GetContractedType(lib.typeLibrary(), sourceType, mask.numberOfComponentsProduced());

			// in general the member is not assignable
			// TODO: maybe it should be? it would allow for passing components (or valid write masks) to function as out/inout parameters
			node->m_dataType = node->m_dataType.unmakePointer();

			return true;
		}

		bool CodeNode::ResolveTypes_AccessMember_Matrix(const DataType& sourceType, base::StringID memberName, SHADER_RESOLVE_FUNC)
		{
			err.reportError(node->location(), base::TempString("Matrices have no direct members, use the array operator to access COLUMNS"));
			return false;
		}

		bool CodeNode::ResolveTypes_AccessMember_Struct(const DataType& sourceType, base::StringID memberName, SHADER_RESOLVE_FUNC)
		{
			// find member in the composite type
			auto memberIndex = sourceType.composite().memberIndex(memberName);
			if (memberIndex == -1)
			{
				err.reportError(node->location(), base::TempString("Structure '{}' has no member '{}'", sourceType, memberName));
				return false;
			}

			// use the member type, preserve parent reference and const flag
			node->m_dataType = sourceType.composite().memberType(memberIndex).applyFlags(sourceType.flags());
			return true;
		}

		bool CodeNode::ResolveTypes_AccessMember_Program(const DataType& sourceType, base::StringID memberName, SHADER_RESOLVE_FUNC)
		{
			// try as variable first
			auto targetProgram = sourceType.program();
			if (auto programParam = targetProgram->findParameter(memberName))
			{
				node->m_dataType = programParam->dataType; // reference is preserved
				return true;
			}

			// try now as a function
			else if (auto programFunc = targetProgram->findFunction(memberName))
			{
				node->m_op = OpCode::FuncRef;
				node->m_dataType = DataType(programFunc);
				return true;
			}

			// unknown
			err.reportError(node->location(), base::TempString("Member '{}' not found in program '{}'", memberName, targetProgram->name()));
			return false;
		}

		bool CodeNode::ResolveTypes_AccessMember_Scalar(const DataType& sourceType, base::StringID memberName, SHADER_RESOLVE_FUNC)
		{
			// for scalar types we allow the .x .xx .xxx etc stuff
			// use the swizzle if possible
			ComponentMask mask;
			if (!ComponentMask::Parse(memberName.c_str(), mask))
			{
				err.reportError(node->location(), base::TempString("Unable to parse swizzle from '{}'", memberName));
				return false;
			}

			// validate the swizzle
			if (!CanUseComponentMask(sourceType, mask))
			{
				err.reportError(node->location(), base::TempString("Swizzle '{}' is not compatible with type '{}'", memberName, sourceType));
				return false;
			}

			// make sure the type is dereferenced before swizzling
			node->m_children[0] = Dereferece(lib, (CodeNode*)node->m_children[0], err);

			// change the node type
			node->m_op = OpCode::ReadSwizzle;
			node->m_extraData.m_mask = mask;

			// expand the scalar to the correct number of components
			node->m_dataType = GetContractedType(lib.typeLibrary(), sourceType, mask.numberOfComponentsProduced());

			// scalar in general is not assignable after expansion
			node->m_dataType = node->m_dataType.unmakePointer();

			return true;
		}

		bool CodeNode::ResolveTypes_Cast(SHADER_RESOLVE_FUNC)
		{
			// do explicit cast
			if (!MatchNodeType(lib, (CodeNode*&)node->m_children[0], node->m_extraData.m_castType, true, err))
				return false;

			// use the casted type
			node->m_dataType = node->m_extraData.m_castType.unmakePointer();
			node->m_typesResolved = true;
			return true;
		}

		bool CodeNode::ResolveTypes_Call(SHADER_RESOLVE_FUNC)
		{
			base::Array<DataType> argumentTypes;
			DataType returnType;

			// the first child must be of a "function ref" type
			const char* functionName = "Unknown";
			auto callContextNode = const_cast<CodeNode*>(node->m_children[0]);
			if (callContextNode->m_op == OpCode::Ident)
			{
				auto nativeFunction = callContextNode->extraData().m_nativeFunctionRef;
				auto funcName = nativeFunction->cls()->findMetadataRef<FunctionNameMetadata>().name();

				// collect the types of the children (arguments)
				for (uint32_t i = 1; i < node->m_children.size(); ++i)
				{
					auto child = node->m_children[i];
					ASSERT(child != nullptr);
					argumentTypes.pushBack(child->dataType());
				}

				// mutate function
				// this is used mostly for changing the "flavor" of multiplication
				callContextNode->extraData().m_nativeFunctionRef = callContextNode->extraData().m_nativeFunctionRef->mutateFunction(lib.typeLibrary(), argumentTypes.size(), argumentTypes.typedData(), node->location(), err);
				if (!callContextNode->extraData().m_nativeFunctionRef)
				{
					err.reportError(node->location(), base::TempString("Failed to mutate function '{}'", funcName));
					return false;
				}

				// ask the function to determine the return type
				// NOTE: the "invalid" return type means something failed
				functionName = funcName;
				returnType = callContextNode->extraData().m_nativeFunctionRef->determineReturnType(lib.typeLibrary(), argumentTypes.size(), argumentTypes.typedData(), node->location(), err);
			}
			else if (callContextNode->dataType().isFunction())
			{
				// no function to call ?
				auto callFunction = callContextNode->dataType().function();
				ASSERT(callFunction != nullptr);

				// use the return type of the function it's defined
				returnType = callFunction->returnType();
				functionName = callFunction->name().c_str();

				// use the types for arguments as specified in the function
				for (auto arg : callFunction->inputParameters())
					argumentTypes.pushBack(arg->dataType);
			}
			else
			{
				err.reportError(node->location(), base::TempString("Context of call ({}) is not a function", callContextNode->dataType()));
				return false;
			}

			// return type of the function is not valid
			if (!returnType.valid())
				return false;

			// apply conversion if needed
			// NOTE: child may be substituted by conversion
			bool typesMatched = true;
			for (uint32_t i = 0; i < argumentTypes.size(); ++i)
			{	
				auto childIndex = 1 + i;
				if (childIndex >= node->m_children.size())
				{
					err.reportError(node->location(), base::TempString("Missing argument {} needed by function call to {}", childIndex, functionName));
					return false;
				}

				if (!argumentTypes[i].valid())
				{
					err.reportError(node->location(), base::TempString("Argument {} of function call to {} takes undefined type", childIndex, functionName));
					return false;
				}

				if (!node->m_children[childIndex])
					continue;

				if (!MatchNodeType(lib, const_cast<CodeNode*&>(node->m_children[1 + i]), argumentTypes[i], false, err))
					typesMatched = false;
			}

			// validate that the types match
			if (!typesMatched)
				return false;

			// mutate the node
			if (callContextNode->m_op == OpCode::Ident)
			{
				// mutate the node
				node->m_op = OpCode::NativeCall;
				node->m_extraData.m_nativeFunctionRef = callContextNode->m_extraData.m_nativeFunctionRef;
				node->m_children.erase(0);
				ASSERT(node->m_extraData.m_nativeFunctionRef != nullptr);
			}

			// we have a valid function call, set the return type
			node->m_dataType = returnType.unmakePointer();
			return true;
		}

		bool CodeNode::ResolveTypes_IfElse(SHADER_RESOLVE_FUNC)
		{
			// every condition should evaluate to bool
			for (uint32_t i = 0; (i + 1) < node->m_children.size(); i += 2)
			{
				if (!MatchNodeType(lib, const_cast<CodeNode*&>(node->m_children[i]), DataType::BoolScalarType(), true, err))
					return false;
			}

			return true;
		}

		bool CodeNode::ResolveTypes_Loop(SHADER_RESOLVE_FUNC)
		{
			// look condition must be resolvable to boolean
			auto*& loopCondition = const_cast<CodeNode*&>(node->m_children[1]);
			if (!MatchNodeType(lib, loopCondition, DataType::BoolScalarType(), true, err))
				return false;

			return true;
		}

		bool CodeNode::ResolveTypes_ProgramInstance(SHADER_RESOLVE_FUNC)
		{
			// get the program
			ASSERT(node->m_dataType.baseType() == BaseType::Program);
			auto targetProgram = node->m_dataType.program();
			ASSERT(targetProgram != nullptr);

			// check types of the parameters
			TRACE_DEEP("{}: instancing for '{}'", node->location(), targetProgram->name());
			for (uint32_t i = 0; i < node->m_children.size(); ++i)
			{
				auto programParamNode = node->m_children[i];
				ASSERT(programParamNode->opCode() == OpCode::ProgramInstanceParam);

				// make sure target program defines given parameter
				auto programParamName = programParamNode->extraData().m_name;
				auto programParam = targetProgram->findParameter(programParamName, true);
				if (!programParam)
				{
					err.reportError(node->location(), base::TempString("Parameter '{}' is not declared in program '{}' or any parent programs", programParamName, targetProgram->name()));
					return false;
				}

				// make sure it's a constexpr param
				if (programParam->scope != DataParameterScope::GlobalConst)
				{
					err.reportError(node->location(), base::TempString("Parameter '{}' from program '{}' was not declared as constexpr", programParamName, targetProgram->name()));
					return false;
				}

				// match type
				auto requestedDataType = programParam->dataType;
				if (!MatchNodeType(lib, const_cast<CodeNode*&>(programParamNode->m_children[0]), requestedDataType, false, err))
					return false;

				TRACE_DEEP("{}: resolved type for '{}', '{}'", programParamNode->location(), programParamName, requestedDataType);
			}

			return true;
		}

		bool CodeNode::ResolveTypes_Return(SHADER_RESOLVE_FUNC)
		{
			// we cannot return stuff if function does not define a return type ;)
			if (func->returnType().baseType() == BaseType::Void)
			{
				if (!node->children().empty())
				{
					err.reportError(node->location(), base::TempString("Function '{}' does not return anything", func->name()));
					return false;
				}
			}
			else
			{
				if (node->children().empty())
				{
					err.reportError(node->location(), "Expected value to return");
					return false;
				}

				// match the source node to the target type
				if (!MatchNodeType(lib, const_cast<CodeNode*&>(node->m_children[0]), func->returnType(), false, err))
					return false;
			}

			return true;
		}

		bool CodeNode::ResolveTypes_CreateVector(SHADER_RESOLVE_FUNC)
		{
			// get the base type
			const auto& dataType = node->m_extraData.m_castType;
			auto baseType = ExtractBaseType(dataType);
			if (!AllowedVectorConstructionType(baseType))
			{
				err.reportError(node->location(), "Vector constructor requires a valid numerical type as a base");
				return false;
			}

			// get the number of components the vector should produce
			const auto expectedComponentCount = ExtractComponentCount(dataType);
			if (expectedComponentCount < 2 || expectedComponentCount > 4)
			{
				err.reportError(node->location(), "Unexpected number of components in vector type");
				return false;
			}

			// we need at least one argument
			const auto numArgs = node->m_children.size();
			if (numArgs == 0)
			{
				err.reportError(node->location(), "Vector constructor requires at least one argument, pass zero if not sure :)");
				return false;
			}

			// make sure all arguments are castable
			uint32_t numComponentsCollected = 0;
			for (uint32_t i = 0; i < numArgs; ++i)
			{
				auto*& argNode = node->m_children[i];
				const auto& argType = argNode->dataType();

				// we must be a conforming vector :)
				if (!argType.isNumericalVectorLikeOperand(true))
				{
					err.reportError(node->location(), base::TempString("Vector constructor cannot accept '{}' as input type", argType));
					return false;
				}

				// get the number of components in the argument
				auto numComponents = ExtractComponentCount(argType);
				if (!numComponents)
				{
					err.reportError(node->location(), base::TempString("Vector constructor cannot accept type '{}' as argument because the component count cannot be determined", argType));
					return false;
				}

				// make sure we get casted to the base type (for conversion), ie ivec2 -> vec2, int -> float
				const auto targetType = GetCastedType(lib.typeLibrary(), argType, baseType);
				if (!MatchNodeType(lib, const_cast<CodeNode*&>(argNode), targetType, false, err))
					return false;

				// remember how many components we have so far
				numComponentsCollected += numComponents;
			}

			// if we have to many components it's not good unless there were all provided in one parameter (vec4 -> vec2, ie. vec2(pos))
			if (numComponentsCollected > expectedComponentCount && numArgs != -1)
			{
				err.reportError(node->location(), base::TempString("Too much data in type constructor, expected {} components, {} provided", expectedComponentCount, numComponentsCollected));
				return false;
			}

			// not enough component collected - not good unless we just have a single scalar to promote (float -> vec3)
			if (numComponentsCollected < expectedComponentCount && numComponentsCollected != 1)
			{
				err.reportError(node->location(), base::TempString("Vector constructor expects {} components, {} provided", expectedComponentCount, numComponentsCollected));
				return false;
			}

			// create the output type
			node->m_dataType = node->m_extraData.m_castType;
			return true;
		}

		bool CodeNode::ResolveTypes_CreateMatrix(SHADER_RESOLVE_FUNC)
		{
			// get the base type
			const auto& dataType = node->m_extraData.m_castType;
			auto baseType = ExtractBaseType(dataType);
			if (!AllowedMatrixConstructionType(baseType))
			{
				err.reportError(node->location(), "Matrix constructor requires a valid numerical type as a base");
				return false;
			}

			// get the number of components the vector should produce
			const auto expectedRowCount = ExtractComponentCount(dataType);
			const auto expectedColumnCount = ExtractRowCount(dataType);
			const auto maxComponents = expectedRowCount * expectedColumnCount;
			if (expectedRowCount < 2 || expectedRowCount > 4 || expectedColumnCount < 2 || expectedColumnCount > 4)
			{
				err.reportError(node->location(), "Unexpected number of components in matrix type");
				return false;
			}

			// we need at least one argument
			const auto numArgs = node->m_children.size();
			if (numArgs == 0)
			{
				err.reportError(node->location(), "Matrix constructor requires at least one argument, pass zero if not sure :)");
				return false;
			}

			// make sure all arguments are castable
			uint32_t numComponentsCollected = 0;
			base::InplaceArray<uint32_t, 16> argumentsComponentCounts;
			for (uint32_t i = 0; i < numArgs; ++i)
			{
				auto*& argNode = node->m_children[i];
				const auto& argType = argNode->dataType();

				// we must be a conforming vector :)
				if (!argType.isNumericalVectorLikeOperand(false) && !argType.isNumericalMatrixLikeOperand())
				{
					err.reportError(node->location(), base::TempString("Matrix constructor cannot accept '{}' as input type", argType));
					return false;
				}

				// get the number of components in the argument
				auto numComponents = ExtractComponentCount(argType) * ExtractRowCount(argType);
				if (!numComponents)
				{
					err.reportError(node->location(), base::TempString("Matrix constructor cannot accept type '{}' as argument because the component count cannot be determined", argType));
					return false;
				}

				// make sure we get casted to the base type (for conversion), ie ivec2 -> vec2, int -> float
				const auto targetType = GetCastedType(lib.typeLibrary(), argType, baseType);
				if (!MatchNodeType(lib, const_cast<CodeNode*&>(argNode), targetType, false, err))
					return false;

				// remember how many components we have so far
				argumentsComponentCounts.pushBack(numComponents);
				numComponentsCollected += numComponents;
			}

			// if we don't have ONE argument that they all must be either scalars or column vectors
			if (numArgs != 1 && numArgs != maxComponents)
			{
				if (numArgs != expectedColumnCount)
				{
					err.reportError(node->location(), base::TempString("Matrix constructor requires {} colum vectors", expectedColumnCount));
					return false;
				}

				for (uint32_t i = 0; i < numArgs; ++i)
				{
					if (argumentsComponentCounts[i] != expectedRowCount)
					{
						err.reportError(node->location(), base::TempString("Matrix constructor requires {} colum vectors of {} components each", expectedColumnCount, expectedRowCount));
						return false;
					}
				}
			}

			// we support only 1-4 components
			// TODO: we can make other counts work as well
			if (numComponentsCollected > maxComponents)
			{
				if (numArgs != 1)
				{
					err.reportError(node->location(), base::TempString("Matrix constructor of this type can accept up to {} components, {} provided", maxComponents, numComponentsCollected));
					return false;
				}
				else
				{
					auto sourceCols = ExtractComponentCount(node->m_children[0]->m_dataType);
					auto sourceRows = ExtractRowCount(node->m_children[0]->m_dataType);

					if (expectedRowCount > sourceRows || sourceCols > sourceCols)
					{
						err.reportError(node->location(), base::TempString("Source matrix does not have enough row/columns for this construction"));
						return false;
					}
					else
					{
						// matrix truncation, ie mat4x3 -> mat3x2
					}
				}
			}

			// not enough components
			else if (numComponentsCollected < maxComponents)
			{
				if (numArgs == 1)
				{
					if (numComponentsCollected != 1)
					{
						auto targetSquare = (expectedRowCount == expectedColumnCount);
						auto sourceSquare = ExtractComponentCount(node->m_children[0]->m_dataType) == ExtractRowCount(node->m_children[0]->m_dataType);
						if (!targetSquare || !sourceSquare)
						{
							err.reportError(node->location(), base::TempString("Matrix constructor of this type works only with squre matrices"));
							return false;
						}
					}
					else
					{
						// trace matrix case
						// mat3(1.0) -> mat3(1,0,0, 0,1,0, 0,0,1);
					}
				}
				else
				{
					// matrix automatic promotion, ie. mat2 -> mat3
					/*
						mat2 m2x2 = mat2(
						   1.1, 2.1,
						   1.2, 2.2
						);
						mat3 m3x3 = mat3(m2x2); // = mat3(
						   // 1.1, 2.1, 0.0,
						   // 1.2, 2.2, 0.0,
						   // 0.0, 0.0, 1.0)
					*/
				}
			}

			// create the output type
			node->m_dataType = node->m_extraData.m_castType;
			return true;
		}

		bool CodeNode::ResolveTypes_CreateArray(SHADER_RESOLVE_FUNC)
		{
			// we need at least one argument
			const auto numArgs = node->m_children.size();
			if (numArgs == 0)
			{
				err.reportError(node->location(), "Array constructor requires at least one argument");
				return false;
			}

			// get the type of element in the array
			const auto arrayType = node->m_extraData.m_castType;
			ASSERT(arrayType.isArray());

			const auto arrayElementType = arrayType.applyArrayCounts(arrayType.arrayCounts().innerCounts());
			const auto expectedElementCount = arrayType.arrayCounts().outermostCount();
			if (expectedElementCount != numArgs && arrayType.arrayCounts().isSizeDefined())
			{
				err.reportError(node->location(), base::TempString("Array constructor requires {} arguments but {} were provided", expectedElementCount, numArgs));
				return false;
			}

			// modify array counts to actually reflect the number of elements
			auto arrayCounts = arrayType.arrayCounts();
			arrayCounts = arrayCounts.innerCounts().appendArray(numArgs);

			// make sure each argument matches, there's some small wiggle room for conversion but not much
			for (uint32_t i = 0; i < numArgs; ++i)
			{
				auto*& argNode = node->m_children[i];
				const auto& argType = argNode->dataType();

				// make sure we get casted to the base type (for conversion), ie ivec2 -> vec2, int -> float
				if (!MatchNodeType(lib, const_cast<CodeNode*&>(argNode), arrayElementType, false, err))
					return false;
			}

			// looks ok
			node->m_dataType = node->m_extraData.m_castType.applyArrayCounts(arrayCounts);
			return true;
		}

		//--        

    } // shader
} // rendering
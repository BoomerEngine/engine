/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\nodes #]
*/

#include "build.h"

#include "renderingShaderProgram.h"
#include "renderingShaderFunction.h"
#include "renderingShaderStaticExecution.h"
#include "renderingShaderProgramInstance.h"
#include "renderingShaderProgram.h"
#include "renderingShaderFileParserHelper.h"
#include "renderingShaderCodeLibrary.h"

#include "renderingShaderFileParserHelper.h"
#include "renderingShaderFileParser.h"
#include "renderingShaderDataType.h"

BEGIN_BOOMER_NAMESPACE(rendering::shadercompiler)

//---

base::ConfigProperty<bool> cvDumpShaderAST("Rendering.Compiler", "DumpShaderAST", false);

//---

static bool ExtractVectorTypeSizes(base::StringView view, uint32_t& numCols, uint32_t& numRows)
{
    base::StringParser parser(view);

    if (!parser.parseUint32(numCols))
        return false;
    if (numCols < 1 || numCols > 4)
        return false;

    if (parser.currentView().empty())
        return true;

    if (!parser.parseKeyword("x"))
        return false;

    if (!parser.parseUint32(numRows))
        return false;
    if (numCols < 1 || numCols > 4)
        return false;

    return true;
}

bool CodeLibrary::resolveType(const parser::TypeReference* typeRef, DataType& outType, base::parser::IErrorReporter& err, bool reportErrors) const
{
    if (!resolveTypeInner(typeRef, outType, err, reportErrors))
        return false;

    if (!typeRef->arraySizes.empty())
    {
        ArrayCounts arrayCounts;
        for (uint32_t i = 0; i < typeRef->arraySizes.size(); ++i)
        {
            auto size = typeRef->arraySizes[i];
            if (size > 0)
            {
                arrayCounts = arrayCounts.appendArray(size);
            }
            else
            {
                if (i != 0)
                {
                    if (reportErrors)
                        err.reportError(typeRef->location, "Only the outermost array dimension can be unbounded");
                    return false;
                }

                arrayCounts = arrayCounts.appendUndefinedArray();
            }
        }

        outType = outType.applyArrayCounts(arrayCounts);
    }

    return true;
}

void CodeLibrary::findDescriptorEntry(const ResourceTable* table, base::StringID name, base::Array<ResolvedDescriptorEntry>& outEntries) const
{
    for (const auto& entry : table->members())
    {
        if (entry.m_type.resource().type == DeviceObjectViewType::ConstantBuffer)
        {
            DEBUG_CHECK_EX(entry.m_type.resource().resolvedLayout != nullptr, "Constant buffer with no layout");

            if (nullptr != entry.m_type.resource().resolvedLayout)
            {
                for (const auto& member : entry.m_type.resource().resolvedLayout->members())
                {
                    if (member.name == name)
                    {
                        auto& info = outEntries.emplaceBack();
                        info.entry = &entry;
                        info.table = table;
                        info.member = &member;
                    }
                }
            }
        }
    }
}

void CodeLibrary::findUniformEntry(const ResourceTable* table, base::StringID name, base::Array<ResolvedDescriptorEntry>& outEntries) const
{
    for (const auto& entry : table->members())
    {
        if (entry.m_name == name)
        {
            auto& info = outEntries.emplaceBack();
            info.entry = &entry;
            info.table = table;
        }
    }
}

void CodeLibrary::findGlobalUniformEntry(base::StringID name, base::Array<ResolvedDescriptorEntry>& outEntries) const
{
    // TODO: cache
    for (const auto* table : m_typeLibrary->allResourceTables())
        findDescriptorEntry(table, name, outEntries);
}

void CodeLibrary::findGlobalDescriptorEntry(base::StringID name, base::Array<ResolvedDescriptorEntry>& outEntries) const
{
    // TODO: cache
    for (const auto* table : m_typeLibrary->allResourceTables())
        findUniformEntry(table, name, outEntries);
}

bool CodeLibrary::resolveTypeInner(const parser::TypeReference* typeRef, DataType& outType, base::parser::IErrorReporter& err, bool reportErrors) const
{
    if (typeRef == nullptr)
    {
        if (reportErrors)
            err.reportError(typeRef->location, base::TempString("Undefined type used"));
        return false;
    }

    if (typeRef->name == "program" || typeRef->name == "shader")
    {
        // find the program
        auto programType  = findProgram(typeRef->innerType);
        if (!programType)
        {
            if (reportErrors)
                err.reportError(typeRef->location, base::TempString("Unrecognized {} {}", typeRef->name, typeRef->innerType));
            return false;
        }

        outType = DataType(programType);
        return true;
    }
    else if (typeRef->name == "bool")
    {
        outType = DataType::BoolScalarType();
        return true;
    }
    else if (typeRef->name == "int")
    {
        outType = DataType::IntScalarType();
        return true;
    }
    else if (typeRef->name == "uint")
    {
        outType = DataType::UnsignedScalarType();
        return true;
    }
    else if (typeRef->name == "float")
    {
        outType = DataType::FloatScalarType();
        return true;
    }
    else if (typeRef->name == "name")
    {
        outType = DataType::NameScalarType();
        return true;
    }
    else if (typeRef->name == "void")
    {
        outType = DataType(BaseType::Void);
        return true;
    }
    else if (typeRef->name.beginsWith("int"))
    {
        uint32_t numRows=1, numCols=1;
        if (!ExtractVectorTypeSizes(typeRef->name.subString(3), numCols, numRows))
        {
            if (reportErrors)
                err.reportError(typeRef->location, base::TempString("Invalid vector/matrix type '{}'", typeRef->name));
            return false;
        }

        outType = m_typeLibrary->simpleCompositeType(BaseType::Int, numCols, numRows);
        return true;
    }
    else if (typeRef->name.beginsWith("ivec"))
    {
        uint32_t numRows = 1, numCols = 1;
        if (!ExtractVectorTypeSizes(typeRef->name.subString(4), numCols, numRows))
        {
            if (reportErrors)
                err.reportError(typeRef->location, base::TempString("Invalid vector/matrix type '{}'", typeRef->name));
            return false;
        }

        outType = m_typeLibrary->simpleCompositeType(BaseType::Int, numCols, numRows);
        return true;
    }
    else if (typeRef->name.beginsWith("float"))
    {
        uint32_t numRows=1, numCols=1;
        if (!ExtractVectorTypeSizes(typeRef->name.subString(5), numCols, numRows))
        {
            if (reportErrors)
                err.reportError(typeRef->location, base::TempString("Invalid vector/matrix type '{}'", typeRef->name));
            return false;
        }

        outType = m_typeLibrary->simpleCompositeType(BaseType::Float, numCols, numRows);
        return true;
    }
    else if (typeRef->name.beginsWith("vec"))
    {
        uint32_t numRows = 1, numCols = 1;
        if (!ExtractVectorTypeSizes(typeRef->name.subString(3), numCols, numRows))
        {
            if (reportErrors)
                err.reportError(typeRef->location, base::TempString("Invalid vector/matrix type '{}'", typeRef->name));
            return false;
        }

        outType = m_typeLibrary->simpleCompositeType(BaseType::Float, numCols, numRows);
        return true;
    }
    else if (typeRef->name.beginsWith("uint") || typeRef->name.beginsWith("uvec"))
    {
        uint32_t numRows=1, numCols=1;
        if (!ExtractVectorTypeSizes(typeRef->name.subString(4), numCols, numRows))
        {
            if (reportErrors)
                err.reportError(typeRef->location, base::TempString("Invalid vector/matrix type '{}'", typeRef->name));
            return false;
        }

        outType = m_typeLibrary->simpleCompositeType(BaseType::Uint, numCols, numRows);
        return true;
    }
    else if (typeRef->name.beginsWith("bool"))
    {
        uint32_t numRows=1, numCols=1;
        if (!ExtractVectorTypeSizes(typeRef->name.subString(4), numCols, numRows))
        {
            if (reportErrors)
                err.reportError(typeRef->location, base::TempString("Invalid vector/matrix type '{}'", typeRef->name));
            return false;
        }

        outType = m_typeLibrary->simpleCompositeType(BaseType::Boolean, numCols, numRows);
        return true;
    }
    else
    {
        auto compositeType  = m_typeLibrary->findCompositeType(typeRef->name);
        if (compositeType)
        {
            outType = DataType(compositeType);
            return true;
        }
    }

    // unresolved type
    if (reportErrors)
        err.reportError(typeRef->location, base::TempString("Unable to resolve type '{}'", typeRef->name));
    return false;
}

static bool ValidateFlags(const parser::Element* elem, const parser::ElementFlags flags, base::parser::IErrorReporter& err)
{
    bool valid = true;

    for (uint32_t i=0; i<(uint32_t)parser::ElementFlag::MAX; ++i)
    {
        auto flag = (parser::ElementFlag)i;
        if (elem->flags.test(flag) && !flags.test(flag))
        {
            err.reportError(elem->location, base::TempString("Flag {} is not allowed on {}", flag, elem->name));
            valid = false;
        }
    }

    return valid;
}

bool CodeLibrary::createStructureElements(const parser::Element* parent, CompositeType* structType, base::parser::IErrorReporter& err)
{
    bool valid = true;

    for (parser::Element* elem : parent->children)
    {
        if (elem->type == parser::ElementType::StructElement || elem->type == parser::ElementType::DescriptorConstantTableElement)
        {
            // member must be unqiue
            auto memberName = base::StringID(elem->name);
            if (INDEX_NONE != structType->memberIndex(memberName))
            {
                err.reportError(elem->location, base::TempString("Structure '{}' already contains member '{}'", structType->name(), memberName));
                valid = false;
                continue;
            }

            // resolve type
            DataType resolvedType;
            if (!resolveType(elem->typeRef, resolvedType, err))
            {
                err.reportError(elem->location, base::TempString("Member '{}' does not have a valid type", memberName));
                valid = false;
                continue;
            }

            // only numerical types are allowed in structures
            if (!resolvedType.isNumericalScalar() && !resolvedType.isComposite() && !resolvedType.isName())
            {
                err.reportError(elem->location, base::TempString("Member '{}' uses unsupported type (structures can only hold numerical data)", memberName));
                valid = false;
                continue;
            }

            // get decorations
            parser::ElementFlags allowedFlags;
            if (!ValidateFlags(elem, allowedFlags, err))
            {
                valid = false;
                continue;
            }

            // get the custom format definition
            auto attributes = elem->gatherAttributes();
            /*base::StringID customFormat;
            int customOffset = -1;
            if (auto formatInfo  = elem->findAttribute("format"))
                customFormat = formatInfo->stringData;
			if (auto offsetInfo  = elem->findAttribute("offset"))
				customOffset = range_cast<int>(offsetInfo->intData);*/

            // add member
            structType->addMember(elem->location, elem->name, resolvedType, std::move(attributes), elem->tokens);
        }
    }

    return valid;
}
        
bool CodeLibrary::createDescriptorElements(const parser::Element* parent, ResourceTable* table, base::parser::IErrorReporter& err)
{
    bool valid = true;

    uint32_t constantBufferIndex = 0;
    base::HashMap<base::StringID, const parser::Element*> alreadyDeclaredParameters;
    for (parser::Element* elem : parent->children)
    {
        if (elem->type == parser::ElementType::DescriptorResourceElement)
        {
            if (INDEX_NONE != table->memberIndex(elem->name))
            {
                err.reportError(elem->location, base::TempString("Member '{}' of descriptor '{}' was already defined", elem->name, table->name()));
                valid = false;
                continue;
            }

            auto attributes = elem->gatherAttributes();

			int localSampler = -1;
			const StaticSampler* staticSampler = nullptr;

			base::StringBuf errorStr;
            auto type = m_typeLibrary->resourceType(elem->stringData, attributes, errorStr);
            if (!type.valid())
            {
                err.reportError(elem->location, base::TempString("Resource type for '{}' in descriptor '{}' is invalid: {}", elem->name, table->name(), errorStr));
                valid = false;
                continue;
            }

			// additional sampler
			if (type.resource().type == DeviceObjectViewType::SampledImage)
			{
				const auto samplerName = attributes.value("sampler"_id);
				if (samplerName.empty())
				{
					err.reportError(elem->location, base::TempString("Sampled image '{}' in descriptor '{}' should have sampler specified via attribute(sampler=xxx)", elem->name, table->name()));
					valid = false;
					continue;
				}

				// use local sampler first
				for (auto i : table->members().indexRange())
				{
					const auto& member = table->members()[i];
					if (member.m_type.resource().type == DeviceObjectViewType::Sampler)
					{
						if (member.m_name == samplerName)
						{
							localSampler = i;
							break;
						}
					}
				}

				// if local sampler was not found fall back to static sampler
				if (localSampler == -1)
				{
					staticSampler = typeLibrary().findStaticSampler(base::StringID::Find(samplerName));
					if (!staticSampler)
					{
						err.reportError(elem->location, base::TempString("Sampler '{}' used in entry '{}' in descriptor '{}' was not found", samplerName, elem->name, table->name()));
						valid = false;
						continue;
					}
				}
			}

            table->addMember(elem->location, elem->name, type, attributes, localSampler, staticSampler);
            alreadyDeclaredParameters[elem->name] = elem;
        }
        else if (elem->type == parser::ElementType::DescriptorConstantTable)
        {
            auto attributes = elem->gatherAttributes();

            auto packingType = CompositePackingRules::Std430;
            if (auto packingValue = attributes.value("packing"_id))
            {
                if (packingValue == "std430")
                    packingType = CompositePackingRules::Std430;
                else if (packingValue == "std140")
                    packingType = CompositePackingRules::Std140;
                else if (packingValue == "std140")
                {
                    err.reportError(elem->location, base::TempString("Unknown packing type '{}' for member '{}' of descriptor '{}'", packingValue, elem->name, table->name()));
                    valid = false;
                    continue;
                }
            }

            auto compositeName = base::StringID(base::TempString("_{}_Consts{}", table->name(), constantBufferIndex++));
            if (INDEX_NONE != table->memberIndex(compositeName))
            {
                err.reportError(elem->location, base::TempString("Member '{}' of descriptor '{}' already defined", compositeName, table->name()));
                valid = false;
                continue;
            }

            auto compositeTable = m_allocator.create<CompositeType>(compositeName, packingType);
            m_typeLibrary->registerCompositeType(compositeTable);
            if (!createStructureElements(elem, compositeTable, err))
            {
                valid = false;
                continue;
            }

                    
            const auto constantBufferType = m_typeLibrary->resourceType(compositeTable, attributes);
            table->addMember(elem->location, compositeName, constantBufferType, attributes);

            for (parser::Element* memberElem : elem->children)
            {
                if (const auto* existingElem = alreadyDeclaredParameters.findSafe(memberElem->name, nullptr))
                {
                    err.reportError(memberElem->location, base::TempString("Member '{}' of constant buffer in descriptor '{}' was already defined somewhere in that descriptor", memberElem->name, table->name()));
                    valid = false;
                    continue;
                }
                else
                {
                    alreadyDeclaredParameters[memberElem->name] = memberElem;
                }
            }
        }
    }

    return valid;
}

bool CodeLibrary::createProgramElementsPass1(const parser::Element* parent, Program* program, base::parser::IErrorReporter& err)
{
    bool valid = true;

	for (const auto& attr : program->attributes().attributes.pairs())
	{
		if (attr.key == "state"_id)
		{
			const auto* rs = typeLibrary().findStaticRenderStates(base::StringID::Find(attr.value));
			if (rs)
			{
				if (program->staticRenderStates().contains(rs))
				{
					err.reportWarning(program->location(), base::TempString("Render states '{}' were already included in this program", attr.value));
				}
				else
				{
					program->addRenderStates(rs);
				}
			}
			else
			{
				err.reportError(program->location(), base::TempString("Unable to find static render states '{}'", attr.value));
				valid = false;
				continue;
			}
		}
	}

    for (parser::Element* elem : parent->children)
    {
        if (elem->type == parser::ElementType::Using)
        {
            // find the base program
            auto baseProgram  = findProgram(elem->name);
            if (!baseProgram)
            {
                err.reportError(elem->location, base::TempString("Unknown base program '{}'", elem->name));
                valid = false;
                continue;
            }

            // derives ?
            if (baseProgram->isBasedOnProgram(program))
            {
                err.reportError(elem->location, base::TempString("Recursive program dependency between '{}' and '{}'", elem->name, program->name()));
                valid = false;
                continue;
            }

            /*// must be of the same type
            if (program->programType() != baseProgram->programType())
            {
                err.reportError(elem->location, base::TempString("Base program '{}' must be the same type as '{}'", elem->name, program->name()));
                valid = false;
                continue;
            }*/

            // already added
            if (program->parentPrograms().contains(baseProgram))
            {
                err.reportWarning(elem->location, base::TempString("Base program '{}' already declared as base for '{}'", elem->name, program->name()));
                continue;
            }

            program->addParentProgram(baseProgram);
        }
    }

    return valid;
}

bool CodeLibrary::createProgramElementsPass2(const parser::Element* parent, Program* program, base::parser::IErrorReporter& err)
{
    bool valid = true;

    for (parser::Element* elem : parent->children)
    {
        if (elem->type == parser::ElementType::Variable)
        {
            // resolve the type
            DataType dataType;
            if (!resolveType(elem->typeRef, dataType, err))
            {
                err.reportError(elem->location, base::TempString("Unable to resolve type for variable '{}'", elem->name));
                valid = false;
                continue;
            }

            // variable can't start with "gl_"
            if (elem->name.beginsWith("gl_"))
            {
                err.reportError(elem->location, base::TempString("Can't start variable names with 'gl_'"));
                valid = false;
                continue;
            }

            // was the variable already defined ?
            auto existingVar  = program->findParameter(elem->name);
            if (existingVar)
            {
                err.reportError(elem->location, base::TempString("Variable '{}' is already defined at '{}'", elem->name, existingVar->loc));
                valid = false;
                continue;
            }

            // create variable
            auto param = m_allocator.create<DataParameter>();
            param->name = elem->name;
            param->loc = elem->location;
            param->dataType = dataType;
            param->attributes = elem->gatherAttributes();
            param->initalizationTokens = elem->tokens;
            program->addParameter(param);

            // scope depends on program type
            {
                // determine type
				if (elem->flags.test(parser::ElementFlag::In))
					param->scope = DataParameterScope::StageInput;
				else if (elem->flags.test(parser::ElementFlag::Out))
					param->scope = DataParameterScope::StageOutput;
                else if (elem->flags.test(parser::ElementFlag::Vertex))
                    param->scope = DataParameterScope::VertexInput;
                else if (elem->flags.test(parser::ElementFlag::Const))
                    param->scope = DataParameterScope::GlobalConst;
                else if (elem->flags.test(parser::ElementFlag::Shared))
                    param->scope = DataParameterScope::GroupShared;
                else
                {
                    err.reportError(elem->location, base::TempString("Variable '{}' has no scope defined, should be 'in', 'out', 'static' or 'uniform'", elem->name));
                    valid = false;
                    continue;
                }
            }
                    
            // resource references must be in the uniform block
            if (dataType.isResource() && param->scope != DataParameterScope::GlobalParameter)
            {
                err.reportError(elem->location, base::TempString("Resource reference '{}' can only be declared as uniform", elem->name));
                valid = false;
            }
            else if (param->scope == DataParameterScope::StageInput || param->scope == DataParameterScope::StageOutput)
            {
                // we can't use array's yet
                if (dataType.isArray())
                {
                    if (dataType.arrayCounts().dimensionCount() != 1)
                    {
                        err.reportError(elem->location, base::TempString("In/Out parameter '{}' can only be one dimensional array", elem->name));
                        valid = false;
                        continue;
                    }

                    if (dataType.arrayCounts().isSizeDefined())
                    {
                        err.reportError(elem->location, base::TempString("In/Out parameter '{}' can only have undefined size (ie. []) but {} was specified", elem->name, dataType.arrayCounts().outermostCount()));
                        valid = false;
                        continue;
                    }
                }
                // we can only have simple numerical data
                else if (!dataType.isNumericalScalar() && !dataType.isNumericalVectorLikeOperand(false))
                {
                    err.reportError(elem->location, base::TempString("In/Out parameter '{}' should be a numerical scalar or a simple vector", elem->name));
                    valid = false;
                    continue;
                }
            }
            else if (param->scope == DataParameterScope::VertexInput)
            {
                // vertex input must be a structure
                if (!dataType.isComposite())
                {
                    err.reportError(elem->location, base::TempString("Vertex input parameter '{}' must be a structure", elem->name));
                    valid = false;
                    continue;;
                }

                // data must be wrapped in proper struct
                if (dataType.composite().hint() != CompositeTypeHint::User)
                {
                    err.reportError(elem->location, base::TempString("Vertex input parameter '{}' must be a proper structure", elem->name));
                    valid = false;
                    continue;;
                }

                // we must have a "packing=vertex" attribute
                if (dataType.composite().packingRules() != CompositePackingRules::Vertex)
                {
                    err.reportError(elem->location, base::TempString("Structure '{}' used for vertex input parameter '{}' must use vertex packing mode 'attribute(packing=vertex)'", dataType.composite().name(), elem->name));
                    valid = false;
                    continue;
                }
            }
            else if (param->scope == DataParameterScope::GlobalParameter)
            {
                if (!dataType.isNumericalScalar() && !dataType.isComposite() && !dataType.isResource() && !dataType.isName())
                {
                    err.reportError(elem->location, base::TempString("Uniform parameter '{}' should be a numerical value or a resource", elem->name));
                    valid = false;
                    continue;
                }
            }

            // constants require initialization
            if (param->scope == DataParameterScope::GlobalConst && elem->tokens.empty())
            {
                err.reportError(elem->location, base::TempString("Constant '{}' should have default value specified", elem->name));
                valid = false;
            }

			// make some scopes assignable
			if (param->scope == DataParameterScope::StageOutput || param->scope == DataParameterScope::GroupShared)
				param->assignable = true;
        }
        else if (elem->type == parser::ElementType::Function)
        {
            /*// if the function already defined ?
            if (auto localExistingFunction  = program->findFunction(elem->name, false))
            {
                err.reportError(elem->location, base::TempString("Function {} was already defined at {}", elem->name, localExistingFunction->location()));
                valid = false;
                continue;
            }*/

            // resolve return type
            DataType returnType;
            if (!resolveType(elem->typeRef, returnType, err))
            {
                err.reportError(elem->location, base::TempString("Function '{}' has undefined return type", elem->name));
                valid = false;
                continue;
            }

            // resolve parameters
            base::InplaceArray<DataParameter*, 16> functionArgs;
            base::InplaceArray<base::StringID, 16> functionArgsNames;
            for (auto param  : elem->children)
            {
                auto name = base::StringID(param->name);

                // already defined ?
                if (functionArgsNames.contains(name))
                {
                    err.reportError(elem->location, base::TempString("Function '{}' argument '{}' was already declared", elem->name, name));
                    valid = false;
                    continue;
                }

                // variable can't start with "gl_"
                if (name.view().beginsWith("gl_"))
                {
                    err.reportError(elem->location, base::TempString("Can't start variable names with 'gl_'"));
                    valid = false;
                    continue;
                }

                // resolve attributes
                auto attributes = elem->gatherAttributes();

                // resolve type
                DataType paramType;
                if (!resolveType(param->typeRef, paramType, err))
                {
                    err.reportError(param->location, base::TempString("Function '{}' argument '{}' has undefined type", elem->name, name));
                    valid = false;
                    continue;
                }

                // if the types are out/inout than the value passed must be a l-value
                if (param->flags.test(parser::ElementFlag::Out))
                {
                    paramType = paramType.makePointer();
                    attributes.add("out"_id, "");
                }

                /*// if we have a parameter of type "resource" it must be a compile time
                if (paramType.isResource() && !paramType.isCompileTimeKnown())
                {
                    err.reportError(elem->location, base::TempString("Argument '{}' of type '{}' must be declared as \"const\" as it has to be known at the compile time", param->name, paramType));
                    valid = false;
                }*/

                // resolve flags
                auto argParam  = m_allocator.create<DataParameter>();
                argParam->name = param->name;
                argParam->loc = param->location;
                argParam->dataType = paramType;
                argParam->scope = DataParameterScope::FunctionInput;
				argParam->assignable = true;
                argParam->attributes = std::move(attributes);
                functionArgs.pushBack(argParam);
                functionArgsNames.pushBack(name);
            }

            // determine function flags
            /*FunctionFlags flags;
            if (elem->flags.test(parser::ElementFlag::Override))
                flags |= FunctionFlag::Override;*/

            // create the function
            auto function = m_allocator.create<Function>(*this, program, elem->location, /*flags, */elem->name, returnType, functionArgs, elem->tokens, elem->gatherAttributes());

            // find base function
            auto baseFunction  = program->findFunction(elem->name);
            if (baseFunction)
            {
                // we are not overriding
                /*if (!elem->flags.test(parser::ElementFlag::Override))
                {
                    err.reportError(elem->location, base::BaseTempString<1024>("Function '{}' was already defined at '{}' yet we are not marking it as override", elem->name, baseFunction->location()));
                    valid = false;
                    continue;
                }*/

                // rename the function that we are overiding
                if (auto baseFunctionInLocalProgram  = program->findFunction(elem->name, false))
                {
                    auto newName = base::StringID(base::TempString("{}_prev", elem->name));
                    const_cast<Function*>(baseFunctionInLocalProgram)->rename(newName);
                }

                // check the compatibility
                if (!DataType::MatchNoFlags(baseFunction->returnType(), function->returnType()))
                {
                    valid = false;
                    err.reportError(function->location(), base::BaseTempString<1024>("Function has different return type '{}' than base function from program {} ({}) defined at {}",
                            function->returnType(), baseFunction->program()->name(), baseFunction->returnType(), baseFunction->location()));
                }

                // check argument count
                if (baseFunction->inputParameters().size() != function->inputParameters().size())
                {
                    valid = false;
                    err.reportError(function->location(), base::BaseTempString<1024>("Function has different argument count ({}) than base function from program '{}' ({}) defined at {}",
                            function->inputParameters().size(), baseFunction->program()->name(), baseFunction->inputParameters().size(), baseFunction->location()));
                }
                else
                {
                    // check the argument types
                    for (uint32_t i = 0; i < baseFunction->inputParameters().size(); ++i)
                    {
                        auto &argType = function->inputParameters()[i]->dataType;
                        auto &baseArgType = baseFunction->inputParameters()[i]->dataType;

                        if (!DataType::MatchNoFlags(argType, baseArgType))
                        {
                            valid = false;
                            err.reportError(function->location(), base::BaseTempString<1024>("Function argument '{}' has different type ({}) than in base function from program '{}' ({}) defined at {}",
                                                                                                function->inputParameters()[i]->name, argType, baseFunction->program()->name(), baseArgType, baseFunction->location()));
                        }
                    }
                }
            }
            /*else
            {
                // we are not overriding
                if (elem->flags.test(parser::ElementFlag::Override))
                {
                    err.reportError(elem->location, base::BaseTempString<1024>("Function '{}' is marked as override but there's no base function to override. Did you add proper 'implements XXX' ?", elem->name));
                    valid = false;
                    continue;
                }
            }*/

            // valid function was defined, add it
            program->addFunction(function);
        }
    }

    return valid;
}

bool CodeLibrary::createElements(const parser::Element* root, base::parser::IErrorReporter& err)
{
    bool valid = true;

    // create stubs
    for (parser::Element* elem : root->children)
    {
        if (elem->type == parser::ElementType::Program)
        {
            // get the program
            auto program = findProgram(elem->name);
            if (!program)
            {
                // create if missing
                program = createProgram(elem->name, elem->location, elem->gatherAttributes());
            }
            else
            {
                // merge attributes
                const_cast<Program*>(program)->attributes().merge(elem->gatherAttributes());
            }
        }
        else if (elem->type == parser::ElementType::Descriptor)
        {
            // get the descriptor
            auto resourceTable  = m_typeLibrary->findResourceTable(elem->name);
            if (resourceTable)
            {
                err.reportError(elem->location, base::TempString("Descriptor '{}' was already defined", elem->name));
                valid = false;
            }
            else
            {
                // create descriptor
                auto table = m_allocator.create<ResourceTable>(elem->name, elem->gatherAttributes());
                m_typeLibrary->registerResourceTable(table);
            }
        }
		else if (elem->type == parser::ElementType::StaticSampler)
		{
			// get the descriptor
			auto sampler = m_typeLibrary->findStaticSampler(elem->name);
			if (sampler)
			{
				err.reportError(elem->location, base::TempString("Static sampler '{}' was already defined", elem->name));
				valid = false;
			}
			else
			{
				SamplerState setup;

				for (const auto* child : elem->children)
				{
					ASSERT(child->type == parser::ElementType::KeyValueParam);

					const auto key = base::StringID(child->name);
					base::StringBuilder errorString;
					if (!setup.configure(key, child->stringData, errorString))
					{
						err.reportError(child->location, errorString.view());
						valid = false;
					}
				}

				if (valid)
				{
					auto sampler = m_allocator.create<StaticSampler>(elem->name, setup);
					m_typeLibrary->registerStaticSampler(sampler);
				}
			}
		}
		else if (elem->type == parser::ElementType::RenderStates)
		{
			// get the descriptor
			auto sampler = m_typeLibrary->findStaticRenderStates(elem->name);
			if (sampler)
			{
				err.reportError(elem->location, base::TempString("Static render states '{}' were already defined", elem->name));
				valid = false;
			}
			else
			{
				GraphicsRenderStatesSetup setup;

				for (const auto* child : elem->children)
				{
					ASSERT(child->type == parser::ElementType::KeyValueParam);

					const auto key = base::StringID(child->name);
					base::StringBuilder errorString;
					if (!setup.configure(key, child->stringData, errorString))
					{
						err.reportError(elem->location, errorString.view());
						valid = false;
					}
				}

				if (valid)
				{
					auto states = m_allocator.create<StaticRenderStates>(elem->name, setup);
					m_typeLibrary->registerStaticRenderStates(states);
				}
			}
		}
        else if (elem->type == parser::ElementType::Struct)
        {
            // get the struct
            auto compoundType  = m_typeLibrary->findCompositeType(elem->name);
            if (compoundType)
            {
                err.reportError(elem->location, base::TempString("Structure '{}' was already defined", elem->name));
                valid = false;
            }
            else
            {
                // rules
                auto packingRules = CompositePackingRules::Std430;
                if (auto layoutAttr  = elem->findAttribute("packing"))
                {
                    if (layoutAttr->stringData == "vertex")
                        packingRules = CompositePackingRules::Vertex;
                    else if (layoutAttr->stringData == "std140")
                        packingRules = CompositePackingRules::Std140;
                    else if (layoutAttr->stringData == "std430")
                        packingRules = CompositePackingRules::Std430;
                    else
                    {
                        err.reportError(elem->location, base::TempString("Structure '{}' uses invalid packing mode '{}'", elem->name, layoutAttr->stringData));
                        valid = false;
                    }
                }

                // create struct
                auto table = m_allocator.create<CompositeType>(elem->name, packingRules);
                m_typeLibrary->registerCompositeType(table);
            }
        }
    }

    // create struct members
    for (auto elem  : root->children)
    {
        if (elem->type == parser::ElementType::Struct)
        {
            auto compositeType  = m_typeLibrary->findCompositeType(elem->name);
            if (compositeType)
                valid &= createStructureElements(elem, const_cast<CompositeType*>(compositeType), err);
        }
    }

    // create descriptor members
    for (auto elem  : root->children)
    {
        if (elem->type == parser::ElementType::Descriptor)
        {
            auto resourceTable  = m_typeLibrary->findResourceTable(elem->name);
            if (resourceTable)
                valid &= createDescriptorElements(elem, const_cast<ResourceTable*>(resourceTable), err);
        }
    }

    // create program elements
    for (auto elem  : root->children)
    {
        if (elem->type == parser::ElementType::Program)
        {
            auto program = findProgram(elem->name);
            if (program)
                valid &= createProgramElementsPass1(elem, const_cast<Program*>(program), err);
        }
    }

    // create program elements pass 2
    for (auto elem  : root->children)
    {
        if (elem->type == parser::ElementType::Program)
        {
            auto program  = findProgram(elem->name);
            if (program)
                valid &= createProgramElementsPass2(elem, const_cast<Program*>(program), err);
        }
    }

    // per-stage export

    // create the global constants
    for (auto elem  : root->children)
    {
        if (elem->type == parser::ElementType::Program)
        {
            if (elem->flags.test(parser::ElementFlag::Export))
            {
                auto type = ShaderStage::Invalid;
                if (elem->name.endsWith("PS")) type = ShaderStage::Pixel;
                else if (elem->name.endsWith("VS")) type = ShaderStage::Vertex;
                else if (elem->name.endsWith("GS")) type = ShaderStage::Geometry;
                else if (elem->name.endsWith("DS")) type = ShaderStage::Domain;
                else if (elem->name.endsWith("HS")) type = ShaderStage::Hull;
                else if (elem->name.endsWith("CS")) type = ShaderStage::Compute;
                else if (elem->name.endsWith("MS")) type = ShaderStage::Mesh;
                else if (elem->name.endsWith("TS")) type = ShaderStage::Task;

                if (type == ShaderStage::Invalid)
                {
                    err.reportError(elem->location, base::TempString("Exported shader name '{}' must end in PS,VS,GS,DS,HS or CS to specify target pipeline", elem->name));
                }
                else
                {
                    auto existingExport = m_exports.exports[(int)type];
                    if (existingExport != nullptr)
                    {
                        err.reportWarning(elem->location, base::TempString("Stage '{}' was already exported from shader '{}'", type, existingExport->name()));
                        err.reportWarning(existingExport->location(), base::TempString("See previous declaration of '{}' here", existingExport->name()));
                    }
                    else
                    {
                        if (auto program = findProgram(elem->name))
                            m_exports.exports[(int)type] = program;
                    }
                }
            }
        }
        else if (elem->type == parser::ElementType::Variable)
        {
            // variable can't start with "gl_"
            if (elem->name.beginsWith("gl_"))
            {
                err.reportError(elem->location, base::TempString("Can't start variable names with 'gl_'"));
                valid = false;
                continue;
            }

            auto existingConst  = findGlobalConstant(elem->name);
            if (existingConst)
            {
                err.reportError(elem->location, base::TempString("Global variable '{}' was already defined at {}", elem->name, existingConst->loc));
                valid = false;
                continue;
            }
            else if (auto existingFunction  = findGlobalFunction(elem->name))
            {
                err.reportError(elem->location, base::TempString("Global variable '{}' hides global function defined at {}", elem->name, existingFunction->location()));
                valid = false;
                continue;
            }

            // we should have the const flag
            if (!elem->flags.test(parser::ElementFlag::Const))
            {
                err.reportError(elem->location, base::TempString("Global variable '{}' must be declared as const", elem->name));
                valid = false;
                continue;
            }
            if (elem->flags.test(parser::ElementFlag::In) || elem->flags.test(parser::ElementFlag::Out))
            {
                err.reportError(elem->location, base::TempString("Global variable '{}' cannot be declared with any other usage", elem->name));
                valid = false;
                continue;
            }

            // resolve the type
            DataType dataType;
            if (!resolveType(elem->typeRef, dataType, err))
            {
                err.reportError(elem->location, base::TempString("Unable to resolve type for variable '{}'", elem->name));
                valid = false;
                continue;
            }

            // we can only have simple numerical data
            if (!dataType.isNumericalScalar() && !dataType.isComposite() /*&& !dataType.isProgram()*/)
            {
                err.reportError(elem->location, base::TempString("Global constant '{}' uses invalid type (only numerical data is allowed)", elem->name));
                valid = false;
                continue;
            }

            // we require initialization tokens
            if (elem->tokens.empty())
            {
                err.reportError(elem->location, base::TempString("Global constant '{}' requires initialization", elem->name));
                valid = false;
                continue;
            }

            // in case of overrides make sure the type is the same
            if (existingConst != nullptr)
            {
                if (!DataType::MatchNoFlags(dataType, existingConst->dataType))
                {
                    err.reportError(elem->location, base::TempString("Global constant override '{}' has different type '{}' than previously '{}' at {}", elem->name, dataType, existingConst->dataType, existingConst->loc));
                    valid = false;
                    continue;
                }
            }

            // create variable
            auto param  = m_allocator.create<DataParameter>();
            param->name = elem->name;
            param->loc = elem->location;
            param->dataType = dataType;
            param->scope = DataParameterScope::StaticConstant;
            param->initalizationTokens = elem->tokens;
            param->attributes = elem->gatherAttributes();
            m_globalConstants[param->name] = param;
        }
        else if (elem->type == parser::ElementType::Function)
        {
            if (auto existingConst  = findGlobalConstant(elem->name))
            {
                err.reportError(elem->location, base::TempString("Global function '{}' hides global constant defined at {}", elem->name, existingConst->loc));
                valid = false;
                continue;
            }
            else if (auto existingFunction  = findGlobalFunction(elem->name))
            {
                err.reportError(elem->location, base::TempString("Global function '{}' was already defined at {}", elem->name, existingFunction->location()));
                valid = false;
                continue;
            }

            // resolve the type
            DataType returnType;
            if (!resolveType(elem->typeRef, returnType, err))
            {
                err.reportError(elem->location, base::TempString("Unable to resolve return type for function '{}'", elem->name));
                valid = false;
                continue;
            }

            /*// resources references must always be compile time
            if (returnType.isResource() && !returnType.isCompileTimeKnown())
            {
                err.reportError(elem->location, base::TempString("Global function '{}' return type '{}' must be declared as \"const\" as it has to be known at the compile time", elem->name, returnType));
                valid = false;
            }*/

            // resolve parameters
            base::InplaceArray<DataParameter*, 16> functionArgs;
            base::InplaceArray<base::StringID, 16> functionArgsNames;
            for (auto param  : elem->children)
            {
                auto name = base::StringID(param->name);

                // variable can't start with "gl_"
                if (elem->name.beginsWith("gl_"))
                {
                    err.reportError(elem->location, base::TempString("Can't start variable names with 'gl_'"));
                    valid = false;
                    continue;
                }

                // already defined ?
                if (functionArgsNames.contains(name))
                {
                    err.reportError(elem->location, base::TempString("Function '{}' argument '{}' was already declared", elem->name, name));
                    valid = false;
                    continue;
                }

                // resolve attributes
                auto attributes = elem->gatherAttributes();

                // resolve type
                DataType paramType;
                if (!resolveType(param->typeRef, paramType, err))
                {
                    err.reportError(param->location, base::TempString("Function '{}' argument '{}' has undefined type", elem->name, name));
                    valid = false;
                    continue;
                }

                // if the types are out/inout than the value passed must be a l-value
                if (param->flags.test(parser::ElementFlag::Out))
                {
                    paramType = paramType.makePointer();
                    attributes.add("out"_id, "");
                }

                /*// if we have a parameter of type "resource" it must be a compile time
                if (paramType.isResource() && !paramType.isCompileTimeKnown())
                {
                    err.reportError(elem->location, base::TempString("Argument '{}' of type '{}' must be declared as \"const\" as it has to be known at the compile time", param->name, paramType));
                    valid = false;
                }*/

                // resolve flags
                auto argParam = m_allocator.create<DataParameter>();
                argParam->name = param->name;
                argParam->loc = param->location;
                argParam->dataType = paramType;
                argParam->scope = DataParameterScope::FunctionInput;
                argParam->assignable = true;
                argParam->attributes = std::move(attributes);
                functionArgs.pushBack(argParam);
                functionArgsNames.pushBack(name);
            }

            // determine function flags
            /*FunctionFlags flags;
            if (elem->flags.test(parser::ElementFlag::Override))
                flags |= FunctionFlag::Override;*/

            // create the function
            auto func = m_allocator.create<Function>(*this, nullptr, elem->location, /*flags, */elem->name, returnType, functionArgs, elem->tokens, elem->gatherAttributes());
            m_globalFunctions[func->name()] = func;
        }
    }

    return valid;
}

static const CodeNode* FindExpression(const CodeNode* node)
{
    if (node->opCode() != OpCode::Scope && node->opCode() != OpCode::Nop)
        return node;

    for (auto child  : node->children())
    {
        auto ret = FindExpression(child);
        if (ret)
            return ret;
    }

    return nullptr;
}

bool CodeLibrary::parseCode(base::parser::IErrorReporter& err)
{
    bool valid = true;

    struct CodeCompilationEntry
    {
        base::parser::Location loc;
        DataParameter* param = nullptr;
        Function* function = nullptr;
        const Program* program = nullptr;
        base::StringID name;
        base::Array<base::parser::Token*> tokens;
        CompositeType::Member* member = nullptr;
    };

    // collect stuff to compile
    base::Array<CodeCompilationEntry> entries;
    {
        for (auto func  : m_globalFunctions.values())
        {
            auto& entry = entries.emplaceBack();
            entry.loc = func->location();
            entry.name = func->name();
            entry.tokens = func->tokens();
            entry.function = const_cast<Function*>(func);
            entry.program = nullptr;
        }

        for (auto param  : m_globalConstants.values())
        {
            auto& entry = entries.emplaceBack();
            entry.loc = param->loc;
            entry.tokens = param->initalizationTokens;
            entry.param = const_cast<DataParameter*>(param);
            entry.name = param->name;
        }

        for (auto composite  : m_typeLibrary->allCompositeTypes())
        {
            for (auto& member : composite->members())
            {
                if (!member.initalizationTokens.empty())
                {
                    auto& entry = entries.emplaceBack();
                    entry.loc = member.location;
                    entry.tokens = member.initalizationTokens;
                    entry.member = const_cast<CompositeType::Member*>(&member);
                    entry.name = member.name;
                }
            }
        }

        for (auto program  : m_programList)
        {
            for (auto param  : program->parameters())
            {
                if (!param->initalizationTokens.empty())
                {
                    auto &entry = entries.emplaceBack();
                    entry.loc = param->loc;
                    entry.tokens = param->initalizationTokens;
                    entry.param = const_cast<DataParameter *>(param);
                    entry.program = program;
                    entry.name = param->name;
                }
            }

            for (auto func  : program->functions())
            {
                auto& entry = entries.emplaceBack();
                entry.loc = func->location();
                entry.tokens = func->tokens();
                entry.function = const_cast<Function*>(func);
                entry.name = func->name();
                entry.program = program;
            }
        }
    }

    // compile stuff
    // TODO: fibers
    for (auto& entry : entries)
    {
        // parse the code from the tokens
        auto code  = parser::AnalyzeShaderCode(allocator(), err, entry.tokens, *this, entry.function, entry.program);
        if (!code)
        {
            valid = false;
            continue;
        }

        // dump shader code if requested
        if (cvDumpShaderAST.get())
            code->print(TRACE_STREAM_INFO().appendf("Initial code for {}:", entry.name), 1, 0);

        // create explicit cast nodes for parameter initializations
        if (entry.param)
        {
            auto castNode  = allocator().create<CodeNode>(code->location(), OpCode::Cast);
            castNode->extraData().m_castType = entry.param->dataType;
            castNode->addChild(code);
            code = castNode;
        }

        // mutate nodes
        base::InplaceArray<CodeNode*, 32> parentStack;
        parentStack.pushBack(nullptr);
        if (!CodeNode::MutateNode(*this, entry.program, entry.function, const_cast<CodeNode*&>(code), parentStack, err))
        {
            valid = false;
            continue;
        }

        // determine types
        TRACE_DEEP("{}: resolving types for '{}'", entry.loc, entry.name);
        if (!CodeNode::ResolveTypes(*this, entry.program, entry.function, const_cast<CodeNode*>(code), err))
        {
            valid = false;
            continue;
        }
        else
        {
            if (code->dataType().valid())
                TRACE_DEEP("{}: type for '{}' resolved to '{}'", entry.loc, entry.name, code->dataType());

            if (cvDumpShaderAST.get())
                code->print(TRACE_STREAM_INFO().appendf("Final code for {}:", entry.name), 1, 0);
        }

        // set the code to target
        if (entry.function)
        {
            entry.function->bindCode(code);
        }
        else if (auto* expr = const_cast<CodeNode*>(FindExpression(code)))
        {
            if (entry.param)
                entry.param->initializerCode = expr;
            else if (entry.member)
                entry.member->initializerCode = expr;
        }
        else
        {
            err.reportError(entry.loc, base::TempString("Unable to compile code for initialization of {}", entry.name));
            valid = false;
        }
    }

    return valid;
}
        
bool CodeLibrary::loadFile(base::parser::TokenList& tokens, base::parser::IErrorReporter& err)
{
    base::ScopeTimer scope;

    // create the content parser
    auto root  = parser::AnalyzeShaderFile(m_allocator, err, tokens);
    if (!root)
        return false;

    // create top level elements
    if (!createElements(root, err))
        return false;

    // update type CodeLibrary
    if (!m_typeLibrary->calculateCompositeLayouts(err))
        return false;

    // dump info
    if (cvDumpShaderAST.get())
    {
        TRACE_INFO("Composite types");
        for (auto table  : m_typeLibrary->allCompositeTypes())
            table->print(TRACE_STREAM_INFO());
    }

    // dump info
    if (cvDumpShaderAST.get())
    {
        TRACE_INFO("Resource tables");
        for (auto table  : m_typeLibrary->allResourceTables())
            table->print(TRACE_STREAM_INFO());
    }               

    // parse code for all functions
    if (!parseCode(err))
        return false;

    // done
    return true;
}

END_BOOMER_NAMESPACE(rendering::shadercompiler)

/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler #]
*/

#include "build.h"
#include "renderingShaderFunction.h"
#include "renderingShaderProgram.h"
#include "renderingShaderProgramInstance.h"
#include "renderingShaderCodeLibrary.h"
#include "renderingShaderLibraryDataBuilder.h"
#include "renderingShaderTypeLibrary.h"
#include "renderingShaderOpcodeGenerator.h"
#include "renderingShaderFunctionFolder.h"
#include "renderingShaderProgramLinker.h"

#include "base/containers/include/stringParser.h"
#include "base/containers/include/stringBuilder.h"

namespace rendering
{
    namespace compiler
    {

        //---

        static void ExtractVertexInputState(const ShaderLibraryBuilder& builder, PipelineIndex vertexInputStateIndex, opcodes::ShaderVertexInputSetup& outShaderVertexInputSetup)
        {
            if (vertexInputStateIndex == INVALID_PIPELINE_INDEX)
                return; // no vertex input state, compute shader bundle ?

            auto& vertexInputState = builder.vertexInputStates()[vertexInputStateIndex];

            const uint8_t maxStreams = 16; // TODO: read from config ?
            for (uint8_t streamIndex = 0; streamIndex < vertexInputState.numStreamLayouts; ++streamIndex)
            {
                auto layoutIndex = builder.indirectIndices()[vertexInputState.firstStreamLayout + streamIndex];
                auto& vertexLayoutState = builder.vertexInputLayouts()[layoutIndex];

                auto bindingName = builder.name(vertexLayoutState.name);

                auto structureIndex = vertexLayoutState.structureIndex;
                DEBUG_CHECK(structureIndex != INVALID_PIPELINE_INDEX);

                auto& dataStructure = builder.dataLayoutStructures()[structureIndex];
                for (auto i = 0; i < dataStructure.numElements; ++i)
                {
                    auto& dataElement = builder.dataLayoutElements()[builder.indirectIndices()[dataStructure.firstElementIndex + i]];
                    auto dataElementName = builder.name(dataElement.name);

                    auto& layoutElement = outShaderVertexInputSetup.emplaceBack();
                    layoutElement.dataFormat = dataElement.format;
                    layoutElement.bindingName = bindingName;
                    layoutElement.memberName = dataElementName;
                    layoutElement.offset = dataElement.offset;
                    layoutElement.streamIndex = streamIndex;
                    layoutElement.layoutIndex = outShaderVertexInputSetup.lastValidIndex();

                    TRACE_SPAM("Vertex element '{}' in binding '{}' bound to stream {}, location {}", layoutElement.memberName, layoutElement.bindingName, layoutElement.streamIndex, layoutElement.layoutIndex);
                }
            }
        }

        void ExtractTouchedParametersFromFunction(const Function* func, base::HashSet<const Function*>& outVisitedSet, base::HashSet<base::StringID>& outParameters);

        void ExtractTouchedParametersFromNode(const CodeNode* code, base::HashSet<const Function*>& outVisitedSet, base::HashSet<base::StringID>& outParameters)
        {
            if (code->opCode() == OpCode::ParamRef)
            {
                if (auto param  = code->extraData().m_paramRef)
                {
                    if (param->scope == DataParameterScope::GlobalParameter)
                    {
                        outParameters.insert(param->name);
                    }
                }
            }
            else if (code->opCode() == OpCode::Const)
            {
                if (code->dataType().isResource() && code->dataValue().isScalar() && code->dataValue().component(0).isName())
                {
                    auto resourceName = code->dataValue().component(0).name();
                    outParameters.insert(resourceName);                            
                }
            }
            else if (code->opCode() == OpCode::Call)
            {
                if (auto func  = code->extraData().m_finalFunctionRef)
                {
                    ExtractTouchedParametersFromFunction(func, outVisitedSet, outParameters);
                }
            }

            for (auto child  : code->children())
                ExtractTouchedParametersFromNode(child, outVisitedSet, outParameters);
        }

        void ExtractTouchedParametersFromFunction(const Function* func, base::HashSet<const Function*>& outVisitedSet, base::HashSet<base::StringID>& outParameters)
        {
            if (func && outVisitedSet.insert(func))
                ExtractTouchedParametersFromNode(&func->code(), outVisitedSet, outParameters);
        }

        void ExtractUsedResourceTables(const CodeLibrary& lib, const base::Array<const Function*>& functions, base::Array<const ResourceTable*>& outUsedResourceTables)
        {
            // extract all data parameters (uniform constants and resources) touched by the shaders
            // NOTE: we found this by recursive visiting functions
            base::HashSet<const Function*> visitedFunctionsSet;
            base::HashSet<base::StringID> usedShaderParameters;
            for (const auto* func : functions)
                ExtractTouchedParametersFromFunction(func, visitedFunctionsSet, usedShaderParameters);

            if (!usedShaderParameters.empty())
            {
                TRACE_DEEP("Discovered {} parameters:", usedShaderParameters.size());
                for (uint32_t i = 0; i < usedShaderParameters.keys().size(); ++i)
                {
                    auto& paramName = usedShaderParameters.keys()[i];
                    TRACE_DEEP("  [{}]: {}", i, paramName);
                }
            }

            base::HashMap<base::StringView, const ResourceTable*> usedDescriptors;

            // extract names of descriptors from the params, yes this is not 100% safe :)
            for (auto& paramName : usedShaderParameters.keys())
            {
                auto desriptorName = paramName.view().beforeFirst("_");

                // lookup existing
                const ResourceTable* desc = nullptr;
                if (!usedDescriptors.find(desriptorName, desc))
                {
                    // linear search
                    for (auto desc  : lib.typeLibrary().allResourceTables())
                    {
                        if (desc->name() == desriptorName)
                        {
                            usedDescriptors[desriptorName] = desc;
                            break;
                        }
                    }
                }
            }

            outUsedResourceTables = usedDescriptors.values();

            // TODO: find better way of sorting the descriptors, but for now make something to make them deterministic
            // NOTE: we try to put heavier descriptors first as those are the "fast slots" in most of the descriptor based implementations but honestly we should look at some kind of metric here
            std::sort(outUsedResourceTables.begin(), outUsedResourceTables.end(), [](const ResourceTable* a, const ResourceTable* b)
                {
                    if (a->members().size() != b->members().size())
                        return a->members().size() > b->members().size(); // larger first

                    return a->name() < b->name();
                });

            if (!outUsedResourceTables.empty())
            {
                TRACE_DEEP("Discovered {} used descriptors (in order of layout):", outUsedResourceTables.size());
                for (uint32_t i = 0; i < outUsedResourceTables.size(); ++i)
                {
                    auto desc  = outUsedResourceTables[i];
                    TRACE_DEEP("  [{}]: {}", i, desc->name());
                }
            }
        }                

        PipelineIndex MapParameterBindingState(ShaderLibraryBuilder& outBuilder, const base::Array<const ResourceTable*>& resourceTables, base::parser::IErrorReporter& err)
        {
            // TODO: extract used static samplers

            // TODO: conform tables

            // build mapping tables
            base::InplaceArray<PipelineIndex, 16> tableIndices;
            for (const auto* table : resourceTables)
            {
                auto paramTableIndex = outBuilder.mapParameterLayout(table);
                DEBUG_CHECK(paramTableIndex != INVALID_PIPELINE_INDEX);
                tableIndices.pushBack(paramTableIndex);
            }

            // map param tables
            ParameterBindingState parameterBinding;
            parameterBinding.firstParameterLayoutIndex = outBuilder.mapIndirectIndices(tableIndices.typedData(), tableIndices.size());
            parameterBinding.numParameterLayoutIndices = tableIndices.size();
            parameterBinding.structureKey = outBuilder.computeResourceBindingKey(parameterBinding);

            // map the whole binding
            return outBuilder.mapParameterBindingState(parameterBinding);
        }

        //--

        void ExtractTouchedVertexInputsFromFunction(const Function* func, base::HashSet<const Function*>& outVisitedSet, base::HashSet<const DataParameter*>& outParameters);

        void ExtractTouchedVertexInputsFromNode(const CodeNode* code, base::HashSet<const Function*>& outVisitedSet, base::HashSet<const DataParameter*>& outParameters)
        {
            if (code->opCode() == OpCode::ParamRef)
            {
                if (auto param = code->extraData().m_paramRef)
                {
                    if (param->scope == DataParameterScope::VertexInput)
                    {
                        outParameters.insert(param);
                    }
                }
            }
            else if (code->opCode() == OpCode::Call)
            {
                if (auto func = code->extraData().m_finalFunctionRef)
                {
                    ExtractTouchedVertexInputsFromFunction(func, outVisitedSet, outParameters);
                }
            }

            for (auto child : code->children())
                ExtractTouchedVertexInputsFromNode(child, outVisitedSet, outParameters);
        }

        void ExtractTouchedVertexInputsFromFunction(const Function* func, base::HashSet<const Function*>& outVisitedSet, base::HashSet<const DataParameter*>& outParameters)
        {
            if (func && outVisitedSet.insert(func))
                ExtractTouchedVertexInputsFromNode(&func->code(), outVisitedSet, outParameters);
        }
                
        struct VertexStream
        {
            const CompositeType* type = nullptr;
            base::parser::Location loc;
            base::StringID bindPointName;
            uint32_t customStride = 0;
            uint8_t instanced = 0;

            void print(base::IFormatStream& f) const
            {
                f << bindPointName;
                f << " type=" << type->name();
                if (customStride != 0)
                    f.appendf("stride={}", customStride);
                if (instanced != 0)
                    f.append("instanced");
            }
        };

        PipelineIndex ExtractUsedVertexStreams(ShaderLibraryBuilder& outBuilder, const CodeLibrary& lib, const Function* a, base::parser::IErrorReporter& err)
        {
            // extract all data parameters (uniform constants and resources) touched by the shaders
            // NOTE: we found this by recursive visiting functions
            base::HashSet<const Function*> visitedFunctionsSet;
            base::HashSet<const DataParameter*> usedShaderParameters;
            ExtractTouchedVertexInputsFromFunction(a, visitedFunctionsSet, usedShaderParameters);

            // all inputs to vertex shader must have a proper "vertex stream" declaration
            bool valid = true;
            base::Array<VertexStream> vertexStreams;
            for (const auto* param : usedShaderParameters.keys())
            {
                DEBUG_CHECK(param->dataType.isComposite());
                DEBUG_CHECK(param->dataType.composite().hint() == CompositeTypeHint::User);
                DEBUG_CHECK(param->dataType.composite().packingRules() == CompositePackingRules::Vertex);

                VertexStream info;
                info.loc = param->loc;
                info.type = &param->dataType.composite();
                info.bindPointName = info.type->name();
                info.customStride = param->attributes.valueAsIntOrDefault("stride"_id);
                info.instanced = param->attributes.has("instanced"_id) ? 1 : 0;

                if (const auto customName = param->attributes.valueOrDefault("binding"_id))
                    info.bindPointName = base::StringID(customName);

                bool addNew = true;
                for (const auto& existingStream : vertexStreams)
                {
                    if (existingStream.bindPointName == info.bindPointName)
                    {
                        if (existingStream.type != info.type)
                        {
                            err.reportError(info.loc, base::TempString("Vertex shader input '{}' was declared previously with different type '{}'", info.bindPointName, existingStream.type->name()));
                            err.reportError(existingStream.loc, base::TempString("Checkout previous declaration"));
                            addNew = false;
                            valid = false;
                        }
                        if (existingStream.customStride != info.customStride)
                        {
                            err.reportError(info.loc, base::TempString("Vertex shader input '{}' was declared previously with different stride '{}'", info.bindPointName, existingStream.customStride));
                            err.reportError(existingStream.loc, base::TempString("Checkout previous declaration"));
                            addNew = false;
                            valid = false;
                        }
                        if (existingStream.instanced != info.instanced)
                        {
                            err.reportError(info.loc, base::TempString("Vertex shader input '{}' was declared previously with different instancing '{}'", info.bindPointName, existingStream.customStride));
                            err.reportError(existingStream.loc, base::TempString("Checkout previous declaration"));
                            addNew = false;
                            valid = false;
                        }
                    }
                }

                if (addNew)
                    vertexStreams.pushBack(info);
            }

            if (!valid)
                return INVALID_PIPELINE_INDEX;

            std::sort(vertexStreams.begin(), vertexStreams.end(), [](const VertexStream& a, const VertexStream& b)
                {
                    if (a.instanced != b.instanced) return a.instanced < b.instanced;
                    return a.bindPointName < b.bindPointName;
                });

            if (!vertexStreams.empty())
            {
                TRACE_DEEP("Discovered {} vertex streams:", vertexStreams.size());
                for (uint32_t i = 0; i < vertexStreams.size(); ++i)
                    TRACE_DEEP("  [{}]: {}", i, vertexStreams[i]);
            }

            /*if (vertexStreams.size() > VertexInputState::MAX_STREAMS)
            {
                const auto& firstOverflowStream = vertexStreams[VertexInputState::MAX_STREAMS];
                err.reportError(firstOverflowStream.loc, base::TempString("To many vertex streams defined, maximum is {}", VertexInputState::MAX_STREAMS));
                return INVALID_PIPELINE_INDEX;
            }*/

            // map layout indices
            base::InplaceArray<PipelineIndex, 16> elements;
            for (const auto& param : vertexStreams)
            {
                // prepare info
                VertexInputLayout elementInfo;
                elementInfo.name = outBuilder.mapName(param.bindPointName.view());
                elementInfo.instanced = param.instanced;
                elementInfo.customStride = param.customStride;
                elementInfo.structureIndex = outBuilder.mapDataLayout(param.type);

                // create the vertex layout mapping entry
                auto vertexLayoutIndex = outBuilder.mapVertexInputLayout(elementInfo);
                DEBUG_CHECK(vertexLayoutIndex != INVALID_PIPELINE_INDEX);
                elements.pushBack(vertexLayoutIndex);
            }

            // map the range of elements, can map to nothing if we have no elements
            const auto indirectIndex = outBuilder.mapIndirectIndices(elements.typedData(), elements.size());
            DEBUG_CHECK_EX(elements.empty() || indirectIndex != INVALID_PIPELINE_INDEX, "Failed to map layout elements");

            // finish with mapping the whole vertex layout structure
            VertexInputState state;
            state.firstStreamLayout = indirectIndex;
            state.numStreamLayouts = elements.size();
            state.structureKey = outBuilder.computeVertexInputStateKey(state);
            return outBuilder.mapVertexInputState(state);
        }

        //--

        base::ConfigProperty<bool> cvDumpFoldedShaders("Rendering.Shader", "DumpFoldedShaders", false);

        PipelineIndex AssembleShaderBundle(
            base::mem::LinearAllocator& mem, // scratch pad JUST for this operation
            ShaderLibraryBuilder& outBuilder,
            const CodeLibrary& readOnlyLib,
            LinkerCache& linkerCache,
            const opcodes::IShaderOpcodeGenerator* generator,
            base::StringView contextPath,
            const ShaderBunderSetup& programInfo,
            base::parser::IErrorReporter& err)
        {
            base::InplaceArray<const Function*, 10> foldedFunctionsList;
            const Function* foldedFunctions[ShaderBunderSetup::MAX_SHADERS];
            memset(foldedFunctions, 0, sizeof(foldedFunctions));
            for (uint32_t i = 0; i < ShaderBunderSetup::MAX_SHADERS; ++i)
            {
                const auto& stage = programInfo.stages[i];

                if (stage.func && stage.pi)
                {
                    auto foldedFunction = linkerCache.foldFunction(stage.func, stage.pi, err);
                    if (!foldedFunction)
                    {
                        err.reportError(stage.func->location(), base::TempString("Failed to fold function '{}' from '{}' for final linking of program",
                            stage.func->name(), stage.pi->program()->name()));
                        err.reportError(programInfo.location, base::TempString("Program was declared here"));
                        return INVALID_PIPELINE_INDEX;
                    }

                    if (cvDumpFoldedShaders.get())
                    {
                        TRACE_INFO("Folded function '{}' from '{}':", stage.func->name(), stage.pi->program()->name());
                        foldedFunction->code().print(TRACE_STREAM_INFO(), 1, 0);
                    }

                    foldedFunctions[i] = foldedFunction;
                    foldedFunctionsList.pushBack(foldedFunction);
                }
            }

            // map the vertex streams, but only if vertex shader was used
            PipelineIndex vertexInputStateIndex = INVALID_PIPELINE_INDEX;
            if (const auto* vertexShaderFunction = foldedFunctions[(uint8_t)ShaderType::Vertex])
            {
                vertexInputStateIndex = ExtractUsedVertexStreams(outBuilder, readOnlyLib, vertexShaderFunction, err);
                if (vertexInputStateIndex == INVALID_PIPELINE_INDEX)
                {
                    err.reportError(vertexShaderFunction->location(), "Unable to map vertex input state used by vertex shader");
                    return INVALID_PIPELINE_INDEX;
                }
            }

            // extract referenced resources
            base::InplaceArray<const ResourceTable*, 16> resourceTables;
            ExtractUsedResourceTables(readOnlyLib, foldedFunctionsList, resourceTables);

            // map the parameter bindings
            auto parameterBindingStateIndex = MapParameterBindingState(outBuilder, resourceTables, err);
            if (parameterBindingStateIndex == INVALID_PIPELINE_INDEX)
            {
                err.reportError(programInfo.location, "Unable to map descriptor binding state");
                return INVALID_PIPELINE_INDEX;
            }

            // build the param mapping for given shader platform
            opcodes::ShaderResourceBindings shaderBindingSetup;
            if (!generator->buildResourceBinding(resourceTables, shaderBindingSetup, err))
            {
                err.reportError(programInfo.location, "Failed to generate platform specific resource binding for this shader");
                return INVALID_PIPELINE_INDEX;
            }

            // extract vertex stream bindings
            opcodes::ShaderVertexInputSetup shaderVertexInputSetup;
            ExtractVertexInputState(outBuilder, vertexInputStateIndex, shaderVertexInputSetup);

            // compile functions in stage order to allow dependencies to flow from one stage to other
            opcodes::ShaderStageDependencies prevStageDependencies; // start with empty list, we assume PS always needs to write to all targets
            base::InplaceArray<PipelineIndex, 10> finalShaderEntries;
            for (uint32_t i = 0; i < ShaderBunderSetup::MAX_SHADERS; ++i)
            {
                if (foldedFunctions[i])
                {
                    opcodes::ShaderStageDependencies generatedDependencies;

                    // lookup function in the cache - maybe we have already compiled one
                    PipelineIndex blobIndex = INVALID_PIPELINE_INDEX;
                    if (!linkerCache.findCompiledShader((ShaderType)i, foldedFunctions[i], &shaderBindingSetup, &shaderVertexInputSetup, &prevStageDependencies, blobIndex, generatedDependencies))
                    {
                        // compile this shader
                        opcodes::ShaderOpcodeGenerationContext context;
                        context.stage = (ShaderType)i;
                        context.bindingSetup = &shaderBindingSetup;
                        context.vertexInput = &shaderVertexInputSetup;
                        context.entryFunction = foldedFunctions[i];
                        context.shaderName = base::TempString("{}_{}_{}", contextPath, Hex(context.entryFunction->foldedKey()), (ShaderType)i);
                        context.requiredShaderOutputs = &prevStageDependencies;

                        // compile the pixel shader
                        base::Buffer compiledShaderCode;
                        if (!generator->generateOpcodes(context, compiledShaderCode, generatedDependencies, err))
                        {
                            err.reportError(programInfo.stages[i].func->location(), "Failed to generate platform specific shader code for this function");
                            return INVALID_PIPELINE_INDEX;
                        }

                        // store blob in output library
                        // NOTE: we may map to existing blob if we generated exactly the same output somehow
                        blobIndex = outBuilder.mapShaderDataBlob((ShaderType)i, compiledShaderCode.data(), compiledShaderCode.size());
                        DEBUG_CHECK(blobIndex != INVALID_PIPELINE_INDEX);

                        // cache for future use
                        linkerCache.storeCompiledShader((ShaderType)i, foldedFunctions[i], &shaderBindingSetup, &shaderVertexInputSetup, &prevStageDependencies, blobIndex, generatedDependencies);

                        // use our dependencies as dependencies for next stage
                        prevStageDependencies = generatedDependencies;
                    }

                    // store blob index for this stage so we can generate a final linked program entry
                    finalShaderEntries.pushBack(blobIndex);
                }
            }

            // no shaders in the end :(
            if (finalShaderEntries.empty())
                return INVALID_PIPELINE_INDEX;

            // map shaders
            ShaderBundle state;
            state.vertexBindingState = vertexInputStateIndex;
            state.firstShaderIndex = outBuilder.mapIndirectIndices(finalShaderEntries.typedData(), finalShaderEntries.size());
            state.numShaders = finalShaderEntries.size();
            state.parameterBindingState = parameterBindingStateIndex;
            state.bundleKey = outBuilder.computeShaderBundleKey(state);
            return outBuilder.mapShaderBundle(state);
        }

        //--

        LinkerCache::LinkerCache(base::mem::LinearAllocator& mem, CodeLibrary& code)
            : m_mem(mem)
        {
            m_folder.create(m_mem, code);
        }

        LinkerCache::~LinkerCache()
        {
            m_folder.reset();
        }

        const Function* LinkerCache::foldFunction(const Function* func, const ProgramInstance* pi, base::parser::IErrorReporter& err)
        {
            return m_folder->foldFunction(func, pi, ProgramConstants(), err);
        }

        static uint64_t CalcBindingSetupHash(const opcodes::ShaderResourceBindings* bindings)
        {
            if (!bindings || bindings->empty())
                return 0;

            base::CRC64 crc;
            crc << bindings->size();
            for (uint32_t i = 0; i < bindings->size(); ++i)
            {
                crc << (*bindings)[i].table->name();
                crc << (*bindings)[i].tableEntry->m_name;
                crc << (*bindings)[i].set;
                crc << (*bindings)[i].position;
            }

            return crc.crc();
        }

        static uint64_t CalcVertexInputSetupHash(const opcodes::ShaderVertexInputSetup* bindings)
        {
            if (!bindings || bindings->empty())
                return 0;

            base::CRC64 crc;
            crc << bindings->size();
            for (uint32_t i = 0; i < bindings->size(); ++i)
            {
                crc << (*bindings)[i].bindingName;
                crc << (*bindings)[i].memberName;
                crc << (*bindings)[i].offset;
                crc << (*bindings)[i].streamIndex;
                crc << (uint8_t)(*bindings)[i].dataFormat;
                crc << (uint8_t)(*bindings)[i].layoutIndex;
            }

            return crc.crc();
        }

        static uint64_t CalcDependencyHash(const opcodes::ShaderStageDependencies* deps)
        {
            if (!deps || deps->empty())
                return 0;

            base::CRC64 crc;
            crc << deps->size();
            for (uint32_t i = 0; i < deps->size(); ++i)
            {
                crc << (*deps)[i].name;
                (*deps)[i].type.calcTypeHash(crc);
                crc << (*deps)[i].assignedLayoutIndex;
            }

            return crc.crc();
        }

        // NOTE: this limits out global caching potential as this key is shit ATM
        // If we find a nice way to distinguishes GLOBALLY a effectively different functions we could save a lot on compilation time
        // Currently the problem is that even the "deep hash" (recursive function hash on code, and all referenced data) is not
        // able to capture differences induced by permutations: e.g.: binding locations shift on descriptor arguments
        // This shitty hash-key results in using a already compiled incompatible cached function when a new one should be compiled
        // If you consider that whole thing can run on fibers and there's a race to the cache what function registers first you really get a proper shit show
        // THUS, AS OF NOW the caching is only permutation-local (ie. we don't reuse compilation results across permutations), 
        // the final shader blobs are of course merged but that's only storage, not time :(
        static uint64_t CalcTempshitFoldedFunctionKey(const Function* func)
        {
            base::CRC64 crc;
            crc << func->name();
            if (func->program())
                crc << func->program()->name();
            crc << func->foldedKey();
            return crc;
        }

        bool LinkerCache::findCompiledShader(ShaderType type, const Function* sourceFunction,
            const opcodes::ShaderResourceBindings* bindingSetup,
            const opcodes::ShaderVertexInputSetup* vertexSetup,
            const opcodes::ShaderStageDependencies* prevStageDeps,
            PipelineIndex& outBlobIndex,
            opcodes::ShaderStageDependencies& outDependencies)
        {
            CompiledFunctionKey key;
            key.type = (uint8_t)type;
            key.functionHash = CalcTempshitFoldedFunctionKey(sourceFunction); // NOTE: this limits out global caching potential as this key is shit ATM
            key.bindingHash = CalcBindingSetupHash(bindingSetup);
            key.vertexInputHash = CalcVertexInputSetupHash(vertexSetup);
            key.prevStageHash = CalcDependencyHash(prevStageDeps);

            if (const auto* data = m_compiledFunctionMap.find(key))
            {
                outBlobIndex = data->blobIndex;
                outDependencies = data->dependencies;
                return true;
            }

            return false;
        }

        void LinkerCache::storeCompiledShader(ShaderType type, const Function* sourceFunction,
            const opcodes::ShaderResourceBindings* bindingSetup,
            const opcodes::ShaderVertexInputSetup* vertexSetup,
            const opcodes::ShaderStageDependencies* prevStageDeps,
            PipelineIndex blobIndex,
            const opcodes::ShaderStageDependencies& dependencies)
        {
            CompiledFunctionKey key;
            key.type = (uint8_t)type;
            key.functionHash = CalcTempshitFoldedFunctionKey(sourceFunction); // NOTE: this limits out global caching potential as this key is shit ATM
            key.bindingHash = CalcBindingSetupHash(bindingSetup);
            key.vertexInputHash = CalcVertexInputSetupHash(vertexSetup);
            key.prevStageHash = CalcDependencyHash(prevStageDeps);

            CompiledFunctionData data;
            data.blobIndex = blobIndex;
            data.dependencies = dependencies;
            m_compiledFunctionMap[key] = data;
        }

        //--

        uint32_t LinkerCache::CompiledFunctionKey::CalcHash(const CompiledFunctionKey& key)
        {
            base::CRC32 crc;
            crc << key.type;
            crc << key.functionHash;
            crc << key.bindingHash;
            crc << key.vertexInputHash;
            crc << key.prevStageHash;
            return crc;
        }

        bool LinkerCache::CompiledFunctionKey::operator==(const CompiledFunctionKey& other) const
        {
            return (type == other.type) && (functionHash == other.functionHash) && (bindingHash == other.bindingHash) && (vertexInputHash == other.vertexInputHash) && (prevStageHash == other.vertexInputHash);
        }

        bool LinkerCache::CompiledFunctionKey::operator!=(const CompiledFunctionKey& other) const
        {
            return !operator==(other);
        }

        //--

    } // pipeline
} // rendering
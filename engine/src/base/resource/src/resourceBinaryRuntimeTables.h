/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization\binary #]
***/

#pragma once

#include "base/object/include/serializationLoader.h"
#include "base/object/include/serializationUnampper.h"
#include "base/fibers/include/fiberSystem.h"

#include "resource.h"

namespace base
{
    namespace res
    {
        namespace binary
        {

            class FileTables;

            /// runtime data mapper, file indices -> actual stuff
            class RuntimeTables : public stream::IDataUnmapper
            {
            public:
                struct ResolvedImport
                {
                    StringBuf path;
                    SpecificClassType<IResource> loadClass;
                    ResourceHandle resource;
                };

                struct ResolvedExport
                {
                    StringID className;
                    SpecificClassType<IObject> objectClass;
                    ObjectPtr object;
                    bool skip = false;
                };

                struct ResolvedProperty
                {
                    const rtti::Property* property = nullptr;
                    StringID name;
                    StringID typeName;
                    StringID className;
                    Type type = nullptr;
                };

                struct ResolvedBuffers
                {
                    stream::DataBufferLatentLoaderPtr access; // Buffer proxy (access wrapper)
                };

                Array< StringID > m_mappedNames;          // Mapped names (runtime data only)
                Array< Type > m_mappedTypes;          // Mapped RTTI types (runtime data only)
                Array< ResolvedProperty > m_mappedProperties;     // Mapped file properties (runtime data only)
                Array< ResolvedImport > m_mappedImports;        // Mapped file imports (runtime data only)
                Array< ResolvedExport > m_mappedExports;        // Mapped file exports (runtime data only)
                Array< ResolvedBuffers > m_mappedBuffers;        // Mapped and resolved buffers

                RuntimeTables();

                // resolve content of file tables into runtime form
                // NOTE: this will wait for all imported resources to be loaded first
                CAN_YIELD void resolve(const stream::LoadingContext& context, const FileTables& fileTables);

                /// create the objects
                void createExports(const stream::LoadingContext& context, const FileTables& fileTables);

                /// load the export data from the source data
                bool loadExports(uint64_t baseOffset, stream::IBinaryReader& file, const stream::LoadingContext& context, const FileTables& fileTables, stream::LoadingResult& outResult);

                /// load all the async buffers that we won't be able to restore
                CAN_YIELD void loadBuffers(stream::IBinaryReader& file, const FileTables& fileTables);

                /// post load all objects
                void postLoad();

                //---

            private:
                virtual void unmapName(const stream::MappedNameIndex index, StringID& outName) override final;
                virtual void unmapType(const stream::MappedTypeIndex index, Type& outTypeRef) override final;
                virtual void unmapProperty(stream::MappedPropertyIndex index, const rtti::Property*& outPropertyRef, StringID& outClassName, StringID& outPropName, StringID& outPropTypeName, Type& outType) override final;
                virtual void unmapPointer(const stream::MappedObjectIndex index, ObjectPtr& outObjectRef) override final;
                virtual void unmapResourceReference(stream::MappedPathIndex index, StringBuf& outResourcePath, ClassType& outResourceClass, ObjectPtr& outObjectPtr) override final;
                virtual void unmapBuffer(const stream::MappedBufferIndex index, stream::DataBufferLatentLoaderPtr& outBufferAccess) override final;

                void resolveNames(const stream::LoadingContext& context, const FileTables& fileTables);
                void resolveImports(const stream::LoadingContext& context, const FileTables& fileTables);
                void resolveProperties(const stream::LoadingContext& context, const FileTables& fileTables);
                void resolveExports(const stream::LoadingContext& context, const FileTables& fileTables);
                void resolveBuffers(const stream::LoadingContext& context, const FileTables& fileTables);
            };

        } // binary
    } // res
} // base
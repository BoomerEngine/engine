/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization\binary #]
***/

#pragma once

#include "resourceBinaryFileTables.h"
#include "base/object/include/serializationSaver.h"
#include "base/containers/include/hashMap.h"
#include "base/object/include/serializationMapper.h"

namespace base
{
    namespace res
    {
        namespace binary
        {
            /// Structure mapper for binary serialization
            class StructureMapper : public stream::IDataMapper
            {
            public:
                struct ImportInfo
                {
                    StringBuf path;
                    ClassType loadClass = nullptr;
                    bool async = false; // resource is used but does not have to be loaded right away

                    INLINE uint64_t crc() const
                    {
                        if (path.empty())
                            return 0;

                        base::CRC64 crc;
                        crc << path;
                        crc << loadClass->name();
                        crc << async;
                        return crc.crc();
                    }
                };

                struct BufferInfo
                {
                    Buffer data;
                    uint64_t crc = 0;     // CRC of buffer's data (serves as the "Key")
                    uint64_t size = 0;
                };

                typedef Array< const IObject* > TExportTable;
                typedef Array< ImportInfo > TImportTable;
                typedef Array< StringID > TNameTable;
                typedef Array< const rtti::Property* > TPropertyTable;
                typedef Array< BufferInfo > TBufferTable;

                typedef HashMap< const void*, stream::MappedObjectIndex > TPointerMap;
                typedef HashMap< StringBuf, stream::MappedPathIndex > TImportMap;
                typedef HashMap< StringID, stream::MappedNameIndex > TNameMap;
                typedef HashMap< uint64_t, stream::MappedBufferIndex > TBufferMap;
                typedef HashMap< const rtti::Property*, stream::MappedPropertyIndex > TPropertyMap;

                const stream::SavingContext* m_context;     // Settings

                TExportTable    m_exports;              // List of contained object
                TImportTable    m_imports;              // List of used external resources
                TNameTable      m_names;                // List of used names
                TPropertyTable  m_properties;           // List of used properties
                TBufferTable    m_buffers;              // List of used buffers

                TPointerMap     m_objectIndices;        // Object remapping indices
                TNameMap        m_nameIndices;          // Name remapping indices
                TBufferMap      m_bufferIndices;        // Buffer remapping indices
                TPropertyMap    m_propertyIndices;      // Property remapping indices
                TImportMap      m_importIndices;        // Import mapping table

                TExportTable    m_tempExports;          // Unsorted list of contained object
                TPointerMap     m_tempObjectIndices;    // Unsorted remapping indices

                StructureMapper();

                /// Map objects from saving set
                void mapObjects(const stream::SavingContext& context);

            private:
                /// Mapping interface - implemented
                virtual void mapName(StringID name, stream::MappedNameIndex& outIndex) override final;
                virtual void mapType(Type rttiType, stream::MappedTypeIndex& outIndex) override final;
                virtual void mapProperty(const rtti::Property* rttiProperty, stream::MappedPropertyIndex& outIndex) override final;
                virtual void mapPointer(const IObject* object, stream::MappedObjectIndex& outIndex) override final;
                virtual void mapResourceReference(StringView<char> path, ClassType resourceClass, bool async, stream::MappedPathIndex& outIndex) override final;
                virtual void mapBuffer(const Buffer& bufferData, stream::MappedBufferIndex& outIndex) override final;

                // Reset tables
                void reset();

                enum class ObjectClassification
                {
                    Ignore,
                    Import,
                    Export,
                };

                // Determine if we should map given object at all and if so should we export it or import it
                bool shouldExport(const IObject* object) const;

                // Map final object, creates sorted export table from unsorted one
                void mapFinalObject(const IObject* object);

                // Follow object - explore the properties to create the full object graph
                void followObject(const IObject* object);
            };

        } // binary
    } // res
} // base
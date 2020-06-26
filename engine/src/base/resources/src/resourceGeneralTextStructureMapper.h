/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization #]
***/

#pragma once

#include "base/containers/include/hashMap.h"
#include "base/object/include/serializationSaver.h"
#include "base/object/include/streamTextWriter.h"

namespace base
{
    namespace res
    {
        /// Structure mapper for binary serialization
        class GeneralStructureMapper : public stream::ITextWriter
        {
        public:
            struct ExportInfo
            {
                const IObject* object;

                Array<uint32_t> childIndices;
                bool root = false;

                INLINE ExportInfo(const IObject*& object)
                    : object(object)
                {}
            };

            struct BufferInfo
            {
                Buffer data;
                uint64_t crc = 0; // crc of buffer's data (serves as the "Key")
                uint64_t size = 0; 
            };

            typedef Array< ExportInfo > TExportTable;
            typedef Array< BufferInfo > TBufferTable;

            typedef Array< uint32_t > TRootObjectIndices;

            typedef HashMap< const void*, uint32_t > TPointerMap;
            typedef HashMap< uint64_t, uint32_t > TImportMap;
            typedef HashMap< uint64_t, uint32_t > TBufferMap;

            const stream::SavingContext* m_context;     // Settings

            TExportTable    m_exports;              // List of contained object
            TBufferTable    m_buffers;              // List of used buffers

            TPointerMap     m_objectIndices;        // Object remapping indices
            TBufferMap      m_bufferIndices;        // Buffer remapping indices

            GeneralStructureMapper();

            /// Map objects from saving set
            void mapObjects(const stream::SavingContext& context);

        private:
            /// ITextWriter interface used for mapping
            virtual void beginArrayElement() override final;
            virtual void endArrayElement() override final;
            virtual void beginProperty(const char* name) override final;
            virtual void endProperty() override final;
            virtual void writeValue(const Buffer& ptr) override final;
            virtual void writeValue(StringView<char> str) override final;
            virtual void writeValue(const IObject* object) override final;
            virtual void writeValue(StringView<char> resourcePath, ClassType resourceClass, const IObject* loadedObject) override final;

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

            // Map the object
            uint32_t mapObject(const IObject* object);

            // Follow object - explore the properties to create the full object graph
            void followObject(const IObject* object);
        };

    } // res
} // base
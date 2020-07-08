/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization #]
***/

#include "build.h"
#include "resourceGeneralTextStructureMapper.h"

namespace base
{
    namespace res
    {

        GeneralStructureMapper::GeneralStructureMapper()
        {}

        void GeneralStructureMapper::reset()
        {
            m_exports.clear();
            m_buffers.clear();
            m_objectIndices.clear();
            m_bufferIndices.clear();
        }

        void GeneralStructureMapper::mapObjects(const stream::SavingContext& context)
        {
            // cleanup
            reset();

            // Add empty entries (for cases when 0 means invalid entry)
            m_context = &context;

            // Map dependencies of initial objects as forced exports
            auto numInitialObjects = m_context->m_initialExports.size();
            for (uint32_t i = 0; i < numInitialObjects; i++)
            {
                auto object = m_context->m_initialExports[i];
                writeValue(object);
            }

            // Make sure the parents are mapped, we don't care about the parent references here
            for (uint32_t i = 0; i < m_exports.size(); ++i)
            {
                auto& info = m_exports[i];

                uint32_t parentIndex = 0;
                auto parentObject = info.object->parent();
                if (!parentObject || m_objectIndices.find(parentObject, parentIndex))
                {
                    if (parentIndex == 0)
                    {
                        info.root = true;
                    }
                    else
                    {
                        auto& parentInfo = m_exports[parentIndex - 1];
                        parentInfo.childIndices.pushBack(i);
                    }
                }
                else
                {
                    FATAL_ERROR("Parent object not mapped");
                }
            }
        }

        void GeneralStructureMapper::beginArrayElement()
        {
        }

        void GeneralStructureMapper::endArrayElement()
        {
        }

        void GeneralStructureMapper::beginProperty(const char* name)
        {
        }

        void GeneralStructureMapper::endProperty()
        {
        }

        void GeneralStructureMapper::writeValue(const Buffer& bufferData)
        {
            // Empty buffer case
            if (!bufferData)
                return;

            // compute the CRC of the data in the buffer, this will be our KEY
            auto crc = base::CRC64().append(bufferData.data(), bufferData.size()).crc();
            DEBUG_CHECK_EX(crc != 0, "Invalid CRC computed from non-zero buffer content");

            // the content may be already mapped
            if (!m_bufferIndices.contains(crc))
            {
                // add to table of buffers
                BufferInfo info;
                info.data = bufferData;
                info.crc = crc;
                info.size = bufferData.size();

                auto index = m_buffers.size();
                m_buffers.pushBack(info);
                m_bufferIndices.set(crc, index);
            }
        }

        void GeneralStructureMapper::writeValue(StringView<char> str)
        {
            // not used during mapping
        }

        uint32_t GeneralStructureMapper::mapObject(const IObject* objectPtr)
        {
            // NULL object case
            if (!objectPtr)
                return 0;

            // Do not map object that are already mapped
            uint32_t index = 0;
            if (m_objectIndices.find(objectPtr, index))
                return index;

            // Classify object based on it's tree placement and class
            if (shouldExport(objectPtr))
            {
                // Add object to export table
                m_exports.pushBack(ExportInfo(objectPtr));
                index = m_exports.size();
            }

            // in the future map to the same index
            m_objectIndices.set(objectPtr, index);

            // Follow the object to explore the dependencies
            if (index != 0)
            {
                auto& info = m_exports[index - 1];
                followObject(objectPtr);
            }

            // return the object index
            return index;
        }

        void GeneralStructureMapper::writeValue(const IObject* object)
        {
            mapObject(object);
        }

        void GeneralStructureMapper::writeValue(StringView<char> resourcePath, ClassType resourceClass, const IObject* loadedObject)
        {
            if (!resourcePath && loadedObject)
                mapObject(loadedObject);
        }

        bool GeneralStructureMapper::shouldExport(const IObject* object) const
        {
            // Don't map NULL objects
            if (!object)
                return false;

            return true;
        }

        void GeneralStructureMapper::followObject(const IObject* object)
        {
            // Notify object
            object->onPreSave();

            // Map parent object
            {
                auto parentObjectRef = object->parent();
                writeValue(parentObjectRef);
            }

            // Serialize (recursive)
            object->onWriteText(*this);
        }

    } // res
} // base
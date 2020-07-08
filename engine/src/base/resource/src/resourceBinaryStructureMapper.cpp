/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization\binary #]
***/

#include "build.h"

#include "resource.h"
#include "resourceBinaryStructureMapper.h"

#include "base/object/include/rttiProperty.h"
#include "base/object/include/rttiType.h"
#include "base/object/include/nullWriter.h"
#include "base/object/include/rttiClassRef.h"
#include "base/object/include/rttiTypeRef.h"

namespace base
{
    namespace res
    {
        namespace binary
        {
            template< typename T >
            static T index_cast(uint64_t index)
            {
                index += 1;

                DEBUG_CHECK_EX(index < (uint64_t)std::numeric_limits<T>::max, "Index value out of range");
                return (T)index;
            }

            StructureMapper::StructureMapper()
            {
            }

            void StructureMapper::reset()
            {
                m_exports.clear();
                m_imports.clear();
                m_names.clear();
                m_properties.clear();
                m_buffers.clear();
                m_objectIndices.clear();
                m_nameIndices.clear();
                m_bufferIndices.clear();
                m_propertyIndices.clear();
                m_importIndices.clear();
                m_tempExports.clear();
                m_tempObjectIndices.clear();
            }

            void StructureMapper::mapObjects(const stream::SavingContext& context)
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

                    stream::MappedObjectIndex index;
                    mapPointer(object, index);
                }

                // Sort mapping tables
                for (uint32_t i = 0; i < m_tempExports.size(); i++)
                    mapFinalObject(m_tempExports[i]);

                // Sanity check
                DEBUG_CHECK_EX(m_tempExports.size() == m_exports.size(), "Mapping error");
            }

            void StructureMapper::mapName(StringID name, stream::MappedNameIndex& outIndex)
            {
                // Empty name case
                if (!!name.empty())
                {
                    outIndex = 0;
                    return;
                }

                // Do not map already mapped objects
                if (!m_nameIndices.find(name, outIndex))
                {
                    // Map
                    outIndex = index_cast<stream::MappedNameIndex>(m_names.size()); // zero has special meaning
                    m_names.pushBack(name);

                    // Map it to an index
                    m_nameIndices.set(name, outIndex);
                }
            }

            void StructureMapper::mapType(Type rttiType, stream::MappedTypeIndex& outIndex)
            {
                // Empty type case
                if (!rttiType)
                {
                    outIndex = 0;
                    return;
                }

                // Map as name
                mapName(rttiType->name(), outIndex);
            }

            void StructureMapper::mapProperty(const rtti::Property* rttiProperty, stream::MappedPropertyIndex& outIndex)
            {
                // Empty property case
                if (!rttiProperty || !rttiProperty->parent())
                {
                    outIndex = 0;
                    return;
                }

                // map only if seen for the first time
                if (!m_propertyIndices.find(rttiProperty, outIndex))
                {
                    stream::MappedNameIndex dummyIndex;
                    mapName(rttiProperty->parent()->name(), dummyIndex);
                    mapName(rttiProperty->name(), dummyIndex);
                    mapName(rttiProperty->type()->name(), dummyIndex);

                    outIndex = index_cast<stream::MappedPropertyIndex>(m_properties.size()); // zero has special meaning
                    m_properties.pushBack(rttiProperty);
                    m_propertyIndices.set(rttiProperty, outIndex);
                }
            }

            void StructureMapper::mapPointer(const IObject* object, stream::MappedObjectIndex& outIndex)
            {
                // NULL object case
                if (!object)
                {
                    outIndex = 0;
                    return;
                }

                // Do not map object that are already mapped
                if (m_tempObjectIndices.find(object, outIndex))
                    return;

                // Classify object based on it's tree placement and class 
                if (shouldExport(object))
                {
                    // Add object to export table
                    outIndex = index_cast<stream::MappedObjectIndex>(m_tempExports.size());
                    m_tempExports.pushBack(object);
                }
                else
                {
                    TRACE_WARNING("Object '{}' (0x{}) not mapped because it's not under any export hierarchy", object->cls()->name(), object);

                    // Map object as NULL
                    outIndex = 0;
                }

                // in the future map to the same index
                m_tempObjectIndices.set(object, outIndex);

                // Follow the object to explore the dependencies
                if (outIndex != 0)
                    followObject(object);
            }

            static void Test(const Type& t)
            {

            }

            void StructureMapper::followObject(const IObject* object)
            {
                // Map class name so we can use it to recreate the object when loading
                {
                    stream::MappedNameIndex dummNameIndex;
                    mapType(object->cls(), dummNameIndex);
                }

                // Notify object
                object->onPreSave();

                // Map parent object
                {
                    stream::MappedObjectIndex parentObjectIndex;

                    auto parentObjectRef = object->parent();
                    mapPointer(parentObjectRef, parentObjectIndex);
                }

                // Serialize (recursive)
                stream::NullWriter nullWriter;
                nullWriter.m_mapper = this;
                //nullWriter.m_saveEditorOnlyProperties = m_context->m_saveEditorOnlyProperties;
                object->onWriteBinary(nullWriter);
            }

            void StructureMapper::mapResourceReference(StringView<char> path, ClassType resourceClass, bool async, stream::MappedPathIndex& outIndex)
            {
                // empty resource case
                if (path.empty())
                {
                    outIndex = 0;
                    return;
                }

                // class name must be specified
                if (resourceClass == nullptr)
                {
                    TRACE_ERROR("No resource class specified for path '{}', reference will not be saved", path);
                    outIndex = 0;
                    return;
                }

                ImportInfo info;
                info.path = StringBuf(path);
                info.async = async;
                info.loadClass = resourceClass;

                // map if seen for the first time
                if (!m_importIndices.find(path, outIndex))
                {
                    outIndex = index_cast<stream::MappedPathIndex>(m_imports.size());

                    m_imports.pushBack(info);
                    m_importIndices.set(StringBuf(path), outIndex);
                }

                // make sure name is mapped
                {
                    stream::MappedNameIndex nameIndex;
                    mapName(info.loadClass->name(), nameIndex);
                }

                // this is not a "soft" dependency, mark that we need this file preloaded
                DEBUG_CHECK_EX(outIndex > 0, "Whopos");
                m_imports[outIndex - 1].async &= async;
            }

            void StructureMapper::mapBuffer(const Buffer& bufferData, stream::MappedBufferIndex& outIndex)
            {
                // Empty buffer case
                if (!bufferData)
                {
                    outIndex = 0;
                    return;
                }
                    
                // compute the CRC of the data in the buffer, this will be our KEY
                auto crc = CRC64().append(bufferData.data(), (uint32_t)bufferData.size()).crc();
                DEBUG_CHECK_EX(crc != 0, "Invalid CRC computed from non-zero buffer content");

                // the content may be already mapped
                if (!m_bufferIndices.find(crc, outIndex))
                {
                    // add to table of buffers
                    BufferInfo info;
                    info.data = bufferData;
                    info.crc = crc;
                    info.size = bufferData.size();

                    outIndex = index_cast<stream::MappedBufferIndex>(m_buffers.size());
                    m_buffers.pushBack(info);
                    m_bufferIndices.set(crc, outIndex);
                }
            }
            
            bool StructureMapper::shouldExport(const IObject* object) const
            {
                // Don't map NULL objects
                if (!object)
                    return false;

                // object must be under the initial export hierarchy
                for (auto& initialExport : m_context->m_initialExports)
                {
                    auto cur = object;
                    while (cur)
                    {
                        if (cur == initialExport)
                            return true;
                        cur = cur->parent();
                    }
                }

                // TODO: add a way NOT to map an object

                // We cannot map objects that are not under the hierarchy
                return false;
            }

            void StructureMapper::mapFinalObject(const IObject* object)
            {
                // NULL pointer
                if (!object)
                    return;

                // Do not map already mapped objects, this also handles imports
                if (m_objectIndices.contains(object))
                    return;

                // Make sure object was initially mapped
                stream::MappedObjectIndex unsortedObjectIndex = 0;
                if (!m_tempObjectIndices.find(object, unsortedObjectIndex))
                {
                    FATAL_ERROR("Unmapped object reached when sorting dependencies");
                    return;
                }

                // Make sure parent object is mapped before
                auto parentObject = object->parent();
                {
                    // map the parent only if it was mapped during the normal phase
                    stream::MappedObjectIndex unsortedParentObjectIndex = 0;
                    if (m_tempObjectIndices.find(parentObject, unsortedParentObjectIndex))
                    {
                        if (unsortedParentObjectIndex != 0)
                            mapFinalObject(parentObject);
                    }
                }

                // Add to final table
                auto objectIndex = m_exports.size();
                m_exports.pushBack(object);

                // Map it to an index
                m_objectIndices.set(object, index_cast<stream::MappedObjectIndex>(objectIndex));
            }

        } // binary
    } // res
} // base
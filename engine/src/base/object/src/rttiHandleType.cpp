/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types #]
***/

#include "build.h"
#include "rttiHandleType.h"
#include "rttiClassType.h"
#include "rttiDataView.h"

#include "streamBinaryWriter.h"
#include "streamBinaryReader.h"
#include "streamTextWriter.h"
#include "streamTextReader.h"

#include "serializationMapper.h"
#include "serializationUnampper.h"

#include "base/containers/include/inplaceArray.h"
#include "dataView.h"

namespace base
{
    namespace rtti
    {

        //--

        IHandleType::IHandleType(StringID name, ClassType pointedType)
            : IType(name)
             , m_pointedType(pointedType)
        {
            //DEBUG_CHECK_EX(m_pointedType->metaType() == MetaType::Class, "Handle internal type must be a class");
            //DEBUG_CHECK_EX(m_pointedType->is< IObject >(), "Handles must use object classes");

            m_traits.initializedFromZeroMem = true;
            m_traits.requiresConstructor = true;
            m_traits.requiresDestructor = true;
        }

        IHandleType::~IHandleType()
        {}

        bool IHandleType::compare(const void* data1, const void* data2) const
        {
            ObjectPtr obj1, obj2;
            readPointedObject(data1, obj1);
            readPointedObject(data2, obj2);

            return obj1 == obj2;
        }

        void IHandleType::copy(void* dest, const void* src) const
        {
            ObjectPtr obj;
            readPointedObject(src, obj);
            writePointedObject(dest, obj);
        }

        void IHandleType::printToText(IFormatStream& f, const void* data, uint32_t flags /*= 0*/) const
        {
            if (flags & PrintToFlag_Editor)
            {
                if (isPointingToNull(data))
                    f << "[Null]";
                else
                    f.appendf("[Pointer to '{}']", pointedClass()->name());
                return;
            }

            return IType::printToText(f, data, flags);
        }
        
        bool IHandleType::writeBinary(const TypeSerializationContext& typeContext, stream::IBinaryWriter& file, const void* data, const void* defaultData) const
        {
            stream::MappedObjectIndex index = 0;

            if (file.m_mapper != nullptr)
            {
                ObjectPtr handle;
                readPointedObject(data, handle);

                file.m_mapper->mapPointer(handle, index);
            }

            file.writeValue(index);
            return true;
        }

        bool IHandleType::readBinary(const TypeSerializationContext& typeContext, stream::IBinaryReader& file, void* data) const
        {
            stream::MappedObjectIndex index = 0;
            file.readValue(index);
            
            if (file.m_unmapper != nullptr)
            {
                ObjectPtr object;
                file.m_unmapper->unmapPointer(index, object);
                writePointedObject(data, object);
            }

            return true;
        }

        bool IHandleType::writeText(const TypeSerializationContext& typeContext, stream::ITextWriter& stream, const void* data, const void* defaultData) const
        {
            ObjectPtr handle;
            readPointedObject(data, handle);

            stream.writeValue(handle);
            return true;
        }

        bool IHandleType::readText(const TypeSerializationContext& typeContext, stream::ITextReader& stream, void* data) const
        {
            ObjectPtr handle;
            if (!stream.readValue(handle))
            {
                TRACE_ERROR("Expected object reference");
                return false;
            }

            writePointedObject(data, handle);
            return true;
        }

        bool IHandleType::CastHandle(const void* srcData, const IHandleType* srcHandleType, void* destData, const IHandleType* destHandleType)
        {
            ASSERT_EX(destHandleType && (destHandleType->metaType() == MetaType::StrongHandle || destHandleType->metaType() == MetaType::WeakHandle), "Invalid destination type");
            ASSERT_EX(srcHandleType && (srcHandleType->metaType() == MetaType::StrongHandle || srcHandleType->metaType() == MetaType::WeakHandle), "Invalid source type");

            ASSERT_EX(destHandleType->pointedClass(), "Destination handle has no base class");
            ASSERT_EX(srcHandleType->pointedClass(), "Destination handle has no base class");

            auto srcPointedType = srcHandleType->pointedClass();
            auto destPointedType = destHandleType->pointedClass();

            // types must be related somehow
            if (srcPointedType.is(destPointedType) || destPointedType.is(srcPointedType))
            {
                ObjectPtr object;
                srcHandleType->readPointedObject(srcData, object);

                if (object && object->is(destPointedType))
                {
                    destHandleType->writePointedObject(destData, object);
                    return true;
                }
            }

            // unable to cast
            return false;
        }

        DataViewResult IHandleType::describeDataView(StringView<char> viewPath, const void* viewData, DataViewInfo& outInfo) const
        {
            if (viewPath.empty())
            {
                if (outInfo.requestFlags.test(DataViewRequestFlagBit::OptionsList))
                {
                    InplaceArray<ClassType, 10> derivedClasses;
                    RTTI::GetInstance().enumClasses(pointedClass(), derivedClasses);

                    for (auto derivedClass : derivedClasses)
                    {
                        auto& optionInfo = outInfo.options.emplaceBack();
                        optionInfo.name = derivedClass->name();
                    }
                }

                ObjectPtr object;
                readPointedObject(viewData, object);

                if (object)
                    if (auto ret = HasError(object->describeDataView(viewPath, outInfo)))
                        return ret;
                    
                outInfo.dataPtr = viewData;
                outInfo.dataType = this; // we can only get ourselves as handle
                outInfo.flags |= DataViewInfoFlagBit::LikeValue;
            }
            else
            {
                ObjectPtr object;
                readPointedObject(viewData, object);

                if (object)
                    return object->describeDataView(viewPath, outInfo);
            }

            return DataViewResultCode::OK;
        }

        DataViewResult IHandleType::readDataView(StringView<char> viewPath, const void* viewData, void* targetData, Type targetType) const
        {
            if (viewPath.empty())
                return IType::readDataView(viewPath, viewData, targetData, targetType);

            ObjectPtr object;
            readPointedObject(viewData, object);

            if (!object)
                return DataViewResultCode::ErrorNullObject;

            return object->readDataView(viewPath, targetData, targetType);
        }

        DataViewResult IHandleType::writeDataView(StringView<char> viewPath, void* viewData, const void* sourceData, Type sourceType) const
        {
            if (viewPath.empty())
                return IType::writeDataView(viewPath, viewData, sourceData, sourceType);

            ObjectPtr object;
            readPointedObject(viewData, object);

            if (!object)
                return DataViewResultCode::ErrorNullObject;

            return object->writeDataView(viewPath, sourceData, sourceType);
        }

        //--

        const char* WeakHandleType::TypePrefix = "weak<";

        StringID FormatWeakHandleTypeName(StringID className)
        {
            StringBuilder builder;
            builder.append(WeakHandleType::TypePrefix);
            builder.append(className.c_str());
            builder.append(">");
            return StringID(builder.c_str());
        }

        WeakHandleType::WeakHandleType(ClassType pointedType)
            : IHandleType(FormatWeakHandleTypeName(pointedType->name()), pointedType)
        {
            m_traits.metaType = MetaType::WeakHandle;
            m_traits.convClass = TypeConversionClass::TypeWeakHandle;
            m_traits.size = sizeof(ObjectWeakPtr);
            m_traits.alignment = __alignof(ObjectWeakPtr);
        }

        WeakHandleType::~WeakHandleType()
        {
        }

        void WeakHandleType::construct(void* object) const
        {
            new (object) ObjectWeakPtr();
        }

        void WeakHandleType::destruct(void* object) const
        {
            auto ptr  = (ObjectWeakPtr*)object;
            ptr->reset();
        }

        void WeakHandleType::calcCRC64(CRC64& crc, const void* data) const
        {
            auto ptr = (ObjectWeakPtr*)data;
            crc << ptr->holder();
        }

        void WeakHandleType::readPointedObject(const void* data, ObjectPtr& outObject) const
        {
            auto& ptr = *(const ObjectWeakPtr*)data;
            outObject = ptr.lock();
        }

        bool WeakHandleType::isPointingToNull(const void* data) const
        {
            auto& ptr = *(const ObjectWeakPtr*)data;
            return ptr.expired();
        }

        void WeakHandleType::writePointedObject(void* data, const ObjectPtr& object) const
        {
            auto& ptr = *(ObjectWeakPtr*)data;

            if (!object || !object->is(m_pointedType))
                ptr.reset();
            else
                ptr = object;
        }

        Type WeakHandleType::ParseType(StringParser& typeNameString, TypeSystem& typeSystem)
        {
            StringID innerTypeName;
            if (!typeNameString.parseTypeName(innerTypeName))
                return nullptr;

            if (!typeNameString.parseKeyword(">"))
                return nullptr;

            if (auto innerType = typeSystem.findClass(innerTypeName))
                return Type(MemNew(WeakHandleType, innerType));

            TRACE_ERROR("Unable to parse a weak handle inner type from '{}'", innerTypeName);
            return nullptr;
        }

        //--

        const char* StrongHandleType::TypePrefix = "strong<";

        StringID FormatStrongHandleTypeName(StringID className)
        {
            StringBuilder builder;
            builder.append(StrongHandleType::TypePrefix);
            builder.append(className.c_str());
            builder.append(">");
            return StringID(builder.c_str());
        }

        StrongHandleType::StrongHandleType(ClassType pointedType)
            : IHandleType(FormatStrongHandleTypeName(pointedType->name()), pointedType)
        {
            m_traits.metaType = MetaType::StrongHandle;
            m_traits.convClass = TypeConversionClass::TypeStrongHandle;
            m_traits.size = sizeof(ObjectPtr);
            m_traits.alignment = __alignof(ObjectPtr);
        }

        StrongHandleType::~StrongHandleType()
        {
        }

        void StrongHandleType::construct(void* object) const
        {
            new (object) ObjectPtr();
        }

        void StrongHandleType::destruct(void* object) const
        {
            auto ptr  = (ObjectPtr*)object;
            ptr->reset();
        }

        void StrongHandleType::calcCRC64(CRC64& crc, const void* data) const
        {
            auto ptr = (ObjectPtr*)data;
            crc << ptr->get();
        }

        void StrongHandleType::readPointedObject(const void* data, ObjectPtr& outObject) const
        {
            auto& ptr = *(const ObjectPtr*)data;
            outObject = ptr;
        }

        bool StrongHandleType::isPointingToNull(const void* data) const
        {
            auto& ptr = *(const ObjectPtr*)data;
            return !ptr;
        }

        void StrongHandleType::writePointedObject(void* data, const ObjectPtr& object) const
        {
            auto& ptr = *(ObjectPtr*)data;

            if (!object || !object->is(m_pointedType))
                ptr.reset();
            else
                ptr = object;
        }

        Type StrongHandleType::ParseType(StringParser& typeNameString, TypeSystem& typeSystem)
        {
            StringID innerTypeName;
            if (!typeNameString.parseTypeName(innerTypeName))
                return nullptr;

            if (!typeNameString.parseKeyword(">"))
                return nullptr;

            if (auto innerType = typeSystem.findClass(innerTypeName))
                return Type(MemNew(StrongHandleType, innerType));

            TRACE_ERROR("Unable to parse a strong handle inner type from '{}'", innerTypeName);
            return nullptr;
        }

        //--

    } // rtti
} // base

/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\typeref #]
***/

#include "build.h"
#include "rttiClassType.h"
#include "rttiClassRef.h"
#include "rttiClassRefType.h"

#include "streamOpcodeWriter.h"
#include "streamOpcodeReader.h"
#include "base/xml/include/xmlWrappers.h"

namespace base
{
    namespace rtti
    {

        //---

        const char* ClassRefType::TypePrefix = "class<";

        //---

        ClassRefType::ClassRefType(ClassType baseClass)
            : IType(FormatClassRefTypeName(baseClass->name()))
            , m_baseClass(baseClass)
        {
            m_traits.metaType = MetaType::ClassRef;
            m_traits.convClass = TypeConversionClass::TypeClassRef;
            m_traits.size = sizeof(ClassType);
            m_traits.alignment = alignof(ClassType);
            m_traits.requiresDestructor = false;
            m_traits.requiresConstructor = true; // we need to zero it
            m_traits.initializedFromZeroMem = true;
            m_traits.simpleCopyCompare = true;
        }

        ClassRefType::~ClassRefType()
        {
        }

        bool ClassRefType::compare(const void* data1, const void* data2) const
        {
            auto& ptr1 = *(const ClassType*)data1;
            auto& ptr2 = *(const ClassType*)data2;
            return ptr1 == ptr2;
        }

        void ClassRefType::copy(void* dest, const void* src) const
        {
            auto& ptr1 = *(ClassType*)dest;
            auto& ptr2 = *(const ClassType*)src;
            ptr1 = ptr2;
        }

        bool ClassRefType::CastClassRef(const void* srcData, const ClassRefType* srcType, void* destData, const ClassRefType* destType)
        {
            ASSERT_EX(destType && (destType->metaType() == MetaType::ClassRef), "Invalid destination type");
            ASSERT_EX(srcType && (srcType->metaType() == MetaType::ClassRef), "Invalid source type");

            auto srcPointedType = srcType->baseClass();
            auto destPointedType = destType->baseClass();

            auto& srcRef = *(const ClassType*)srcData;

            if (!destPointedType || srcRef.is(destPointedType))
            {
                auto& destRef = *(ClassType*)destData;
                destRef = srcRef;
                return true;
            }

            return false;
        }

        void ClassRefType::printToText(IFormatStream& f, const void* data, uint32_t flags) const
        {
            ClassType classType;
            readReferencedClass(data, classType);
            f << classType;
        }

        bool ClassRefType::parseFromString(StringView txt, void* data, uint32_t flags) const
        {
            ClassType classType = nullptr;
            if (txt != "null")
            {
                auto className = StringID::Find(txt);
                classType = RTTI::GetInstance().findClass(className);
                if (!classType)
                    return false;
            }

            if (classType && !classType->is(m_baseClass))
                return false;

            writeReferencedClass(data, classType);
            return true;
        }

        void ClassRefType::readReferencedClass(const void* data, ClassType& outClassRef) const
        {
            outClassRef = *static_cast<const ClassType*>(data);
        }

        void ClassRefType::writeReferencedClass(void* data, ClassType newClassRef) const
        {
            auto& classRef = *static_cast<ClassType*>(data);

            if (newClassRef && newClassRef->metaType() == MetaType::Class && newClassRef->is(m_baseClass))
                classRef = newClassRef;
            else
                classRef = ClassType();
        }

        void ClassRefType::writeBinary(TypeSerializationContext& typeContext, stream::OpcodeWriter& file, const void* data, const void* defaultData) const
        {
            auto& classRef = *static_cast<const ClassType*>(data);
            file.writeType(classRef.ptr());
        }

        void ClassRefType::readBinary(TypeSerializationContext& typeContext, stream::OpcodeReader& file, void* data) const
        {
            StringID typeName;
            auto typeRef = file.readType(typeName);
            writeReferencedClass(data, typeRef.toClass().ptr());
        }

        void ClassRefType::writeXML(TypeSerializationContext& typeContext, xml::Node& node, const void* data, const void* defaultData) const
        {
            auto& classRef = *static_cast<const ClassType*>(data);
            if (classRef)
                node.writeValue(classRef.name().view());
        }

        void ClassRefType::readXML(TypeSerializationContext& typeContext, const xml::Node& node, void* data) const
        {
            base::ClassType classType;

            const auto className = node.value();
            if (className)
            {
                if (auto foundClassType = RTTI::GetInstance().findClass(StringID::Find(className)))
                {
                    if (foundClassType->is(m_baseClass))
                    {
                        classType = foundClassType;
                    }
                    else
                    {
                        TRACE_WARNING("Incompatible class type '{}' referenced at {}", className, typeContext);
                    }
                }
                else
                {
                    TRACE_WARNING("Unknown class type '{}' referenced at {}", className, typeContext);
                }
            }
            
            writeReferencedClass(data, classType);
        }
       
        Type ClassRefType::ParseType(StringParser& typeNameString, TypeSystem& typeSystem)
        {
            StringID innerTypeName;
            if (!typeNameString.parseTypeName(innerTypeName))
                return nullptr;

            if (!typeNameString.parseKeyword(">"))
                return nullptr;

            if (auto innerType = typeSystem.findClass(innerTypeName))
                return new ClassRefType(innerType);

            TRACE_ERROR("Unable to parse class type from '{}'", innerTypeName);
            return nullptr;
        }

        void ClassRefType::construct(void* object) const
        {
            *(ClassType*)object = ClassType();
        }

        void ClassRefType::destruct(void* object) const
        {}

        //---

        extern StringID FormatClassRefTypeName(StringID className)
        {
            StringBuilder builder;
            builder.append(ClassRefType::TypePrefix);
            builder.append(className.c_str());
            builder.append(">");
            return StringID(builder.c_str());
        }

        //---

    } // rtti
} // base
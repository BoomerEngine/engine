/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\typeref #]
***/

#pragma once

#include "rttiType.h"
#include "rttiClassRef.h"

namespace base
{
    namespace rtti
    {
        /// Type definition for a ClassRef, note: a custom base class is allowed
        class BASE_OBJECT_API ClassRefType : public IType
        {
        public:
            ClassRefType(ClassType baseClass);
            virtual ~ClassRefType();

            /// get the base class we are pointing to
            INLINE ClassType baseClass() const { return m_baseClass; }

            /// get the referenced class
            void readReferencedClass(const void* data, ClassType& outClassRef) const;

            /// set the referenced class
            /// NOTE: class must be compatible with the base class
            void writeReferencedClass(void* data, ClassType classRef) const;

            //----

            virtual void construct(void* object) const override final;
            virtual void destruct(void* object) const  override final;
            virtual bool compare(const void* data1, const void* data2) const override final;
            virtual void copy(void* dest, const void* src) const override final;

            virtual void printToText(IFormatStream& f, const void* data, uint32_t flags = 0) const override final;
            virtual bool parseFromString(StringView<char> txt, void* data, uint32_t flags = 0) const override final;

            virtual bool writeBinary(const TypeSerializationContext& typeContext, stream::IBinaryWriter& file, const void* data, const void* defaultData) const override final;
            virtual bool readBinary(const TypeSerializationContext& typeContext, stream::IBinaryReader& file, void* data) const override final;

            virtual bool writeText(const TypeSerializationContext& typeContext, stream::ITextWriter& stream, const void* data, const void* defaultData) const override final;
            virtual bool readText(const TypeSerializationContext& typeContext, stream::ITextReader& stream, void* data) const override final;

            //----

            static const char* TypePrefix;
            static Type ParseType(StringParser& typeNameString, TypeSystem& typeSystem);

            //----

            // cast a class reference to a required type
            static bool CastClassRef(const void* srcData, const ClassRefType* srcType, void* destData, const ClassRefType* destType);

        private:
            ClassType m_baseClass;
        };

    } // rtti
} // base
/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization\mapping #]
***/

#pragma once

#include "base/containers/include/stringBuf.h"
#include "base/containers/include/stringID.h"

namespace base
{
    class StringID;

    namespace stream
    {
    

        class BASE_OBJECT_API PreloadedBufferLatentLoader : public IDataBufferLatentLoader
        {
        public:
            PreloadedBufferLatentLoader(const Buffer& data, uint64_t crc);

            virtual uint32_t size() const override final;
            virtual Buffer loadAsync() const override final;
            virtual uint64_t crc() const override final;
            virtual bool resident() const override final;

        private:
            Buffer m_data;
            uint64_t m_crc;
        };

        class BASE_OBJECT_API IDataUnmapper
        {
        public:
            IDataUnmapper();
            virtual ~IDataUnmapper();

            virtual void unmapName(MappedNameIndex index, StringID& outName) = 0;

            virtual void unmapType(MappedTypeIndex index, Type& outTypeRef) = 0;

            virtual void unmapProperty(MappedPropertyIndex index, const rtti::Property*& outPropertyRef, StringID& outClassName, StringID& outPropName, StringID& outPropTypeName, Type& outType) = 0;

            virtual void unmapPointer(MappedObjectIndex index, ObjectPtr& outObjectRef) = 0;

            virtual void unmapResourceReference(MappedPathIndex index, StringBuf& outResourcePath, ClassType& outResourceClass, ObjectPtr& outObjectPtr) = 0;

            virtual void unmapBuffer(MappedBufferIndex index, RefPtr<IDataBufferLatentLoader>& outBufferAccess) = 0;
        };
    }

} // base

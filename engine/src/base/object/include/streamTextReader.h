/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization\stream\text #]
***/

#pragma once

namespace base
{
    namespace stream
    {

        /// generic text reader interface
        class BASE_OBJECT_API ITextReader
        {
        public:
            virtual ~ITextReader();

            virtual bool hasErrors() const = 0;

            virtual bool beginArrayElement() = 0;
            virtual void endArrayElement() = 0;

            virtual bool beginProperty(StringView<char>& outPropertyName) = 0;
            virtual void endProperty() = 0;

            virtual bool readValue(StringView<char>& outValue) = 0;
            virtual bool readValue(Buffer& outData) = 0;
            virtual bool readValue(ObjectPtr& outValue) = 0;
            virtual bool readValue(ResourceLoadingPolicy policy, StringBuf& outPath, ClassType& outClass, ObjectPtr& outObject) = 0;
        };

    } // stream
} // base

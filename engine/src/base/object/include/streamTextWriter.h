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

        /// generic writer interface
        class BASE_OBJECT_API ITextWriter : public base::NoCopy
        {
        public:
            ITextWriter();
            virtual ~ITextWriter();

            virtual void beginArrayElement() = 0;
            virtual void endArrayElement() = 0;

            virtual void beginProperty(const char* name) = 0;
            virtual void endProperty() = 0;

            virtual void writeValue(const Buffer& ptr) = 0;
            virtual void writeValue(StringView<char> str) = 0;
            virtual void writeValue(const IObject* object) = 0;
            virtual void writeValue(StringView<char> resourcePath, ClassType resourceClass, const IObject* loadedObject) = 0;

            //--

            bool m_saveEditorOnlyProperties:1;
        };

    } // stream
} // base

/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types #]
***/

#include "build.h"
#include "rttiType.h"
#include "rttiTypeSystem.h"

#include "streamBinaryWriter.h"
#include "streamBinaryReader.h"
#include "streamTextReader.h"
#include "streamTextWriter.h"

namespace base
{
    namespace rtti
    {
        /// simple type definition
        template < class T >
        class SimpleValueType : public IType
        {
        public:
            SimpleValueType(const char* typeName, TypeConversionClass typeConversionClass)
                : IType(StringID(typeName))
            {
                m_traits.metaType = MetaType::Simple;
                m_traits.convClass = typeConversionClass;
                m_traits.size = sizeof(T);
                m_traits.alignment = __alignof(T);
                m_traits.nativeHash = typeid(T).hash_code();
                m_traits.requiresConstructor = false;
                m_traits.requiresDestructor = false;
                m_traits.initializedFromZeroMem = true;
                m_traits.simpleCopyCompare = true;
            }

            virtual void construct(void* object) const override final
            {
                new (object) T();
            }

            virtual void destruct(void* object) const override final
            {
                ((T*)object)->~T();
            }

            virtual bool compare(const void* data1, const void* data2) const override final
            {
                return *(const T*)data1 == *(const T*)data2;
            }

            virtual void copy(void* dest, const void* src) const override final
            {
                *(T*)dest = *(const T*)src;
            }

            virtual bool writeBinary(const TypeSerializationContext& typeContext, stream::IBinaryWriter& file, const void* data, const void* defaultData) const override final
            {
                file.writeValue(*(const T*)data);
                return true;
            }

            virtual bool readBinary(const TypeSerializationContext& typeContext, stream::IBinaryReader& file, void* data) const override final
            {
                file.readValue(*(T*)data);
                return true;
            }

            virtual bool writeText(const TypeSerializationContext& typeContext, stream::ITextWriter& stream, const void* data, const void* defaultData) const override final
            {
                stream.writeValue(TempString("{}", *(const T*)data).c_str());
                return true;
            }

            virtual bool readText(const TypeSerializationContext& typeContext, stream::ITextReader& stream, void* data) const override final
            {
                StringView<char> val;
                if (!stream.readValue(val))
                {
                    TRACE_ERROR("Expected string value");
                    return false;
                }

                if (MatchResult::OK != val.match(*(T*)data))
                {
                    TRACE_ERROR("Data conversion from string has failed");
                    return false;
                }

                return true;
            }

            virtual void calcCRC64(CRC64& crc, const void* data) const override
            {
                crc << *(const T*)data;
            }

            virtual void printToText(IFormatStream& f, const void* data, uint32_t flags) const override
            {
                f << *(const T*)data;
            }

            virtual bool parseFromString(StringView<char> txt, void* data, uint32_t flags) const override
            {
                if (txt.empty())
                    return false;
                return MatchResult::OK == txt.match(*(T*)data);
            }
        };

        static bool NeedsQuotes(StringView<char> txt, uint32_t flags)
        {
            // we never need quotes for the editor
            if (flags & PrintToFlag_Editor)
                return false;

            auto readPtr  = txt.data();
            auto endPtr  = txt.data() + txt.length();
            while (readPtr < endPtr)
            {
                auto ch = *readPtr++;

                // if we are printing to a text buffer as a part of a structure printout than we can't use some characters directly (naked) since they are used as string delimiters
                if (ch == '=' || ch == '[' || ch == ']' || ch == '(' || ch == ')' || ch == '\"')
                    return true;
            }

            return false;
        }

        static StringView<char> EatQuotes(StringView<char> txt)
        {
            txt = txt.trim();

            if (txt.beginsWith("\"") && txt.endsWith("\"") && txt.length() >= 2)
                return txt.subString(1, txt.length() - 2);
            return txt;
        }

        class SimpleTypeStringBuf : public SimpleValueType<StringBuf>
        {
        public:
            SimpleTypeStringBuf()
                : SimpleValueType<StringBuf>("StringBuf", TypeConversionClass::TypeStringBuf)
            {
                m_traits.requiresConstructor = true;
                m_traits.initializedFromZeroMem = true;
                m_traits.requiresDestructor = true;
                m_traits.simpleCopyCompare = false;
            }

            virtual void calcCRC64(CRC64& crc, const void* data) const override
            {
                auto& str = *(const StringBuf*)data;
                crc << str;
            }

            virtual void printToText(IFormatStream& f, const void* data, uint32_t flags) const override
            {
                auto& str = *(const StringBuf*)data;

                auto needsQuotes = NeedsQuotes(str.view(), flags);
                if (needsQuotes)
                    f << "\"";

                f.appendCEscaped(str.c_str(), str.length(), needsQuotes);

                if (needsQuotes)
                    f << "\"";
            }

            virtual bool parseFromString(StringView<char> txt, void* data, uint32_t flags) const override
            {
                txt = EatQuotes(txt);

                uint32_t textDataLength = 0;
                auto textData  = DecodeCString(txt.data(), txt.data() + txt.length(), textDataLength, POOL_TEMP);
                if (!textData)
                    return false;

                auto& str = *(StringBuf*)data;
                str = StringBuf(StringView<char>(textData, textData + textDataLength));
                MemFree(textData);

                return  true;
            }
        };

        class SimpleTypeStringID : public SimpleValueType<StringID>
        {
        public:
            SimpleTypeStringID()
                : SimpleValueType<StringID>("StringID", TypeConversionClass::TypeStringID)
            {
                m_traits.requiresConstructor = true;
                m_traits.initializedFromZeroMem = true;
                m_traits.requiresDestructor = false;
                m_traits.simpleCopyCompare = true;
            }

            virtual void calcCRC64(CRC64& crc, const void* data) const override
            {
                auto& str = *(const StringID*)data;
                crc << str.view();
            }

            virtual void printToText(IFormatStream& f, const void* data, uint32_t flags) const override
            {
                auto& str = *(const StringID*)data;
                
                auto needsQuotes = NeedsQuotes(str.view(), flags);
                if (needsQuotes)
                    f << "\"";

                f.appendCEscaped(str.c_str(), str.view().length(), needsQuotes);

                if (needsQuotes)
                    f << "\"";
            }

            virtual bool parseFromString(StringView<char> txt, void* data, uint32_t flags) const override
            {
                uint32_t textDataLength = 0;
                auto textData  = DecodeCString(txt.data(), txt.data() + txt.length(), textDataLength, POOL_TEMP);
                if (!textData)
                    return false;

                auto& str = *(StringID*)data;
                str = StringID(StringView<char>(textData, textData + textDataLength));
                MemFree(textData);

                return true;
            }
        };

        #define REGISTER_FUNDAMENTAL_TYPE( type )   typeSystem.registerType(Type(MemNew(SimpleValueType<type>, #type, TypeConversionClass::Type##type)));

        void RegisterFundamentalTypes(TypeSystem& typeSystem)
        {
            REGISTER_FUNDAMENTAL_TYPE(bool);
            REGISTER_FUNDAMENTAL_TYPE(uint8_t);
            REGISTER_FUNDAMENTAL_TYPE(char);
            REGISTER_FUNDAMENTAL_TYPE(uint16_t);
            REGISTER_FUNDAMENTAL_TYPE(short);
            REGISTER_FUNDAMENTAL_TYPE(uint32_t);
            REGISTER_FUNDAMENTAL_TYPE(int);
            REGISTER_FUNDAMENTAL_TYPE(float);
            REGISTER_FUNDAMENTAL_TYPE(double);
            REGISTER_FUNDAMENTAL_TYPE(int64_t);
            REGISTER_FUNDAMENTAL_TYPE(uint64_t);

            typeSystem.registerType(Type(MemNew(SimpleTypeStringBuf)));
            typeSystem.registerType(Type(MemNew(SimpleTypeStringID)));
        }

    } /// rtti

}  // base
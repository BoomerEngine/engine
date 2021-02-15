/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types #]
***/

#include "build.h"
#include "rttiEnumType.h"
#include "rttiDataView.h"

#include "streamOpcodeWriter.h"
#include "streamOpcodeReader.h"
#include "base/xml/include/xmlWrappers.h"

namespace base
{
    namespace rtti
    {

        EnumType::EnumType(StringID name, uint32_t size, uint64_t nativeHash, bool scripted)
            : IType(name)
            , m_minValue(std::numeric_limits<int64_t>::max())
            , m_maxValue(std::numeric_limits<int64_t>::min())
        {
            DEBUG_CHECK_EX(size == 1 || size == 2 || size == 4 || size == 8, "Unsupported enum size");
            m_traits.metaType = MetaType::Enum;
            m_traits.scripted = scripted;
            m_traits.convClass = TypeConversionClass::TypeEnum;
            m_traits.size = size;
            m_traits.alignment = 1;
            m_traits.nativeHash = nativeHash;
            m_traits.initializedFromZeroMem = true;
            m_traits.requiresConstructor = false;
            m_traits.requiresDestructor = false;
            m_traits.simpleCopyCompare = true;
        }

        void EnumType::clear()
        {
            m_minValue = std::numeric_limits<int64_t>::max();
            m_maxValue = std::numeric_limits<int64_t>::min();
            m_options.reset();
            m_values.reset();
        }

        void EnumType::add(StringID name, int64_t value)
        {
            for (auto existingVal : m_values)
            {
                DEBUG_CHECK_EX(existingVal != value, "Duplicated enum value");
            }

            for (auto existingName : m_options)
            {
                DEBUG_CHECK_EX(existingName != name, "Duplicated enum value");
            }

            m_options.pushBack(name);
            m_values.pushBack(value);

            m_minValue = std::min(m_minValue, value);
            m_maxValue = std::max(m_maxValue, value);
        }

        bool EnumType::findValue(StringID name, int64_t& outValue) const
        {
            for (uint32_t i = 0; i < m_options.size(); ++i)
            {
                if (m_options[i] == name)
                {
                    outValue = m_values[i];
                    return true;
                }
            }

            return false;
        }

        bool EnumType::findName(int64_t value, StringID& outName) const
        {
            for (uint32_t i = 0; i < m_values.size(); ++i)
            {
                if (m_values[i] == value)
                {
                    outName = m_options[i];
                    return true;
                }
            }

            return false;
        }

        bool EnumType::compare(const void* data1, const void* data2) const
        {
            int64_t a, b;

            readInt64(data1, a);
            readInt64(data2, b);

            return (a == b);
        }

        void EnumType::copy(void* dest, const void* src) const
        {
            int64_t val;
            readInt64(src, val);
            writeInt64(dest, val);
        }

        //----

        bool EnumType::toString(const void* value, StringBuf& outName) const
        {
            int64_t val = 0;
            readInt64(value, val);

            StringID name;
            if (!findName(val, name))
                return false;

            outName = StringBuf(name.view());
            return true;
        }

        bool EnumType::toStringID(const void* value, StringID& outNam) const
        {
            int64_t val = 0;
            readInt64(value, val);

            return findName(val, outNam);
        }

        bool EnumType::toNumber(const void* value, int64_t& outNumber) const
        {
            readInt64(value, outNumber);
            return true;
        }

        bool EnumType::fromString(const StringBuf& name, void* outValue) const
        {
            int64_t val = 0;
            if (!findValue(StringID(name.c_str()), val))
                return false;

            writeInt64(outValue, val);
            return true;
        }

        bool EnumType::fromStringID(StringID name, void* outValue) const
        {
            int64_t val = 0;
            if (!findValue(name, val))
                return false;

            writeInt64(outValue, val);
            return true;
        }

        bool EnumType::fromNumber(int64_t number, void* outValue) const
        {
            writeInt64(outValue, number);
            return true;
        }
        
        //----

        void EnumType::printToText(IFormatStream& f, const void* data, uint32_t flags) const
        {
            int64_t val = 0;
            readInt64(data, val);

            StringID optionName;
            if (findName(val, optionName))
                f << optionName;
            else
                f << val;
        }

        bool EnumType::parseFromString(StringView txt, void* data, uint32_t flags) const
        {
            if (auto name = StringID::Find(txt))
            {
                int64_t value = 0;
                if (findValue(name, value))
                {
                    writeInt64(data, value);
                    return true;
                }
            }
            else
            {
                int64_t value = 0;
                if (MatchResult::OK == txt.match(value))
                {
                    writeInt64(data, value);
                    return true;
                }
            }

            return false;
        }

        //----

        void EnumType::writeBinary(TypeSerializationContext& typeContext, stream::OpcodeWriter& file, const void* data, const void* defaultData) const
        {
            int64_t val = 0;
            readInt64(data, val);

            StringID optionName;
            if (!findName(val, optionName))
            {
                TRACE_WARNING("Missing option name for enum %ld in {}, the value would be lost", val, name());
            }

            file.writeStringID(optionName);
        }

        void EnumType::readBinary(TypeSerializationContext& typeContext, stream::OpcodeReader& file, void* data) const
        {
            auto optionName = file.readStringID();

            int64_t val = 0;
            if (findValue(optionName, val))
            {
                writeInt64(data, val);
            }
            else
            {
                TRACE_WARNING("Failed to find numerical value for option '{}' in enum '{}'", optionName, name());
            }
        }
       
        void EnumType::readInt64(const void* data, int64_t& outValue) const
        {
            switch (size())
            {
                case 1: outValue = (int64_t) *(const char*)data; break;
                case 2: outValue = (int64_t) *(const short*)data; break;
                case 4: outValue = (int64_t) *(const int*)data; break;
                case 8: outValue = (int64_t) *(const int64_t*)data; break;
            }
        }

        void EnumType::writeInt64(void* data, int64_t value) const
        {
            switch (size())
            {
                case 1: *(char*)data = (char)value; break;
                case 2: *(short*)data = (short)value; break;
                case 4: *(int*)data = (int)value; break;
                case 8: *(int64_t*)data = value; break;
            }
        }

        const char* GetEnumValueName(const rtti::EnumType* enumType, int64_t enumValue)
        {
            if (!enumType)
                return "InvalidType";

            StringID valueName;
            if (!enumType->findName(enumValue, valueName))
                return "UnknownEnumOption";

            return valueName.c_str();
        }

        bool GetEnumNameValue(const rtti::EnumType* enumType, StringID name, int64_t& outEnumValue)
        {
            if (!enumType)
                return false;

            return enumType->findValue(name, outEnumValue);
        }

        //--

        void EnumType::writeXML(TypeSerializationContext& typeContext, xml::Node& node, const void* data, const void* defaultData) const
        {
            int64_t val = 0;
            readInt64(data, val);

            StringID optionName;
            if (!findName(val, optionName))
            {
                TRACE_WARNING("Missing option name for enum %ld in {} when saving {}, the value would be lost", val, name(), typeContext);
            }
            else
            {
                node.writeValue(optionName.view());
            }
        }

        void EnumType::readXML(TypeSerializationContext& typeContext, const xml::Node& node, void* data) const
        {
            const auto optionName = node.value();

            if (optionName.empty())
            {
                TRACE_WARNING("No option value loaded for enum '{}' at {}, the value will remain as it is", name(), typeContext);
            }
            else
            {
                int64_t value = 0;
                if (!findValue(StringID::Find(optionName), value))
                {
                    TRACE_WARNING("Unknown option '{}' loaded for enum '{}' at {}, the value will remain as it is", optionName, name(), typeContext);
                }
                else
                {
                    writeInt64(data, value);
                }
            }
        }

        //--

        DataViewResult EnumType::describeDataView(StringView viewPath, const void* viewData, DataViewInfo& outInfo) const
        {
            if (viewPath.empty())
            {
                outInfo.dataPtr = viewData;
                outInfo.dataType = this;
                outInfo.flags |= DataViewInfoFlagBit::LikeValue;
                outInfo.flags |= DataViewInfoFlagBit::Constrainded;
                
                // list all options, IF REQUESTED
                if (outInfo.requestFlags.test(DataViewRequestFlagBit::OptionsList))
                {
                    outInfo.options.reserve(m_options.size());
                    for (auto name : m_options)
                    {
                        auto& optionInfo = outInfo.options.emplaceBack();
                        optionInfo.name = name;
                    }
                }
            }

            return IType::describeDataView(viewPath, viewData, outInfo);
        }

        DataViewResult EnumType::readDataView(StringView viewPath, const void* viewData, void* targetData, Type targetType) const
        {
            // NOTE: type conversion enum -> string are supported natively, no more work needed
            return IType::readDataView(viewPath, viewData, targetData, targetType);
        }

        DataViewResult EnumType::writeDataView(StringView viewPath, void* viewData, const void* sourceData, Type sourceType) const
        {
            // NOTE: type conversion string -> enum are supported natively, no more work needed
            return IType::writeDataView(viewPath, viewData, sourceData, sourceType);
        }

        //--

    } // rtti
} // base
/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\view #]
***/

#include "build.h"
#include "rttiType.h"
#include "rttiDataHolder.h"

#include "base/containers/include/stringBuilder.h"

namespace base
{
    namespace rtti
    {
        ///--

        DataHolder::DataHolder(Type type, const void* copyFrom /*= nullptr*/)
        {
            init(type, copyFrom);
        }

        DataHolder::DataHolder(const DataHolder& other)
        {
            init(other.m_type, other.m_data);
        }

        void DataHolder::init(Type type, const void* data)
        {
            ASSERT(m_data == nullptr);

            if (type != nullptr)
            {
                m_type = type;
                m_data = mem::AllocateBlock(POOL_RTTI_DATA, type->size(), type->alignment(), type.name().c_str());

                if (type->traits().requiresConstructor)
                {
                    if (type->traits().initializedFromZeroMem)
                        memset(m_data, 0, type->size());
                    else
                        type->construct(m_data);
                }

                if (data)
                    type->copy(m_data, data);
            }
        }

        DataHolder::DataHolder(DataHolder&& other)
        {
            m_data = other.m_data;
            m_type = other.m_type;
            other.m_type = nullptr;
            other.m_data = nullptr;
        }

        DataHolder::~DataHolder()
        {
            reset();
        }

        DataHolder& DataHolder::operator=(const DataHolder& other)
        {
            if (this != &other)
            {
                reset();
                init(other.m_type, other.m_data);
            }

            return *this;
        }

        DataHolder& DataHolder::operator=(DataHolder&& other)
        {
            if (this != &other)
            {
                reset();
                m_data = other.m_data;
                m_type = other.m_type;
                other.m_type = nullptr;
                other.m_data = nullptr;
            }

            return *this;
        }

        bool DataHolder::operator==(const DataHolder& other) const
        {
            if (m_type != other.m_type)
                return false;

            if (m_type)
                return m_type->compare(m_data, other.m_data);

            return true;
        }

        bool DataHolder::operator!=(const DataHolder& other) const
        {
            return !operator==(other);
        }

        //--

        void DataHolder::print(IFormatStream& f) const
        {
            if (m_type)
                m_type->printToText(f, m_data);
        }

        void DataHolder::printWithDebugInfo(IFormatStream& f) const
        {
            if (m_type)
            {
                m_type->printToText(f, m_data);
                f.appendf(" ({})", m_type);
            }
            else
            {
                f << "empty";
            }
        }

        StringBuf DataHolder::toString() const
        {
            StringBuilder txt;
            print(txt);
            return txt.toString();
        }

        //--

        void DataHolder::reset()
        {
            if (m_type)
            {
                if (m_type->traits().requiresDestructor)
                    m_type->destruct(m_data);

                mem::FreeBlock(m_data);
                m_data = nullptr;
                m_type = nullptr;
            }
        }

        bool DataHolder::writeData(const void* fromData, Type type)
        {
            if (m_type)
                return ConvertData(fromData, type.ptr(), m_data, m_type.ptr());
            else
                return false;
        }

        bool DataHolder::readData(void* toData, Type type)
        {
            if (m_type)
                return ConvertData(m_data, m_type.ptr(), toData, type.ptr());
            else
                return false;
        }

        DataHolder DataHolder::cast(Type type) const
        {
            if (empty())
                return DataHolder();

            DataHolder output(type);
            if (output.writeData(m_data, m_type))
                return std::move(output);
            else
                return DataHolder();
        }

        //--

        DataHolder DataHolder::Parse(StringView txt, Type type)
        {
            DataHolder ret(type);

            if (type && type->parseFromString(txt, ret.m_data))
                return ret;
            else
                return nullptr;
        }

        //--

    } // rtti
} // base
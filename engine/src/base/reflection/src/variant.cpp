/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: variant #]
***/

#include "build.h"
#include "variant.h"
#include "base/object/include/streamTextWriter.h"
#include "base/object/include/streamTextReader.h"
#include "base/object/include/streamBinaryWriter.h"
#include "base/object/include/streamBinaryReader.h"
#include "base/object/include/rttiType.h"

#include "base/containers/include/stringBuilder.h"

namespace base
{
    //--

    mem::PoolID POOL_VARIANT("Engine.Variant");

    //--

    static bool MustAllocateVariantMemory(Type type)
    {
        if (type->traits().requiresConstructor || type->traits().requiresDestructor)
            return true;
        if (type->alignment() > 4 || type->size() > Variant::INTERNAL_STORAGE_SIZE)
            return true;

        //return false;
        return true;
    }

    //--

    Variant::Variant(const Variant& other)
    {
#ifdef BUILD_DEBUG
        other.validate();
#endif
        if (!other.empty())
            init(other.type(), other.data());
    }

    Variant::Variant(Variant&& other)
    {
#ifdef BUILD_DEBUG
        other.validate();
#endif
        if (other)
            suckFrom(other);
    }

    Variant::Variant(Type type, const void* initialData)
    {
        init(type, initialData);
    }

    Variant::~Variant()
    {
        reset();
    }

    Variant& Variant::operator=(const Variant& other)
    {
#ifdef BUILD_DEBUG
        validate();
        other.validate();
#endif
        auto old = std::move(*this);
        if (!other.empty())
            init(other.type(), other.data());
        return *this;
    }

    Variant& Variant::operator=(Variant&& other)
    {
#ifdef BUILD_DEBUG
        validate();
        other.validate();
#endif
        if (this != &other)
        {
            auto old = std::move(*this);
            suckFrom(other);
        }
        return *this;
    }

    bool Variant::operator==(const Variant& other) const
    {
#ifdef BUILD_DEBUG
        validate();
        other.validate();
#endif
        if (type() && type() == other.type())
            return type()->compare(data(), other.data());
        return false;
    }

    bool Variant::operator!=(const Variant& other) const
    {
#ifdef BUILD_DEBUG
        validate();
        other.validate();
#endif
        return !operator==(other);
    }

    void Variant::init(Type type, const void* contentToCopyFrom)
    {
        ASSERT_EX(!m_type, "Variant should be empty before initialization");
        DEBUG_CHECK_EX(type, "Trying to create variant from non existing type");

        if (!type)
            return;        

        void* targetPtr = nullptr;

        m_allocated = MustAllocateVariantMemory(type);
        if (m_allocated)
        {
            targetPtr = MemAlloc(POOL_VARIANT, type->size(), type->alignment());
            m_data.ptr = targetPtr;
        }
        else
        {
            targetPtr = m_data.bytes;
        }

        memset(targetPtr, 0, type->size());
        type->construct(targetPtr);

        /*if (type->traits().requiresConstructor)
            type->construct(targetPtr);
        else if (type->traits().initializedFromZeroMem)
            memset(targetPtr, 0, type->size());*/

        if (contentToCopyFrom)
            type->copy(targetPtr, contentToCopyFrom);

        m_type = type;
    }

    void Variant::suckFrom(Variant& other)
    {
#ifdef BUILD_DEBUG
        validate();
        other.validate();
#endif
        ASSERT_EX(!m_type, "Variant should be empty before initialization");

        if (other.m_allocated)
        {
            m_allocated = true;
            m_data.ptr = other.m_data.ptr;
            m_type = other.m_type;
        }
        else
        {
            init(other.type(), other.data());
            other.reset();
        }

        memset(other.m_data.bytes, 0, INTERNAL_STORAGE_SIZE);
        other.m_type = nullptr;
        other.m_allocated = false;
    }

    void Variant::validate() const
    {
        DEBUG_CHECK(!m_type || (m_allocated == MustAllocateVariantMemory(m_type)));
    }

    void* Variant::data()
    {
#ifdef BUILD_DEBUG
        validate();
#endif
        return m_type ? (m_allocated ? m_data.ptr : m_data.bytes) : nullptr;
    }

    const void* Variant::data() const
    {
#ifdef BUILD_DEBUG
        validate();
#endif        
        return m_type ? (m_allocated ? m_data.ptr : m_data.bytes) : nullptr;
    }

    void Variant::reset()
    {
#ifdef BUILD_DEBUG
        validate();
#endif

        if (m_type)
        {
            if (m_allocated)
            {
                if (m_type->traits().requiresDestructor)
                    m_type->destruct(m_data.ptr);
                MemFree(m_data.ptr);
            }
            else
            {
                if (m_type->traits().requiresDestructor)
                    m_type->destruct(m_data.bytes);
            }

            memset(m_data.bytes, 0, sizeof(m_data));
            m_type = nullptr;
            m_allocated = false;
        }
    }

    bool Variant::set(const void* srcData, Type srcType)
    {
#ifdef BUILD_DEBUG
        validate();
#endif
        return rtti::ConvertData(srcData, srcType, data(), type());
    }

    bool Variant::get(void* destData, Type destType) const
    {
#ifdef BUILD_DEBUG
        validate();
#endif
        return rtti::ConvertData(data(), type(), destData, destType);
    }

    bool Variant::setFromString(StringView<char> txt)
    {
#ifdef BUILD_DEBUG
        validate();
#endif
        if (type())
            return type()->parseFromString(txt, data(), false);
        else
            return false;
    }

    void Variant::getAsString(IFormatStream& f) const
    {
#ifdef BUILD_DEBUG
        validate();
#endif
        if (type())
            return type()->printToText(f, data(), false);
    }

    void Variant::print(base::IFormatStream& f) const
    {
#ifdef BUILD_DEBUG
        validate();
#endif
        if (type())
            type()->printToText(f, data(), false);
    }

    void Variant::calcCRC64(CRC64& crc) const
    {
        crc << type();
        if (data())
            type()->calcCRC64(crc, data());
    }

    void Variant::printDebug(base::IFormatStream& f) const
    {
#ifdef BUILD_DEBUG
        validate();
#endif
        if (type())
        {
            type()->printToText(f, data(), false);
            f.append(" ({})", type());
        }
        else
        {
            f << "empty";
        }
    }

    static const Variant theEmpty;

    const Variant& Variant::EMPTY()
    {
        return theEmpty;
    }

    //---

    namespace prv
    {
        static bool VariantWriteBinary(const base::rtti::TypeSerializationContext& typeContext, base::stream::IBinaryWriter& stream, const void* data, const void* defaultData)
        {
            auto v  = (const Variant*)data;
            stream.writeType(v->type());

            if (v->type())
                return v->type()->writeBinary(typeContext, stream, v->data(), nullptr);
            return true;
        }

        static bool VariantReadBinary(const base::rtti::TypeSerializationContext& typeContext, base::stream::IBinaryReader& stream, void* data)
        {
            auto type = stream.readType();

            auto v  = (Variant*)data;
            v->reset();

            if (type)
            {
                v->init(type, nullptr);
                if (!v->type()->readBinary(typeContext, stream, v->data()))
                    return false;
            }

            return true;
        }

        static bool VariantWriteText(const base::rtti::TypeSerializationContext& typeContext, base::stream::ITextWriter& stream, const void* data, const void* defaultData)
        {
            auto v  = (const Variant*)data;

            if (!v->empty())
            {
                stream.beginProperty("type");
                stream.writeValue(v->type().name().view());
                stream.endProperty();

                stream.beginProperty("value");

                if (!v->type()->writeText(typeContext, stream, v->data(), nullptr))
                {
                    stream.endProperty();
                    return false;
                }

                stream.endProperty();
            }

            return true;
        }

        static bool VariantReadText(const rtti::TypeSerializationContext& typeContext, stream::ITextReader& stream, void* data)
        {
            auto v  = (Variant*)data;
            v->reset();

            Type variantType;
            StringView<char> propName;
            if (stream.beginProperty(propName) && propName == "type")
            {
                StringView<char> typeName;
                stream.readValue(typeName);
                stream.endProperty();

                // find type
                variantType = rtti::TypeSystem::GetInstance().findType(StringID(typeName));
                if (!variantType)
                {
                    TRACE_ERROR("Failed to restore variant because the type '{}' is unknown", typeName);
                    return false;
                }

                // re initialize the variant to the proper type
                v->init(variantType, nullptr);

                // load the value of the variant
                if (stream.beginProperty(propName) && propName == "value")
                {
                    if (variantType->readText(typeContext, stream, v->data()))
                    {
                        stream.endProperty();
                        return true;
                    }

                    stream.endProperty();
                }
            }

            return false;
        }


    } // prv

    RTTI_BEGIN_CUSTOM_TYPE(Variant);
        RTTI_BIND_NATIVE_CTOR_DTOR(Variant);
        RTTI_BIND_NATIVE_COPY(Variant);
        RTTI_BIND_NATIVE_COMPARE(Variant);
        RTTI_BIND_NATIVE_PRINT(Variant);
        RTTI_BIND_NATIVE_HASH(Variant);
        RTTI_BIND_CUSTOM_BINARY_SERIALIZATION(&prv::VariantWriteBinary, &prv::VariantReadBinary);
        RTTI_BIND_CUSTOM_TEXT_SERIALIZATION(&prv::VariantWriteText, &prv::VariantReadText);
    RTTI_END_TYPE();

    //--

    namespace prv
    {

        static bool WriteBinary(const base::rtti::TypeSerializationContext& typeContext, base::stream::IBinaryWriter& stream, const void* data, const void* defaultData)
        {
            auto v = *(const Type*)data;
            stream.writeType(v);
            return true;
        }

        static bool ReadBinary(const base::rtti::TypeSerializationContext& typeContext, base::stream::IBinaryReader& stream, void* data)
        {
            *(Type*)data = stream.readType();
            return true;
        }

        static bool WriteText(const base::rtti::TypeSerializationContext& typeContext, base::stream::ITextWriter& stream, const void* data, const void* defaultData)
        {
            auto v = *(const Type*)data;
            stream.writeValue(v->name().view());
            return true;
        }

        static bool ReadText(const rtti::TypeSerializationContext& typeContext, stream::ITextReader& stream, void* data)
        {
            StringView<char> typeName;
            stream.readValue(typeName);
            *(Type*)data = RTTI::GetInstance().findType(StringID(typeName));;
            return false;
        }

    } // prv

    RTTI_BEGIN_CUSTOM_TYPE(Type);
        RTTI_BIND_NATIVE_CTOR_DTOR(Type);
        RTTI_BIND_NATIVE_COPY(Type);
        RTTI_BIND_NATIVE_COMPARE(Type);
        RTTI_BIND_NATIVE_PRINT(Type);
        RTTI_BIND_NATIVE_HASH(Type);
        RTTI_BIND_CUSTOM_BINARY_SERIALIZATION(&prv::WriteBinary, &prv::ReadBinary);
        RTTI_BIND_CUSTOM_TEXT_SERIALIZATION(&prv::WriteText, &prv::ReadText);
    RTTI_END_TYPE();

    //--

    namespace prv2
    {

        static bool WriteBinary(const base::rtti::TypeSerializationContext& typeContext, base::stream::IBinaryWriter& stream, const void* data, const void* defaultData)
        {
            auto v = *(const Type*)data;
            stream.writeType(v);
            return true;
        }

        static bool ReadBinary(const base::rtti::TypeSerializationContext& typeContext, base::stream::IBinaryReader& stream, void* data)
        {
            *(ClassType*)data = stream.readType().toClass();
            return true;
        }

        static bool WriteText(const base::rtti::TypeSerializationContext& typeContext, base::stream::ITextWriter& stream, const void* data, const void* defaultData)
        {
            auto v = *(const ClassType*)data;
            stream.writeValue(v->name().view());
            return true;
        }

        static bool ReadText(const rtti::TypeSerializationContext& typeContext, stream::ITextReader& stream, void* data)
        {
            StringView<char> typeName;
            stream.readValue(typeName);
            *(ClassType*)data = RTTI::GetInstance().findClass(StringID(typeName));;
            return false;
        }

    } // prv2

    RTTI_BEGIN_CUSTOM_TYPE(ClassType);
    RTTI_BIND_NATIVE_CTOR_DTOR(ClassType);
    RTTI_BIND_NATIVE_COPY(ClassType);
    RTTI_BIND_NATIVE_COMPARE(ClassType);
    RTTI_BIND_NATIVE_PRINT(ClassType);
    RTTI_BIND_NATIVE_HASHER(ClassType);
    RTTI_BIND_CUSTOM_BINARY_SERIALIZATION(&prv2::WriteBinary, &prv2::ReadBinary);
    RTTI_BIND_CUSTOM_TEXT_SERIALIZATION(&prv2::WriteText, &prv2::ReadText);
    RTTI_END_TYPE();

} // base

/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: variant #]
***/

#include "build.h"
#include "variant.h"

#include "core/object/include/serializationWriter.h"
#include "core/object/include/serializationReader.h"
#include "core/object/include/rttiType.h"

#include "core/containers/include/stringBuilder.h"
#include "core/xml/include/xmlWrappers.h"

BEGIN_BOOMER_NAMESPACE()

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
        targetPtr = AllocateBlock(POOL_VARIANT, type->size(), type->alignment(), type->name().c_str());
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
            FreeBlock(m_data.ptr);
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
    return ConvertData(srcData, srcType, data(), type());
}

bool Variant::get(void* destData, Type destType) const
{
#ifdef BUILD_DEBUG
    validate();
#endif
    return ConvertData(data(), type(), destData, destType);
}

bool Variant::setFromString(StringView txt)
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

void Variant::print(IFormatStream& f) const
{
#ifdef BUILD_DEBUG
    validate();
#endif
    if (type())
        type()->printToText(f, data(), false);
}

void Variant::printDebug(IFormatStream& f) const
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
    static void VariantWriteBinary(TypeSerializationContext& typeContext, SerializationWriter& stream, const void* data, const void* defaultData)
    {
        const auto& v = *(const Variant*)data;

        if (v.empty())
        {
            stream.writeTypedData<uint8_t>(0);
        }
        else
        {
            stream.writeTypedData<uint8_t>(1);
            stream.writeType(v.type());

            stream.beginSkipBlock();
            v.type()->writeBinary(typeContext, stream, v.data(), nullptr);
            stream.endSkipBlock();
        }
    }

    static void VariantReadBinary(TypeSerializationContext& typeContext, SerializationReader& stream, void* data)
    {
        auto& v = *(Variant*)data;

        uint8_t code = 0;
        stream.readTypedData(code);

        if (code == 0)
        {
            v.reset();
        }
        else if (code == 1)
        {
            StringID typeName;
            const auto type = stream.readType(typeName);

            if (type)
            {
                stream.enterSkipBlock();

                v.init(type, nullptr);
                type->readBinary(typeContext, stream, v.data());
                stream.leaveSkipBlock();
            }
            else
            {
                TRACE_WARNING("Variant at {}: Missing type '{}' that was used to save variant", typeContext, typeName);
                stream.discardSkipBlock();
            }
        }
    }

} // prv

RTTI_BEGIN_CUSTOM_TYPE(Variant);
    RTTI_BIND_NATIVE_CTOR_DTOR(Variant);
    RTTI_BIND_NATIVE_COPY(Variant);
    RTTI_BIND_NATIVE_COMPARE(Variant);
    RTTI_BIND_NATIVE_PRINT(Variant);
    RTTI_BIND_CUSTOM_BINARY_SERIALIZATION(&prv::VariantWriteBinary, &prv::VariantReadBinary);
RTTI_END_TYPE();

//--

namespace prv
{

    static void WriteBinary(TypeSerializationContext& typeContext, SerializationWriter& stream, const void* data, const void* defaultData)
    {
        auto v = *(const Type*)data;
        stream.writeType(v);
    }

    static void ReadBinary(TypeSerializationContext& typeContext, SerializationReader& stream, void* data)
    {
        StringID typeName;
        *(Type*)data = stream.readType(typeName);
    }

} // prv

RTTI_BEGIN_CUSTOM_TYPE(Type);
    RTTI_BIND_NATIVE_CTOR_DTOR(Type);
    RTTI_BIND_NATIVE_COPY(Type);
    RTTI_BIND_NATIVE_COMPARE(Type);
    RTTI_BIND_NATIVE_PRINT(Type);
    RTTI_BIND_CUSTOM_BINARY_SERIALIZATION(&prv::WriteBinary, &prv::ReadBinary);
RTTI_END_TYPE();

//--

namespace prv
{

    void WriteClassTypeBinary(TypeSerializationContext& typeContext, SerializationWriter& stream, const void* data, const void* defaultData)
    {
        const auto& cls = *(const ClassType*)data;
        stream.writeType(cls.ptr());
    }

    void ReadClassTypeBinary(TypeSerializationContext& typeContext, SerializationReader& stream, void* data)
    {
        StringID typeName;
        const auto clsType = stream.readType(typeName);

        auto& cls = *(ClassType*)data;
        cls = clsType.toClass();
    }

    void WriteClassTypeXML(TypeSerializationContext& typeContext, xml::Node& node, const void* data, const void* defaultData)
    {
        const auto& cls = *(const ClassType*)data;
        node.writeValue(cls.name().view());
    }

    void ReadClassTypeXML(TypeSerializationContext& typeContext, const xml::Node& node, void* data)
    {
        StringView typeName = node.value();

        auto& cls = *(ClassType*)data;
        cls = RTTI::GetInstance().findClass(typeName);
    }

} // prv

RTTI_BEGIN_CUSTOM_TYPE(ClassType);
RTTI_BIND_NATIVE_CTOR_DTOR(ClassType);
RTTI_BIND_NATIVE_COMPARE(ClassType);
RTTI_BIND_NATIVE_COPY(ClassType);
RTTI_BIND_CUSTOM_BINARY_SERIALIZATION(&prv::WriteClassTypeBinary, &prv::ReadClassTypeBinary);
RTTI_BIND_CUSTOM_XML_SERIALIZATION(&prv::WriteClassTypeXML, &prv::ReadClassTypeXML);
RTTI_END_TYPE();

END_BOOMER_NAMESPACE()


/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types\class #]
***/

#include "build.h"
#include "rttiClassType.h"
#include "rttiProperty.h"
#include "rttiFunction.h"
#include "rttiTypeSystem.h"
#include "rttiHandleType.h"
#include "rttiArrayType.h"
#include "rttiDataView.h"
#include "rttiResourceReferenceType.h"

#include "streamBinaryBase.h"
#include "streamBinaryWriter.h"
#include "streamBinaryReader.h"
#include "streamTextWriter.h"
#include "streamTextReader.h"
#include "serializationUnampper.h"

namespace base
{
    namespace rtti
    {
        //--

        IClassType::IClassType(StringID name, uint32_t size, uint32_t alignment)
            : IType(name)
            , m_baseClass(nullptr)
            , m_allPropertiesCached(false)
            , m_allFunctionsCached(false)
            , m_userIndex(INDEX_NONE)
        {
            m_traits.metaType = MetaType::Class;
            m_traits.size = size;
            m_traits.alignment = alignment;
        }

        IClassType::~IClassType()
        {
            //m_localProperties.clearPtr();
            //m_localFunctions.clearPtr();
        }

        bool IClassType::compare(const void* data1, const void* data2) const
        {
            for (auto prop  : allProperties())
                if (!prop->type()->compare(prop->offsetPtr(data1), prop->offsetPtr(data2)))
                    return false;

            return true;
        }

        void IClassType::copy(void* dest, const void* src) const
        {
            for (auto prop  : allProperties())
                prop->type()->copy(prop->offsetPtr(dest), prop->offsetPtr(src));
        }

        void IClassType::calcCRC64(CRC64& crc, const void* data) const
        {
            if (traits().simpleCopyCompare)
            {
                crc.append(data, size());
            }
            else
            {
                const auto& props = allProperties();
                crc << props.size();
                for (auto prop : props)
                {
                    crc << prop->name();
                    prop->type()->calcCRC64(crc, prop->offsetPtr(data));
                }
            }
        }

        void IClassType::printToText(IFormatStream& f, const void* data, uint32_t flags) const
        {
            auto defaultData  = defaultObject();

            auto& allProps = allProperties();
            for (auto prop  : allProps)
            {
                // compare the property value with default, do not save if the same
                auto propData  = prop->offsetPtr(data);
                auto propDefaultData  = prop->offsetPtr(defaultData);
                if (propDefaultData)
                    if (prop->type()->compare(propData, propDefaultData))
                        continue;

                // write property
                f << "(";
                f << prop->name();
                f << "=";

                // write property data
                prop->type()->printToText(f, propData, flags | PrintToFlag_TextSerializaitonStructElement);

                // tail
                f << ")";
            }
        }

        static bool IsValidPropertyNameChar(char ch)
        {
            return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_';
        }

        static bool ReadPropertyName(const char*& ptr, const char* endPtr, StringView<char>& outName)
        {
            auto start  = ptr;
            while (ptr < endPtr)
            {
                auto ch = *ptr++;
                if (ch == '=')
                {
                    outName = StringView<char>(start, ptr-1);
                    return true;
                }
                else if (!IsValidPropertyNameChar(ch))
                {
                    return false;
                }
            }

            return false;
        }

        bool SkipString(const char*& ptr, const char* endPtr)
        {
            ASSERT(*ptr == '\"');
            ++ptr;

            if (ptr == endPtr)
                return false;

            while (ptr < endPtr)
            {
                if (*ptr == '\\')
                {
                    ptr += 2;
                    continue;
                }
                else if (*ptr == '\"')
                {
                    ptr += 1;
                    return true;
                }

                ++ptr;
            }

            return false;
        }

        static bool ReadPropertyValue(const char*& ptr, const char* endPtr, StringView<char>& outValue)
        {
            int innerDepth = 0;
            int arrayDepth = 0;

            auto start  = ptr;
            while (ptr < endPtr)
            {
                if (*ptr == '\"')
                {
                    if (!SkipString(ptr, endPtr))
                        return false;
                    continue;
                }

                auto ch = *ptr++;

                if (ch == ')')
                {
                    if (innerDepth == 0 && arrayDepth == 0)
                    {
                        outValue = StringView<char>(start, ptr-1);
                        return true;
                    }
                    else
                    {
                        if (innerDepth == 0)
                            return false;
                        else
                            innerDepth -= 1;
                    }
                }
                else if (ch == '(')
                {
                    innerDepth += 1;
                }
                else if (ch == '[')
                {
                    arrayDepth += 1;
                }
                else if (ch == ']')
                {
                    if (arrayDepth == 0)
                        return false;
                    else
                        arrayDepth -= 1;
                }
            }

            return false;
        }

        bool IClassType::parseFromString(StringView<char> txt, void* data, uint32_t flags) const
        {
            auto readPtr  = txt.data();
            auto endPtr  = txt.data() + txt.length();
            while (readPtr < endPtr)
            {
                auto ch = *readPtr++;
                if (ch <= ' ')
                    continue;

                if (ch != '(')
                    return false;

                StringView<char> propName;
                if (!ReadPropertyName(readPtr, endPtr, propName))
                    return false;

                StringView<char> propValue;
                if (!ReadPropertyValue(readPtr, endPtr, propValue))
                    return false;

                TRACE_INFO("Parsed '{}' = '{}'", propName, propValue);

                auto propID = StringID::Find(propName);
                if (propID)
                {
                    if (auto prop  = findProperty(propID))
                    {
                        auto propData  = prop->offsetPtr(data);
                        if (!prop->type()->parseFromString(propValue, propData, flags | PrintToFlag_TextSerializaitonStructElement))
                            return false;
                    }
                }
            }

            return true;
        }
        
        namespace prv
        {

            //--

            class BinaryBlockSkipProtectorWriter : public base::NoCopy
            {
            public:
                INLINE BinaryBlockSkipProtectorWriter(stream::IBinaryWriter& writer)
                    : m_stream(writer)
                    , m_archiveOffset(writer.pos())
                {
                    uint32_t placeholder = 0;
                    writer.writeValue(placeholder);
                }

                INLINE ~BinaryBlockSkipProtectorWriter()
                {
                    uint64_t diskPastOffset = m_stream.pos();
                    uint32_t skipOffset = static_cast<uint32_t>(diskPastOffset - m_archiveOffset);

                    // Serialize skip offset
                    m_stream.seek(m_archiveOffset);
                    m_stream.writeValue(skipOffset);
                    m_stream.seek(diskPastOffset);
                }

            private:
                stream::IBinaryWriter& m_stream;
                uint64_t m_archiveOffset;
            };

            //--

            class BinaryBlockSkipProtectorReader : public base::NoCopy
            {
            public:
                INLINE BinaryBlockSkipProtectorReader(stream::IBinaryReader& reader)
                    : m_stream(reader)
                {
                    m_postDataOffset = reader.pos();

                    uint32_t skipOffset = 0;
                    reader.readValue(skipOffset);

                    m_postDataOffset += skipOffset;
                }

                INLINE ~BinaryBlockSkipProtectorReader()
                {
                    m_stream.seek(m_postDataOffset);
                }

            private:
                stream::IBinaryReader& m_stream;
                uint64_t m_postDataOffset;
            };

            //--

        } // prv


        bool IClassType::writeBinary(const TypeSerializationContext& typeContext, stream::IBinaryWriter& file, const void* data, const void* defaultData) const
        {
            TypeSerializationContext localContext;
            localContext.classContext = this;

            // get the default context for saving if not specified
            if (!defaultData)
                defaultData = defaultObject();

            // save properties
            auto& allProps = allProperties();
            for (auto prop  : allProps)
            {
                // compare the property value with default, do not save if the same
                auto propData  = prop->offsetPtr(data);
                auto propDefaultData  = defaultData ? prop->offsetPtr(defaultData) : nullptr;
                if (propDefaultData)
                    if (prop->type()->compare(propData, propDefaultData))
                        continue;

                // skip transient properties
                if (prop->flags().test(PropertyFlagBit::Transient))
                    continue;

                // properties are identified by the hash and are mapped in the file to save the space
                file.writeProperty(prop);

                // save property data in a skip able block so we can go over it if we can't read it
                {
                    localContext.propertyContext = prop;

                    prv::BinaryBlockSkipProtectorWriter block(file);
                    if (!prop->type()->writeBinary(localContext, file, propData, propDefaultData))
                    {
                        TRACE_ERROR("Failed to save value for property '{}' in class '{}', type '{}'",
                                                  prop->name().c_str(), name().c_str(), prop->type()->name().c_str());
                        return false;
                    }
                }
            }

            // end of property list
            file.writeProperty(nullptr);
            return true;
        }

        namespace helper
        {
            static bool AreTypesBinaryComaptible(Type originalType, Type targetType)
            {
                if (!originalType)
                    return false;

                else if (originalType->metaType() == MetaType::ResourceRef && targetType->metaType() == MetaType::ResourceRef)
                    return true;

                else if (originalType->metaType() == MetaType::StrongHandle && targetType->metaType() == MetaType::StrongHandle)
                    return true;

                else if (originalType->metaType() == MetaType::WeakHandle && targetType->metaType() == MetaType::WeakHandle)
                    return true;

                else if (originalType->metaType() == MetaType::Array && targetType->metaType() == MetaType::Array)
                {
                    if (originalType.innerType() == targetType.innerType())
                        return true;
                }

                return false;
            }
        }

        bool IClassType::readBinary(const TypeSerializationContext& typeContext, stream::IBinaryReader& file, void* data) const
        {
            TypeSerializationContext localContext;
            localContext.classContext = this;

            bool status = true;

            // load until we get the "end of the list" marker
            while (1)
            {
                // get the actual property
                const Property* prop = nullptr;
                Type originalType = nullptr;
                StringID propName, propClassName, propTypeName;
                if (file.m_unmapper)
                {
                    // read property reference
                    stream::MappedPropertyIndex propIndex = 0;
                    file.readValue(propIndex);

                    // end of the list
                    if (!propIndex)
                        break;

                    // unmap from file
                    file.m_unmapper->unmapProperty(propIndex, prop, propClassName, propName, propTypeName, originalType);
                }
                else
                {
                    // read direct property values
                    propName = file.readName();
                    if (propName.empty())
                        break;

                    // read original data type
                    originalType = file.readType();

                    // find property
                    prop = findProperty(propName);
                    propClassName = name();
                    propTypeName = originalType ? originalType->name() : StringID();
                }

                // load property data
                {
                    prv::BinaryBlockSkipProtectorReader block(file);

                    // valid property
                    if (prop)
                    {
                        // set property to operation context
                        localContext.propertyContext = prop;

                        // type matches ?
                        if (originalType == prop->type() || helper::AreTypesBinaryComaptible(originalType.ptr(), prop->type()))
                        {
                            void* targetData = prop->offsetPtr(data);
                            if (!prop->type()->readBinary(localContext, file, targetData))
                            {
                                TRACE_ERROR("Failed to load data for property '{}' in class '{}', type '{}'", prop->name(), name(), propTypeName);
                                status = false;
                            }
                        }
                        // type does not match but we have original type
                        else if (originalType)
                        {
                            DataHolder originalData(originalType);
                            if (originalType && !originalType->readBinary(localContext, file, originalData.data()))
                            {
                                TRACE_WARNING("Failed to load data for property '{}' in class '{}', type '{}' that was saved as '{}'", prop->name(), name(), propTypeName, originalType.name());
                                // NOTE: we don't fail loading since this property might have been removed
                                continue;
                            }

                            // try to convert the data using built in conversions
                            void* targetData = prop->offsetPtr(data);
                            if (!ConvertData(originalData.data(), originalType, targetData, prop->type()))
                            {
                                // handle type conversion for property, this will try to convert the data in an automatic way
                                handlePropertyTypeChange(typeContext, propName, originalType, originalData.data(), prop->type(), targetData);
                            }
                        }
                        // original type of property was lost, there's no way to load the data any more
                        else
                        {
                            TRACE_WARNING("Failed to load data for property '{}' in class '{}', original data type was removed", prop->name(), name());
                        }
                    }
                    // we've lost property
                    else
                    {
                        if (originalType)
                        {
                            DataHolder originalData(originalType);
                            if (originalType && !originalType->readBinary(localContext, file, originalData.data()))
                            {
                                TRACE_WARNING("Failed to load data for missing property '{}' in class '{}', type '{}' that was saved as '{}'", propName, name(), propTypeName, originalType.name());
                                // NOTE: we don't fail loading since this property might have been removed
                                continue;
                            }

                            // handle missing property, usually gives object a chance to copy data to other fields, etc
                            void* targetData = prop->offsetPtr(data);
                            handlePropertyMissing(typeContext, propName, originalType, originalData.data());
                        }
                        // original type of property was lost, there's no way to load the data any more
                        else
                        {
                            TRACE_WARNING("Failed to load data for property '{}' in class '{}', original data type was removed and property is missing", propName, name());
                        }
                    }
                }
            }

            // return cumulative status
            return status;
        }   

        bool IClassType::writeText(const TypeSerializationContext& typeContext, stream::ITextWriter& stream, const void* data, const void* defaultData) const
        {
            TypeSerializationContext localContext;
            localContext.classContext = this;

            // get the default context for saving if not specified
            if (!defaultData)
                defaultData = defaultObject();

            // save properties
            auto& allProps = allProperties();
            for (auto prop  : allProps)
            {
                // compare the property value with default, do not save if the same
                auto propData  = prop->offsetPtr(data);
                auto propDefaultData  = defaultData ? prop->offsetPtr(defaultData) : nullptr;
                if (propDefaultData)
                    if (prop->type()->compare(propData, propDefaultData))
                        continue;

                // skip transient properties
                if (prop->flags().test(PropertyFlagBit::Transient))
                    continue;

                // begin name property data
                ASSERT_EX(!prop->name().empty(), "Property without name is not allowed");
                stream.beginProperty(prop->name().c_str());
                
                // set property to operation context
                localContext.propertyContext = prop;

                // save property data in a skip able block so we can go over it if we can't read it
                if (!prop->type()->writeText(localContext, stream, propData, propDefaultData))
                {
                    TRACE_ERROR("Failed to save value for property '{}' in class '{}', type '{}'",
                                prop->name().c_str(), name().c_str(), prop->type()->name().c_str());
                    return false;
                }

                // end property data block
                stream.endProperty();
            }

            return true;
        }

        bool IClassType::readText(const TypeSerializationContext& typeContext, stream::ITextReader& stream, void* data) const
        {
            TypeSerializationContext localContext;
            localContext.classContext = this;

            StringView<char> propName;
            while (stream.beginProperty(propName))
            {
                //TRACE_INFO("Loading prop '{}' for '{}'", propName, name());

                auto prop  = findProperty(StringID(propName));
                if (prop)
                {
                    // set property to operation context
                    localContext.propertyContext = prop;

                    auto propData  = prop->offsetPtr(data);
                    if (!prop->type()->readText(localContext, stream, propData))
                    {
                        stream.endProperty();
                        TRACE_ERROR("Failed to load value for property '{}' in class '{}', type '{}'",
                            prop->name(), name(), prop->type()->name());
                        return false;
                    }
                }
                else
                {
                    TRACE_WARNING("Missing propertry '{}' in '{}'", propName, name());
                }
                    
                stream.endProperty();
            }

            return true;
        }

        bool IClassType::describeDataView(StringView<char> viewPath, const void* viewData, DataViewInfo& outInfo) const
        {
            StringView<char> propertyName;

            if (viewPath.empty())
            {
                if (outInfo.requestFlags.test(DataViewRequestFlagBit::MemberList))
                {
                    auto& props = allProperties();

                    outInfo.members.reserve(props.size());
                    for (auto prop : props)
                    {
                        if (prop->editable())
                        {
                            auto& memberInfo = outInfo.members.emplaceBack();
                            memberInfo.name = prop->name();
                            memberInfo.category = prop->category();
                        }
                    }
                }

                outInfo.flags |= DataViewInfoFlagBit::LikeStruct;
                
                return IType::describeDataView(viewPath, viewData, outInfo);
            }
            else if (ParsePropertyName(viewPath, propertyName))
            {
                if (auto prop  = findProperty(StringID::Find(propertyName)))
                {
                    auto propViewData  = prop->offsetPtr(viewData);

                    if (viewPath.empty())
                    {
                        if (outInfo.requestFlags.test(DataViewRequestFlagBit::PropertyMetadata))
                            prop->collectMetadataList(outInfo.metadata);

                        if (prop->inlined())
                            outInfo.flags |= DataViewInfoFlagBit::Inlined;
                        if (prop->readonly())
                            outInfo.flags |= DataViewInfoFlagBit::ReadOnly;
                    }
                    else if (prop->type().metaType() == MetaType::Array)
                    {
                        uint32_t arrayIndex = 0;
                        auto tempViewPath = viewPath;
                        if (ParseArrayIndex(tempViewPath, arrayIndex) && tempViewPath.empty())
                        {
                            if (outInfo.requestFlags.test(DataViewRequestFlagBit::PropertyMetadata))
                                prop->collectMetadataList(outInfo.metadata);

                            if (prop->inlined())
                                outInfo.flags |= DataViewInfoFlagBit::Inlined;
                            if (prop->readonly())
                                outInfo.flags |= DataViewInfoFlagBit::ReadOnly;
                        }
                    }

                    return prop->type()->describeDataView(viewPath, propViewData, outInfo);
                }
            }

            return false;
        }

        bool IClassType::readDataView(IObject* context, const IDataView* rootView, StringView<char> rootViewPath, StringView<char> viewPath, const void* viewData, void* targetData, Type targetType) const
        {
            StringView<char> propertyName;

            if (viewPath.empty())
            {
                return IType::readDataView(context, rootView, rootViewPath, viewPath, viewData, targetData, targetType);
            }
            else if (ParsePropertyName(viewPath, propertyName))
            {
                if (auto prop  = findProperty(StringID::Find(propertyName)))
                {
                    StringBuilder newRootViewPath;
                    newRootViewPath << rootViewPath;
                    if (newRootViewPath.empty())
                        newRootViewPath << ".";
                    newRootViewPath << propertyName;

                    auto propViewData  = prop->offsetPtr(viewData);

                    return prop->type()->readDataView(context, rootView, newRootViewPath.view(), viewPath, propViewData, targetData, targetType);
                }
            }

            return false;
        }

        bool IClassType::writeDataView(IObject* context, const IDataView* rootView, StringView<char> rootViewPath, StringView<char> viewPath, void* viewData, const void* sourceData, Type sourceType) const
        {
            StringView<char> propertyName;

            if (viewPath.empty())
            {
                return IType::writeDataView(context, rootView, rootViewPath, viewPath, viewData, sourceData, sourceType);
            }
            else if (ParsePropertyName(viewPath, propertyName))
            {
                if (auto prop  = findProperty(StringID::Find(propertyName)))
                {
                    StringBuilder newRootViewPath;
                    newRootViewPath << rootViewPath;
                    if (newRootViewPath.empty())
                        newRootViewPath << ".";
                    newRootViewPath << propertyName;

                    auto propViewData  = prop->offsetPtr(viewData);
                    return prop->type()->writeDataView(context, rootView, newRootViewPath.view(), viewPath, propViewData, sourceData, sourceType);
                }
            }

            return false;
        }

        bool IClassType::is(ClassType otherClass) const
        {
            ClassType testClass = this;
            while (testClass != nullptr)
            {
                if (otherClass == testClass)
                    return true;

                testClass = testClass->baseClass();
            }

            return !otherClass; // "null" class is a base of all classes
        }

        const Property* IClassType::findProperty(StringID propertyName) const
        {
            auto& allProps = allProperties();
            for (auto prop  : allProps)
                if (prop->name() == propertyName)
                    return prop;

            return nullptr;
        }

        const Function* IClassType::findFunction(StringID functionName) const
        {
            allFunctions();

            const Function* ret = nullptr;
            m_allFunctionsMap.find(functionName, ret);
            return ret;
        }

        const Function* IClassType::findFunctionNoCache(StringID functionName) const
        {
            for (auto func  : allFunctions())
                if (func->name() == functionName)
                    return func;

            return nullptr;
        }

        const IClassType::TConstProperties& IClassType::allProperties() const
        {
            if (!m_allPropertiesCached)
            {
                m_allTablesLock.acquire();

                if (!m_allPropertiesCached)
                {
                    m_allProperties.clear();

                    if (m_baseClass)
                        m_allProperties = m_baseClass->allProperties();

                    for (auto prop : m_localProperties)
                        m_allProperties.pushBack(prop);

                    m_allPropertiesCached = true;
                }

                m_allTablesLock.release();
            }

            return m_allProperties;
        }

        const IClassType::TConstFunctions& IClassType::allFunctions() const
        {
            if (!m_allFunctionsCached)
            {
                m_allTablesLock.acquire();

                if (!m_allFunctionsCached)
                {
                    m_allFunctions.clear();
                    m_allFunctionsMap.clear();

                    for (auto func : m_localFunctions)
                        m_allFunctions.pushBack(func);

                    if (m_baseClass)
                    {
                        for (auto func : m_baseClass->allFunctions())
                        {
                            m_allFunctions.pushBack(func);
                            m_allFunctionsMap[func->name()] = func;
                        }
                    }

                    for (auto func  : m_localFunctions)
                        m_allFunctionsMap[func->name()] = func;

                    m_allFunctionsCached = true;
                }

                m_allTablesLock.release();
            }

            return m_allFunctions;
        }

        const IMetadata* IClassType::metadata(ClassType metadataType) const
        {
            // check the whole class hierarchy
            ClassType testClass = this;
            while (testClass != nullptr)
            {
                auto metadata  = testClass->MetadataContainer::metadata(metadataType);
                if (metadata != nullptr)
                    return metadata;

                testClass = testClass->baseClass();
            }

            // meta data not found
            return nullptr;
        }

        void IClassType::collectMetadataList(Array<const IMetadata*>& outMetadataList) const
        {
            if (m_baseClass)
                m_baseClass->collectMetadataList(outMetadataList);
            MetadataContainer::collectMetadataList(outMetadataList);
        }

        void IClassType::baseClass(ClassType baseClass)
        {
            if (baseClass == this)
                baseClass = nullptr; // special case for  root classes

            DEBUG_CHECK_EX(m_baseClass == nullptr, "Base class already set");
            DEBUG_CHECK_EX(!baseClass || !baseClass->is(this), "Recursive class chain");

            m_baseClass = baseClass.ptr();
            m_allPropertiesCached = false;

            if (m_baseClass && m_baseClass->name().view().endsWith("Metadata"))
            {
                DEBUG_CHECK_EX(name().view().endsWith("Metadata"), TempString("Metadata class '{}' name must end with Metadata", name()));
            }
        }

        void IClassType::addProperty(Property* property)
        {
            DEBUG_CHECK(property != nullptr);
            DEBUG_CHECK(property->parent() == this);

            m_localProperties.pushBack(property);
            m_allPropertiesCached = false;
        }

        void IClassType::addFunction(Function* function)
        {
            DEBUG_CHECK(function != nullptr);
            DEBUG_CHECK(function->parent() == this);

            m_localFunctions.pushBack(function);
            m_allFunctionsCached = false;
        }

        bool IClassType::handlePropertyMissing(const TypeSerializationContext& context, StringID name, Type dataType, const void* data) const
        {
            if (context.objectContext)
                return context.objectContext->onPropertyMissing(name, dataType, data);

            return false;
        }

        bool IClassType::handlePropertyTypeChange(const TypeSerializationContext& context, StringID name, Type originalDataType, const void* originalData, Type currentType, void* currentData) const
        {
            if (context.objectContext)
                return context.objectContext->onPropertyTypeChanged(name, originalDataType, originalData, currentType, currentData);

            return false;
        }

        void IClassType::cacheTypeData()
        {
            IType::cacheTypeData();

            if (auto metadata  = static_cast<const ShortTypeNameMetadata*>(MetadataContainer::metadata(ShortTypeNameMetadata::GetStaticClass())))
                m_shortName = StringID(metadata->shortName());
            else
                m_shortName = StringID(name().view().afterLastOrFull("::").afterLastOrFull("_"));
        }

        void IClassType::releaseTypeReferences()
        {
            m_localProperties.clearPtr();
            m_localFunctions.clearPtr();
        }

        void IClassType::assignUserIndex(short index) const
        {
            ASSERT_EX(m_userIndex == -1 || m_userIndex == index, "User index already assigned");
            ASSERT_EX(index >= 0, "Cannot assign negative indices");
            m_userIndex = index;
        }

        //--

        BASE_OBJECT_API bool PatchResourceReferences(Type type, void* data, res::IResource* currentResource, res::IResource* newResource)
        {
            bool patched = false;

            switch (type.metaType())
            {
                case MetaType::ResourceRef:
                case MetaType::AsyncResourceRef:
                {
                    const auto* specificType = static_cast<const IResourceReferenceType*>(type.ptr());
                    patched = specificType->referencePatchResource(data, currentResource, newResource);
                    break;
                }

                case MetaType::Array:
                {
                    const auto* specificType = static_cast<const IArrayType*>(type.ptr());
                    const auto innerType = specificType->innerType();
                    const auto size = specificType->arraySize(data);
                    for (uint32_t i = 0; i < size; ++i)
                    {
                        auto arrayElementData = specificType->arrayElementData(data, i);
                        patched |= PatchResourceReferences(innerType, arrayElementData, currentResource, newResource);
                    }
                    break;
                }

                case MetaType::Class:
                {
                    const auto* specificType = static_cast<const IClassType*>(type.ptr());
                    return specificType->patchResourceReferences(data, currentResource, newResource, nullptr);
                }
            }

            return patched;
        }

        bool IClassType::patchResourceReferences(void* data, res::IResource* currentResource, res::IResource* newResource, base::Array<base::StringID>* outPatchedProperties) const
        {
            bool patched = false;

            for (const auto* prop : allProperties())
            {
                auto* propertyData = prop->offsetPtr(data);
                if (PatchResourceReferences(prop->type(), propertyData, currentResource, newResource))
                {
                    if (outPatchedProperties)
                        outPatchedProperties->pushBack(prop->name());
                    patched = true;
                }
            }

            return patched;
        }

        //--

    } // rtti

    //--

} // base

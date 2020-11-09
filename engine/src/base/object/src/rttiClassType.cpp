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

#include "streamOpcodeWriter.h"
#include "streamOpcodeReader.h"
#include "base/xml/include/xmlWrappers.h"

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
        
        void IClassType::writeBinary(TypeSerializationContext& typeContext, stream::OpcodeWriter& file, const void* data, const void* defaultData) const
        {
            TypeSerializationContextSetClass classContext(typeContext, this);

            // get the default context for saving if not specified
            if (!defaultData)
                defaultData = defaultObject();

            // enter compound block
            file.beginCompound(this);

            // save properties
            auto& allProps = allProperties();
            for (auto prop  : allProps)
            {
                // skip transient properties
                if (prop->flags().test(PropertyFlagBit::Transient))
                    continue;

                // ask object if we want to save this property
                auto propData = prop->offsetPtr(data);
                auto propDefaultData = defaultData ? prop->offsetPtr(defaultData) : nullptr;
                if (typeContext.directObjectContext)
                {
                    // object may request property NOT to be saved for "reasons" or may tell us to save property even if the data is different, also for "reasons"
                    if (!typeContext.directObjectContext->onPropertyShouldSave(prop))
                        continue;
                }
                else
                {
                    // compare the property value with default, do not save if the same
                    if (propDefaultData)
                        if (prop->type()->compare(propData, propDefaultData))
                            continue;
                }

                // write the reference to the property (it'll be mapped to an index as it's much faster to load it like that compared to ie. finding it by name every time)
                file.writeProperty(prop);

                // write the type of the data we are saving
                file.writeType(prop->type());

                // save property data in a skip able block so we can go over it if we can't read it
                {
                    TypeSerializationContextSetProperty propertyContext(typeContext, prop);

                    file.beginSkipBlock();
                    prop->type()->writeBinary(typeContext, file, propData, propDefaultData);
                    file.endSkipBlock();
                }
            }

            // leave compound block
            file.endCompound();
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

        void IClassType::readBinary(TypeSerializationContext& typeContext, stream::OpcodeReader& file, void* data) const
        {
            TypeSerializationContextSetClass classContext(typeContext, this);

            // enter compound block and read properties
            uint32_t propertyCount = 0;
            file.enterCompound(propertyCount);

            for (uint32_t i = 0; i < propertyCount; ++i)
            {
                // read the property reference
                StringID propertyName;
                const auto* prop = file.readProperty(propertyName);

                // ask if we want to read this property
                if (prop && typeContext.directObjectContext && !typeContext.directObjectContext->onPropertyShouldLoad(prop))
                {
                    TRACE_WARNING("Discarded property at {}: property '{}' was discarded by object", typeContext, propertyName);
                    file.discardSkipBlock();
                    continue;
                }

                // read the type of data
                StringID typeName;
                if (const auto type = file.readType(typeName))
                {
                    // we will attempt to read the data
                    file.enterSkipBlock();

                    // missing property
                    if (!prop)
                    {
                        // load to a temporary data holder
                        DataHolder tempData(type);
                        type->readBinary(typeContext, file, tempData.data());

                        // allow some extend of recovery
                        handlePropertyMissing(typeContext, propertyName, type, tempData.data());
                    }

                    // try to read directly
                    else if (type == prop->type() || helper::AreTypesBinaryComaptible(type, prop->type()))
                    {
                        TypeSerializationContextSetProperty propertyContext(typeContext, prop);

                        void* targetData = prop->offsetPtr(data);
                        prop->type()->readBinary(typeContext, file, targetData);
                    }

                    // we have property by the type is not compatible
                    else
                    {
                        TypeSerializationContextSetProperty propertyContext(typeContext, prop);

                        // load to a temporary data holder
                        DataHolder tempData(type);
                        type->readBinary(typeContext, file, tempData.data());

                        // try automatic conversion
                        void* targetData = prop->offsetPtr(data);
                        if (!ConvertData(tempData.data(), tempData.type(), targetData, prop->type()))
                        {
                            // we failed to convert the type automatically but still allow for the recovery
                            handlePropertyTypeChange(typeContext, propertyName, type, tempData.data(), prop->type(), targetData);
                        }
                    }

                    // exit the skip block
                    file.leaveSkipBlock();
                }
                else
                {
                    // we don't be able to read data - the type that was used to serialize it is GONE
                    TRACE_WARNING("Lost property at {}: property '{}' was saved with missing type '{}'", typeContext, propertyName, typeName);
                    file.discardSkipBlock();
                }
            }

            // leave class
            file.leaveCompound();
        }

        //--

        void IClassType::writeXML(TypeSerializationContext& typeContext, xml::Node& node, const void* data, const void* defaultData) const
        {
            TypeSerializationContextSetClass classContext(typeContext, this);

            // get the default context for saving if not specified
            if (!defaultData)
                defaultData = defaultObject();

            // save properties
            auto& allProps = allProperties();
            for (auto prop : allProps)
            {
                // skip transient properties
                if (prop->flags().test(PropertyFlagBit::Transient))
                    continue;

                // ask object if we want to save this property
                auto propData = prop->offsetPtr(data);
                auto propDefaultData = defaultData ? prop->offsetPtr(defaultData) : nullptr;
                if (typeContext.directObjectContext)
                {
                    // object may request property NOT to be saved for "reasons" or may tell us to save property even if the data is different, also for "reasons"
                    if (!typeContext.directObjectContext->onPropertyShouldSave(prop))
                        continue;
                }
                else
                {
                    // compare the property value with default, do not save if the same
                    if (propDefaultData)
                        if (prop->type()->compare(propData, propDefaultData))
                            continue;
                }

                // properties are saved in child nodes
                if (auto childNode = node.writeChild(prop->name().view()))
                {
                    TypeSerializationContextSetProperty propertyContext(typeContext, prop);
                    prop->type()->writeXML(typeContext, childNode, propData, propDefaultData);
                }
            }
        }

        void IClassType::readXML(TypeSerializationContext& typeContext, const xml::Node& node, void* data) const
        {
            TypeSerializationContextSetClass classContext(typeContext, this);

            for (xml::NodeIterator it(node, ""); it; ++it)
            {
                // get property name - it will be the node name
                auto propertyName = it->name();

                // read the property reference
                const auto* prop = findProperty(StringID::Find(propertyName));
                if (!prop)
                {
                    TRACE_WARNING("Missing property at {}: property '{}' was discarded by object", typeContext, propertyName);
                    continue;
                }

                // ask if we want to read this property
                if (prop && typeContext.directObjectContext && !typeContext.directObjectContext->onPropertyShouldLoad(prop))
                {
                    TRACE_WARNING("Discarded property at {}: property '{}' was discarded by object", typeContext, propertyName);
                    continue;
                }

                // read the data
                {
                    TypeSerializationContextSetProperty propertyContext(typeContext, prop);

                    void* targetData = prop->offsetPtr(data);
                    prop->type()->readXML(typeContext, *it, targetData);
                }
            }
        }

        //--

        DataViewResult IClassType::describeDataView(StringView<char> viewPath, const void* viewData, DataViewInfo& outInfo) const
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
                        if (outInfo.categoryFilter && prop->category() != outInfo.categoryFilter)
                            continue;

                        if (prop->editable())
                        {
                            auto& memberInfo = outInfo.members.emplaceBack();
                            memberInfo.name = prop->name();
                            memberInfo.category = prop->category();
                            memberInfo.type = prop->type();
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
                else
                {
                    return DataViewResultCode::ErrorUnknownProperty;
                }
            }

            return DataViewResultCode::ErrorIllegalAccess;
        }

        DataViewResult IClassType::readDataView(StringView<char> viewPath, const void* viewData, void* targetData, Type targetType) const
        {
            const auto orgDataView = viewPath;

            if (viewPath.empty())
                return IType::readDataView(viewPath, viewData, targetData, targetType);

            StringView<char> propertyName; 
            if (ParsePropertyName(viewPath, propertyName))
            {
                if (auto prop  = findProperty(StringID::Find(propertyName)))
                {
                    auto propViewData  = prop->offsetPtr(viewData);
                    return prop->type()->readDataView(viewPath, propViewData, targetData, targetType);
                }
            }

            return IType::readDataView(orgDataView, viewData, targetData, targetType);
        }

        DataViewResult IClassType::writeDataView(StringView<char> viewPath, void* viewData, const void* sourceData, Type sourceType) const
        {
            const auto orgDataView = viewPath;

            if (viewPath.empty())
                return IType::writeDataView(viewPath, viewData, sourceData, sourceType);

            StringView<char> propertyName;
            if (ParsePropertyName(viewPath, propertyName))
            {
                if (auto prop  = findProperty(StringID::Find(propertyName)))
                {
                    auto propViewData  = prop->offsetPtr(viewData);
                    return prop->type()->writeDataView(viewPath, propViewData, sourceData, sourceType);
                }
            }

            return IType::writeDataView(orgDataView, viewData, sourceData, sourceType);
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

        bool IClassType::handlePropertyMissing(TypeSerializationContext& context, StringID name, Type dataType, const void* data) const
        {
            if (context.directObjectContext)
                return context.directObjectContext->onPropertyMissing(name, dataType, data);

            return false;
        }

        bool IClassType::handlePropertyTypeChange(TypeSerializationContext& context, StringID name, Type originalDataType, const void* originalData, Type currentType, void* currentData) const
        {
            if (context.directObjectContext)
                return context.directObjectContext->onPropertyTypeChanged(name, originalDataType, originalData, currentType, currentData);

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

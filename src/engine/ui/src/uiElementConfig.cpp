/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements #]
***/

#include "build.h"
#include "uiElementConfig.h"
#include "core/config/include/storage.h"
#include "core/config/include/group.h"
#include "core/config/include/entry.h"
#include "core/app/include/configProperty.h"
#include "core/xml/include/xmlUtils.h"
#include "core/xml/include/xmlDocument.h"
#include "core/xml/include/xmlWrappers.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//---

static bool IsValidChar(char ch)
{
    if (ch >= 'a' && ch <= 'z') return true;
    if (ch >= 'A' && ch <= 'Z') return true;
    if (ch >= '0' && ch <= '9') return true;

    if (ch < ' ') return false;

    switch (ch)
    {
    case '<':
    case '>':
    case ':':
    case '\"':
    case '\\':
    case '|':
    case '?':
    case '*':
        return false;
    }
    
    return true;
}

//---

IConfigDataInterface::~IConfigDataInterface()
{}

//---

ConfigBlock::ConfigBlock(IConfigDataInterface* data, StringView rootTag)
    : m_data(data)
    , m_path(rootTag)
{}

ConfigBlock::~ConfigBlock()
{}


ConfigBlock ConfigBlock::tag(StringView tag) const
{
    StringBuilder txt;

    bool addSeparator = false;
        
    if (m_path)
    {
        txt << m_path;
        addSeparator = true;
    }

    for (auto ch : tag)
    {
        if (ch == '\\')
            ch = '/';

        if (ch == '/' && txt.empty())
            continue;

        if (IsValidChar(ch))
        {
            if (addSeparator)
            {
                addSeparator = false;
                txt << "/";
            }

            txt.appendch(ch);
        }
    }

    return ConfigBlock(m_data, txt.view());
}

//---

ConfigFileStorageDataInterface::ConfigFileStorageDataInterface()
{
    m_root = new Group();
}

ConfigFileStorageDataInterface::~ConfigFileStorageDataInterface()
{
    delete m_root;
}

bool ConfigFileStorageDataInterface::loadFromFile(StringView path)
{
    auto xml = xml::LoadDocument(xml::ILoadingReporter::GetDefault(), path);
    if (!xml)
        return false;

    auto root = new Group();
    root->load(*xml, xml->root());

    delete m_root;
    m_root = root;

    return true;
}

bool ConfigFileStorageDataInterface::saveToFile(StringView path) const
{
    auto xml = xml::CreateDocument("config");
    m_root->save(*xml, xml->root());
    return xml::SaveDocument(*xml, path);
}

//--

ConfigFileStorageDataInterface::Value::Value()
{}

ConfigFileStorageDataInterface::Value::~Value()
{}

bool ConfigFileStorageDataInterface::Value::read(Type type, void* data) const
{
    return value.get(data, type);
}

bool ConfigFileStorageDataInterface::Value::write(Type type, const void* data)
{
    DEBUG_CHECK_RETURN_EX_V(type, "Invalid type", false);
    DEBUG_CHECK_RETURN_EX_V(data, "Invalid data", false);

    if (value.type() == type)
        if (value.set(data, type))
            return true;

    value = std::move(Variant(type, data));
    return true;
}

void ConfigFileStorageDataInterface::Value::save(xml::IDocument& doc, xml::NodeID nodeId) const
{
    doc.nodeAttribute(nodeId, "type", value.type()->name().view());

    rtti::TypeSerializationContext context;
    xml::Node xmlNode(&doc, nodeId);
    value.type()->writeXML(context, xmlNode, value.data(), nullptr);
}

bool ConfigFileStorageDataInterface::Value::load(const xml::IDocument& doc, xml::NodeID nodeId)
{
    auto entryTypeName = doc.nodeAttributeOfDefault(nodeId, "type", "");
    if (entryTypeName.empty())
        return false;

    auto entryType = RTTI::GetInstance().findType(StringID::Find(entryTypeName));
    if (!entryType)
    {
        TRACE_INFO("Unknown type '{}' used in config", entryTypeName);
        return false;
    }

    auto value = Variant(entryType);
    if (value.empty())
        return false;

    rtti::TypeSerializationContext context;
    xml::Node xmlNode(&doc, nodeId);
    value.type()->readXML(context, xmlNode, value.data());

    //--

    this->value = std::move(value);

    return true;
}

//--

ConfigFileStorageDataInterface::Group::Group()
{}

ConfigFileStorageDataInterface::Group::~Group()
{
    children.clearPtr();
    values.clearPtr();
}

ConfigFileStorageDataInterface::Value* ConfigFileStorageDataInterface::Group::findOrCreate(StringView key)
{
    DEBUG_CHECK_RETURN_EX_V(key, "Empty value key", nullptr);

    for (auto* val : values)
        if (val->key == key)
            return val;

    auto* val = new Value();
    val->key = StringBuf(key);
    values.pushBack(val);
    return val;
}

const ConfigFileStorageDataInterface::Value* ConfigFileStorageDataInterface::Group::findExisting(StringView key) const
{
    for (const auto* val : values)
        if (val->key == key)
            return val;

    return nullptr;
}

void ConfigFileStorageDataInterface::Group::save(xml::IDocument& doc, xml::NodeID nodeId) const
{
    for (const auto* child : values)
    {
        if (!child->value.empty())
        {
            auto childNodeId = doc.createNode(nodeId, "value");
            doc.nodeAttribute(childNodeId, "key", child->key);
            child->save(doc, childNodeId);
        }
    }

    for (const auto* child : children)
    {
        if (!child->values.empty() || !child->children.empty())
        {
            auto childNodeId = doc.createNode(nodeId, "group");
            doc.nodeAttribute(childNodeId, "name", child->name);
            child->save(doc, childNodeId);
        }
    }
}

bool ConfigFileStorageDataInterface::Group::load(const xml::IDocument& doc, xml::NodeID nodeId)
{
    for (auto childID = doc.nodeFirstChild(nodeId); childID; childID = doc.nodeSibling(childID))
    {
        auto nodeType = doc.nodeName(childID);
        if (nodeType == "group")
        {
            auto childGroupName = doc.nodeAttributeOfDefault(childID, "name", "");
            if (!childGroupName.empty())
            {
                auto* childGroup = new Group();
                childGroup->name = StringBuf(childGroupName);

                if (childGroup->load(doc, childID))
                    this->children.pushBack(childGroup);
                else
                    delete childGroup;
            }
        }
        else if (nodeType == "value")
        {
            auto valueKey = doc.nodeAttributeOfDefault(childID, "key", "");
            if (!valueKey.empty())
            {
                auto* childValue = new Value();
                childValue->key = StringBuf(valueKey);

                if (childValue->load(doc, childID))
                    this->values.pushBack(childValue);
                else
                    delete childValue;
            }
        }
    }

    return !values.empty() || !children.empty();
}

//--

ConfigFileStorageDataInterface::Group* ConfigFileStorageDataInterface::findOrCreateEntry(StringView name)
{
    InplaceArray<StringView, 10> parts;
    name.slice("/", false, parts);

    auto* cur = m_root;
    for (auto name : parts)
    {
        Group* found = nullptr;
        for (auto* childEntry : cur->children)
        {
            if (childEntry->name == name)
            {
                found = childEntry;
                break;
            }
        }

        if (!found)
        {
            found = new Group();
            found->name = StringBuf(name);
            cur->children.pushBack(found);
        }

        cur = found;
    }

    return cur;
}

const ConfigFileStorageDataInterface::Group* ConfigFileStorageDataInterface::findExistingEntry(StringView name) const
{
    InplaceArray<StringView, 10> parts;
    name.slice("/", false, parts);

    auto* cur = m_root;
    for (auto name : parts)
    {
        Group* found = nullptr;
        for (auto* childEntry : cur->children)
        {
            if (childEntry->name == name)
            {
                found = childEntry;
                break;
            }
        }

        if (!found)
            return nullptr;

        cur = found;
    }

    return cur;
}

//--

void ConfigFileStorageDataInterface::writeRaw(StringView path, StringView key, Type type, const void* data)
{
    DEBUG_CHECK_RETURN(type);
    DEBUG_CHECK_RETURN(data);
    DEBUG_CHECK_RETURN(path);
    DEBUG_CHECK_RETURN(key);

    if (auto* group = findOrCreateEntry(path))
        if (auto* value = group->findOrCreate(key))
            value->write(type, data);
}

bool ConfigFileStorageDataInterface::readRaw(StringView path, StringView key, Type type, void* data) const
{
    DEBUG_CHECK_RETURN_V(type, false);
    DEBUG_CHECK_RETURN_V(data, false);
    DEBUG_CHECK_RETURN_V(path, false);
    DEBUG_CHECK_RETURN_V(key, false);

    if (const auto* group = findExistingEntry(path))
        if (const auto* value = group->findExisting(key))
            return value->read(type, data);

    return false;
}

//---

END_BOOMER_NAMESPACE_EX(ui)

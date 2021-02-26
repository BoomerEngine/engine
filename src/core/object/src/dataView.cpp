/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: view #]
***/

#include "build.h"
#include "dataView.h"
#include "core/object/include/rttiDataView.h"
#include "rttiClassType.h"

BEGIN_BOOMER_NAMESPACE()

//----

const char* CodeName(DataViewResultCode code)
{
    switch (code)
    {
        case DataViewResultCode::OK: return "OK";
        case DataViewResultCode::ErrorUnknownProperty: return "ErrorUnknownProperty";
        case DataViewResultCode::ErrorIllegalAccess: return "ErrorIllegalAccess";
        case DataViewResultCode::ErrorIllegalOperation: return "ErrorIllegalOperation";
        case DataViewResultCode::ErrorTypeConversion: return "ErrorTypeConversion";
        case DataViewResultCode::ErrorManyValues: return "ErrorManyValues";
        case DataViewResultCode::ErrorReadOnly: return "ErrorReadOnly";
        case DataViewResultCode::ErrorNullObject: return "ErrorNullObject";
        case DataViewResultCode::ErrorIndexOutOfRange: return "ErrorIndexOutOfRange";
        case DataViewResultCode::ErrorInvalidValid: return "ErrorInvalidValid";
    }

    return "Unknown";
}

void DataViewActionResult::print(IFormatStream& f) const
{
    f.append(CodeName(result.code));
}

void DataViewResult::print(IFormatStream& f) const
{
    f.append(CodeName(code));
}

void DataViewErrorResult::print(IFormatStream& f) const
{
    f.append(CodeName(result.code));
}

//----

IDataViewObserver::~IDataViewObserver()
{}

//----

IDataView::IDataView()
{
    const auto rootPathHash = StringView("").calcCRC64();
    m_rootPath = new Path;
    m_rootPath->parent = nullptr;
    m_paths[rootPathHash] = m_rootPath;
}

IDataView::~IDataView()
{
    m_paths.clearPtr();
    m_rootPath = nullptr;
}

void IDataView::attachObserver(StringView path, IDataViewObserver* observer)
{
    if (auto* pathEntry = createPathEntry(path))
    {
        if (0 == m_callbackDepth)
            pathEntry->observers.removeUnorderedAll(nullptr);
        pathEntry->observers.pushBackUnique(observer);
    }
}

void IDataView::detachObserver(StringView path, IDataViewObserver* observer)
{
    if (auto* pathEntry = findPathEntry(path))
    {
        const auto index = pathEntry->observers.find(observer);
        if (index != -1)
        {
            pathEntry->observers[index] = nullptr;

            if (0 == m_callbackDepth)
                pathEntry->observers.removeUnorderedAll(nullptr);
        }
    }
}

//--

void IDataView::dispatchPropertyChanged(StringView eventPath)
{
    ++m_callbackDepth;

    bool parentEvent = false;
    StringView childPath;
    do
    {
        if (auto* ret = findPathEntry(eventPath))
        {
            for (uint32_t i=0; i<ret->observers.size(); ++i)
                if (ret->observers[i])
                    ret->observers[i]->handlePropertyChanged(eventPath, parentEvent); // NOTE: may remove entries
        }

        parentEvent = true;
    }
    while (rtti::ExtractParentPath(eventPath, childPath));

    if (0 == --m_callbackDepth)
    {
        // TODO: cleanup observers
    }
}

void IDataView::dispatchFullStructureChanged()
{
    ++m_callbackDepth;

    if (m_rootPath)
        for (uint32_t i = 0; i < m_rootPath->observers.size(); ++i)
            if (m_rootPath->observers[i])
                m_rootPath->observers[i]->handleFullObjectChange(); // NOTE: may remove entries

    if (0 == --m_callbackDepth)
    {
        // TODO: cleanup observers
    }
}

//----

IDataView::Path* IDataView::createPathEntryInternal(Path* parent, StringView fullPath)
{
    const auto pathCode = fullPath.calcCRC64();

    Path* ret = findPathEntry(fullPath);
    if (!m_paths.find(pathCode, ret))
    {
        ret = new Path;
        ret->parent = parent;
        m_paths[pathCode] = ret;
    }
    else
    {
        DEBUG_CHECK(ret->parent == parent);
    }

    return ret;
}

IDataView::Path* IDataView::findPathEntry(StringView path)
{
    const auto pathCode = path.calcCRC64();

    Path* ret = nullptr;
    m_paths.find(pathCode, ret);
    return ret;
}

IDataView::Path* IDataView::createPathEntry(StringView path)
{
    // path will most likely exists
    if (auto* ret = findPathEntry(path))
        return ret;

    // dissect path, start with root
    auto* curPath = m_rootPath;
    auto curPathString = path;
    while (!curPathString.empty())
    {
        uint32_t arrayIndex = 0;
        StringView propName;
        if (rtti::ParsePropertyName(curPathString, propName))
        {
            auto fullPathString = StringView(path.data(), curPathString.data());
            curPath = createPathEntryInternal(curPath, fullPathString);
        }
        else if (rtti::ParseArrayIndex(curPathString, arrayIndex))
        {
            auto fullPathString = StringView(path.data(), curPathString.data());
            curPath = createPathEntryInternal(curPath, fullPathString);
        }
        else
        {
            TRACE_WARNING("Invalid data view path: '{}'", path);
            return nullptr;
        }
    }

    // return the current path entry after dissection - this is the place we have arrived
    return curPath;
}

///--

END_BOOMER_NAMESPACE()

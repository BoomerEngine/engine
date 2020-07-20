/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#include "build.h"
#include "resource.h"
#include "resourcePath.h"
#include "resourcePathCache.h"

#include "base/memory/include/linearAllocator.h"
#include "base/containers/include/inplaceArray.h"

namespace base
{
    namespace res
    {
        //---

        namespace prv
        {

            mem::PoolID POOL_RESOURCE_PATHS("Engine.Paths");

            /// registry for resource paths
            class ResourcePathDataRegistry : public ISingleton
            {
                DECLARE_SINGLETON(ResourcePathDataRegistry);

            public:
                ResourcePathDataRegistry()
                    : m_mem(POOL_RESOURCE_PATHS)
                {}

                static bool IsValidPathChar(char ch)
                {
                    //if (ch == '%' || ch == '$' || ch == '?' || ch == '&' || ch == ':' || ch == '\\') return false;
                    if (ch == '%' || ch == '?' || ch == '&' || ch == '\\' || ch == '$' || ch == '%') return false;
                    if (ch >= 0 && ch < 32) return false;
                    return true;
                }

                static bool IsValidKeyChar(char ch)
                {
                    if (ch <= 32) return false;
                    if (ch >= 'a' && ch <= 'z') return true;
                    if (ch >= '0' && ch <= '9') return true;
                    if (ch == '_') return true;
                    return true;
                }

                static bool IsValidValueChar(char ch)
                {
                    if (ch < 32) return false;
                    return true;
                }

                struct SantizedKey
                {
                    StringView<char> key;
                    StringView<char> value;

                    INLINE bool operator==(const SantizedKey& other) const { return key == other.key; }
                    INLINE bool operator<(const SantizedKey& other) const { return key < other.key; }
                };

                static bool ParseKeyPairs(StringView<char> params, Array<SantizedKey>& outKeys)
                {
                    auto readPtr  = params.data();
                    auto endPtr  = params.data() + params.length();

                    while (readPtr < endPtr)
                    {
                        auto keyStart = readPtr;
                        while (readPtr < endPtr && *readPtr != '=')
                        {
                            if (!IsValidKeyChar(*readPtr))
                            {
                                TRACE_ERROR("Path contains invalid character {} in key name", *readPtr);
                                return false;
                            }

                            ++readPtr;
                        }

                        if (readPtr == endPtr)
                        {
                            TRACE_ERROR("Path contains key without a value");
                            return false;
                        }
                        else if (keyStart == readPtr)
                        {
                            TRACE_ERROR("Path contains empty key name");
                            return false;
                        }

                        auto key = StringView<char>(keyStart, readPtr);
                        readPtr += 1;

                        auto valueStart = readPtr;
                        while (readPtr < endPtr && *readPtr != ':')
                        {
                            if (!IsValidValueChar(*readPtr))
                            {
                                TRACE_ERROR("Path contains invalid character {} in value for key '{}'", *readPtr, key);
                                return false;
                            }

                            ++readPtr;
                        }

                        auto value = StringView<char>(valueStart, readPtr);
                        if (readPtr < endPtr) ++readPtr; // skip the ':'

                        auto& entry = outKeys.emplaceBack();
                        entry.key = key;
                        entry.value = value;
                    }

                    return true;
                }

                static bool ParseKeyPairs(StringView<char> view, Array<SantizedKey>& outKeys, StringView<char>& normalPath)
                {
                    auto params = view.afterFirst(":");
                    if (params)
                    {
                        normalPath = view.beforeFirst(":");
                        return ParseKeyPairs(params, outKeys);
                    }
                    else
                    {
                        normalPath = view;
                        return true;
                    }
                }

                const ResourcePathData* alloc(StringView<char> data)
                {
                    DEBUG_CHECK_EX(data.length() <= ResourcePathData::MAX_PATH_LENGTH, "Resource path is to long");

                    if (data.length() > ResourcePathData::MAX_PATH_LENGTH)
                        return nullptr; // yes, do ignore such paths

                    // try to get a hit via the original hash
                    auto originalHash = data.calcCRC64();
                    {
                        ResourcePathData* ret = nullptr;
                        auto lock = CreateLock(m_remapMapLock);
                        if (m_remapMap.find(originalHash, ret))
                            return ret;
                    }

                    // sanitize path
                    char buffer[ResourcePathData::MAX_PATH_LENGTH + 1];
                    memcpy(buffer, data.data(), data.length());
                    buffer[data.length()] = 0;

                    // change all "\" to "/"
                    {
                        auto readPtr  = buffer;
                        auto endReadPtr  = buffer + data.length();
                        while (readPtr < endReadPtr)
                        {
                            if (*readPtr == ResourcePathData::WRONG_PATH_SEPARATOR)
                                *readPtr = ResourcePathData::PATH_SEPARATOR;

                            if (!IsValidPathChar(*readPtr))
                            {
                                TRACE_INFO("Path contains invalid char code {}", *readPtr);
                                return nullptr;
                            }

                            // make path lower case
                            if (*readPtr >= 'A' && *readPtr <= 'Z')
                                *readPtr = 'a' + (*readPtr - 'A');

                            ++readPtr;
                        }
                    }

                    // calculate confirmed hash
                    const auto conformedHash = base::StringView<char>(buffer).calcCRC64();

                    // look for existing definition
                    {
                        ResourcePathData* ret = nullptr;
                        auto lock = CreateLock(m_dataMapLock);
                        if (m_dataMap.find(conformedHash, ret))
                        {
                            auto lock2 = CreateLock(m_remapMapLock);
                            m_remapMap[originalHash] = ret;
                            return ret;
                        }
                    }

                    // create a new definition
                    auto memorySizeNeeded = sizeof(ResourcePathData) + data.length() + 1;
                    auto ret  = (ResourcePathData*)m_mem.alloc(memorySizeNeeded, 1);

                    // write string 
                    auto strPtr = (char*)ret + sizeof(ResourcePathData);
                    memcpy(strPtr, buffer, data.length() + 1);

                    // slice the entries
                    ret->path = StringView<char>(strPtr, strPtr + data.length());
                    ret->dirPart = ret->path.beforeLast("/");
                    if (!ret->dirPart.empty()) ret->dirPart = StringView<char>(ret->dirPart.data(), ret->dirPart.length() + 1);
                    ret->fileNamePart = ret->path.afterLastOrFull("/").beforeFirstOrFull(":");
                    ret->fileStemPart = ret->fileNamePart.beforeLastOrFull(".");
                    ret->extensionPart = ret->fileNamePart.afterLast(".");
                    //ret->paramsPart = ret->full.afterFirst(":");

                    // store in cache
                    {
                        auto lock = CreateLock(m_dataMapLock);
                        if (m_dataMap.find(conformedHash, ret))
                        {
                            auto lock2 = CreateLock(m_remapMapLock);
                            m_remapMap[originalHash] = ret;
                            return ret;
                        }

                        m_dataMap[conformedHash] = ret;

                        {
                            auto lock2 = CreateLock(m_remapMapLock);
                            m_remapMap[originalHash] = ret;
                        }
                    }

                    return ret;
                }

            private:
                typedef HashMap<uint64_t, ResourcePathData*> TPathMap;

                TPathMap m_remapMap;
                SpinLock m_remapMapLock;

                TPathMap m_dataMap;
                SpinLock m_dataMapLock;


                mem::LinearAllocator m_mem;

                virtual void deinit() override
                {
                    m_remapMap.clear();
                    m_dataMap.clear();
                    m_mem.clear();
                }
            };

        } // prv

        const ResourcePathData* ResourcePathData::Build(const StringView<char> fullPathString)
        {
            if (fullPathString.empty())
                return nullptr;

            return prv::ResourcePathDataRegistry::GetInstance().alloc(fullPathString);
        }

        //---

        ResourcePath::ResourcePath(StringView<char> path)
        {
            m_data = ResourcePathData::Build(path);
            DEBUG_CHECK_EX(!path || m_data, "Invalid path passed");
        }

        ResourcePath& ResourcePath::operator=(StringView<char> path)
        {
            m_data = ResourcePathData::Build(path);
            DEBUG_CHECK_EX(!path || m_data, "Invalid path passed");
            return *this;
        }

        const ResourcePath& ResourcePath::EMPTY()
        {
            static ResourcePath theEmptyPath;
            return theEmptyPath;
        }

        void ResourcePath::print(base::IFormatStream& f) const
        {
            if (m_data)
                f << m_data->path;
        }

        //---

        RTTI_BEGIN_CUSTOM_TYPE(ResourceKey);
        RTTI_END_TYPE();

        const ResourceKey& ResourceKey::EMPTY()
        {
            static ResourceKey theEmptyKey;
            return theEmptyKey;
        }

        void ResourceKey::print(IFormatStream& f) const
        {
            if (!empty())
            {
                if (m_class->shortName())
                    f << m_class->shortName();
                else
                    f << m_class->name();

                f << "$";
                f << m_path;
            }
            else
            {
                f << "null";
            }
        }

        GlobalEventKey ResourceKey::buildEventKey() const
        {
            StringBuilder tempString;
            print(tempString);
            return MakeSharedEventKey(tempString.view());
        }

        void ResourceKey::printForFileName(IFormatStream& f) const
        {
            if (!empty())
            {
                if (m_path.directory())
                {
                    DEBUG_CHECK(m_path.directory().endsWith("/"));
                    f << m_path.directory();
                }

                f << m_path.fileStem();
                
                f << ".";

                if (m_class->shortName())
                    f << m_class->shortName();
                else
                    f << m_class->name().view().afterLastOrFull("::");

                f << ".";
                f << m_path.extension();
            }
        }

        bool ResourceKey::Parse(StringView<char> path, ResourceKey& outKey)
        {
            if (path.empty() || path == "null")
            {
                outKey = ResourceKey();
                return true;
            }

            auto index = path.findFirstChar('$');
            if (index == INDEX_NONE)
                return false;

            auto className = path.leftPart(index);
            auto classNameStringID = StringID::Find(className);
            if (classNameStringID.empty())
                return false; // class "StringID" is not known - ie class was never registered

            auto resourceClass  = RTTI::GetInstance().findClass(classNameStringID).cast<IResource>();
            if (!resourceClass)
                return false;

            auto pathPart = path.subString(index + 1);
            auto returnPath = ResourcePath(pathPart);
            if (returnPath.empty())
                return false;

            outKey.m_class = resourceClass;
            outKey.m_path = returnPath;
            return true;
        }

        //---

        ResourcePathBuilder::ResourcePathBuilder()
        {}

        StringBuf ResourcePathBuilder::filePath() const
        {
            StringBuilder txt;
            txt << directory;
            txt << file;

            if (!extension.empty())
                txt << "." << extension;

            return txt.toString();
        }

        void ResourcePathBuilder::filePath(StringView<char> txt)
        {
            directory = StringBuf(txt.beforeLast("/"));
            if (!directory.empty() && !directory.endsWith("/"))
                directory = TempString("{}/", directory);

            file = StringBuf(txt.afterLastOrFull("/").beforeLastOrFull("."));
            extension = StringBuf(txt.afterLast("."));
        }

        ResourcePathBuilder::ResourcePathBuilder(ResourcePath path)
        {
            if (!path.empty())
            {
                directory = StringBuf(path.directory());
                file = StringBuf(path.fileStem());
                extension = StringBuf(path.extension());
            }
        }

        void ResourcePathBuilder::print(IFormatStream& f) const
        {
            f << directory;
            f << file;

            if (extension)
                f << "." << extension;
        }

        ResourcePath ResourcePathBuilder::toPath() const
        {
            TempString f;
            print(f);
            return ResourcePath(f.c_str());
        }

        //---

        bool ApplyRelativePath(StringView<char> contextPath, StringView<char> relativePath, StringBuf& outPath)
        {
            // nothing to resolve
            if (relativePath.empty())
                return false;

            // split path into parts
            InplaceArray<StringView<char>, 20> referencePathParts;
            contextPath.slice("/\\", false, referencePathParts);

            // remove the last path that's usually the file name
            if (!referencePathParts.empty() && !contextPath.endsWith("/"))
                referencePathParts.popBack();

            // split control path
            InplaceArray<StringView<char>, 20> pathParts;
            relativePath.slice("/\\", false, pathParts);

            // append
            for (auto& part : pathParts)
            {
                // single path, nothing
                if (part == ".")
                    continue;

                // go up
                if (part == "..")
                {
                    if (referencePathParts.empty())
                        return false;
                    referencePathParts.popBack();
                    continue;
                }

                // append
                referencePathParts.pushBack(part);
            }

            // reassmeble into a full path
            StringBuilder reassemblePathBuilder;
            for (auto& part : referencePathParts)
            {
                if (!reassemblePathBuilder.empty())
                    reassemblePathBuilder << "/";
                reassemblePathBuilder << part;
            }

            // if the path to resolve was a directory then add the final separator
            auto relativePathIsDir = relativePath.endsWith("/");
            if (relativePathIsDir)
                reassemblePathBuilder << "/";

            // load content
            outPath = reassemblePathBuilder.toString();
            return true;
        }

        //--

    } // res
} // base
/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

#include "base/containers/include/hashMap.h"

namespace base
{
    namespace res
    {

        //--

        INLINE ResourceKey::ResourceKey(ResourceKey&& other)
            : m_class(other.m_class)
            , m_path(std::move(other.m_path))
        {
            other.m_class = nullptr;
        }

        INLINE ResourceKey::ResourceKey(StringView path, SpecificClassType<IResource> classType)
            : m_class(classType)
            , m_path(path)
        {
            ASSERT_EX(classType != nullptr && !path.empty(), "Invalid initialization");
            ASSERT_EX(path.empty() || ValidateDepotPath(path, DepotPathClass::AbsoluteFilePath), "Invalid file path used in initialization");
        }

        INLINE ResourceKey& ResourceKey::operator=(ResourceKey&& other)
        {
            if (this != &other)
            {
                m_class = other.m_class;
                m_path = std::move(other.m_path);
                other.m_class = nullptr;
            }
            return *this;
        }

        INLINE bool ResourceKey::operator==(const ResourceKey& other) const
        {
            return (m_path == other.m_path) && (m_class == other.m_class);
        }

        INLINE bool ResourceKey::operator!=(const ResourceKey& other) const
        {
            return !operator==(other);
        }

        INLINE bool ResourceKey::empty() const
        {
            return m_class == nullptr || m_path.empty();
        }

        INLINE bool ResourceKey::valid() const
        {
            return !empty();
        }

        INLINE ResourceKey::operator bool() const
        {
            return !empty();
        }

        ALWAYS_INLINE const StringBuf& ResourceKey::path() const
        {
            return m_path;
        }

        ALWAYS_INLINE SpecificClassType<IResource> ResourceKey::cls() const
        {
            return m_class;
        }

        INLINE StringView ResourceKey::view() const
        {
            return m_path.view();
        }

        INLINE uint32_t ResourceKey::CalcHash(const ResourceKey& key)
        {
            return StringBuf::CalcHash(key.m_path);
        }

        //--

        INLINE StringView ResourceKey::fileName() const
        {
            return m_path.view().afterLastOrFull("/");
        }

        INLINE StringView ResourceKey::fileStem() const
        {
            return fileName().beforeFirstOrFull(".");
        }

        INLINE StringView ResourceKey::extension() const
        {
            return fileName().afterFirst(".");
        }

        INLINE StringView ResourceKey::directories() const
        {
            return m_path.view().beforeLast("/");
        }

        //--

    } // res
} // base

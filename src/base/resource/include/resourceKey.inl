/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

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

        INLINE ResourceKey::ResourceKey(const ResourcePath& path, SpecificClassType<IResource> classType)
            : m_class(classType)
            , m_path(path)
        {
            ASSERT_EX(classType != nullptr && !path.empty(), "Invalid initialization");
            ASSERT_EX(path.empty() || ValidateDepotPath(path.view(), DepotPathClass::AbsoluteFilePath), "Invalid file path used in initialization");
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

        ALWAYS_INLINE const ResourcePath& ResourceKey::path() const
        {
            return m_path;
        }

        ALWAYS_INLINE SpecificClassType<IResource> ResourceKey::cls() const
        {
            return m_class;
        }

        INLINE uint32_t ResourceKey::CalcHash(const ResourceKey& key)
        {
            return ResourcePath::CalcHash(key.m_path);
        }

        //--

    } // res
} // base

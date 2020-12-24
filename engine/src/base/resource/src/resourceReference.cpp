/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\reference #]
***/

#include "build.h"

#include "resource.h"
#include "resourceReference.h"
#include "resourceLoader.h"

namespace base
{
    namespace res
    {

        //---

        BaseReference::BaseReference()
        {
        }

        BaseReference::BaseReference(std::nullptr_t)
        {}

        BaseReference::BaseReference(const BaseReference& other)
            : m_handle(other.m_handle)
            , m_key(other.m_key)
        {
        }

        BaseReference::BaseReference(const ResourceKey& key)
            : m_key(key)
        {}

        BaseReference::BaseReference(const ResourcePtr& ptr)
        {
            m_handle = ptr;

            if (ptr)
                m_key = ptr->key();
        }

        BaseReference::BaseReference(BaseReference&& other)
            : m_handle(std::move(other.m_handle))
            , m_key(std::move(other.m_key))
        {
            other.reset();
        }

        BaseReference::~BaseReference()
        {
        }

        BaseReference& BaseReference::operator=(const BaseReference& other)
        {
            if (this != &other)
            {
                m_handle = other.m_handle;
                m_key = other.m_key;
            }

            return *this;
        }

        BaseReference& BaseReference::operator=(BaseReference&& other)
        {
            if (this != &other)
            {
                m_handle = std::move(other.m_handle);
                m_key = std::move(other.m_key);
                other.reset();
            }

            return *this;
        }

        void BaseReference::reset()
        {
            m_handle.reset();
            m_key = ResourceKey();
        }

        void BaseReference::set(const ResourceHandle& object)
        {
            m_handle = object;

            if (object)
                m_key = object->key();
            else
                m_key = ResourceKey();
        }

        const ResourceHandle& BaseReference::EMPTY_HANDLE()
        {
            static ResourceHandle theEmptyHandle;
            return theEmptyHandle;
        }

        bool BaseReference::operator==(const BaseReference& other) const
        {
            if (key() != other.key())
                return false;

            return m_handle == other.m_handle;
        }

        bool BaseReference::operator!=(const BaseReference& other) const
        {
            return !operator==(other);
        }

        void BaseReference::print(IFormatStream& f) const
        {
            const auto k = key();
            if (k)
                f << k;
            else
                f << "null";
        }

        //--

    } // res
} // base

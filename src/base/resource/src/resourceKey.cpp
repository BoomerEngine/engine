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
#include "base/containers/include/utf8StringFunctions.h"

namespace base
{
    namespace res
    {
        //---

        RTTI_BEGIN_CUSTOM_TYPE(ResourceKey);
            RTTI_BIND_NATIVE_COPY(ResourceKey);
            RTTI_BIND_NATIVE_COMPARE(ResourceKey);
            RTTI_BIND_NATIVE_CTOR_DTOR(ResourceKey);
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

        bool ResourceKey::Parse(StringView path, ResourceKey& outKey)
        {
            if (path.empty() || path == "null")
            {
                outKey = ResourceKey();
                return true;
            }

            auto index = path.findFirstChar(':');
            if (index == INDEX_NONE)
                return false;

            auto className = path.leftPart(index);
            auto classNameStringID = StringID::Find(className);
            if (classNameStringID.empty())
                return false; // class "StringID" is not known - ie class was never registered

            auto resourceClass = RTTI::GetInstance().findClass(classNameStringID).cast<IResource>();
            if (!resourceClass)
                return false;

            auto pathPart = path.subString(index + 1);
            if (pathPart.empty())
                return false;

            outKey.m_class = resourceClass;
            outKey.m_path = ResourcePath(pathPart);
            return true;
        }

    } // res

    //---

} // base
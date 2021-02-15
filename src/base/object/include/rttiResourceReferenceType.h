/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types #]
***/

#pragma once

#include "rttiType.h"

namespace base
{
    namespace rtti
    {

        //---

        /// RTTI-side base interface for a resource reference type, main purpose of having it here it facilitate resource reloading
        class BASE_OBJECT_API IResourceReferenceType : public IType
        {
        public:
            IResourceReferenceType(StringID name);
            virtual ~IResourceReferenceType();

            //--

            /// get the class we are pointing to
            virtual ClassType referenceResourceClass() const = 0;

            /*/// read the currently referenced resource
            virtual void referenceReadResource(const void* data, RefPtr<res::IResource>& outRef) const = 0;

            /// write new resource reference
            virtual void referenceWriteResource(void* data, res::IResource* resource) const = 0;*/

            /// patch resource reference, returns true if indeed it was patched
            virtual bool referencePatchResource(void* data, res::IResource* currentResource, res::IResource* newResources) const = 0;
        };

        //---

    } // rtti
} // base
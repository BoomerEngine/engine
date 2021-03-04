/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\utils #]
***/

#pragma once

#include "core/containers/include/hashMap.h"

BEGIN_BOOMER_NAMESPACE_EX(res)

namespace prv
{
    /// helper class to map resource extension to a engine class
    class ResourceClassLookup : public ISingleton
    {
        DECLARE_SINGLETON(ResourceClassLookup);

    public:
        ResourceClassLookup();

        /// find an engine class that matches given class hash
        SpecificClassType<IResource> resolveResourceClassHash(ResourceClassHash classHash) const;

        /// find an engine class that matches given extension
        SpecificClassType<IResource> resolveResourceExtension(StringView extension) const;

        /// find an engine class that matches given short name (ie. Texture, Mesh) used to decode paths
        SpecificClassType<IResource> resolveResourceShortName(StringView shortName) const;

        //---

    private:
        void buildClassMap();

        typedef HashMap< ResourceClassHash, SpecificClassType<IResource> > TResourceClassesByHash;
        TResourceClassesByHash m_classesByHash; // only classes that have unique extension->ClassType mapping

        typedef HashMap< uint64_t, SpecificClassType<IResource> > TResourceClassesByExtension;
        TResourceClassesByExtension m_classesByExtension;

        typedef HashMap< uint64_t, SpecificClassType<IResource> > TResourceClassesByShortName;
        TResourceClassesByShortName m_classesByShortName;

        virtual void deinit() override;
    };

} // prv

END_BOOMER_NAMESPACE_EX(res)

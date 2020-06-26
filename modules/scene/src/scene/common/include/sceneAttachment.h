/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
*
***/

#pragma once

#include "base/script/include/scriptObject.h"

namespace scene
{
    //---

    // attachment between two hierarchy elements, attachment is always unidirectional and has a clear source and destination
    class SCENE_COMMON_API IAttachment : public base::script::ScriptedObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(IAttachment, base::script::ScriptedObject);
        RTTI_SHARED_FROM_THIS_TYPE(IAttachment);

    public:
        IAttachment();
        virtual ~IAttachment();

        // get source object
        INLINE const ElementWeakPtr& source() const { return m_source; }

        // get destination object
        INLINE const ElementWeakPtr& destination() const { return m_dest; }

        //--

        /// break components attached via this attachment, object may be reused afterwards
        void detach();

        /// attach two elements with this attachment, must be empty
        /// if components are not attachable for any reason (incompatibility, recursion, etc) a false is returned and object is unchanged
        /// if "true" is returned then attachment is created
        bool attach(const ElementPtr& source, const ElementPtr& dest);

        //--

        // check if two components can be attached via this attachment
        virtual bool canAttach(const ElementPtr& source, const ElementPtr& dest) const;

    protected:
        ElementWeakPtr m_source;
        ElementWeakPtr m_dest;
    };

    //---

} // scene

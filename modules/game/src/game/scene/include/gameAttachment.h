/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entity #]
*
***/

#pragma once

#include "base/script/include/scriptObject.h"

namespace game
{
    //---

    // attachment between two components, usually in the same entity, attachment is always unidirectional and has a clear source and destination
    class GAME_SCENE_API IAttachment : public base::script::ScriptedObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(IAttachment, base::script::ScriptedObject);

    public:
        IAttachment();
        virtual ~IAttachment();

        // get source object
        INLINE const ComponentWeakPtr& source() const { return m_source; }

        // get destination object
        INLINE const ComponentWeakPtr& destination() const { return m_dest; }

        //--

        /// break components attached via this attachment, object may be reused afterwards
        void detach();

        /// attach two elements with this attachment, must be empty
        /// if components are not attachable for any reason (incompatibility, recursion, etc) a false is returned and object is unchanged
        /// if "true" is returned then attachment is created
        bool attach(Component* source, Component* dest);

        //--

        // check if two components can be attached via this attachment
        virtual bool canAttach(const Component* source, const Component* dest) const;

    protected:
        ComponentWeakPtr m_source;
        ComponentWeakPtr m_dest;
    };

    //---

} // game

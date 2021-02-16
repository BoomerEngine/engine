/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types\class #]
***/

#pragma once

#include "base/containers/include/stringID.h"
#include "base/containers/include/stringBuf.h"
#include "rttiMetadata.h"

namespace base
{
    namespace res
    {
        class ResourceLoader;
    }

    namespace rtti
    {
        class IType;

        /// flags for properties
        enum class PropertyFlagBit : uint32_t
        {
            Editable = FLAG(0), // property is user editable
            ReadOnly = FLAG(1), // property cannot be modified
            Inlined = FLAG(2), // property allows inlined objects to be created
            Transient = FLAG(3), // property is never saved but can be edited
            Overridable = FLAG(4), // property can be overridden (when used in a class derived from IObjectTemplate)
            ScriptHidden = FLAG(10), // property is explicitly hidden from being exposed to scripts
            ScriptReadOnly = FLAG(11), // property is explicitly marked as not being writable by scripts
            Scripted = FLAG(12), // property was created from scripts
            ExternalBuffer = FLAG(13), // property data is in the external buffer
            NoResetToDefault = FLAG(14), // property can't be reset to default
        };

        typedef DirectFlags<PropertyFlagBit> PropertyFlags;

        /// property setup
        struct PropertySetup
        {
            uint32_t m_offset = 0;
            Type m_type;
            StringID m_name;
            StringID m_category;
            PropertyFlags m_flags;
        };

        /// property editor data
        struct BASE_OBJECT_API PropertyEditorData
        {
            StringID m_customEditor;
            StringBuf m_comment;
            StringBuf m_units;
            double m_rangeMin = -DBL_MAX;
            double m_rangeMax = DBL_MAX;

            uint8_t m_digits = 2;
            bool m_colorWithAlpha = true;
            bool m_noDefaultValue = false;
            bool m_widgetDrag = false;
            bool m_widgetSlider = false;
            bool m_widgetDragWrap = false;
            bool m_primaryResource = false;

            PropertyEditorData();
            PropertyEditorData& comment(StringView comment) { m_comment = StringBuf(comment); return* this; }
            PropertyEditorData& primaryResource() { m_primaryResource = true; return *this; }

            bool rangeEnabled() const;
        };

        /// a property in the class
        class BASE_OBJECT_API Property : public MetadataContainer
        {
        public:
            Property(const IType* parent, const PropertySetup& setup, const PropertyEditorData& editorData = PropertyEditorData());
            virtual ~Property();

            /// get the name of the property
            INLINE StringID name() const { return m_name; }

            /// get name of the category property is in
            INLINE StringID category() const { return m_category; }

            /// get the owner of this property
            INLINE Type parent() const { return m_parent; }

            /// get the type of the property
            INLINE Type type() const { return m_type; }

            /// get the offset of data
            INLINE uint32_t offset() const { return m_offset; }

            /// get unique property hash
            INLINE uint64_t hash() const { return m_hash; }

            /// get flags (raw view)
            INLINE PropertyFlags flags() const { return m_flags; }

            /// is this a editable ?
            INLINE bool editable() const { return m_flags.test(PropertyFlagBit::Editable); }

            /// is this a readonly ?
            INLINE bool readonly() const { return m_flags.test(PropertyFlagBit::ReadOnly); }

            /// is this a inlined ?
            INLINE bool inlined() const { return m_flags.test(PropertyFlagBit::Inlined); }

            /// is this property created by scripts ?
            INLINE bool scripted() const { return m_flags.test(PropertyFlagBit::Scripted); }

            /// is this property data hold in external object buffer (object scripted property)
            INLINE bool externalBuffer() const { return m_flags.test(PropertyFlagBit::ExternalBuffer); }

            // can this property be reset to default/base value in the editor ?
            INLINE bool resetable() const { return !m_flags.test(PropertyFlagBit::NoResetToDefault); }

            /// is this an overridable template property ?
            INLINE bool overridable() const { return m_flags.test(PropertyFlagBit::Overridable); }

            //--

            /// get editor data
            INLINE const PropertyEditorData& editorData() const { return m_editorData; }

            //--

            /*/// get property data given the pointer to the object
            virtual const void* offsetPtr(const void* data) const { return OffsetPtr(data, offset()); }

            /// get property data given the pointer to the object
            virtual void* offsetPtr(void* data) const { return OffsetPtr(data, offset()); }*/

            /// get property data given the pointer to the object
            ALWAYS_INLINE const void* offsetPtr(const void* data) const { return OffsetPtr(data, offset()); }

            /// get property data given the pointer to the object
            ALWAYS_INLINE void* offsetPtr(void* data) const { return OffsetPtr(data, offset()); }

        protected:
            uint32_t m_offset;
            PropertyFlags m_flags;

            StringID m_name;
            StringID m_category;
            Type m_type;
            Type m_parent;

            PropertyEditorData m_editorData;

            short m_overrideIndex = -1;

            uint64_t m_hash;
        };

    } // rtti
} // base

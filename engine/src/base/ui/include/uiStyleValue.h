/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: styles #]
***/

#pragma once

#include "base/font/include/font.h"
#include "base/image/include/image.h"

namespace ui
{
    namespace style
    {

        //---

        /// type of the raw value
        enum class RawValueType
        {
            Invalid,
            Numerical,
            Percentage,
            Color,
            String,
            Function,
            Variable,
        };

        /// raw value for style property
        /// used during parsing
        class BASE_UI_API RawValue
        {
        public:
            RawValue(float number, bool percentage=false);
            RawValue(base::Color color);
            RawValue(const char* txt);
            RawValue(const base::StringID variableName);
            RawValue(const base::StringID functionName, uint32_t numArguments, const RawValue** args);

            /// get type of the value
            INLINE RawValueType type() const { return m_type; }

            /// get the numerical value
            INLINE float number() const { return m_number; }

            /// get the color value
            INLINE base::Color color() const { return m_color; }

            /// get the string value
            INLINE base::StringView string() const { return base::StringView(m_text); }

            /// get name of function/variable
            INLINE base::StringID name() const { return m_name; }

            /// get number of function arguments
            INLINE uint32_t numArguments() const { return m_numArguments; }

            /// get the function argument
            INLINE const RawValue** arguments() const { return m_arguments; }

            //--

            // get a string representation of the value
            void print(base::IFormatStream& f) const;

            // compute value hash
            void calcHash(base::CRC64& crc) const;

        private:
            RawValueType m_type = RawValueType::Invalid;
            const char* m_text = nullptr; // text, NOTE: in external memory
            base::StringID m_name; // name of the function/variable
            base::Color m_color; // only color value
            float m_number = 0.0f; // number
            uint32_t m_numArguments = 0; // number of function arguments
            const RawValue** m_arguments = nullptr; // function arguments
        };

        ///---

        /// rendering style description for various parts of UI
        struct BASE_UI_API RenderStyle
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(RenderStyle);

            //--

            INLINE RenderStyle() {};
            INLINE RenderStyle(const RenderStyle& other) = default;
            INLINE RenderStyle& operator=(const RenderStyle& other) = default;

            //--

            uint32_t hash() const;

            bool operator==(const RenderStyle& other) const;
            bool operator!=(const RenderStyle& other) const;

            Size measure(float pixelScale) const;

            base::canvas::RenderStyle evaluate(float pixelScale, const ElementArea& area) const;

            //--

            uint8_t type = 0;
            uint8_t flags = 0;
            uint8_t imageFlags = 0x22; // clamp/wrap modes
            float angle = 0.0f;
            float offsetX = 0.0f;
            float offsetY = 0.0f;
            float sizeX = 1.0f;
            float sizeY = 1.0f;
            float radius = 0.0f;
            float feather = 0.0f;
            base::Color innerColor = base::Color::WHITE;
            base::Color outerColor = base::Color::WHITE;

            base::image::ImagePtr image; // locally stored
        };

        //---

        struct ImageReference
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(ImageReference);

            INLINE ImageReference() {};
            INLINE ImageReference(const ImageReference& other) = default;
            INLINE ImageReference& operator=(const ImageReference& other) = default;


            base::StringID name; // icon name to be loaded from data stash
            base::image::ImageRef image; // locally stored

            //--

            uint32_t hash() const;

            bool operator==(const ImageReference& other) const;
            bool operator!=(const ImageReference& other) const;
        };

        //---

        enum class FontStyle : uint8_t
        {
            Normal,
            Italic,
        };

        enum class FontWeight : uint8_t
        {
            Normal,
            Bold,
        };

        //---

        struct FontFamily
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(FontFamily);

            INLINE FontFamily() {};
            INLINE FontFamily(const FontFamily& other) = default;
            INLINE FontFamily& operator=(const FontFamily& other) = default;

            base::FontPtr normal;
            base::FontPtr bold;
            base::FontPtr italic;
            base::FontPtr boldItalic;

            //--

            uint32_t hash() const;

            bool operator==(const FontFamily& other) const;
            bool operator!=(const FontFamily& other) const;
        };

        //---

        enum class StyleVarType
        {
            Number,
            Color,
            String,
            Font,
            FontStyle,
            FontWeight,
            HorizontalAlign,
            VerticalAlign,
            RenderStyle,
            ImageReference,
        };

        // compile the final style value from a raw value
        extern BASE_UI_API bool CompileStyleValue(const base::parser::Location& loc, StyleVarType type, const RawValue& value, IStyleLibraryContentLoader& loader, base::Variant& outValue, base::parser::IErrorReporter& err);

        // get style var type for name
        extern BASE_UI_API StyleVarType StyleVarTypeForParamName(base::StringID name);

        //---

    } // style
 }// ui
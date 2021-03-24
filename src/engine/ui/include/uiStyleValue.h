/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: styles #]
***/

#pragma once

#include "engine/font/include/font.h"
#include "core/image/include/image.h"

BEGIN_BOOMER_NAMESPACE_EX(ui::style)

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
class ENGINE_UI_API RawValue
{
public:
    RawValue(float number, bool percentage=false);
    RawValue(Color color);
    RawValue(const char* txt);
    RawValue(const StringID variableName);
    RawValue(const StringID functionName, uint32_t numArguments, const RawValue** args);

    /// get type of the value
    INLINE RawValueType type() const { return m_type; }

    /// get the numerical value
    INLINE float number() const { return m_number; }

    /// get the color value
    INLINE Color color() const { return m_color; }

    /// get the string value
    INLINE StringView string() const { return StringView(m_text); }

    /// get name of function/variable
    INLINE StringID name() const { return m_name; }

    /// get number of function arguments
    INLINE uint32_t numArguments() const { return m_numArguments; }

    /// get the function argument
    INLINE const RawValue** arguments() const { return m_arguments; }

    //--

    // get a string representation of the value
    void print(IFormatStream& f) const;

    // compute value hash
    void calcHash(CRC64& crc) const;

private:
    RawValueType m_type = RawValueType::Invalid;
    const char* m_text = nullptr; // text, NOTE: in external memory
    StringID m_name; // name of the function/variable
    Color m_color; // only color value
    float m_number = 0.0f; // number
    uint32_t m_numArguments = 0; // number of function arguments
    const RawValue** m_arguments = nullptr; // function arguments
};

///---

/// rendering style description for various parts of UI
struct ENGINE_UI_API RenderStyle
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

    CanvasRenderStyle evaluate(DataStash& stash, float pixelScale, const ElementArea& area) const;

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
    Color innerColor = Color::WHITE;
    Color outerColor = Color::WHITE;

    ImagePtr image; // locally stored

    mutable CanvasImageEntry cachedImageEntry;
};

//---

struct ImageReference
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(ImageReference);

    INLINE ImageReference() {};
    INLINE ImageReference(const ImageReference& other) = default;
    INLINE ImageReference& operator=(const ImageReference& other) = default;


    StringID name; // icon name to be loaded from data stash
    ImagePtr image; // locally stored

	mutable CanvasImageEntry canvasImage; // not saved

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

    FontPtr normal;
    FontPtr bold;
    FontPtr italic;
    FontPtr boldItalic;

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
extern ENGINE_UI_API bool CompileStyleValue(const parser::Location& loc, StyleVarType type, const RawValue& value, ContentLoader& loader, Variant& outValue, parser::IErrorReporter& err);

// get style var type for name
extern ENGINE_UI_API StyleVarType StyleVarTypeForParamName(StringID name);

//---

END_BOOMER_NAMESPACE_EX(ui::style)

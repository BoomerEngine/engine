/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: styles #]
***/

#include "build.h"
#include "uiStyleValue.h"
#include "uiStyleLibraryPacker.h"
#include "uiElementLayout.h"
#include "uiDataStash.h"

#include "core/containers/include/stringBuilder.h"
#include "core/containers/include/utf8StringFunctions.h"
#include "core/resource/include/referenceType.h"
#include "engine/canvas/include/style.h"
#include "core/containers/include/stringParser.h"
#include "engine/font/include/font.h"
#include "core/parser/include/textToken.h"

BEGIN_BOOMER_NAMESPACE_EX(ui::style)

//--

RawValue::RawValue(float number, bool percentage)
    : m_type(percentage ? RawValueType::Percentage : RawValueType::Numerical)
    , m_number(number)
{}

RawValue::RawValue(Color color)
    : m_type(RawValueType::Color)
    , m_color(color)
{}

RawValue::RawValue(const char* txt)
    : m_type(RawValueType::String)
    , m_text(txt)
{}

RawValue::RawValue(const StringID variableName)
    : m_type(RawValueType::Variable)
    , m_name(variableName)
{}

RawValue::RawValue(const StringID fuinctionName, uint32_t numArguments, const RawValue** args)
    : m_type(RawValueType::Function)
    , m_name(fuinctionName)
    , m_numArguments(numArguments)
    , m_arguments(args)
{}

void RawValue::print(IFormatStream& ret) const
{
    if (m_type == RawValueType::Numerical)
    {
        ret << Prec(m_number, 1);
    }
    else if (m_type == RawValueType::Percentage)
    {
        ret.appendf("{}%", Prec(m_number * 100.0f, 2));
    }
    else if (m_type == RawValueType::Color)
    {
        ret.appendf("[{},{},{},{}]", m_color.r, m_color.g, m_color.b, m_color.a);
    }
    else if (m_type == RawValueType::String)
    {
        ret.appendf("\"{}\"", m_text);
    }
    else if (m_type == RawValueType::Function)
    {
        ret.appendf("{}(", m_name);
        for (uint32_t i = 0; i < m_numArguments; ++i)
        {
            if (i > 0)
                ret.append(", ");

            const auto* arg = m_arguments[i];
            ret << (*arg);
        }
        ret.append(")");
    }
    else if (m_type == RawValueType::Variable)
    {
        ret.appendf("${}", m_name);
    }
}

void RawValue::calcHash(CRC64& crc) const
{
    crc << (uint8_t)m_type;

    if (m_type == RawValueType::Numerical)
    {
        crc << m_number;
    }
    else if (m_type == RawValueType::Color)
    {
        crc << m_color.r;
        crc << m_color.g;
        crc << m_color.b;
        crc << m_color.a;
    }
    else if (m_type == RawValueType::String)
    {
        crc << m_text;
    }
    else if (m_type == RawValueType::Function)
    {
        crc << m_name;
        crc << m_numArguments;

        for (uint32_t i = 0; i < m_numArguments; ++i)
            m_arguments[i]->calcHash(crc);
    }
    else if (m_type == RawValueType::Variable)
    {
        crc << m_name;
    }
}

//---

RTTI_BEGIN_TYPE_STRUCT(RenderStyle);
    RTTI_PROPERTY(type);
    RTTI_PROPERTY(flags);
    RTTI_PROPERTY(angle);
    RTTI_PROPERTY(offsetX);
    RTTI_PROPERTY(offsetY);
    RTTI_PROPERTY(sizeX);
    RTTI_PROPERTY(sizeY);
    RTTI_PROPERTY(radius);
    RTTI_PROPERTY(feather);
    RTTI_PROPERTY(innerColor);
    RTTI_PROPERTY(outerColor);
    RTTI_PROPERTY(image);
RTTI_END_TYPE();

uint32_t RenderStyle::hash() const
{
    CRC32 crc;
    crc << type;
    crc << flags;
    crc << imageFlags;
    crc << angle;
    crc << offsetX;
    crc << offsetY;
    crc << sizeX;
    crc << sizeY;
    crc << radius;
    crc << feather;
    crc << innerColor.toNative();
    crc << outerColor.toNative();
    crc << image.get();
    return crc;
}

bool RenderStyle::operator==(const RenderStyle& other) const
{
    return (type == other.type) && (angle == other.angle) && (flags == other.flags)
        && (offsetX == other.offsetX) && (offsetY == other.offsetY)
        && (sizeX == other.sizeX) && (sizeY == other.sizeY)
        && (radius == other.radius) && (radius == other.feather)
        && (innerColor == other.innerColor) && (outerColor == other.outerColor)
        && (image == other.image) && (imageFlags == other.imageFlags);
}

bool RenderStyle::operator!=(const RenderStyle& other) const
{
    return !operator==(other);
}

static Vector2 ProjectPointOnBox(const Vector2& size, const Vector2& dir)
{
    Vector2 c;
    c.x = size.x / 2.0f;
    c.y = size.y / 2.0f;

    Vector2 ret = c;
    float retDist = VERY_LARGE_FLOAT;

    if (fabs(dir.x) > 0.0001f)
    {
        float dist = c.x / fabs(dir.x);
        auto p = c + dir * dist;
        retDist = dist;
        ret = p;
    }

    if (fabs(dir.y) > 0.0001f)
    {
        float dist = c.y / fabs(dir.y);
        auto p = c + dir * dist;

        if (dist < retDist)
            ret = p;
    }

    return ret;
}

static const uint8_t PERC_SIZE_X = FLAG(0);
static const uint8_t PERC_SIZE_Y = FLAG(1);
static const uint8_t PERC_OFFSET_X = FLAG(2);
static const uint8_t PERC_OFFSET_Y = FLAG(3);
static const uint8_t PERC_FEATHER = FLAG(4);
static const uint8_t PERC_RADIUS = FLAG(5);

static const uint8_t IMAGE_X_NODPI = FLAG(0); // do not scale by DPI on X axis 
static const uint8_t IMAGE_X_CLAMP = FLAG(1); // put image in the area, do not scale, do not wrap, what fits fits, rest will be empty
static const uint8_t IMAGE_X_WRAP = FLAG(2); // put image in the area, do not scale but do wrap image if the area is bigger
static const uint8_t IMAGE_X_STRETCH = FLAG(3); // stretch image in the area, scale to fit

static const uint8_t IMAGE_Y_NODPI = FLAG(4); // do not scale by DPI on X axis 
static const uint8_t IMAGE_Y_CLAMP = FLAG(5); // put image in the area, do not scale, do not wrap, what fits fits, rest will be empty
static const uint8_t IMAGE_Y_WRAP = FLAG(6); // put image in the area, do not scale but do wrap image if the area is bigger
static const uint8_t IMAGE_Y_STRETCH = FLAG(7); // stretch image in the area, scale to fit        

static const uint8_t IMAGE_X_MASK = 0x0E;
static const uint8_t IMAGE_Y_MASK = 0xE0;

static float EvalFloat(float val, float size, float pixelScale, bool perc)
{
    return val * (perc ? size : pixelScale);
}

Size RenderStyle::measure(float pixelScale) const
{
    Size ret(0, 0);

    if (type == 3)
    {
        if (imageFlags & IMAGE_X_CLAMP)
        {
            if (imageFlags & IMAGE_X_NODPI)
                ret.x = image->width();
            else
                ret.x = image->width() * pixelScale;
        }

        if (imageFlags & IMAGE_Y_CLAMP)
        {
            if (imageFlags & IMAGE_Y_NODPI)
                ret.y = image->height();
            else
                ret.y = image->height() * pixelScale;
        }
    }

    return ret;
}

CanvasRenderStyle RenderStyle::evaluate(DataStash& stash, float pixelScale, const ElementArea& area) const
{
    switch (type)
    {
        case 1: // linear
        {
            Vector2 d;
            d.x = cos(DEG2RAD * angle);
            d.y = sin(DEG2RAD * angle);

            auto start = ProjectPointOnBox(area.size(), -d);
            auto end = ProjectPointOnBox(area.size(), d);
            return CanvasStyle_LinearGradient(start, end, innerColor, outerColor);
        }

        case 2: // box
        {
            auto width = EvalFloat(this->sizeX, area.size().x, pixelScale, flags & PERC_SIZE_X);
            auto height = EvalFloat(this->sizeY, area.size().y, pixelScale, flags & PERC_SIZE_Y);
            auto radius = EvalFloat(this->radius, area.size().x, pixelScale, flags & PERC_RADIUS); // TODO: size proportion should take into account both axis
            auto feather = EvalFloat(this->feather, area.size().x, pixelScale, flags & PERC_FEATHER); // TODO: size proportion should take into account both axis

            return CanvasStyle_BoxGradient(0.0f, 0.0f, width, height, radius, feather, innerColor, outerColor);
        }

        case 3: // image - NOTE, this still is never set if image is NULL
        {
            auto ox = area.absolutePosition().x;
            auto ex = area.size().x;
            auto sx = image->width();

            auto oy = area.absolutePosition().y;
            auto ey = area.size().y;
            auto sy = image->height();

            ImagePatternSettings params;

            if (imageFlags & IMAGE_X_WRAP)
                params.m_wrapU = true;
            else if (imageFlags & IMAGE_X_STRETCH)
                params.m_scaleX = sx / ex;

            if (imageFlags & IMAGE_Y_WRAP)
                params.m_wrapV = true;
            else if (imageFlags & IMAGE_Y_STRETCH)
                params.m_scaleY = sy / ey;

            if (image && !cachedImage)
                cachedImage = stash.cacheImage(image, params.m_wrapU || params.m_wrapV);

            auto renderStyle = CanvasStyle_ImagePattern(cachedImage, params);
            renderStyle.innerColor = innerColor;
            renderStyle.outerColor = outerColor;
            return renderStyle;
        }
    }

    // default
    return CanvasStyle_SolidColor(innerColor);
}

//--

RTTI_BEGIN_TYPE_ENUM(FontStyle);
    RTTI_ENUM_OPTION(Normal);
    RTTI_ENUM_OPTION(Italic);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_ENUM(FontWeight);
    RTTI_ENUM_OPTION(Normal);
    RTTI_ENUM_OPTION(Bold);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_STRUCT(FontFamily);
    RTTI_PROPERTY(normal);
    RTTI_PROPERTY(bold);
    RTTI_PROPERTY(italic);
    RTTI_PROPERTY(boldItalic);
RTTI_END_TYPE();

uint32_t FontFamily::hash() const
{
    CRC32 crc;
    crc << normal.get();
    crc << bold.get();
    crc << italic.get();
    crc << boldItalic.get();
    return crc;
}

bool FontFamily::operator==(const FontFamily& other) const
{
    return (normal == other.normal) && (bold == other.bold) && (italic == other.italic) && (boldItalic == other.boldItalic);
}

bool FontFamily::operator!=(const FontFamily& other) const
{
    return !operator==(other);
}

//--

RTTI_BEGIN_TYPE_STRUCT(ImageReference);
    RTTI_PROPERTY(rawImage);
    RTTI_PROPERTY(canvasImage);
    RTTI_PROPERTY(name);
RTTI_END_TYPE();

uint32_t ImageReference::hash() const
{
    CRC32 crc;
    crc << name;
    if (rawImage)
        crc << rawImage->loadPath().view();
    return crc;
}

bool ImageReference::operator==(const ImageReference& other) const
{
    return (name == other.name) && (rawImage == other.rawImage) && (canvasImage == other.canvasImage);
}

bool ImageReference::operator!=(const ImageReference& other) const
{
    return !operator==(other);
}

//--

bool CompileStyleValue(const TextTokenLocation& loc, StyleVarType type, const RawValue& value, ContentLoader& loader, Variant& outValue, ITextErrorReporter& err)
{
    switch (type)
    {
        case StyleVarType::Number:
        {
            if (value.type() == RawValueType::Numerical || value.type() == RawValueType::Percentage)
            {
                outValue = CreateVariant<float>(value.number());
                return true;
            }
            break;
        }

        case StyleVarType::Color:
        {
            if (value.type() == RawValueType::Color)
            {
                outValue = CreateVariant<Color>(value.color());
                return true;
            }
            else if (value.type() == RawValueType::Function)
            {
                if (value.name() == "solid"_id && value.numArguments() == 1)
                {
                    if (value.arguments()[0]->type() == RawValueType::Color)
                    {
                        outValue = CreateVariant<Color>(value.arguments()[0]->color());
                        return true;
                    }
                }
            }
            break;
        }

        case StyleVarType::String:
        {
            if (value.type() == RawValueType::String)
            {
                StringBuilder builder;

                // parse the raw value, decode the "&#magic;" encoding
                const auto* content = value.string().data();
                const auto* contentEnd = content + value.string().length();
                while (content < contentEnd)
                {
                    if (content[0] == '&' && content[1] == '#')
                    {
                        content += 2;

                        uint64_t newCode = 0;
                        StringParser parser(StringView(content, contentEnd));
                        if (parser.parseHex(newCode))
                        {
                            char buf[6];
                            const auto size = utf8::ConvertChar(buf, (uint32_t)newCode);
                            builder.append(StringView(buf, size));
                            content += parser.fullView().length() - parser.currentView().length();
                        }

                        if (content[0] == ';')
                            content += 1;
                        continue;
                    }
                    else
                    {
                        builder.append(content, 1);
                        content += 1;
                    }
                }

                outValue = CreateVariant(builder.toString());
                return true;
            }
            break;
        }

        case StyleVarType::Font:
        {
            if (value.type() == RawValueType::Function && value.name() == "url")
            {
                if (value.numArguments() == 0 || value.numArguments() > 4)
                    return false;

                FontFamily family;

                for (uint32_t i = 0; i < value.numArguments(); ++i)
                {
                    if (value.arguments()[i]->type() != RawValueType::String)
                        return false;

                    auto fontPath = value.arguments()[i]->string();
                    auto font = loader.loadFont(fontPath);
                    if (!font)
                    {
                        err.reportError(loc, TempString("Font '{}' has not been found", fontPath));
                        return false;
                    }

                    if (i == 0)
                        family.normal = font;
                    else if (i == 1)
                        family.bold = font;
                    else if (i == 2)
                        family.italic = font;
                    else if (i == 3)
                        family.boldItalic = font;
                }

                outValue = CreateVariant(family);
                return true;
            }

            break;
        }

        case StyleVarType::HorizontalAlign:
        {
            if (value.type() == RawValueType::String)
            {
                if (value.string() == "left")
                {
                    outValue = CreateVariant(ElementHorizontalLayout::Left);
                    return true;
                }
                else if (value.string() == "center" || value.string() == "centre")
                {
                    outValue = CreateVariant(ElementHorizontalLayout::Center);
                    return true;
                }
                else if (value.string() == "right")
                {
                    outValue = CreateVariant(ElementHorizontalLayout::Right);
                    return true;
                }
                else if (value.string() == "expand")
                {
                    outValue = CreateVariant(ElementHorizontalLayout::Expand);
                    return true;
                }
            }

            break;
        }

        case StyleVarType::VerticalAlign:
        {
            if (value.type() == RawValueType::String)
            {
                if (value.string() == "top")
                {
                    outValue = CreateVariant(ElementVerticalLayout::Top);
                    return true;
                }
                else if (value.string() == "middle")
                {
                    outValue = CreateVariant(ElementVerticalLayout::Middle);
                    return true;
                }
                else if (value.string() == "bottom")
                {
                    outValue = CreateVariant(ElementVerticalLayout::Bottom);
                    return true;
                }
                else if (value.string() == "expand")
                {
                    outValue = CreateVariant(ElementVerticalLayout::Expand);
                    return true;
                }
            }

            break;
        }

        case StyleVarType::FontStyle:
        {
            if (value.type() == RawValueType::String)
            {
                if (value.string() == "normal")
                {
                    outValue = CreateVariant(FontStyle::Normal);
                    return true;
                }
                else if (value.string() == "italic")
                {
                    outValue = CreateVariant(FontStyle::Italic);
                    return true;
                }
            }

            break;
        }

        case StyleVarType::FontWeight:
        {
            if (value.type() == RawValueType::String)
            {
                if (value.string() == "normal")
                {
                    outValue = CreateVariant(FontWeight::Normal);
                    return true;
                }
                else if (value.string() == "bold")
                {
                    outValue = CreateVariant(FontWeight::Bold);
                    return true;
                }
            }

            break;
        }

        case StyleVarType::ImageReference:
        {
            if (value.type() == RawValueType::String)
            {
                ImageReference style;
                style.name = value.string();
                outValue = CreateVariant(style);
                return true;
            }
            else if (value.type() == RawValueType::Function && value.name() == "url")
            {
                if (value.numArguments() != 1)
                    return false;
                if (value.arguments()[0]->type() != RawValueType::String)
                    return false;

                auto url = value.arguments()[0]->string();

                ImageReference style;
                style.rawImage = loader.loadImage(url);
                if (!style.rawImage)
                {
                    err.reportError(loc, TempString("Image '{}' has not been found", url));
                    return false;
                }

                outValue = CreateVariant(style);
                return true;
            }

            break;
        }

        case StyleVarType::RenderStyle:
        {
            RenderStyle style;
            if (value.type() == RawValueType::Color)
            {
                style.type = 0;
                style.innerColor = style.outerColor = value.color();
                outValue = CreateVariant(style);
                return true;
            }
            else if (value.type() == RawValueType::Function && value.name() == "solid")
            {
                const auto& args = value.arguments();
                if (value.numArguments() != 1)
                    return false;
                if (args[0]->type() != RawValueType::Color)
                    return false;

                style.type = 0;
                style.innerColor = style.outerColor = args[0]->color();
                outValue = CreateVariant(style);
                return true;
            }
            else if (value.type() == RawValueType::Function && value.name() == "linear")
            {
                const auto& args = value.arguments();
                if (value.numArguments() != 3)
                    return false;
                if (args[0]->type() != RawValueType::Numerical)
                    return false;
                if (args[1]->type() != RawValueType::Color)
                    return false;
                if (args[2]->type() != RawValueType::Color)
                    return false;

                style.type = 1;
                style.angle = args[0]->number();
                style.innerColor = args[1]->color();
                style.outerColor = args[2]->color();
                outValue = CreateVariant(style);
                return true;
            }
            else if (value.type() == RawValueType::Function && value.name() == "box")
            {
                const auto& args = value.arguments();
                if (value.numArguments() != 6)
                    return false;
                if (args[0]->type() != RawValueType::Numerical && args[0]->type() != RawValueType::Percentage)
                    return false;
                if (args[1]->type() != RawValueType::Numerical && args[1]->type() != RawValueType::Percentage)
                    return false;
                if (args[2]->type() != RawValueType::Numerical && args[2]->type() != RawValueType::Percentage)
                    return false;
                if (args[3]->type() != RawValueType::Numerical && args[3]->type() != RawValueType::Percentage)
                    return false;
                if (args[4]->type() != RawValueType::Color)
                    return false;
                if (args[5]->type() != RawValueType::Color)
                    return false;

                style.type = 2;
                style.sizeX = args[0]->number();
                style.sizeY = args[1]->number();
                style.radius = args[2]->number();
                style.feather = args[3]->number();
                style.innerColor = args[4]->color();
                style.outerColor = args[5]->color();

                style.flags = 0;
                if (args[0]->type() == RawValueType::Percentage) style.flags |= PERC_SIZE_X;
                if (args[1]->type() == RawValueType::Percentage) style.flags |= PERC_SIZE_Y;
                if (args[2]->type() == RawValueType::Percentage) style.flags |= PERC_RADIUS;
                if (args[3]->type() == RawValueType::Percentage) style.flags |= PERC_FEATHER;

                outValue = CreateVariant(style);
                return true;
            }
            else if (value.type() == RawValueType::Function && value.name() == "url")
            {
                if (value.numArguments() != 1)
                    return false;
                if (value.arguments()[0]->type() != RawValueType::String)
                    return false;

                auto url = value.arguments()[0]->string();

                style.type = 3;
                style.image = loader.loadImage(url);
                if (!style.image)
                {
                    err.reportError(loc, TempString("Image '{}' has not been found", url));
                    return false;
                }

                outValue = CreateVariant(style);
                return true;
            }
            else if (value.type() == RawValueType::Function && value.name() == "image")
            {
                if (value.numArguments() < 1)
                    return false;
                if (value.arguments()[0]->type() != RawValueType::String)
                    return false;

                style.type = 3;
                style.imageFlags = IMAGE_X_CLAMP | IMAGE_Y_CLAMP;

                auto url = value.arguments()[0]->string();
                for (uint32_t i = 1; i < value.numArguments(); ++i)
                {
                    const auto& arg = *value.arguments()[i];

                    if (arg.type() == RawValueType::Color)
                    {
                        style.innerColor = arg.color();
                        style.outerColor = arg.color();
                    }
                    else if (arg.type() == RawValueType::String)
                    {
                        const auto& option = arg.string();

                        if (option == "wrap")
                        {
                            style.imageFlags &= ~(IMAGE_X_MASK | IMAGE_Y_MASK);
                            style.imageFlags |= IMAGE_X_WRAP | IMAGE_Y_MASK;
                        }
                        else if (option == "clamp")
                        {
                            style.imageFlags &= ~(IMAGE_X_MASK | IMAGE_Y_MASK);
                            style.imageFlags |= IMAGE_X_CLAMP | IMAGE_Y_CLAMP;
                        }
                        else if (option == "stretch" || option == "scale")
                        {
                            style.imageFlags &= ~(IMAGE_X_MASK | IMAGE_Y_MASK);
                            style.imageFlags |= IMAGE_X_STRETCH | IMAGE_Y_STRETCH;
                        }
                        else if (option == "wrapX")
                        {
                            style.imageFlags &= ~IMAGE_X_MASK;
                            style.imageFlags |= IMAGE_X_WRAP;
                        }
                        else if (option == "clampX")
                        {
                            style.imageFlags &= ~IMAGE_X_MASK;
                            style.imageFlags |= IMAGE_X_CLAMP;
                        }
                        else if (option == "stretchX" || option == "scaleX")
                        {
                            style.imageFlags &= ~IMAGE_X_MASK;
                            style.imageFlags |= IMAGE_X_STRETCH;
                        }
                        else if (option == "wrapY")
                        {
                            style.imageFlags &= ~IMAGE_Y_MASK;
                            style.imageFlags |= IMAGE_Y_WRAP;
                        }
                        else if (option == "clampY")
                        {
                            style.imageFlags &= ~IMAGE_Y_MASK;
                            style.imageFlags |= IMAGE_Y_CLAMP;
                        }
                        else if (option == "stretchY" || option == "scaleY")
                        {
                            style.imageFlags &= ~IMAGE_Y_MASK;
                            style.imageFlags |= IMAGE_Y_STRETCH;
                        }
                        else if (option == "nodpi")
                        {
                            style.imageFlags |= IMAGE_X_NODPI | IMAGE_Y_NODPI;
                        }
                        else if (option == "nodpiX")
                        {
                            style.imageFlags |= IMAGE_X_NODPI;
                        }
                        else if (option == "nodpiY")
                        {
                            style.imageFlags |= IMAGE_Y_NODPI;
                        }
                        else
                        {
                            err.reportError(loc, TempString("Unrecognized parameter '{}'", option));
                            return false;
                        }
                    }
                    else if (arg.type() == RawValueType::Function)
                    {
                        auto name = arg.name();
                        if (name == "pivot")
                        {
                            if (value.numArguments() != 2)
                                return false;
                            if (arg.arguments()[0]->type() != RawValueType::Numerical)
                                return false;
                            if (arg.arguments()[1]->type() != RawValueType::Numerical)
                                return false;

                            //setup.m_pivot.x = (int)arg.arguments()[0]->number().value();
                            //setup.m_pivot.y = (int)arg.arguments()[1]->number().value();
                        }
                        else if (name == "scale")
                        {
                            if (value.numArguments() != 2)
                                return false;
                            if (arg.arguments()[0]->type() != RawValueType::Numerical && arg.arguments()[0]->type() != RawValueType::Percentage)
                                return false;
                            if (arg.arguments()[1]->type() != RawValueType::Numerical && arg.arguments()[1]->type() != RawValueType::Percentage)
                                return false;

                            style.sizeX = arg.arguments()[0]->number();
                            style.sizeY = arg.arguments()[1]->number();

                            if (arg.arguments()[0]->type() == RawValueType::Percentage)
                                style.flags |= PERC_SIZE_X;
                            if (arg.arguments()[1]->type() == RawValueType::Percentage)
                                style.flags |= PERC_SIZE_Y;
                        }
                        else
                        {
                            err.reportError(loc, TempString("Unrecognized function '{}'", name));
                            return false;
                        }
                    }
                }

                style.image = loader.loadImage(url);
                if (!style.image)
                {
                    err.reportError(loc, TempString("Image '{}' has not been found", url));
                    return false;
                }

                outValue = CreateVariant(style);
                return true;
            }

            break;
        }

        default:
        {
            DEBUG_CHECK(!"Unexpected style variable type");
        }
    }

    return false;
}

//---

class StyleVarParamMapping : public ISingleton
{
    DECLARE_SINGLETON(StyleVarParamMapping);
public:

    HashMap<StringID, StyleVarType> m_typeMap;

    StyleVarParamMapping()
    {
        m_typeMap["image-valign"_id] = StyleVarType::VerticalAlign;
        m_typeMap["image-halign"_id] = StyleVarType::HorizontalAlign;
        m_typeMap["image-shadow-rx"_id] = StyleVarType::Number;
        m_typeMap["image-shadow-ry"_id] = StyleVarType::Number;
        m_typeMap["image-shadow-alpha"_id] = StyleVarType::Number;
        m_typeMap["image-scale"_id] = StyleVarType::Number;
        m_typeMap["image-offset-x"_id] = StyleVarType::Number;
        m_typeMap["image-offset-y"_id] = StyleVarType::Number;
        m_typeMap["color"_id] = StyleVarType::Color;
        m_typeMap["selection"_id] = StyleVarType::Color;
        m_typeMap["highlight"_id] = StyleVarType::Color;
        m_typeMap["font-family"_id] = StyleVarType::Font;
        m_typeMap["font-size"_id] = StyleVarType::Number;
        m_typeMap["font-style"_id] = StyleVarType::FontStyle;
        m_typeMap["font-weight"_id] = StyleVarType::FontWeight;
        m_typeMap["image-shadow-rx"_id] = StyleVarType::Number;
        m_typeMap["image-shadow-ry"_id] = StyleVarType::Number;
        m_typeMap["image-shadow-alpha"_id] = StyleVarType::Number;
        m_typeMap["image-shadow-blur"_id] = StyleVarType::Number;
        m_typeMap["image-shadow-color"_id] = StyleVarType::Color;
        m_typeMap["content"_id] = StyleVarType::String;
        m_typeMap["margin-left"_id] = StyleVarType::Number;
        m_typeMap["margin-top"_id] = StyleVarType::Number;
        m_typeMap["margin-right"_id] = StyleVarType::Number;
        m_typeMap["margin-bottom"_id] = StyleVarType::Number;
        m_typeMap["padding-left"_id] = StyleVarType::Number;
        m_typeMap["padding-top"_id] = StyleVarType::Number;
        m_typeMap["padding-right"_id] = StyleVarType::Number;
        m_typeMap["padding-bottom"_id] = StyleVarType::Number;
        m_typeMap["width"_id] = StyleVarType::Number;
        m_typeMap["height"_id] = StyleVarType::Number;
        m_typeMap["min-width"_id] = StyleVarType::Number;
        m_typeMap["min-height"_id] = StyleVarType::Number;
        m_typeMap["max-width"_id] = StyleVarType::Number;
        m_typeMap["max-height"_id] = StyleVarType::Number;
        m_typeMap["hide-scale"_id] = StyleVarType::Number;
        m_typeMap["horizontal-align"_id] = StyleVarType::HorizontalAlign;
        m_typeMap["vertical-align"_id] = StyleVarType::VerticalAlign;
        m_typeMap["inner-horizontal-align"_id] = StyleVarType::HorizontalAlign;
        m_typeMap["inner-vertical-align"_id] = StyleVarType::VerticalAlign;
        m_typeMap["relative-x"_id] = StyleVarType::Number;
        m_typeMap["relative-y"_id] = StyleVarType::Number;
        m_typeMap["proportion"_id] = StyleVarType::Number;
        m_typeMap["background"_id] = StyleVarType::RenderStyle;
        m_typeMap["overlay"_id] = StyleVarType::RenderStyle;
        m_typeMap["border-color"_id] = StyleVarType::Color;
        m_typeMap["border-width"_id] = StyleVarType::Number;
        m_typeMap["border-radius"_id] = StyleVarType::Number;
        m_typeMap["shadow"_id] = StyleVarType::RenderStyle;
        m_typeMap["shadow-margin"_id] = StyleVarType::Number;
        m_typeMap["shadow-padding"_id] = StyleVarType::Number;
        m_typeMap["fadeout"_id] = StyleVarType::RenderStyle;
        m_typeMap["border-left"_id] = StyleVarType::Number;
        m_typeMap["border-right"_id] = StyleVarType::Number;
        m_typeMap["border-top"_id] = StyleVarType::Number;
        m_typeMap["border-bottom"_id] = StyleVarType::Number;
        m_typeMap["opacity"_id] = StyleVarType::Number;
        m_typeMap["force-clip"_id] = StyleVarType::Number;
        m_typeMap["image-scale-x"_id] = StyleVarType::Number;
        m_typeMap["image-scale-y"_id] = StyleVarType::Number;
        m_typeMap["image"_id] = StyleVarType::ImageReference;
        m_typeMap["title-color"_id] = StyleVarType::Color;

        m_typeMap["socket-padding-left"_id] = StyleVarType::Number;
        m_typeMap["socket-padding-top"_id] = StyleVarType::Number;
        m_typeMap["socket-padding-right"_id] = StyleVarType::Number;
        m_typeMap["socket-padding-bottom"_id] = StyleVarType::Number;
        m_typeMap["circle-block-radius"_id] = StyleVarType::Number;
    }

    StyleVarType getTypeForName(StringID name) const
    {
        StyleVarType ret = StyleVarType::Number;
        m_typeMap.find(name, ret);
        return ret;
    }

    virtual void deinit() override
    {
        m_typeMap.clear();
    }
};

StyleVarType StyleVarTypeForParamName(StringID name)
{
    return StyleVarParamMapping::GetInstance().getTypeForName(name);
}

//---

END_BOOMER_NAMESPACE_EX(ui::style)

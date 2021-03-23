/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: styles\compiler #]
***/

#include "build.h"
#include "uiStyleLibrary.h"
#include "uiStyleLibraryTree.h"
#include "uiStyleLibraryPacker.h"

#include "core/containers/include/stringBuilder.h"
#include "core/containers/include/stringParser.h"
#include "core/parser/include/textParser.h"
#include "core/memory/include/linearAllocator.h"
#include "core/resource/include/depot.h"

BEGIN_BOOMER_NAMESPACE_EX(ui::style)

namespace prv
{

    struct ParsingContext : public NoCopy
    {
    public:
        ParsingContext(parser::IErrorReporter& err, parser::IIncludeHandler& inc, RawLibraryData& library)
            : m_err(err)
            , m_inc(inc)
            , m_library(library)
        {}

        INLINE RawLibraryData& library() const
        {
            return m_library;
        }

        INLINE parser::IErrorReporter& errorHandler() const
        {
            return m_err;
        }

        INLINE parser::IIncludeHandler& includeHelper() const
        {
            return m_inc;
        }

        parser::Location extractLocation(const parser::TextParser& ctx)
        {
            return parser::Location(ctx.contextName(), ctx.line());
        }

    private:
        parser::IErrorReporter& m_err;
        parser::IIncludeHandler& m_inc;
        RawLibraryData& m_library;

        HashMap<uint64_t, StringBuf> m_fileNames;
    };

    INLINE static float SRGBToLinear(float srgb)
    {
        if (srgb <= 0.04045f)
            return srgb / 12.92f;
        else
            return powf((srgb + 0.055f) / 1.055f, 2.4f);
    }

    const RawValue* ParseValue(parser::TextParser& p, ParsingContext& pc)
    {
        StringView token;

        // try a number first
        double value = 0.0f;
        if (p.parseDouble(value, true, false))
        {
            // parse the unit
            bool percent = false;
            if (p.parseKeyword("%"))
            {
                percent = true;
                value /= 100.0;
            }
            else if (p.parseKeyword("em"))
            {
                // just eat it
            }
            else if (p.parseKeyword("px"))
            {
                // just eat it
            }
            /*else
            {
                p.error("Invalid value unit, allowed units: em, px, %");
                return nullptr;
            }*/

            return pc.library().alloc<RawValue>(value, percent);
        }

        // hash color
        else if (p.parseKeyword("#"))
        {
            // parse value
            uint64_t value = 0;
            uint32_t len = 0;
            if (p.parseHex(value, 0, &len, true, false))
            {
                if (len == 3)
                {
                    uint8_t r = (value >> 8) & 0xF;
                    uint8_t g = (value >> 4) & 0xF;
                    uint8_t b = (value >> 0) & 0xF;

                    r = r | (r << 4);
                    g = g | (g << 4);
                    b = b | (b << 4);

                    auto color = Color(r, g, b, 255);
                    return pc.library().alloc<RawValue>(color);
                }
                else if (len == 4)
                {
                    uint8_t r = (value >> 12) & 0xF;
                    uint8_t g = (value >> 8) & 0xF;
                    uint8_t b = (value >> 4) & 0xF;
                    uint8_t a = (value >> 0) & 0xF;

                    r = r | (r << 4);
                    g = g | (g << 4);
                    b = b | (b << 4);
                    a = a | (a << 4);

                    auto color = Color(r, g, b, a);
                    return pc.library().alloc<RawValue>(color);
                }
                else if (len == 6)
                {
                    uint8_t r = (value >> 16) & 0xFF;
                    uint8_t g = (value >> 8) & 0xFF;
                    uint8_t b = (value >> 0) & 0xFF;

                    auto color = Color(r, g, b, 255);
                    return pc.library().alloc<RawValue>(color);
                }
                else if (len == 8)
                {
                    uint8_t r = (value >> 24) & 0xFF;
                    uint8_t g = (value >> 16) & 0xFF;
                    uint8_t b = (value >> 8) & 0xFF;
                    uint8_t a = (value >> 0) & 0xFF;

                    auto color = Color(r, g, b, a);
                    return pc.library().alloc<RawValue>(color);
                }
            }

            // invalid value
            p.error("Invalid hexadecimal value");
            return nullptr;
        }

        // a raw string
        else if (p.parseString(token, true, false))
        {
            const auto* stringCopy = pc.library().allocString(token);
            return pc.library().alloc<RawValue>(stringCopy);
        }

        // variable reference
        else if (p.parseKeyword("$"))
        {
            if (p.parseIdentifier(token, true, false, "-"))
            {
                return pc.library().alloc<RawValue>(StringID(token));
            }
            else
            {
                p.error("Invalid variable name");
                return nullptr;
            }
        }

        // function / enum
        else if (p.parseIdentifier(token, true, false, "."))
        {
            if (p.parseKeyword("("))
            {
                InplaceArray<const RawValue*, 8> rawValues;

                // parse function arguments
                for (;;)
                {
                    if (!p.findNextContent(true))
                    {
                        p.error("Unexpected end of line before closing function bracket was found");
                    }

                    if (p.parseKeyword(")"))
                        break;

                    const auto* value = ParseValue(p, pc);
                    if (!value)
                        return nullptr;

                    rawValues.pushBack(value);

                    if (p.parseKeyword(")"))
                        break;

                    if (!p.parseKeyword(","))
                    {
                        p.error("Expected coma separated values");
                        break;
                    }
                }

                auto* rawArguments = pc.library().copyContainer(rawValues);
                return pc.library().alloc<RawValue>(StringID(token), rawValues.size(), rawArguments);
            }
            else
            {
                const auto* stringCopy = pc.library().allocString(token);
                return pc.library().alloc<RawValue>(stringCopy);
            }
        }

        p.error("Invalid value");
        return nullptr;
    }

    const RawValue* ReduceValue(const parser::Location& loc, const RawValue* p, ParsingContext& pc)
    {
        if (p->type() == RawValueType::Variable)
        {
            if (const auto* var = pc.library().findVariable(p->name()))
                return ReduceValue(var->location(), var->value(), pc);

            pc.errorHandler().reportError(loc, TempString("Unknown variable '{}'", p->name()));
        }
        else if (p->type() == RawValueType::Function)
        {
            if (p->name() == "color.opacity" || p->name() == "color.alpha")
            {
                if (p->numArguments() == 1)
                {
                    auto* color = ReduceValue(loc, p->arguments()[0], pc);
                    if (color->type() == RawValueType::Color)
                    {
                        float alpha = color->color().a / 255.0f;
                        return pc.library().alloc<RawValue>(alpha);
                    }
                    else
                    {
                        pc.errorHandler().reportError(loc, "Function 'color.opacity' requires color argument");
                    }
                }
                else
                {
                    pc.errorHandler().reportError(loc, "Function 'color.opacity' takes 1 argument");
                }
            }
            else if (p->name() == "color.red")
            {
                if (p->numArguments() == 1)
                {
                    auto* color = ReduceValue(loc, p->arguments()[0], pc);
                    if (color->type() == RawValueType::Color)
                    {
                        float val = color->color().r;
                        return pc.library().alloc<RawValue>(val);
                    }
                    else
                    {
                        pc.errorHandler().reportError(loc, "Function 'color.red' requires color argument");
                    }
                }
                else
                {
                    pc.errorHandler().reportError(loc, "Function 'color.red' takes 1 argument");
                }
            }
            else if (p->name() == "color.green")
            {
                if (p->numArguments() == 1)
                {
                    auto* color = ReduceValue(loc, p->arguments()[0], pc);
                    if (color->type() == RawValueType::Color)
                    {
                        float val = color->color().g;
                        return pc.library().alloc<RawValue>(val);
                    }
                    else
                    {
                        pc.errorHandler().reportError(loc, "Function 'color.green' requires color argument");
                    }
                }
                else
                {
                    pc.errorHandler().reportError(loc, "Function 'color.green' takes 1 argument");
                }
            }
            else if (p->name() == "color.blue")
            {
                if (p->numArguments() == 1)
                {
                    auto* color = ReduceValue(loc, p->arguments()[0], pc);
                    if (color->type() == RawValueType::Color)
                    {
                        float val = color->color().b;
                        return pc.library().alloc<RawValue>(val);
                    }
                    else
                    {
                        pc.errorHandler().reportError(loc, "Function 'color.blue' requires color argument");
                    }
                }
                else
                {
                    pc.errorHandler().reportError(loc, "Function 'color.blue' takes 1 argument");
                }
            }
            else if (p->name() == "color.rgba")
            {
                if (p->numArguments() == 4)
                {
                    auto* r = ReduceValue(loc, p->arguments()[0], pc);
                    auto* g = ReduceValue(loc, p->arguments()[1], pc);
                    auto* b = ReduceValue(loc, p->arguments()[2], pc);
                    auto* a = ReduceValue(loc, p->arguments()[3], pc);
                    if (r->type() == RawValueType::Numerical && g->type() == RawValueType::Numerical && b->type() == RawValueType::Numerical && a->type() != RawValueType::Numerical)
                    {
                        pc.errorHandler().reportError(loc, "Function 'color.rgba' requires numerical arguments");
                    }
                    else
                    {
                        uint8_t byteR = std::clamp<float>(r->number(), 0.0f, 255.0f);
                        uint8_t byteG = std::clamp<float>(g->number(), 0.0f, 255.0f);
                        uint8_t byteB = std::clamp<float>(b->number(), 0.0f, 255.0f);
                        uint8_t byteA = FloatTo255(a->number());
                        return pc.library().alloc<RawValue>(Color(byteR, byteG, byteB, byteA));
                    }
                }
                else
                {
                    pc.errorHandler().reportError(loc, "Function 'color.rgba' takes 4 arguments");
                }
            }
            else if (p->name() == "color.rgb")
            {
                if (p->numArguments() == 3)
                {
                    auto* r = ReduceValue(loc, p->arguments()[0], pc);
                    auto* g = ReduceValue(loc, p->arguments()[1], pc);
                    auto* b = ReduceValue(loc, p->arguments()[2], pc);
                    if (r->type() == RawValueType::Numerical && g->type() == RawValueType::Numerical && b->type() == RawValueType::Numerical)
                    {
                        pc.errorHandler().reportError(loc, "Function 'color.rgb' requires numerical arguments");
                    }
                    else
                    {
                        uint8_t byteR = std::clamp<float>(r->number(), 0.0f, 255.0f);
                        uint8_t byteG = std::clamp<float>(g->number(), 0.0f, 255.0f);
                        uint8_t byteB = std::clamp<float>(b->number(), 0.0f, 255.0f);
                        auto* ret = pc.library().alloc<RawValue>(Color(byteR, byteG, byteB, 255));
                    }
                }
                else
                {
                    pc.errorHandler().reportError(loc, "Function 'color.rgb' takes 3 arguments");
                }
            }
            else if (p->name() == "color.scale")
            {
                if (p->numArguments() == 2)
                {
                    auto* color = ReduceValue(loc, p->arguments()[0], pc);
                    auto* scale = ReduceValue(loc, p->arguments()[1], pc);
                    if (color->type() == RawValueType::Color && scale->type() == RawValueType::Percentage)
                    {
                        Color val = color->color() * scale->number();
                        val.a = color->color().a;
                        return pc.library().alloc<RawValue>(val);
                    }
                    else
                    {
                        pc.errorHandler().reportError(loc, "Function 'color.scale' requires color, percentage arguments");
                    }
                }
                else
                {
                    pc.errorHandler().reportError(loc, "Function 'color.scale' takes 2 arguments");
                }
            }
            else if (p->name() == "color.transparent")
            {
                if (p->numArguments() == 2)
                {
                    auto* color = ReduceValue(loc, p->arguments()[0], pc);
                    auto* scale = ReduceValue(loc, p->arguments()[1], pc);
                    if (color->type() == RawValueType::Color && scale->type() == RawValueType::Percentage)
                    {
                        Color val = color->color();
                        val.a = FloatTo255(FloatFrom255(val.a) * scale->number());
                        return pc.library().alloc<RawValue>(val);
                    }
                    else
                    {
                        pc.errorHandler().reportError(loc, "Function 'color.scale' requires color, percentage arguments");
                    }
                }
                else
                {
                    pc.errorHandler().reportError(loc, "Function 'color.scale' takes 2 arguments");
                }
            }
            else if (p->name() == "color.invert")
            {
                if (p->numArguments() == 1)
                {
                    auto* color = ReduceValue(loc, p->arguments()[0], pc);
                    if (color->type() == RawValueType::Color)
                    {
                        Color val = color->color();
                        val.r = 255 - val.r;
                        val.g = 255 - val.g;
                        val.b = 255 - val.b;
                        return pc.library().alloc<RawValue>(val);
                    }
                    else
                    {
                        pc.errorHandler().reportError(loc, "Function 'color.invert' requires color argument");
                    }
                }
                else
                {
                    pc.errorHandler().reportError(loc, "Function 'color.invert' takes 1 argument");
                }
            }
            else if (p->name() == "color.grayscale")
            {
                if (p->numArguments() == 1)
                {
                    auto* color = ReduceValue(loc, p->arguments()[0], pc);
                    if (color->type() == RawValueType::Color)
                    {
                        Color val = color->color();
                        int lightness = (val.r + val.g + val.b) / 3;
                        val.r = lightness;
                        val.g = lightness;
                        val.b = lightness;
                        return pc.library().alloc<RawValue>(val);
                    }
                    else
                    {
                        pc.errorHandler().reportError(loc, "Function 'color.grayscale' requires color argument");
                    }
                }
                else
                {
                    pc.errorHandler().reportError(loc, "Function 'color.grayscale' takes 1 argument");
                }
            }
            else if (p->name() == "color.mix")
            {
                if (p->numArguments() == 3)
                {
                    auto* colorA = ReduceValue(loc, p->arguments()[0], pc);
                    auto* colorB = ReduceValue(loc, p->arguments()[1], pc);
                    auto* perc = ReduceValue(loc, p->arguments()[2], pc);
                    if (colorA->type() == RawValueType::Color && colorB->type() == RawValueType::Color && perc->type() == RawValueType::Percentage)
                    {
                        Color valA = colorA->color();
                        Color valB = colorB->color();

                        LinearInterpolation lerp(perc->number());
                        return pc.library().alloc<RawValue>(lerp.lerpGamma(valA, valB));
                    }
                    else
                    {
                        pc.errorHandler().reportError(loc, "Function 'color.grayscale' requires color argument");
                    }
                }
                else
                {
                    pc.errorHandler().reportError(loc, "Function 'color.mix' takes 3 arguments");
                }
            }
            else if (p->name() == "neg")
            {
                if (p->numArguments() == 1)
                {
                    auto* val = ReduceValue(loc, p->arguments()[0], pc);
                    if (val->type() == RawValueType::Numerical)
                    {
                        return pc.library().alloc<RawValue>(-val->number());
                    }
                    else
                    {
                        pc.errorHandler().reportError(loc, "Function 'neg' requires numerical argument");
                    }
                }
                else
                {
                    pc.errorHandler().reportError(loc, "Function 'neg' takes 1 argument");
                }
            }
            else if (p->name() == "add")
            {
                if (p->numArguments() == 2)
                {
                    auto* valA = ReduceValue(loc, p->arguments()[0], pc);
                    auto* valB = ReduceValue(loc, p->arguments()[1], pc);
                    if (valA->type() == RawValueType::Numerical && valB->type() == RawValueType::Numerical)
                    {
                        return pc.library().alloc<RawValue>(valA->number() + valB->number());
                    }
                    else
                    {
                        pc.errorHandler().reportError(loc, "Function 'add' requires numerical arguments");
                    }
                }
                else
                {
                    pc.errorHandler().reportError(loc, "Function 'add' takes 2 arguments");
                }
            }
            else if (p->name() == "mul")
            {
                if (p->numArguments() == 2)
                {
                    auto* valA = ReduceValue(loc, p->arguments()[0], pc);
                    auto* valB = ReduceValue(loc, p->arguments()[1], pc);
                    if (valA->type() == RawValueType::Numerical && valB->type() == RawValueType::Numerical)
                    {
                        return pc.library().alloc<RawValue>(valA->number() * valB->number());
                    }
                    else
                    {
                        pc.errorHandler().reportError(loc, "Function 'mul' requires numerical arguments");
                    }
                }
                else
                {
                    pc.errorHandler().reportError(loc, "Function 'mul' takes 2 arguments");
                }
            }
            else if (p->name() == "sub")
            {
                if (p->numArguments() == 2)
                {
                    auto* valA = ReduceValue(loc, p->arguments()[0], pc);
                    auto* valB = ReduceValue(loc, p->arguments()[1], pc);
                    if (valA->type() == RawValueType::Numerical && valB->type() == RawValueType::Numerical)
                    {
                        return pc.library().alloc<RawValue>(valA->number() - valB->number());
                    }
                    else
                    {
                        pc.errorHandler().reportError(loc, "Function 'sub' requires numerical arguments");
                    }
                }
                else
                {
                    pc.errorHandler().reportError(loc, "Function 'sub' takes 2 arguments");
                }
            }
            else if (p->name() == "div")
            {
                if (p->numArguments() == 2)
                {
                    auto* valA = ReduceValue(loc, p->arguments()[0], pc);
                    auto* valB = ReduceValue(loc, p->arguments()[1], pc);
                    if (valA->type() == RawValueType::Numerical && valB->type() == RawValueType::Numerical)
                    {
                        if (valB->number() != 0.0f)
                        {
                            return pc.library().alloc<RawValue>(valA->number() / valB->number());
                        }
                        else
                        {
                            pc.errorHandler().reportError(loc, "Division by zero in 'div'");
                        }
                    }
                    else
                    {
                        pc.errorHandler().reportError(loc, "Function 'div' requires numerical arguments");
                    }
                }
                else
                {
                    pc.errorHandler().reportError(loc, "Function 'div' takes 2 arguments");
                }
            }

            for (uint32_t i = 0; i < p->numArguments(); ++i)
                p->arguments()[i] = ReduceValue(loc, p->arguments()[i], pc);
        }

        return p;
    }

    RawSelector* ParseSelector(parser::TextParser& p, ParsingContext& pc, const RawSelector* topSelector, bool& outPopTopSelector)
    {
        // get combinator for the selector
        auto combinator = SelectorCombinatorType::AnyParent;
        if (p.parseKeyword("~", true))
            combinator = SelectorCombinatorType::GeneralSibling;
        else if (p.parseKeyword("+", true))
            combinator = SelectorCombinatorType::AdjecentSibling;
        else if (p.parseKeyword(">", true))
            combinator = SelectorCombinatorType::DirectParent;

        // get the class identifier or "*" to indicate any class
        RawSelector* retSelector = pc.library().alloc<RawSelector>(pc.extractLocation(p), combinator);
        bool generalSelectorUsed = false;
        if (p.parseKeyword("&", true))
        {
            if (!topSelector)
            {
                p.error("Current selector reference '&' can only be used inside a hierarchy");
                return nullptr;
            }

            // create a copy of the top-level selector
            outPopTopSelector = true;
            retSelector->pseudoClasses() = topSelector->pseudoClasses();
            retSelector->identifiers() = topSelector->identifiers();
            retSelector->classes() = topSelector->classes();
            retSelector->types() = topSelector->types();

        }
        else if (p.parseKeyword("*", true))
        {
            // select all, no class filter
            generalSelectorUsed = true;
        }
        else
        {
            StringView ident;
            while (p.parseIdentifier(ident, true, false))
            {
                auto typeName = StringID(ident);
                retSelector->types().add(typeName);
            }
        }

        // parse the pseudo classes
        for (;;)
        {
            StringView ident;
            if (p.parseKeyword("."))
            {
                if (!p.parseIdentifier(ident, true, false))
                    return nullptr;
                retSelector->classes().add(ident);
            }
            else if (p.parseKeyword("#"))
            {
                if (!p.parseIdentifier(ident, true, false))
                    return nullptr;
                retSelector->identifiers().add(ident);
            }
            else if (p.parseKeyword(":"))
            {
                if (!p.parseIdentifier(ident, true, false))
                    return nullptr;
                retSelector->pseudoClasses().add(ident);
            }
            else
            {
                break;
            }
        }

        if ((retSelector->types().empty() && !generalSelectorUsed))
        {
            if (retSelector->pseudoClasses().empty() && retSelector->classes().empty() && retSelector->identifiers().empty())
                return nullptr;
        }


        // return valid selector
        return retSelector;
    }

    const RawRule* ProcessRule(parser::TextParser& p, ParsingContext& pc, const RawRule* parentRule)
    {
        // create new rule
        auto* rule = pc.library().alloc<RawRule>(pc.extractLocation(p));

        // copy selectors from previous rule
        // this creates the SCSS style hierarchy of styles
        if (parentRule != nullptr)
            for (const auto* parentSelector : parentRule->selectors())
                rule->addSelector(parentSelector);

        // get the top selector
        const RawSelector* topSelector = rule->selectors().empty() ? nullptr : rule->selectors().back();

        // try to parse the selector, this may fail indicating we can't pare a rule
        bool hasAtLeastOneSelector = false;
        for (;;)
        {
            // start of rules block
            if (p.peekKeyword("{"))
                break;

            // if was a property all along
            if (p.peekKeyword(";"))
                return nullptr;

            bool popTopSelector = false;
            const auto* rawSelector = ParseSelector(p, pc, topSelector, popTopSelector);
            if (!rawSelector)
                break;

            // add to hierarchy, the current mode "$" the top selector is replaced
            hasAtLeastOneSelector = true;
            topSelector = nullptr;
            if (popTopSelector)
                rule->replaceTopSelector(rawSelector);
            else
                rule->addSelector(rawSelector);
        }

        // if we haven't parsed any selectors than it's not a rule
        if (!hasAtLeastOneSelector)
            return nullptr;

        // rule is ready
        return rule;
    }

    RawRuleSet* ParseRuleSetHeader(parser::TextParser &p, ParsingContext &pc, const RawRule *parentRule)
    {
        bool valid = true;

        auto* ruleSet = pc.library().alloc<RawRuleSet>(pc.extractLocation(p));

        // extract rule
        for (;;)
        {
            const auto* rawRule = ProcessRule(p, pc, parentRule);
            if (!rawRule)
                break;

            // it's a variable
            if (p.peekKeyword(";"))
                return nullptr;

            // add parser rule
            ruleSet->addRule(rawRule);

            // end of header
            if (p.peekKeyword("{"))
                break;

            // more rules must be separated with comma
            if (!p.parseKeyword(","))
                return nullptr;
        }

        // if we haven't parsed any rules than it's not a rule set
        if (ruleSet->rules().empty())
            return nullptr;

        // rule is ready
        return ruleSet;
    }

    bool ParseRuleSetContent(parser::TextParser& p, ParsingContext& pc, RawRuleSet* ruleSet)
    {
        // parse until we reached '}'
        while (!p.parseKeyword("}"))
        {
            // skip to next content
            if (!p.findNextContent())
            {
                p.error("Unexpected end of file");
                return false;
            }

            // try to parse a rule set
            p.pushState();
            auto* childRuleSet = ParseRuleSetHeader(p, pc, ruleSet ? ruleSet->rules()[0] : nullptr);
            if (nullptr != childRuleSet && p.parseKeyword("{"))
            {
                p.discardState();
                pc.library().addRuleSet(childRuleSet);
                if (!ParseRuleSetContent(p, pc, childRuleSet))
                    return false;
                continue;
            }
            else
            {
                p.popState();
            }

            // parse the property ID
            StringView propertyId;
            if (!p.parseIdentifier(propertyId, false, false, "-"))
            {
                p.error("Expecting property ID");
                return false;
            }

            // parse the ":"
            if (!p.parseKeyword(":"))
            {
                p.error("Expecting ':' after property name");
                return false;
            }

            // parse property value
            auto currentLoc = pc.extractLocation(p);
            InplaceArray<const RawValue*, 8> values;
            for (;;)
            {
                // end of list
                if (p.parseKeyword(";", true))
                    break;

                // ensure not EOL
                if (!p.findNextContent(true))
                {
                    p.error("Expecting ';' after property value");
                    return false;
                }

                // parse value
                const auto *value = ParseValue(p, pc);
                if (!value)
                    return false;

                // collapse value
                const auto* orgValue = value;
                value = ReduceValue(currentLoc, value, pc);
                if (orgValue != value)
                {
                    TRACE_INFO("Reduced '{}' from '{}'", *value, *orgValue);
                }

                // add to list
                values.pushBack(value);
            }

            // no values parsed
            if (values.empty())
            {
                p.error("Property requires values");
                return false;
            }

            // add property
            auto* rawValues = pc.library().copyContainer(values);
            auto* property = pc.library().alloc<RawProperty>(currentLoc, StringID(propertyId), values.size(), rawValues);
            ruleSet->addProperty(property);
        }

        // done, no errors
        return true;
    }

    bool ParserFile(StringView context, StringView data, ParsingContext& pc)
    {
        bool valid = true;

        // initialize parser
        parser::TextParser parser(context, pc.errorHandler(), parser::ICommentEater::StandardComments());
        parser.reset(data);

        // process the file
        while (parser.findNextContent())
        {
            // import
            if (parser.parseKeyword("@import"))
            {
                StringView path;
                if (parser.parseString(path))
                {
                    parser.parseKeyword(";", true);

                    Buffer includeContent;
                    StringBuf includeContextPath;
                    if (!pc.includeHelper().loadInclude(false, path, context, includeContent, includeContextPath))
                    {
                        parser.error(TempString("Unable to load include file '{}'", path));
                        return false;
                    }
                    else
                    {
                        valid &= ParserFile(includeContextPath, includeContent, pc);
                        continue;

                    }
                }
                else
                {
                    parser.error(TempString("Expected file path after @import"));
                    return false;
                }
            }

            // variable
            else if (parser.parseKeyword("$"))
            {
                StringView path;
                if (parser.parseIdentifier(path, true, false, "-"))
                {
                    auto varName = StringID(path);
                    if (auto* existingVar = pc.library().findVariable(varName))
                    {
                        parser.error(TempString("Variable '{}' already declared", path));
                        pc.errorHandler().reportError(existingVar->location(), "Previous definition here");
                        return false;
                    }

                    if (!parser.parseKeyword(":", true, true))
                        return false;

                    // parse value
                    const auto* value = ParseValue(parser, pc);
                    if (!value)
                        return false;

                    if (!parser.parseKeyword(";", true, true))
                        return false;

                    auto* variable = pc.library().alloc<RawVariable>(pc.extractLocation(parser), varName, value);
                    pc.library().addVariable(variable);
                    //TRACE_INFO("Declared variable '{}': '{}'", varName, *value);
                    continue;
                }
                else
                {
                    parser.error(TempString("Expected variable name after $"));
                    return false;
                }
            }

            // rule set header
            if (RawRuleSet* ruleSet = ParseRuleSetHeader(parser, pc, nullptr))
            {
                if (!parser.parseKeyword("{", true, true))
                    return false;

                pc.library().addRuleSet(ruleSet);

                if (!ParseRuleSetContent(parser, pc, ruleSet))
                    return false;

                continue;
            }

            // invalid file content
            parser.error("Expecting @import, @image or rule/variable declaration");
            return false;
        }

        // done
        return true;
    }

} // prv

//--

// parsing error reporter for cooker based shader compilation
class LocalErrorReporter : public parser::IErrorReporter
{
public:
    virtual void reportError(const parser::Location& loc, StringView message) override
    {
        logging::Log::Print(logging::OutputLevel::Error, loc.contextName().c_str(), loc.line(), "", "", TempString("{}", message));
    }

    virtual void reportWarning(const parser::Location& loc, StringView message) override
    {
        logging::Log::Print(logging::OutputLevel::Warning, loc.contextName().c_str(), loc.line(), "", "", TempString("{}", message));
    }
};

//---

// include handler that loads appropriate dependencies
class LocalIncludeHandler : public parser::IIncludeHandler
{
public:
    LocalIncludeHandler(StringView baseDirectory)
        : m_baseDirectory(baseDirectory)
    {}

    bool loadContent(StringView depotPath, Buffer& outContent, StringBuf& outPath) const
    {
        if (!GetService<DepotService>()->loadFileToBuffer(depotPath, outContent))
            return false;

        GetService<DepotService>()->queryFileAbsolutePath(depotPath, outPath);
        return true;
    }

    virtual bool loadInclude(bool global, StringView path, StringView referencePath, Buffer& outContent, StringBuf& outPath) override
    {
        return loadContent(TempString("{}{}", m_baseDirectory, path), outContent, outPath);
    }

private:
    StringView m_baseDirectory;
};

//--

StyleLibraryPtr LoadStyleLibrary(StringView depotFilePath)
{
    ScopeTimer timer;

    LocalErrorReporter err;
    LocalIncludeHandler inc(depotFilePath.baseDirectory());

    LinearAllocator mem(POOL_UI_STYLES);
    prv::RawLibraryData rawData(mem);
    prv::ParsingContext pc(err, inc, rawData);

    {
        Buffer mainContent;
        StringBuf mainPath;
        if (!inc.loadContent(depotFilePath, mainContent, mainPath))
            return nullptr;

        if (!prv::ParserFile(mainPath, mainContent, pc))
            return nullptr;
    }

    // print parsed rules
    //TRACE_INFO("Parsed rules: '{}'", rawData);

    // create the selector tree
    ContentLoader loader(depotFilePath.baseDirectory());
    prv::RawValueTable values(loader);
    prv::RawSelectorTree tree(values);

    // resolve the selection rules
    bool valid = true;
    for (const auto* ruleSet : rawData.ruleSets())
        valid &= tree.mapRuleSet(*ruleSet, err);

    // exit on errors
    if (!valid)
    {
        TRACE_ERROR("Failed to process loaded CSS files, style library will not be compiled");
        return false;
    }

    // convert selector nodes
    Array<SelectorNode> selectorNodes;
    tree.extractSelectorNodes(selectorNodes);

    // final stats
    TRACE_INFO("Compiled {} style variables and {} style selectors", selectorNodes.size(), values.values().size());

    // set the output content
    return RefNew<Library>(std::move(selectorNodes), std::move(values.values()));
}

//--

END_BOOMER_NAMESPACE_EX(ui::style)
 


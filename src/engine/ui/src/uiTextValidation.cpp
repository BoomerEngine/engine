/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\text #]
***/

#include "build.h"
#include "uiTextValidation.h"
#include "core/containers/include/utf8StringFunctions.h"
#include "core/containers/include/path.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

// TODO: make sure all UTF32 chars are supported

/// make a simple validation function that accepts only range of chars
TInputValidationFunction MakeCustomValidationFunction(StringView validChars)
{
    auto validCharCodes = UTF16StringVector(validChars);

    return [validCharCodes](StringView text)
    {
        for (utf8::CharIterator it(text); it; ++it)
            if (validCharCodes.view().findFirstChar(*it) == -1)
                return false;
        return true;
    };
}

//--

bool ValidateIdentifier(StringView name)
{
    if (name.empty())
        return false;

    static const auto validCharCodes = UTF16StringVector("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_");
    static const auto validCharCodesFirst = UTF16StringVector("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_");

    bool firstChar = true;

    for (utf8::CharIterator it(name); it; ++it)
    {
        const auto& charSet = firstChar ? validCharCodesFirst : validCharCodes;
        if (charSet.view().findFirstChar(*it) == -1)
            return false;

        firstChar = false;
    }

    return true;
}

TInputValidationFunction MakeAlphaNumValidationFunction(StringView additionalChars /*= ""*/)
{
    auto validCharCodes = UTF16StringVector("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_");
    auto validCharCodesFirst = UTF16StringVector("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_");

    if (additionalChars)
    {
        validCharCodes += UTF16StringVector(additionalChars);
        validCharCodesFirst += UTF16StringVector(additionalChars);
    }

    return [validCharCodes, validCharCodesFirst](StringView text)
    {
        if (text.empty())
            return false;

        bool firstChar = true;
        for (utf8::CharIterator it(text); it; ++it)
        {
            const auto& charSet = firstChar ? validCharCodesFirst : validCharCodes;
            if (charSet.view().findFirstChar(*it) == -1)
                return false;

            firstChar = false;
        }

        return true;
    };
}

TInputValidationFunction MakeFilenameValidationFunction(bool withExtension /*= false*/)
{
    return [withExtension](StringView text)
    {
        return withExtension
            ? ValidateFileNameWithExtension(text)
            : ValidateFileName(text);
    };
}

//--

TInputValidationFunction MakeDirectoryValidationFunction(bool allowRelative)
{
    return [allowRelative](StringView text)
    {
        return ValidateDepotDirPath(text);
    };
}

//--

TInputValidationFunction MakePathValidationFunction(bool allowRelative)
{
    return [allowRelative](StringView text)
    {
        return ValidateDepotFilePath(text);
    };
}

//--

TInputValidationFunction MakeIntegerNumbersValidationFunction(bool allowNegative, bool allowZero)
{
    return [allowNegative, allowZero](StringView text)
    {
        text = text.trim();

        if (text.beginsWith("+"))
            text = text.subString(1);

        if (text.beginsWith("-"))
        {
            if (!allowNegative)
                return false;
            text = text.subString(1);
        }

        bool notZero = false;
        for (const auto ch : text)
        {
            notZero |= (ch != '0');

            if (!(ch >= '0' && ch <= '9'))
                return false;
        }

        if (!allowZero && !notZero)
            return false;
                
        return true;
    };
}

//--

TInputValidationFunction MakeIntegerNumbersInRangeValidationFunction(int64_t minValue, int64_t maxValue)
{
    return [minValue, maxValue](StringView text)
    {
        text = text.trim();

        int64_t val = 0;
        if (text.match(val) != MatchResult::OK)
            return false;

        return (val >= minValue) && (val <= maxValue);
    };
}

//--

TInputValidationFunction MakeRealNumbersValidationFunction(bool allowNegative)
{
    return [allowNegative](StringView text)
    {
        text = text.trim();

        double val = 0;
        if (text.match(val) != MatchResult::OK)
            return false;

        if (!allowNegative && val < 0.0)
            return false;

        return true;
    };
}

//--

/// make a function to validate real numbers
TInputValidationFunction MakeRealNumbersInRangeValidationFunction(double minValue, double maxValue)
{
    return [minValue, maxValue](StringView text)
    {
        text = text.trim();

        double val = 0;
        if (text.match(val) != MatchResult::OK)
            return false;


        return (val >= minValue) && (val <= maxValue);
    };
}

//--

END_BOOMER_NAMESPACE_EX(ui)

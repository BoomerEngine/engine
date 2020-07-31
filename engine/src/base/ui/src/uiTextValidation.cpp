/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\text #]
***/

#include "build.h"
#include "uiTextValidation.h"
#include "base/containers/include/utf8StringFunctions.h"

namespace ui
{
    //--

    // TODO: make sure all UTF32 chars are supported

    /// make a simple validation function that accepts only range of chars
    TInputValidationFunction MakeCustomValidationFunction(base::StringView<char> validChars)
    {
        auto validCharCodes = base::UTF16StringBuf(validChars);

        return [validCharCodes](base::StringView<char> text)
        {
            for (base::utf8::CharIterator it(text); it; ++it)
                if (validCharCodes.view().findFirstChar(*it) == -1)
                    return false;
            return true;
        };
    }

    //--

    TInputValidationFunction MakeAlphaNumValidationFunction(base::StringView<char> additionalChars /*= ""*/)
    {
        auto validCharCodes = base::UTF16StringBuf("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_");

        if (additionalChars)
            validCharCodes += base::UTF16StringBuf(additionalChars);

        return [validCharCodes](base::StringView<char> text)
        {
            for (base::utf8::CharIterator it(text); it; ++it)
                if (validCharCodes.view().findFirstChar(*it) == -1)
                    return false;
            return true;
        };
    }

    TInputValidationFunction MakeFilenameValidationFunction(bool withExtension /*= false*/)
    {
        return [withExtension](base::StringView<char> text)
        {
            return withExtension
                ? base::ValidateFileNameWithExtension(text)
                : base::ValidateFileName(text);
        };
    }

    //--

    TInputValidationFunction MakeDirectoryValidationFunction(bool allowRelative)
    {
        return [allowRelative](base::StringView<char> text)
        {
            return base::ValidateDepotPath(text, allowRelative ? base::DepotPathClass::AnyDirectoryPath : base::DepotPathClass::AbsoluteDirectoryPath);
        };
    }

    //--

    TInputValidationFunction MakePathValidationFunction(bool allowRelative)
    {
        return [allowRelative](base::StringView<char> text)
        {
            return base::ValidateDepotPath(text, allowRelative ? base::DepotPathClass::AnyFilePath : base::DepotPathClass::AbsoluteFilePath);
        };
    }

    //--

    TInputValidationFunction MakeIntegerNumbersValidationFunction(bool allowNegative, bool allowZero)
    {
        return [allowNegative, allowZero](base::StringView<char> text)
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
        return [minValue, maxValue](base::StringView<char> text)
        {
            text = text.trim();

            int64_t val = 0;
            if (text.match(val) != base::MatchResult::OK)
                return false;

            return (val >= minValue) && (val <= maxValue);
        };
    }

    //--

    TInputValidationFunction MakeRealNumbersValidationFunction(bool allowNegative)
    {
        return [allowNegative](base::StringView<char> text)
        {
            text = text.trim();

            double val = 0;
            if (text.match(val) != base::MatchResult::OK)
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
        return [minValue, maxValue](base::StringView<char> text)
        {
            text = text.trim();

            double val = 0;
            if (text.match(val) != base::MatchResult::OK)
                return false;


            return (val >= minValue) && (val <= maxValue);
        };
    }

    //--

} // ui

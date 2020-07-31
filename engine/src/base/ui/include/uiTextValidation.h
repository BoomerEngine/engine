/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\text #]
***/

#pragma once

namespace ui
{
    ///--

    /// make a simple validation function that accepts only range of chars
    extern BASE_UI_API TInputValidationFunction MakeCustomValidationFunction(base::StringView<char> validChars);

    /// make a alpha numerical identifier validation function (A-Za-z0-9_), we can't start from number, typical GOOD identifier
    extern BASE_UI_API TInputValidationFunction MakeAlphaNumValidationFunction(base::StringView<char> additionalChars = "");

    /// make a filename validation function (allows only valid filename characters + tests for invalid names)
    extern BASE_UI_API TInputValidationFunction MakeFilenameValidationFunction(bool withExtension = false);

    /// make a directory validation function - must use valid names + end with path separator
    extern BASE_UI_API TInputValidationFunction MakeDirectoryValidationFunction(bool allowRelative = false);

    /// make a whole path validation function (allows only path characters) but whole thing must be a valid path
    extern BASE_UI_API TInputValidationFunction MakePathValidationFunction(bool allowRelative = false);

    /// make a function to validate integer numbers
    extern BASE_UI_API TInputValidationFunction MakeIntegerNumbersValidationFunction(bool allowNegative = true, bool allowZero = true);

    /// make a function to validate integer numbers that must be in specified range
    extern BASE_UI_API TInputValidationFunction MakeIntegerNumbersInRangeValidationFunction(int64_t minValue, int64_t maxValue);

    /// make a function to validate real numbers
    extern BASE_UI_API TInputValidationFunction MakeRealNumbersValidationFunction(bool allowNegative = true);

    /// make a function to validate real numbers
    extern BASE_UI_API TInputValidationFunction MakeRealNumbersInRangeValidationFunction(double minValue, double maxValue);

    ///--

} // ui
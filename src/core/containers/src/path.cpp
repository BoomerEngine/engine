/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: smartptr #]
***/

#include "build.h"
#include "stringView.h"
#include "stringBuf.h"
#include "path.h"
#include "utf8StringFunctions.h"
#include "inplaceArray.h"

BEGIN_BOOMER_NAMESPACE()

//--

static const char* InvalidFileNames[] = {
    "CON", "PRN", "AUX", "NU", "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
};

bool IsValidPathChar(uint32_t ch)
{
    if (ch <= 31)
        return false;

    if (ch >= 0xFFFE)
        return false;

    switch (ch)
    {
    case '<':
    case '>':
    case ':':
    case '\"':
    case '/':
    case '\\':
    case '|':
    case '?':
    case '*':
        return false;

    case '.': // disallow dot in file names - it's confusing AF
        return false;

    case 0:
        return false;
    }

    return true;
}

static bool IsValidName(StringView name)
{
    for (const auto testName : InvalidFileNames)
        if (name.caseCmp(testName) == 0)
            return false;

    return true;
}

bool MakeSafeFileName(StringView fileName, StringBuf& outFixedFileName)
{
    if (ValidateFileName(fileName))
    {
        outFixedFileName = StringBuf(fileName);
        return true;
    }

    StringBuilder txt;

    for (utf8::CharIterator it(fileName); it; ++it)
    {
        if (*it < 32)
            continue;

        if (IsValidPathChar(*it))
            txt.appendUTF32(*it);
        else
            txt.append("_");
    }

    if (txt.empty())
        return false; // not a single char was valid

    outFixedFileName = txt.toString();
    return true;
}

bool ValidateFileName(StringView txt)
{
    if (txt.empty())
        return false;

    if (!IsValidName(txt))
        return false;

    if (txt.beginsWith("."))
        txt = txt.subString(1);

    for (utf8::CharIterator it(txt); it; ++it)
        if (!IsValidPathChar(*it))
            return false;

    return true;
}

bool ValidateFileNameWithExtension(StringView text)
{
    StringView mainPart, rest;
    text.splitAt(".", mainPart, rest);

    if (rest.empty())
        return false;

    if (!ValidateFileName(mainPart))
        return false;

    if (rest.beginsWith("."))
        rest = rest.subString(1);

    rest = rest.beforeFirstOrFull(".renamed");

    if (!ValidateFileName(rest))
        return false;

    return true;
}

bool ValidateDepotPath(StringView text, bool expectFileNameAtTheEnd)
{
    bool absolute = text.beginsWith("/");

    if (text.beginsWith("/"))
        text = text.subString(1);
    if (text.endsWith("/"))
        text = text.leftPart(text.length() - 1);

    InplaceArray<StringView, 20> parts;
    text.slice("/", true, parts);

    bool hadFileName = false;

    for (auto i : parts.indexRange())
    {
        const auto& part = parts[i];

        if (part.empty())
            return false;

        if (part == "." || part == "..")
        {
            if (absolute)
                return false;
            continue;
        }

        if (expectFileNameAtTheEnd && i == parts.lastValidIndex())
        {
            if (!ValidateFileNameWithExtension(part))
                return false;
            hadFileName = true;
        }
        else
        {
            if (!ValidateFileName(part))
                return false;
        }
    }

    if (expectFileNameAtTheEnd && !hadFileName)
        return false;

    return true;
}

bool ValidateDepotFilePath(StringView text)
{
    if (!text.beginsWith("/"))
        return false;

    return ValidateDepotPath(text, true);
}

bool ValidateDepotDirPath(StringView text)
{
    if (!text.beginsWith("/"))
        return false;
    if (!text.endsWith("/"))
        return false;

    return ValidateDepotPath(text, false);
}

//---

StringBuf ConformDepotFilePath(StringView path)
{
    StringBuilder txt;

    for (auto ch : path)
    {
        if (ch == '\\')
            ch = '/';
        txt.appendch(ch);
    }

    return txt.toString();
}

StringBuf ConformDepotDirectoryPath(StringView path)
{
    StringBuilder txt;

    for (auto ch : path)
    {
        if (ch == '\\')
            ch = '/';
        txt.appendch(ch);
    }

    if (!txt.view().endsWith("/"))
        txt.append("/");

    return txt.toString();
}

//---

bool BuildRelativePath(StringView basePath, StringView targetPath, StringBuf& outRelativePath)
{
    InplaceArray<StringView, 20> basePathParts;
    basePath.slice("/\\", false, basePathParts);

    InplaceArray<StringView, 20> targetPathParts;
    targetPath.slice("/\\", false, targetPathParts);

    StringView finalFileName;
    if (targetPath.endsWith("/") || targetPath.endsWith("\\"))
    {
        finalFileName = targetPathParts.back();
        targetPathParts.popBack();
    }

    // check how many parts are the same
    uint32_t numSame = 0;
    {
        uint32_t maxTest = std::min<uint32_t>(basePathParts.size(), targetPathParts.size());
        for (uint32_t i = 0; i < maxTest; ++i)
        {
            auto basePart = basePathParts[i];
            auto targetPart = targetPathParts[i];
            if (basePart.caseCmp(targetPart) == 0)
                numSame += 1;
            else
                break;
        }
    }

    // for all remaining base path parts we will have to exit the directories via ..
    InplaceArray<StringView, 20> finalParts;
    for (uint32_t i = numSame; i < basePathParts.size(); ++i)
        finalParts.pushBack("..");

    // in similar fashion all different parts of the target path has to be added as well
    for (uint32_t i = numSame; i < targetPathParts.size(); ++i)
        finalParts.pushBack(targetPathParts[i]);

    // build the relative path
    StringBuilder ret;
    if (finalParts.empty())
    {
        ret << "./";
    }
    else
    {
        for (const auto& part : finalParts)
        {
            ret << part;
            ret << "/";
        }
    }

    if (finalFileName)
        ret << finalFileName;

    outRelativePath = ret.toString();
    return true;
}

bool ApplyRelativePath(StringView contextPath, StringView relativePath, StringBuf& outPath)
{
    // starts with absolute marker ?
    const auto contextAbsolute = contextPath.beginsWith("/");
    if (contextAbsolute)
        contextPath = contextPath.subString(1);

    // split path into parts
    InplaceArray<StringView, 20> referencePathParts;
    contextPath.slice("/\\", false, referencePathParts);

    // remove the last path that's usually the file name
    if (!referencePathParts.empty() && !contextPath.endsWith("/"))
        referencePathParts.popBack();

    // if relative path is in fact absolute path (starts with path separator) discard all context data
    if (relativePath.beginsWith("/"))
        referencePathParts.reset();

    // split control path
    InplaceArray<StringView, 20> pathParts;
    relativePath.slice("/\\", false, pathParts);

    // append
    for (auto& part : pathParts)
    {
        // single path, nothing
        if (part == ".")
            continue;

        // go up
        if (part == "..")
        {
            if (referencePathParts.empty())
                return false;
            referencePathParts.popBack();
            continue;
        }

        // append
        referencePathParts.pushBack(part);
    }

    // reassemble into a full path
    StringBuilder reassemblePathBuilder;
    if (contextAbsolute)
        reassemblePathBuilder << "/";

    const auto relativePathIsDir = relativePath.endsWith("/");
    for (auto i : referencePathParts.indexRange())
    {
        const auto& part = referencePathParts[i];

        reassemblePathBuilder << part;

        if (relativePathIsDir || i < referencePathParts.lastValidIndex())
            reassemblePathBuilder << "/";
    }

    // export created path
    outPath = reassemblePathBuilder.toString();
    return true;
}

//--

bool ScanRelativePaths(StringView contextPath, StringView pathPartsStr, uint32_t maxScanDepth, StringBuf& outPath, const std::function<bool(StringView)>& testFunc)
{
    // slice the path parts that are given, we don't assume much about their structure
    InplaceArray<StringView, 20> pathParts;
    pathPartsStr.slice("\\/", false, pathParts);
    if (pathParts.empty())
        return false; // nothing was given

    // slice the context path
    InplaceArray<StringView, 20> contextParts;
    contextPath.slice("/", false, contextParts);

    // remove the file name of the reference pat
    if (!contextPath.endsWith("/") && !contextParts.empty())
        contextParts.popBack();

    // outer search (on the reference path)
    for (uint32_t i = 0; i < maxScanDepth; ++i)
    {
        // try all allowed combinations of reference path as well
        auto innerSearchDepth = std::min<uint32_t>(maxScanDepth, pathParts.size());
        for (uint32_t j = 0; j < innerSearchDepth; ++j)
        {
            StringBuilder pathBuilder;

            // build base part of the path
            auto printSeparator = contextPath.beginsWith("/");
            for (auto& str : contextParts)
            {
                if (printSeparator) pathBuilder << "/";
                pathBuilder << str;
                printSeparator = true;
            }

            // add rest of the parts
            auto firstInputPart = pathParts.size() - j - 1;
            for (uint32_t k = firstInputPart; k < pathParts.size(); ++k)
            {
                if (printSeparator) pathBuilder << "/";
                pathBuilder << pathParts[k];
                printSeparator = true;
            }

            // does the file exist ?
            if (testFunc(pathBuilder.view()))
            {
                outPath = pathBuilder.toString();
                return true;
            }
        }

        // ok, we didn't found anything, retry with less base directories
        if (contextParts.empty())
            break;
        contextParts.popBack();
    }

    // no matching file found
    return false;
}

//--

StringBuf ReplaceExtension(StringView path, StringView newExtension)
{
    auto corePath = path.baseDirectory();
    auto coreName = path.fileStem();

    if (newExtension)
        return TempString("{}{}.{}", corePath, coreName, newExtension);
    else
        return TempString("{}{}", corePath, coreName);
}

//--

END_BOOMER_NAMESPACE()


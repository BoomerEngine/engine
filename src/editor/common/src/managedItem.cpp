/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#include "build.h"
#include "managedDirectory.h"
#include "managedItem.h"
#include "managedDepot.h"
#include "editorService.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(ManagedItem);
RTTI_END_TYPE();

ManagedItem::ManagedItem(ManagedDepot* depot, ManagedDirectory* parentDir, StringView fileName)
    : m_directory(parentDir)
    , m_name(fileName)
    , m_depot(depot)
    , m_isDeleted(false)
{
}

ManagedItem::~ManagedItem()
{}

StringBuf ManagedItem::depotPath() const
{
    StringBuilder ret;
    ret.append(m_directory->depotPath());
    ret.append(name());
    return ret.toString();
}

StringBuf ManagedItem::absolutePath() const
{
    StringBuf ret;
    m_depot->depot().queryFileAbsolutePath(depotPath(), ret);
    return ret;
}

static bool IsValidChar(wchar_t ch)
{
    if (ch <= 31)
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

static const wchar_t* InvalidFileNames[] = { 
    L"CON", L"PRN", L"AUX", L"NUL", L"COM1", L"COM2", L"COM3", L"COM4", L"COM5", L"COM6", L"COM7", L"COM8", L"COM9", L"LPT1", L"LPT2", L"LPT3", L"LPT4", L"LPT5", L"LPT6", L"LPT7", L"LPT8", L"LPT9"
};

static bool IsValidName(BaseStringView<wchar_t> name)
{
    for (const auto testName : InvalidFileNames)
        if (name == testName)
            return false;

    return true;
}

bool ManagedItem::ValidateFileName(StringView txt)
{
    UTF16StringVector wide(txt);

    if (txt.empty())
        return false;

    if (!IsValidName(wide))
        return false;

    for (auto wch : wide.view()) // we iterate over view to get rid of the TZ
        if (!IsValidChar(wch)) return false;

    return true;
}

bool ManagedItem::ValidateDirectoryName(StringView txt)
{
    return ValidateFileName(txt);
}

//--

END_BOOMER_NAMESPACE_EX(ed)



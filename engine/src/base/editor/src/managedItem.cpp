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

namespace ed
{

    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(ManagedItem);
    RTTI_END_TYPE();

    ManagedItem::ManagedItem(ManagedDepot* depot, ManagedDirectory* parentDir, StringView<char> fileName)
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

    io::AbsolutePath ManagedItem::absolutePath() const
    {
        io::AbsolutePath ret;
        m_depot->loader().queryFileAbsolutePath(depotPath(), ret);
        return ret;
    }

    res::ResourceMountPoint ManagedItem::mountPoint() const
    {
        res::ResourceMountPoint mountPoint;
        m_depot->loader().queryFileMountPoint(depotPath(), mountPoint);
        return mountPoint;
    }

    static bool IsValidChar(char ch, bool first)
    {
        if (ch >= 'A' && ch <= 'Z') return true;
        if (ch >= 'a' && ch <= 'z') return true;
        if (!first && (ch >= '0' && ch <= '9')) return true;
        if (ch == '_' || ch == ' ' || ch == '(' || ch == ')') return true;
        return false;
    }

    bool ManagedItem::ValidateName(StringView<char> txt)
    {
        if (txt.empty())
            return false;

        bool first = true;
        for (auto ch : txt)
        {
            if (!IsValidChar(ch, first)) return false;
            first = false;
        }
        return true;
    }

    //--

} // depot



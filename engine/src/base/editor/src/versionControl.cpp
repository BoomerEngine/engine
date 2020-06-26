/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: vsc #]
***/

#include "build.h"
#include "versionControl.h"
#include "base/containers/include/stringBuilder.h"

namespace ed
{
    namespace vsc
    {

        //---

        RTTI_BEGIN_TYPE_ENUM(FileLocalAction);
            RTTI_ENUM_OPTION(None);
            RTTI_ENUM_OPTION(Added);
            RTTI_ENUM_OPTION(Deleted);
            RTTI_ENUM_OPTION(MoveAdded);
            RTTI_ENUM_OPTION(MoveDeleted);
            RTTI_ENUM_OPTION(CheckedOut);
        RTTI_END_TYPE();

        //---

        RTTI_BEGIN_TYPE_ENUM(FileActionType);
            RTTI_ENUM_OPTION(Checkout);
            RTTI_ENUM_OPTION(Revert);
            RTTI_ENUM_OPTION(Delete);
            RTTI_ENUM_OPTION(Add);
            RTTI_ENUM_OPTION(GetLatest);
            RTTI_ENUM_OPTION(ForceGetLatest);
            RTTI_ENUM_OPTION(GetRevision);
            RTTI_ENUM_OPTION(Submit);
        RTTI_END_TYPE();

        //---

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IVersionControl);
        RTTI_END_TYPE();

        //---

        void FileState::print(IFormatStream& ret) const
        {
            if (m_isOutdated)
                ret.append("Outdated ");
            else
                ret.append("Current ");

            if (m_isExternalCheckout)
                ret.append("ExternalCheckout ");
            if (m_isExternalLock)
                ret.append("ExternalLock ");

            if (m_isLocked)
                ret.append("Locked ");

            switch (m_localAction)
            {
                case FileLocalAction::CheckedOut:
                    ret.append("Checkedout");
                    break;

                case FileLocalAction::MoveAdded:
                    ret.append("MovedTo");
                    break;

                case FileLocalAction::MoveDeleted:
                    ret.append("MovedOut");
                    break;

                case FileLocalAction::Deleted:
                    ret.append("Deleted");
                    break;

                case FileLocalAction::Added:
                    ret.append("Added");
                    break;
            }
        }

        bool FileState::canCheckout() const
        {
            return m_isManaged && (m_localAction == FileLocalAction::None);
        }

        bool FileState::canCheckin() const
        {
            return m_isManaged && (m_localAction != FileLocalAction::None);
        }

        bool FileState::canRevert() const
        {
            return m_isManaged && (m_localAction != FileLocalAction::None);
        }

        bool FileState::canGetLatest() const
        {
            return m_isManaged && m_isOutdated;
        }

        //---

        FileLocks::FileLocks()
        {
        }

        //---

        Settings::Settings()
        {
        }

        //---

        Result::Result()
        {
        }

        Result::Result(const StringBuf &txt)
                : m_errorText(txt.empty() ? "Unknown error" : txt) {}

        //---

        HistoryEntry::HistoryEntry()
                : m_time(), m_user("Unknown"), m_workspace("Unknown"), m_desc("Unknown"), m_changelist(-1), m_revision(0) {}

        //----

        History::History() {}

        //----

        IChangelist::IChangelist() {}

        IChangelist::~IChangelist() {}

        //----

        IHostListener::~IHostListener() {}

        //----

        IVersionControl::~IVersionControl() {}


        //----

    } // vsc
} // ed

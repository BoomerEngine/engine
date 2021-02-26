/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#include "build.h"
#include "managedFile.h"
#include "managedDirectory.h"
#include "managedItem.h"
#include "managedItemCollection.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

ManagedFileCollection::ManagedFileCollection()
{}

ManagedFileCollection::~ManagedFileCollection()
{}

ManagedFileCollection::ManagedFileCollection(const ManagedFileCollection& other) = default;
ManagedFileCollection::ManagedFileCollection(ManagedFileCollection&& other) = default;
ManagedFileCollection& ManagedFileCollection::operator=(const ManagedFileCollection& other) = default;
ManagedFileCollection& ManagedFileCollection::operator=(ManagedFileCollection&& other) = default;

ManagedFileCollection::ManagedFileCollection(const Array<ManagedFile*>& files)
{
    m_files.reserve(files.size());

    for (auto* file : files)
        if (file)
            m_files.insert(file);
}

void ManagedFileCollection::clear()
{
    m_files.clear();
}

bool ManagedFileCollection::collectFile(ManagedFile* file)
{
    return m_files.insert(file);
}

bool ManagedFileCollection::removeFile(ManagedFile* file)
{
    return m_files.remove(file);
}

bool ManagedFileCollection::containsFile(ManagedFile* file) const
{
    return m_files.contains(file);
}

//--

END_BOOMER_NAMESPACE_EX(ed)



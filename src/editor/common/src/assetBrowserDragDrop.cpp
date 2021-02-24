/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"
#include "managedFile.h"
#include "managedDirectory.h"
#include "assetBrowser.h"

#include "base/ui/include/uiTextLabel.h"

BEGIN_BOOMER_NAMESPACE(ed)

//---

// file visualization for drag&drop
class AssetBrowserItemPreview : public ui::IElement
{
public:
    AssetBrowserItemPreview(ManagedItem* item)
    {
        /*m_thumbnail = RefNew<FileThumbnail>();
        m_thumbnail->name("ThumbnailIcon");
        m_thumbnail->file(file);
        attachChild(m_thumbnail);*/

        auto caption = createChild<ui::TextLabel>(item->name());
    }
};

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetBrowserFileDragDrop);
RTTI_END_TYPE();

AssetBrowserFileDragDrop::AssetBrowserFileDragDrop(ManagedFile* file)
    : m_file(file)
{}

ui::ElementPtr AssetBrowserFileDragDrop::createPreview() const
{
    return RefNew<AssetBrowserItemPreview>(m_file);
}

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetBrowserDirectoryDragDrop);
RTTI_END_TYPE();

AssetBrowserDirectoryDragDrop::AssetBrowserDirectoryDragDrop(ManagedDirectory* dir)
    : m_dir(dir)
{}

ui::ElementPtr AssetBrowserDirectoryDragDrop::createPreview() const
{
    return RefNew<AssetBrowserItemPreview>(m_dir);
}

//---

END_BOOMER_NAMESPACE(ed)
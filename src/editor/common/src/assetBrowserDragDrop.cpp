/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"
#include "assetBrowserWindow.h"

#include "engine/ui/include/uiTextLabel.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

// file visualization for drag&drop
class AssetBrowserItemPreview : public ui::IElement
{
public:
    AssetBrowserItemPreview(StringView depotPath)
    {
        /*m_thumbnail = RefNew<FileThumbnail>();
        m_thumbnail->name("ThumbnailIcon");
        m_thumbnail->file(file);
        attachChild(m_thumbnail);*/

        auto caption = createChild<ui::TextLabel>(depotPath);
    }
};

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetBrowserFileDragDrop);
RTTI_END_TYPE();

AssetBrowserFileDragDrop::AssetBrowserFileDragDrop(StringView depotPath)
    : m_depotPath(depotPath)
{}

ui::ElementPtr AssetBrowserFileDragDrop::createPreview() const
{
    return RefNew<AssetBrowserItemPreview>(m_depotPath);
}

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetBrowserDirectoryDragDrop);
RTTI_END_TYPE();

AssetBrowserDirectoryDragDrop::AssetBrowserDirectoryDragDrop(StringView depotPath)
    : m_depotPath(depotPath)
{}

ui::ElementPtr AssetBrowserDirectoryDragDrop::createPreview() const
{
    return RefNew<AssetBrowserItemPreview>(m_depotPath);
}

//---

END_BOOMER_NAMESPACE_EX(ed)

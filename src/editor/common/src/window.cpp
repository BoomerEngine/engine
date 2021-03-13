/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#include "build.h"
#include "service.h"
#include "window.h"

#include "engine/canvas/include/canvas.h"
#include "engine/ui/include/uiRenderer.h"
#include "engine/ui/include/uiDockLayout.h"
#include "engine/ui/include/uiDockNotebook.h"
#include "engine/ui/include/uiDockContainer.h"
#include "engine/ui/include/uiElementConfig.h"
#include "engine/ui/include/uiButton.h"

#include "core/app/include/launcherPlatform.h"
#include "core/resource/include/metadata.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IEditorWindow);
RTTI_END_TYPE();

IEditorWindow::IEditorWindow(StringView tag, StringView title)
    : Window(ui::WindowFeatureFlagBit::DEFAULT_FRAME, title)
    , m_tag(tag)
{
    customMinSize(1200, 900);

    m_dockArea = createChild<ui::DockContainer>();
    m_dockArea->layout().notebook()->styleType("FileNotebook"_id);
}

IEditorWindow::~IEditorWindow()
{
}

ui::DockLayoutNode& IEditorWindow::layout()
{
    return m_dockArea->layout();
}

bool IEditorWindow::unnecessary() const
{
    return false;
}

void IEditorWindow::update()
{
}

void IEditorWindow::configLoad(const ui::ConfigBlock& block)
{
    TBaseClass::configLoad(block);
    m_dockArea->configLoad(block.tag("Docking"));
}

void IEditorWindow::configSave(const ui::ConfigBlock& block) const
{
    TBaseClass::configSave(block);
    m_dockArea->configSave(block.tag("Docking"));
}

//--

END_BOOMER_NAMESPACE_EX(ed)


/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#include "build.h"
#include "mainPanel.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IEditorPanel);
RTTI_END_TYPE();

IEditorPanel::IEditorPanel(StringView title, StringView id)
    : ui::DockPanel(title, id)
{}

IEditorPanel::~IEditorPanel()
{}

bool IEditorPanel::handleEditorClose()
{
    return true;
}

void IEditorPanel::configLoad(const ui::ConfigBlock& block)
{

}

void IEditorPanel::configSave(const ui::ConfigBlock& block) const
{

}

void IEditorPanel::update()
{

}

//--

END_BOOMER_NAMESPACE_EX(ed)


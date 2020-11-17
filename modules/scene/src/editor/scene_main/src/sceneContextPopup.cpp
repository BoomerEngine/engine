/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: actions #]
***/

#include "build.h"
#include "sceneEditor.h"
#include "sceneEditorStructure.h"
#include "sceneEditorStructureLayer.h"
#include "sceneEditorStructureNode.h"
#include "sceneSelectionContext.h"
#include "sceneNodeClassSelector.h"
#include "sceneContextPopup.h"

#include "scene/common/include/sceneLayer.h"
#include "scene/common/include/sceneWorld.h"
#include "scene/common/include/sceneNodeTemplate.h"

#include "ui/toolkit/include/uiWindowOverlay.h"
#include "ui/widgets/include/uiMessageBox.h"
#include "ui/widgets/include/uiMenuPopup.h"
#include "ui/widgets/include/uiInputBox.h"
#include "ui/widgets/include/uiWindowMessage.h"
#include "base/image/include/image.h"

namespace ed
{
    namespace world
    {

        static base::res::StaticResource<base::image::Image> resAdd("engine/ui/styles/icons/add.png");
        static base::res::StaticResource<base::image::Image> resDisk("engine/ui/styles/icons/disk.png");
        static base::res::StaticResource<base::image::Image> resCross("engine/ui/styles/icons/cross.png");
        static base::res::StaticResource<base::image::Image> resTableAdd("engine/ui/styles/icons/table_add.png");
        static base::res::StaticResource<base::image::Image> resFolderAdd("engine/ui/styles/icons/folder_add.png");
        static base::res::StaticResource<base::image::Image> resPageCopy("engine/ui/styles/icons/page_copy.png");
        static base::res::StaticResource<base::image::Image> resPagePaste("engine/ui/styles/icons/page_paste.png");
        static base::res::StaticResource<base::image::Image> resCut("engine/ui/styles/icons/cut.png");
        static base::res::StaticResource<base::image::Image> resEye("engine/ui/styles/icons/eye.png");
        static base::res::StaticResource<base::image::Image> resEyeCross("engine/ui/styles/icons/eye_cross.png");
        static base::res::StaticResource<base::image::Image> resStar("engine/ui/styles/icons/star.png");

        class ScenePopupMenu : public ui::PopupMenu
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ScenePopupMenu, ui::PopupMenu);

        public:
            ScenePopupMenu(const base::RefPtr<SelectionContext>& selection)
                : m_selection(selection)
            {
                if (selection->single() && selection->unifiedAndMask().test(ContentElementBit::NodeContainer))
                {
                    auto canAdd =  selection->unifiedAndMask().test(ContentElementBit::NodeContainer);
                    this->addItem().caption("Add node...").group("Create").image(resAdd.loadAndGet()).enable(canAdd).OnClick = [this](UI_CALLBACK)
                    {
                        cmdAddNode(callingElement);
                    };
                }

                if (selection->unifiedOrMask().test(ContentElementBit::CanSave))
                {
                    auto canSave = selection->unifiedOrMask().test(ContentElementBit::Modified);
                    this->addItem().caption("Save changes").group("Content").image(resDisk.loadAndGet()).enable(canSave).OnClick = [this](UI_CALLBACK)
                    {
                        cmdSave(callingElement);
                    };
                }

                if (selection->unifiedAndMask().test(ContentElementBit::Layer))
                {
                    this->addItem().caption("Delete").group("Content").image(resCross.loadAndGet()).OnClick = [this](UI_CALLBACK)
                    {
                        cmdDeleteLayer(callingElement);
                    };
                }
                else if (selection->unifiedAndMask().test(ContentElementBit::Group))
                {
                    if (selection->single())
                    {
                        this->addItem().caption("Add layer...").group("Content").image(resTableAdd.loadAndGet()).OnClick = [this](UI_CALLBACK)
                        {
                            cmdAddLayer(callingElement);
                        };

                        this->addItem().caption("Add group...").group("Content").image(resFolderAdd.loadAndGet()).OnClick = [this](UI_CALLBACK)
                        {
                            cmdAddGroup(callingElement);
                        };

                        if (!selection->unifiedAndMask().test(ContentElementBit::Root))
                        {
                            this->addItem().caption("Delete").group("Content").image(resCross.loadAndGet()).OnClick = [this](UI_CALLBACK)
                            {
                                cmdDeleteGroup(callingElement);
                            };
                        }
                    }
                }
                else if (selection->unifiedAndMask().test(ContentElementBit::Node))
                {
                    auto canCopy = selection->unifiedAndMask().test(ContentElementBit::CanCopy);
                    this->addItem().caption("Copy").group("Content").image(resPageCopy.loadAndGet()).enable(canCopy).OnClick = [this](UI_CALLBACK)
                    {
                        cmdCopy(callingElement);
                    };

                    auto canCut = selection->unifiedAndMask().test(ContentElementBit::CanCopy) && selection->unifiedAndMask().test(ContentElementBit::CanDelete);
                    this->addItem().caption("Cut").group("Content").image(resCut.loadAndGet()).enable(canCut).OnClick = [this](UI_CALLBACK)
                    {
                        cmdCut(callingElement);
                    };

                    auto canDelete = selection->unifiedAndMask().test(ContentElementBit::CanDelete);
                    this->addItem().caption("Delete").group("Content").image(resCross.loadAndGet()).enable(canDelete).OnClick = [this](UI_CALLBACK)
                    {
                        cmdCut(callingElement);
                    };
                }

                if (selection->single() && selection->unifiedAndMask().test(ContentElementBit::NodeContainer) && selection->unifiedAndMask().test(ContentElementBit::CanBePasteRoot))
                {
                    bool hasData = false;// m_selection->m_handler.clipboard().hasClipboardData("BoomerEngineSceneNodes"_id);
                    this->addItem().caption("Paste here").group("Content").image(resPagePaste.loadAndGet()).enable(hasData).OnClick = [this](UI_CALLBACK)
                    {
                        cmdPaste(callingElement);
                    };
                }

                if (selection->unifiedAndMask().test(ContentElementBit::CanActivate))
                {
                    auto active = selection->unifiedAndMask().test(ContentElementBit::Active);
                    this->addItem().caption("Make active").group("Editor context").image(resStar.loadAndGet()).enable(!active).OnClick = [this](UI_CALLBACK)
                    {
                        cmdMakeActive(callingElement);
                    };
                }

                if (selection->unifiedOrMask().test(ContentElementBit::CanToggleVisibility))
                {
                    this->addItem().caption("Show all nodes").group("Visibility").image(resEye.loadAndGet()).OnClick = [this](UI_CALLBACK)
                    {
                        cmdShowAll(callingElement);
                    };

                    this->addItem().caption("Hide all nodes").group("Visibility").image(resEyeCross.loadAndGet()).OnClick = [this](UI_CALLBACK)
                    {
                        cmdHideAll(callingElement);
                    };
                }
            }

            void cmdShowAll(const ui::ElementPtr& callingElement)
            {
                /*m_selection->visit([](const ContentElementPtr& element) -> SelectionContextVisitResult
                                   {
                                       element->toggleLocalVisibility(true);
                                       return SelectionContextVisitResult::Recurse;
                                   });*/
            }

            void cmdHideAll(const ui::ElementPtr& callingElement)
            {
                /*m_selection->visit([](const ContentElementPtr& element) -> SelectionContextVisitResult
                                   {
                                       element->toggleLocalVisibility(false);
                                       return SelectionContextVisitResult::Recurse;
                                   });*/
            }

            void cmdSave(const ui::ElementPtr& callingElement)
            {
                /*base::InplaceArray<ContentLayer*, 128> layersToSave;
                m_selection->visit([&layersToSave](const ContentElementPtr& element) -> SelectionContextVisitResult
                                   {
                                       if (auto layer = base::rtti_cast<ContentLayer>(element))
                                       {
                                           if (layer->isModified())
                                               layersToSave.pushBack(layer.get());
                                       }
                                       else if (auto layer = base::rtti_cast<ContentGroup>(element))
                                       {
                                           return SelectionContextVisitResult::Recurse;
                                       }

                                       return SelectionContextVisitResult::Continue;
                                   });

                // TODO: "save all"
                for (auto layer  : layersToSave)
                    layer->save(callingElement);*/
            }

            void cmdDeleteLayer(const ui::ElementPtr& callingElement)
            {
                /*base::InplaceArray<ContentLayerPtr, 128> layers;
                m_selection->visit([&layers](const ContentElementPtr& element) -> SelectionContextVisitResult
                       {
                           if (auto layer = base::rtti_cast<ContentLayer>(element))
                                   layers.pushBack(layer);
                           else if (auto layer = base::rtti_cast<ContentGroup>(element))
                               return SelectionContextVisitResult::Recurse;

                           return SelectionContextVisitResult::Continue;
                       });

                if (layers.empty())
                    return;

                if (layers.size() == 1)
                {
                    if (ui::MessageButton::Yes != ui::ShowMessageBox(callingElement,
                                                                     ui::MessageBoxSetup().question().yes().no().title("Confirm destructive action").
                                                                         message(base::TempString("Are you sure to delete layer '{}' ? Operation cannot be undone.", layers[0]->name()))))
                        return;
                }
                else
                {
                    if (ui::MessageButton::Yes != ui::ShowMessageBox(callingElement,
                                                                     ui::MessageBoxSetup().question().yes().no().title("Confirm destructive action").
                                                                         message(base::TempString("Are you sure to delete {} layers ? Operation cannot be undone.", layers.size()))))
                        return;
                }

                bool valid = true;
                for (auto& layer : layers)
                {
                    auto parentGroup = layer->parent();
                    valid &= parentGroup->removeLayer(layer);
                }

                if (!valid)
                {
                    ui::ShowMessageBox(callingElement, ui::MessageBoxSetup().error().title("Delete layers").message("Unable to delete some layers, check log for details"));
                }*/
            }

            void cmdDeleteGroup(const ui::ElementPtr& callingElement)
            {
                /*if (auto singleGroup = m_selection->singleGroup())
                {
                    if (ui::MessageButton::Yes != ui::ShowMessageBox(callingElement,
                                                                     ui::MessageBoxSetup().question().yes().no().title("Confirm destructive action").
                                                                             message(base::TempString("Are you sure to delete group '{}'. Operation cannot be undone.", singleGroup->name()))))
                        return;

                    auto parentGroup = singleGroup->parent();
                    if (!parentGroup->removeGroup(singleGroup))
                    {
                        ui::ShowMessageBox(callingElement, ui::MessageBoxSetup().error().title("Delete group").message("Unable to delete group, check log for details"));
                    }
                }*/
            }

            void cmdMakeActive(const ui::ElementPtr& callingElement)
            {
                if (m_selection->unifiedSelection().size() == 1)
                {
                    auto elem = m_selection->unifiedSelection().front();
                    m_selection->content().activeElement(elem);
                }
            }

            void cmdAddNode(const ui::ElementPtr& callingElement)
            {
                /*if (m_selection->m_singleElement)
                {
                    auto parentID = m_selection->m_singleElement->documentId();

                    base::AbsoluteTransform parentTransform;
                    m_selection->m_handler.gizmoGetObjectAbsoluteTransform(parentID, parentTransform);

                    auto classSelector = base::RefNew<NodeClassSelector>();
                    auto selection = m_selection;

                    classSelector->OnClassSelected = [selection, parentID, parentTransform, callingElement](const base::rtti::IClassType *newClass)
                    {
                        base::edit::DocumentObjectCreationInfo info;
                        info.m_parentId = parentID;
                        info.className = newClass;
                        info.position = parentTransform.position().approximate();
                        info.rotation = parentTransform.rotation().toRotator();
                        info.m_scale = parentTransform.scale();

                        selection->m_handler.createObject(info, true);
                    };

                    ui::AttachOverlay(callingElement, classSelector, ui::OverlayAnchorMode::Cursor, ui::Size(0, 0), ui::Position(-100.0f, -100.0f));
                }*/
            }

            void cmdAddLayer(const ui::ElementPtr& callingElement)
            {
                /*if (auto group = m_selection->singleGroup())
                {
                    base::StringBuf layerName;
                    if (ui::ShowInputBox(callingElement, ui::InputBoxSetup().title("New layer").message("Enter name of the new layer:"), layerName))
                    {
                        auto createdLayer = group->createLayer(base::StringID(layerName.c_str()));
                        if (!createdLayer)
                        {
                            ui::ShowMessageBox(callingElement, ui::MessageBoxSetup().error().title("Create layer").message("Unable to create new layer, check log for details"));
                        }
                        else
                        {
                            auto layerId = createdLayer->documentId();
                            m_selection->m_handler.changeSelection(layerId);
                        }
                    }
                }*/
            }

            void cmdAddGroup(const ui::ElementPtr& callingElement)
            {
                /*if (auto group = m_selection->singleGroup())
                {
                    base::StringBuf groupName;
                    if (ui::ShowInputBox(callingElement, ui::InputBoxSetup().title("New group").message("Enter name of the new group:"), groupName))
                    {
                        auto createdGroup = group->createGroup(base::StringID(groupName.c_str()));
                        if (!createdGroup)
                        {
                            ui::ShowMessageBox(callingElement, ui::MessageBoxSetup().error().title("Create group").message("Unable to create new group, check log for details"));
                        }
                        else
                        {
                            auto groupId = createdGroup->documentId();
                            m_selection->m_handler.changeSelection(groupId);
                        }
                    }
                }*/
            }

            void cmdCopy(const ui::ElementPtr& callingElement)
            {
                /*if (m_selection->m_handler.copyObjects(m_selection->m_objectIDs))
                {
                    if (!m_selection->m_objectIDs.empty())
                        ui::PostGeneralMessageWindow(callingElement, ui::MessageType::Info, base::TempString("Copied '{}' node(s)", m_selection->m_objectIDs.size()));
                }
                else
                {
                    ui::PostGeneralMessageWindow(callingElement, ui::MessageType::Error, base::TempString("Failed to copy node(s)"));
                }*/
            }

            void cmdCut(const ui::ElementPtr& callingElement)
            {
                /*if (m_selection->m_handler.cutObjects(m_selection->m_objectIDs))
                {
                    if (!m_selection->m_objectIDs.empty())
                        ui::PostGeneralMessageWindow(callingElement, ui::MessageType::Info, base::TempString("Cut '{}' node(s)", m_selection->m_objectIDs.size()));
                }
                else
                {
                    ui::PostGeneralMessageWindow(callingElement, ui::MessageType::Error, base::TempString("Failed to cut node(s)"));
                }*/
            }

            void cmdPaste(const ui::ElementPtr& callingElement)
            {
                /*if (m_selection->single())
                {
                    base::edit::DocumentObjectPlacementInfo placement;
                    placement.m_overrideParentId = m_selection->m_objectIDs.ids()[0];

                    if (m_selection->m_handler.pasteObjects(placement))
                    {
                        ui::PostGeneralMessageWindow(callingElement, ui::MessageType::Info, base::TempString("Pasted node"));
                    }
                    else
                    {
                        ui::PostGeneralMessageWindow(callingElement, ui::MessageType::Error, base::TempString("Failed to paste node(s)"));
                    }
                }*/
            }

            void cmdDelete(const ui::ElementPtr& callingElement)
            {
                /*if (m_selection->m_handler.deleteObjects(m_selection->m_objectIDs))
                {
                    ui::PostGeneralMessageWindow(callingElement, ui::MessageType::Info, base::TempString("Deleted '{}' node(s)", m_selection->m_objectIDs.size()));
                }
                else
                {
                    ui::PostGeneralMessageWindow(callingElement, ui::MessageType::Error, base::TempString("Failed to delete node(s0"));
                }*/
            }


        private:
            base::RefPtr<SelectionContext> m_selection;
        };

        RTTI_BEGIN_TYPE_CLASS(ScenePopupMenu);
        RTTI_END_TYPE();

        //--

        ui::PopupMenuPtr BuildPopupMenu(const base::RefPtr<SelectionContext>& selection)
        {
            return base::RefNew<ScenePopupMenu>(selection);
        }

        //--

    } // mesh
} // ed
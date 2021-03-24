/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: test #]
***/

#include "build.h"
#include "uiTestApp.h"
#include "uiTestWindow.h"
#include "engine/ui/include/uiTextLabel.h"
#include "engine/ui/include/uiButton.h"
#include "engine/ui/include/uiCheckBox.h"
#include "engine/ui/include/uiStyleValue.h"
#include "engine/ui/include/uiImage.h"
#include "engine/ui/include/uiEditBox.h"
#include "engine/ui/include/uiScrollBar.h"
#include "engine/ui/include/uiTrackBar.h"
#include "engine/ui/include/uiComboBox.h"
#include "engine/ui/include/uiNotebook.h"
#include "engine/ui/include/uiToolBar.h"
#include "engine/ui/include/uiMenuBar.h"
#include "engine/ui/include/uiWindowPopup.h"
#include "engine/ui/include/uiListViewEx.h"
#include "engine/ui/include/uiTreeViewEx.h"
#include "engine/canvas/include/atlas.h"
#include "core/resource/include/depot.h"

BEGIN_BOOMER_NAMESPACE_EX(test)

//--

class TestDepotFile : public ui::ITreeItem
{
    RTTI_DECLARE_VIRTUAL_CLASS(TestDepotFile, ui::ITreeItem);

public:
    TestDepotFile(StringView name)
    {
        createChild<ui::TextLabel>(TempString("[img:page] {}", name));
        tooltip(name);
    }

    bool handleItemCanExpand() const override
    {
        return false;
    }

    void handleItemExpand() override
    {
    }

private:
    StringBuf m_path;
};


RTTI_BEGIN_TYPE_NATIVE_CLASS(TestDepotFile);
RTTI_END_TYPE();

class TestDepotDirLazy : public ui::ITreeItem
{
    RTTI_DECLARE_VIRTUAL_CLASS(TestDepotDirLazy, ui::ITreeItem);

public:
    TestDepotDirLazy(StringView path, StringView name)
        : m_path(path)
    {
        createChild<ui::TextLabel>(TempString("[img:folder] {}", name));
    }

    bool handleItemCanExpand() const override
    {
        return true;
    }

    void handleItemExpand() override
    {
        GetService<DepotService>()->enumDirectoriesAtPath(m_path, [this](StringView name)
            {
                auto child = RefNew< TestDepotDirLazy>(TempString("{}{}/", m_path, name), name);
                addChild(child);
            });

        GetService<DepotService>()->enumFilesAtPath(m_path, [this](StringView name)
            {
                auto child = RefNew<TestDepotFile>(name);
                addChild(child);
            });
    }

    void handleItemCollapse() override
    {
        removeAllChildren();
    }

private:
    StringBuf m_path;
};

RTTI_BEGIN_TYPE_NATIVE_CLASS(TestDepotDirLazy);
RTTI_END_TYPE();

TestWindow::TestWindow()
    : ui::Window({ui::WindowFeatureFlagBit::DEFAULT_FRAME}, "Boomer Engine - Test UI")
{
    customMinSize(300, 300);
    layoutMode(ui::LayoutMode::Vertical);

	m_honklerAtlas = RefNew<canvas::DynamicAtlas>(2048, 1);
            
    static bool canEdit = true;

    {
        auto menuBar = createChild<ui::MenuBar>();

        {
            auto wnd2 = RefNew<ui::PopupWindow>();
            auto cnt2 = wnd2->createChild<ui::MenuButtonContainer>();
            cnt2->createChild<ui::MenuButton>("Undo", "[img:undo]");
            cnt2->createChild<ui::MenuButton>("Redo", "[img:redo]");
            menuBar->createMenu("File", wnd2);
        }

        {
            auto wnd2 = RefNew<ui::PopupWindow>();
            auto cnt2 = wnd2->createChild<ui::MenuButtonContainer>();
            cnt2->createChild<ui::MenuButton>("Undo", "[img:undo]");
            cnt2->createChild<ui::MenuButton>("Redo", "[img:redo]");

            auto wnd = RefNew<ui::PopupWindow>();
            auto cnt = wnd->createChild<ui::MenuButtonContainer>();
            cnt->createChild<ui::MenuButton>("Undo", "[img:undo]");
            cnt->createChild<ui::MenuButton>("Redo", "[img:redo]");
            cnt->createSeparator();
            cnt->createChild<ui::MenuButton>("Copy", "[img:copy]");
            cnt->createChild<ui::MenuButton>("Cut", "[img:cut]");
            cnt->createChild<ui::MenuButton>("Paste", "[img:paste]");
            cnt->createChild<ui::MenuButton>("Delete", "[img:delete]");
            cnt->createSeparator();
            cnt->createSubMenu(wnd2, "Depot", "[img:database]");
                    
            menuBar->createMenu("Edit", wnd);
        }
    }

    {
        auto toolbar = createChild<ui::ToolBar>();
        //toolbar->createButton("Edit.Undo"_id, "[center][img:arrow_undo][br][center][size:-][color:#AAA]Undo", "Undo last operation");
        //toolbar->createButton("Edit.Redo"_id, "[center][img:arrow_redo][br][center][size:-][color:#AAA]Redo", "Redo last operation");
    }

/*    actions().bindCommand("Edit.Undo"_id) = [](TestWindow* a)
    {

    };

    actions().bindCommand("Edit.Redo"_id) = [](TestWindow* a)
    {
        canEdit = !canEdit;
    };

    actions().bindToggle("Edit.Redo"_id) = [](TestWindow* a) { return canEdit; };
    actions().bindFilter("Edit.Undo"_id) = [](TestWindow* a) { return canEdit; };*/

    /*{
        auto box = RefNew<ui::TextLabel>();
        box->customMinSize(100, 100);
        box->customBackgroundColor(Color::RED);
        attachChild(box);
    }

    {
        auto box = RefNew<ui::TextLabel>();
        box->customMinSize(100, 100);
        box->customBackgroundColor(Color::GREEN);

        {
            auto vert = RefNew<ui::IElement>();
            vert->layoutMode(ui::LayoutMode::Vertical);

            auto smallBox = RefNew<ui::TextLabel>();
            smallBox->customMinSize(30, 30);
            smallBox->customBackgroundColor(Color::BLUE);
            smallBox->customHorizontalAligment(ui::ElementHorizontalLayout::Center);
            vert->attachChild(smallBox);

            auto txt = RefNew<ui::TextLabel>("Hello world!");
            txt->customHorizontalAligment(ui::ElementHorizontalLayout::Center);
            txt->customMargins(5, 5, 5, 5);
            vert->attachChild(txt);

            vert->customVerticalAligment(ui::ElementVerticalLayout::Middle);
            vert->customHorizontalAligment(ui::ElementHorizontalLayout::Center);
            box->attachChild(vert);
        }

        attachChild(box);
    }*/

    {
        auto txt = RefNew<ui::TextLabel>("Hello world! Text example :)");
        txt->customStyle("font-weight"_id, ui::style::FontWeight::Bold);
        attachChild(txt);
    }

    {
        auto txt = RefNew<ui::TextLabel>("Hello world! Text example :)");
        txt->customStyle("font-style"_id, ui::style::FontStyle::Italic);
        attachChild(txt);
    }

    {
        auto txt = RefNew<ui::TextLabel>("Hello world! Text example :)");
        txt->customStyle("font-style"_id, ui::style::FontStyle::Italic);
        txt->customStyle("font-weight"_id, ui::style::FontWeight::Bold);
        attachChild(txt);
    }

    {
        auto txt = RefNew<ui::TextLabel>("Hello world! Bigger text");
        txt->customStyle("font-size"_id, 25.0f);
        attachChild(txt);
    }

    {
        createChild<ui::TextLabel>(
            "[size:+][b]H[/b][/size]ello World! [i]Text[/i] example :)\n"
            "[valign:middle][img:bullet_yellow][left]Redq: [color:#F00]RED[/color]\n"
            "[img:tick][center]Greenq: [color:#0F0]GREEN[/color]\n"
            "[img:blue_next][right]Blueq: [color:#00F]BLUE[/color]\n");
    }

    {
        auto but = RefNew<ui::Button>();
        but->styleType("PushButton"_id);

        auto txt = RefNew<ui::TextLabel>("Press me!");
        txt->name("ButtonText"_id);
        but->attachChild(txt);

        attachChild(but);
    }

    {
        auto but = RefNew<ui::Button>();
        but->styleType("PushButton"_id);
        but->addStyleClass("disabled"_id);

        auto txt = RefNew<ui::TextLabel>("I'm disabled");
        txt->name("ButtonText"_id);
        but->attachChild(txt);

        attachChild(but);
    }

    {
        auto but = RefNew<ui::Button>();
        but->styleType("PushButton"_id);
        but->addStyleClass("red"_id);

        auto txt = RefNew<ui::TextLabel>("[img:delete]Delete!");
        txt->name("ButtonText"_id);
        but->attachChild(txt);

        attachChild(but);

        but->bind(ui::EVENT_CLICKED) = [this]()
        {
            auto wnd = RefNew<ui::PopupWindow>();

            auto wnd2 = RefNew<ui::PopupWindow>();
            auto cnt2 = wnd2->createChild<ui::MenuButtonContainer>();
            cnt2->createChild<ui::MenuButton>("Undo", "[img:arrow_undo]");
            cnt2->createChild<ui::MenuButton>("Redo", "[img:arrow_redo]");

            auto wnd3 = RefNew<ui::PopupWindow>();
            auto cnt3 = wnd3->createChild<ui::MenuButtonContainer>();
            cnt3->createChild<ui::MenuButton>("Undo", "[img:arrow_undo]");
            cnt3->createChild<ui::MenuButton>("Redo", "[img:arrow_redo]");

            auto cnt = wnd->createChild<ui::MenuButtonContainer>();
            cnt->createChild<ui::MenuButton>("Undo", "[img:arrow_undo]");
            cnt->createChild<ui::MenuButton>("Redo", "[img:arrow_redo]");
            cnt->createSeparator();
            cnt->createChild<ui::MenuButton>("Copy", "[img:page_copy]");
            cnt->createChild<ui::MenuButton>("Cut", "[img:cut_red]");
            cnt->createChild<ui::MenuButton>("Paste", "[img:paste_plain]");
            cnt->createChild<ui::MenuButton>("Delete", "[img:cross]");
            cnt->createSeparator();
            cnt->createSubMenu(wnd2, "Depot", "[img:database]");
            cnt->createSubMenu(wnd3, "Package", "[img:package]");

            wnd->show(this, ui::PopupWindowSetup().relativeToCursor());
        };
    }

    {
        auto but = RefNew<ui::Button>();
        but->styleType("PushButton"_id);
        but->addStyleClass("green"_id);

        auto txt = RefNew<ui::TextLabel>("[img:add]Create!");
        txt->name("ButtonText"_id);
        but->attachChild(txt);

        /*but->OnClick = [this](UI_CALLBACK)
        {
            requestClose();
        };*/

        attachChild(but);
    }

    {
        auto but = RefNew<ui::Button>();
        but->styleType("PushButton"_id);
        but->addStyleClass("blue"_id);

        auto txt = RefNew<ui::TextLabel>("[img:page_edit]Edit!");
        txt->name("ButtonText"_id);
        but->attachChild(txt);

           
        attachChild(but);
    }

    {
        createChild<ui::Button>("ToggleMe!", ui::ButtonMode{ ui::ButtonModeBit::AutoToggle, ui::ButtonModeBit::Toggle });
    }

    {
        auto box = RefNew<ui::IElement>();
        box->layoutMode(ui::LayoutMode::Horizontal);

        auto but = RefNew<ui::CheckBox>();
        box->attachChild(but);

        auto txt = RefNew<ui::TextLabel>("Toggle me!");
        box->attachChild(txt);

        attachChild(box);
    }

    {
        auto box = RefNew<ui::IElement>();
        box->layoutMode(ui::LayoutMode::Horizontal);
        attachChild(box);

        {
            auto image = RefNew<ui::CustomImage>();
            image->styleType("TestCrap"_id);
            image->hitTest(true);
            image->tooltip("[size:+][b]H[/b][/size]ello World! [i]Text[/i] example :)\n"
                "[valign:middle][img:bullet_yellow][left]Redq: [color:#F00]RED[/color]\n"
                "[img:tick][center]Greenq: [color:#0F0]GREEN[/color]\n"
                "[img:blue_next][right]Blueq: [color:#00F]BLUE[/color]\n");
            box->attachChild(image);
        }

        {
            auto image = RefNew< ui::CustomImage>();
            image->styleType("TestCrap"_id);
            image->hitTest(true);
            image->addStyleClass("test1"_id);
            box->attachChild(image);
        }

        {
            auto image = RefNew< ui::CustomImage>();
            image->styleType("TestCrap"_id);
            image->hitTest(true);
            image->customMaxSize(80, 80);
            box->attachChild(image);
        }

        {
            auto image = RefNew< ui::CustomImage>();
            image->styleType("TestCrap"_id);
            image->hitTest(true);
            image->customMaxSize(80, 80);
            image->customForegroundColor(Color::LIGHTBLUE);
            box->attachChild(image);
        }
    }

    {
        auto elem = RefNew<ui::EditBox>();
        elem->text("Ala ma kota!");
        elem->selectWholeText();
        attachChild(elem);
    }

    {
        auto elem = RefNew<ui::ComboBox>();
        elem->addOption("First");
        elem->addOption("Second");
        elem->addOption("Third");
        elem->addOption("Fourth");
        elem->addOption("Fifth");
        elem->selectOption(2);
        attachChild(elem);
    }

    {
        auto elem = RefNew<ui::TrackBar>();
        elem->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        elem->range(-50, 100);
        elem->resolution(2);
        elem->value(42);
        elem->allowEditBox(true);
        attachChild(elem);
    }


    {
        auto tabs = createChild<ui::Notebook>();
        tabs->customMinSize(400, 250);

        for (int i = 0; i < 4; ++i)
        {
            auto elem = RefNew<ui::ScrollArea>(ui::ScrollMode::Auto, ui::ScrollMode::Auto);
            elem->customStyle<StringBuf>("title"_id, TempString("[img:page] Image{}", i));

            if (i == 1)
            {
                auto data = LoadImageFromDepotPath(("/engine/interface/images/honkler.png"));
				auto entry = m_honklerAtlas->registerImage(data);
                auto image = elem->createChild<ui::CustomImage>(entry);
            }
            else if (i == 2)
            {
                //elem->layoutIcons();

                auto listElem = elem->createChild<ui::ListViewEx>();
                listElem->layoutIcons();
                listElem->expand();

                for (int i=0; i<200; ++i)
                {
                    auto item = RefNew<ui::IListItem>();
                    item->layoutVertical();

                    auto image = item->createChild<ui::CustomImage>();
                    image->styleType("TestCrap"_id);
                    image->hitTest(true);
                    image->customMaxSize(80, 80);

                    auto box = item->createChild<ui::EditBox>();
                    box->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
                    box->text("Ala ma kota");

                    listElem->addItem(item);
                }
            }
            else if (i == 3)
            {                        
                auto label = elem->createChild<ui::TextLabel>();

                auto box = elem->createChild<ui::EditBox>();
                box->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
                box->bind(ui::EVENT_TEXT_MODIFIED) = [label, box]()
                {
                    label->text(box->text());
                };
            } 
            else
            {
                auto image = elem->createChild<ui::TextLabel>(TempString("Tab {}", i));
                image->customStyle<float>("font-size"_id, 30);
                image->customForegroundColor(Color::YELLOW);
                image->customBackgroundColor(Color::LIGHTCORAL);
            }

            tabs->attachChild(elem);
        }

        {
            auto elem = RefNew<ui::IElement>();
            elem->customStyle<StringBuf>("title"_id, TempString("[img:database] Tree"));

            auto tree = elem->createChild<ui::TreeViewEx>();
            tree->expand();

            {
                auto root = RefNew<TestDepotDirLazy>("/engine/", "<engine>");
                tree->addRoot(root);
            }

            {
                auto root = RefNew<TestDepotDirLazy>("/project/", "<project>");
                tree->addRoot(root);
            }

            tabs->attachChild(elem);
        }
    }
}

void TestWindow::queryInitialPlacementSetup(ui::WindowInitialPlacementSetup& outSetup) const
{
    Window::queryInitialPlacementSetup(outSetup);
}

void TestWindow::handleExternalCloseRequest()
{
    requestClose();
}

//--

END_BOOMER_NAMESPACE_EX(test)

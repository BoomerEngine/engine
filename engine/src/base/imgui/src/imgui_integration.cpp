// dear imgui, v1.71 WIP
// (main code and documentation)
// [# filter:imgui #]

#include "build.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_integration.h"

#include "base/image/include/image.h"
#include "base/image/include/imageView.h"
#include "base/canvas/include/canvas.h"
#include "base/input/include/inputStructures.h"
#include "base/image/include/imageUtils.h"

namespace ImGui
{
    //--

    base::mem::PoolID POOL_IMGUI("Engine.IMGUI");
    //base::mem::PoolID POOL_IMGUI_IMAGES("Engine.IMGUI.Images");

    //--

    class IconRegistry : public base::ISingleton
    {
        DECLARE_SINGLETON(IconRegistry);

    public:
        IconRegistry()
        {
            m_searchPaths.emplaceBack("engine/icons");
            m_defaultImage = base::CreateSharedPtr<base::image::Image>(16, 16, base::Color::GRAY);
            m_imageList.reserve(1024);
            m_imageList.pushBack(m_defaultImage); // ID 0
        }

        void addSearchPath(base::StringView<char> path)
        {
            if (path && !m_searchPaths.contains(path))
            {
                if (path.endsWith("/"))
                    m_searchPaths.emplaceBack(path.leftPart(path.length() - 1));
                else
                    m_searchPaths.emplaceBack(path);
            }
        }

        base::image::ImagePtr getImage(ImTextureID id)
        {
            if (id < m_imageList.size())
                return m_imageList[id];
            return m_defaultImage;
        }

        ImVec2 getImageSize(ImTextureID id)
        {
            if (id < m_imageList.size())
            {
                auto image = m_imageList[id];
                return ImVec2(image->width(), image->height());
            }
            return ImVec2(16,16);
        }

        ImTextureID registerImage(const base::image::ImagePtr& loadedImage)
        {
            auto ret = m_imageList.size();
            m_imageList.pushBack(loadedImage);
            return ret;
        }

        ImTextureID loadIcon(base::StringView<char> name)
        {
            ImTextureID ret = 0;
            if (!m_iconMap.find(name, ret))
            {
                // look in the search paths
                base::image::ImagePtr loadedImage;
                for (const auto& path : m_searchPaths)
                {
                    auto fullPath = base::res::ResourcePath(base::TempString("{}/{}.png", path, name));
                    loadedImage = base::LoadResource<base::image::Image>(fullPath).acquire();
                    if (loadedImage)
                    {
                        TRACE_INFO("ImGui: Loaded '{}' from '{}'", name, fullPath);
                        break;
                    }
                }

                // create empty image so we don't return nulls
                if (!loadedImage)
                {
                    TRACE_WARNING("ImGui: Missing icon '{}'", name);
                    m_iconMap[base::StringBuf(name)] = 0;
                }
                else
                {
                    ret = registerImage(loadedImage);
                    m_iconMap[base::StringBuf(name)] = ret;
                }
            }

            return ret;
        }

    private:
        base::Array<base::StringBuf> m_searchPaths;
        base::Array<base::image::ImagePtr> m_imageList;
        base::HashMap<base::StringBuf, ImTextureID> m_iconMap;
        base::image::ImagePtr m_defaultImage;

        virtual void deinit() override
        {
            m_searchPaths.clear();
            m_iconMap.clear();
        }
    };

    //--

    bool ProcessInputEvent(ImGuiIO& io, const base::input::BaseEvent& evt)
    {
        if (const auto* keyEvent = evt.toKeyEvent())
        {
            if (keyEvent->pressedOrRepeated())
            {
                io.KeyCtrl = keyEvent->keyMask().isCtrlDown();
                io.KeyShift = keyEvent->keyMask().isShiftDown();
                io.KeyAlt = keyEvent->keyMask().isAltDown();
                io.KeysDown[(uint32_t)keyEvent->keyCode()] = true;
            }
            else if (keyEvent->released())
            {
                io.KeysDown[(uint32_t)keyEvent->keyCode()] = false;
            }

            return io.WantCaptureKeyboard;
        }
        else if (const auto* moveEvent = evt.toMouseMoveEvent())
        {
            if (moveEvent->isCaptured())
            {
                io.MousePos.x += moveEvent->delta().x;
                io.MousePos.y += moveEvent->delta().y;
            }
            else
            {
                io.MousePos.x = moveEvent->windowPosition().x;
                io.MousePos.y = moveEvent->windowPosition().y;
            }

            io.MouseWheel += moveEvent->delta().z / 10.0f;
            return io.WantCaptureMouse;
        }
        else if (const auto* mouseClick = evt.toMouseClickEvent())
        {
            int index = -1;
            switch (mouseClick->keyCode())
            {
                case base::input::KeyCode::KEY_MOUSE0: index = 0; break;
                case base::input::KeyCode::KEY_MOUSE1: index = 1; break;
                case base::input::KeyCode::KEY_MOUSE2: index = 2; break;
                case base::input::KeyCode::KEY_MOUSE3: index = 3; break;
                case base::input::KeyCode::KEY_MOUSE4: index = 4; break;
            }

            if (index != -1)
            {
                if (mouseClick->type() == base::input::MouseEventType::DoubleClick || mouseClick->type() == base::input::MouseEventType::Click)
                {
                    io.MouseDown[index] = true;
                }
                else if (mouseClick->type() == base::input::MouseEventType::Release)
                {
                    io.MouseDown[index] = false;
                }
            }

            return io.WantCaptureMouse;
        }
        else if (const auto* charEvent = evt.toCharEvent())
        {
            io.AddInputCharacter(charEvent->scanCode());
            return io.WantTextInput;
        }

        return false;
    }

    bool ProcessInputEvent(ImGuiContext* ctx, const base::input::BaseEvent& evt)
    {
        return ProcessInputEvent(ctx->IO, evt);
    }

    void BeginCanvasFrame(base::canvas::Canvas& c)
    {
        ImGui::PrepareCanvasImages(ImGui::GetIO());

        //ImGui::SetCurrentFont(ImGui::GetFont(ImGui::Font::Default));

        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui::GetIO().KeyMap[ImGuiKey_Tab] = (int)base::input::KeyCode::KEY_TAB;
        ImGui::GetIO().KeyMap[ImGuiKey_LeftArrow] = (int)base::input::KeyCode::KEY_LEFT;
        ImGui::GetIO().KeyMap[ImGuiKey_RightArrow] = (int)base::input::KeyCode::KEY_RIGHT;
        ImGui::GetIO().KeyMap[ImGuiKey_UpArrow] = (int)base::input::KeyCode::KEY_UP;
        ImGui::GetIO().KeyMap[ImGuiKey_DownArrow] = (int)base::input::KeyCode::KEY_DOWN;
        ImGui::GetIO().KeyMap[ImGuiKey_PageUp] = (int)base::input::KeyCode::KEY_PRIOR;
        ImGui::GetIO().KeyMap[ImGuiKey_PageDown] = (int)base::input::KeyCode::KEY_NEXT;
        ImGui::GetIO().KeyMap[ImGuiKey_Home] = (int)base::input::KeyCode::KEY_HOME;
        ImGui::GetIO().KeyMap[ImGuiKey_End] = (int)base::input::KeyCode::KEY_END;
        ImGui::GetIO().KeyMap[ImGuiKey_Insert] = (int)base::input::KeyCode::KEY_INSERT;
        ImGui::GetIO().KeyMap[ImGuiKey_Delete] = (int)base::input::KeyCode::KEY_DELETE;
        ImGui::GetIO().KeyMap[ImGuiKey_Backspace] = (int)base::input::KeyCode::KEY_BACK;
        ImGui::GetIO().KeyMap[ImGuiKey_Space] = (int)base::input::KeyCode::KEY_SPACE;
        ImGui::GetIO().KeyMap[ImGuiKey_Enter] = (int)base::input::KeyCode::KEY_RETURN;
        ImGui::GetIO().KeyMap[ImGuiKey_Escape] = (int)base::input::KeyCode::KEY_ESCAPE;
        ImGui::GetIO().KeyMap[ImGuiKey_KeyPadEnter] = (int)base::input::KeyCode::KEY_NAVIGATION_ACCEPT;
        ImGui::GetIO().KeyMap[ImGuiKey_A] = (int)base::input::KeyCode::KEY_A;
        ImGui::GetIO().KeyMap[ImGuiKey_C] = (int)base::input::KeyCode::KEY_C;
        ImGui::GetIO().KeyMap[ImGuiKey_V] = (int)base::input::KeyCode::KEY_V;
        ImGui::GetIO().KeyMap[ImGuiKey_X] = (int)base::input::KeyCode::KEY_X;
        ImGui::GetIO().KeyMap[ImGuiKey_Y] = (int)base::input::KeyCode::KEY_Y;
        ImGui::GetIO().KeyMap[ImGuiKey_Z] = (int)base::input::KeyCode::KEY_Z;

        ImVec4* colors = ImGui::GetStyle().Colors;
        /*colors[ImGuiCol_WindowBg] = ImVec4(0.24f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.24f, 0.25f, 0.25f, 1.0f);
        colors[ImGuiCol_Border] = ImVec4(0.39f, 0.39f, 0.39f, 1.0f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.27f, 0.29f, 0.29f, 1.0f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.29f, 0.43f, 0.69f, 1.0f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.30f, 0.32f, 0.33f, 1.00f);
        //colors[ImGuiCol_TitleBgActive] = ImVec4(0.29f, 0.43f, 0.69f, 0.80f);
        //colors[ImGuiCol_TitleBgActive] = ImVec4(0.30f, 0.32f, 0.33f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.27f, 0.29f, 0.29f, 1.0f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.24f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.21f, 0.22f, 0.23f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.30f, 0.31f, 0.32f, 1.0f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.29f, 0.43f, 0.69f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.36f, 0.38f, 0.39f, 1.0f);
        colors[ImGuiCol_Tab] = ImVec4(0.26f, 0.27f, 0.27f, 1.0f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.29f, 0.43f, 0.69f, 1.0f);
        colors[ImGuiCol_TabActive] = ImVec4(0.31f, 0.32f, 0.33f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.26f, 0.27f, 0.27f, 1.0f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.31f, 0.32f, 0.33f, 1.00f);

        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);*/

        ImGui::GetStyle().FrameBorderSize = 1;

        ImGui::GetIO().DisplaySize.x = c.width();
        ImGui::GetIO().DisplaySize.y = c.height();
        ImGui::GetIO().DisplayFramebufferScale.x = c.pixelScale();
        ImGui::GetIO().DisplayFramebufferScale.y = c.pixelScale();

        ImGui::NewFrame();
    }

    void EndCanvasFrame(base::canvas::Canvas& c)
    {
        ImGui::Render();
        ImGui::RenderToCanvas(ImGui::GetDrawData(), c);
    }

    static_assert(sizeof(ImDrawIdx) == sizeof(uint16_t), "Expected ImGui to use 16-bit indices");

    void RenderToCanvas(const ImDrawData* data, base::canvas::Canvas& c)
    {
        auto transform = c.transform();
        c.placement(0.0f, 0.0f);

        bool hasScissorRect = false;
        base::Vector4 currentScissorRect;

        for (uint32_t i = 0; i < data->CmdListsCount; ++i)
        {
            const auto* list = data->CmdLists[i];

            // prepare geometry
            static_assert(sizeof(base::canvas::Canvas::RawVertex) == sizeof(ImDrawVert), "Something does not match");

            // collect draw operations
            for (const auto& srcCommand : list->CmdBuffer)
            {
                // apply new scissor
                const base::Vector4 scissor(
                    srcCommand.ClipRect.x - data->DisplayPos.x, srcCommand.ClipRect.y - data->DisplayPos.y,
                    srcCommand.ClipRect.z - data->DisplayPos.x, srcCommand.ClipRect.w - data->DisplayPos.y);

                if (scissor != currentScissorRect)
                {
                    if (hasScissorRect)
                        c.popScissorRect();

                    c.pushScissorRect();
                    c.intersectScissorBounds(scissor.x, scissor.y, scissor.z, scissor.w);
                    currentScissorRect = scissor;
                    hasScissorRect = true;
                }

                // draw
                if (srcCommand.UserCallback)
                {
                    // custom canvas commands
                }
                else
                {
                    base::canvas::Canvas::RawGeometry rawGeometry;
                    rawGeometry.indices = list->IdxBuffer.Data + srcCommand.IdxOffset;
                    rawGeometry.numIndices = srcCommand.ElemCount;
                    rawGeometry.vertices = (base::canvas::Canvas::RawVertex*) list->VtxBuffer.Data;

                    const auto op = base::canvas::CompositeOperation::Blend; // ImGui does not support premultiplied alpha
                    const auto& image = IconRegistry::GetInstance().getImage(srcCommand.TextureId);
                    auto style = srcCommand.TextureId ? base::canvas::ImagePattern(image, base::canvas::ImagePatternSettings()) : base::canvas::SolidColor(base::Color::WHITE);
                    style.customUV = true; // in ImGui we always use custom UV that are computed by the system
                    c.place(style, rawGeometry, 0, nullptr, op);
                }
            }
        }

        if (hasScissorRect)
            c.popScissorRect();
    }
    
    void* ImGuiAlloc(size_t size)
    {
        return MemAlloc(POOL_IMGUI, size, 4);
    }

    void ImGuiFree(void* ptr)
    {
        return MemFree(ptr);
    }

    //--



    void AddIconSearchPath(base::StringView<char> path)
    {
        IconRegistry::GetInstance().addSearchPath(path);
    }

    ImTextureID RegisterImage(const base::image::ImagePtr& image)
    {
        return IconRegistry::GetInstance().registerImage(image);
    }

    ImTextureID LoadIcon(base::StringView<char> name)
    {
        return IconRegistry::GetInstance().loadIcon(name);
    }

    ImVec2 DrawImage(ImTextureID user_texture_id, float ox /*= 0.0f*/, float oy /*= 0.0*/, const ImVec4& tint_col /*= ImVec4(1, 1, 1, 1)*/)
    {
        auto imin = ImGui::GetCursorScreenPos();
        imin.x += ox;
        imin.y += oy;

        const auto size = IconRegistry::GetInstance().getImageSize(user_texture_id);

        auto imax = imin;
        imax.x += size.x;
        imax.y += size.y;
        ImGui::GetWindowDrawList()->AddImage(user_texture_id, imin, imax);

        return size;
    }

    void Image(ImTextureID user_texture_id, const ImVec4& tint_col /*= ImVec4(1, 1, 1, 1)*/, const ImVec4& border_col /*= ImVec4(0, 0, 0, 0)*/)
    {
        const auto& image = IconRegistry::GetInstance().getImage(user_texture_id);
        ImVec2 size(image->width(), image->height());
        ImGui::Image(user_texture_id, size, ImVec2(0, 0), ImVec2(1, 1), tint_col, border_col);
    }

    bool ImageButton(ImTextureID user_texture_id, int frame_padding /*= -1*/, const ImVec4& bg_col /*= ImVec4(0, 0, 0, 0)*/, const ImVec4& tint_col /*= ImVec4(1, 1, 1, 1)*/)
    {
        const auto& image = IconRegistry::GetInstance().getImage(user_texture_id);
        ImVec2 size(image->width(), image->height());
        return ImGui::ImageButton(user_texture_id, size, ImVec2(0, 0), ImVec2(1, 1), frame_padding, bg_col, tint_col);
    }

    bool ImageToggle(ImTextureID user_texture_id, bool* state, int frame_padding /*= -1*/, bool enabled /*= true*/)
    {
        return ImageToggle(user_texture_id, user_texture_id, state, frame_padding, enabled);
    }

    bool ImageToggle(ImTextureID user_texture_id_off, ImTextureID user_texture_id_on, bool* state, int frame_padding /*= -1*/, bool enabled /*= true*/)
    {
        static const ImVec4 Transparent = ImVec4(0, 0, 0, 0);

        const auto& tintColor = enabled ? ImGui::GetStyle().Colors[ImGuiCol_Text] : ImGui::GetStyle().Colors[ImGuiCol_TextDisabled];
        const auto& backColor = (*state && user_texture_id_off == user_texture_id_on) ? ImGui::GetStyle().Colors[ImGuiCol_FrameBg] : Transparent;
        ImGui::PushStyleColor(ImGuiCol_Button, backColor);

        auto ret = ImageButton(*state ? user_texture_id_on : user_texture_id_off, frame_padding, Transparent, tintColor);
        if (ret)
            *state = !*state;

        ImGui::PopStyleColor(1);

        return ret;
    }

    bool IsKeyDown(const base::input::KeyCode code)
    {
        return IsKeyDown((int)code);
    }

    bool IsKeyPressed(const base::input::KeyCode code, bool ctrl /*= false*/, bool shift /*= false*/, bool alt /*= false*/)
    {
        if (IsKeyPressed((int)code))
        {
            if (ctrl && !IsKeyDown(base::input::KeyCode::KEY_LEFT_CTRL))
                return false;
            if (shift && !IsKeyDown(base::input::KeyCode::KEY_LEFT_SHIFT))
                return false;
            if (alt && !IsKeyDown(base::input::KeyCode::KEY_LEFT_ALT))
                return false;

            return true;
        }

        return false;
    }

    bool IsKeyReleased(const base::input::KeyCode code)
    {
        return IsKeyReleased((int)code);
    }

    bool Button(ButtonCategory cat, const char* label, const ImVec2& size /*= ImVec2(0, 0)*/)
    {
        if (cat == ButtonCategory::Constructive)
        {
            static ImVec4 colorBase = ImVec4(0.00f, 0.80f, 0.20f, 0.56f);
            static ImVec4 colorHovered = ImVec4(0.00f, 0.75f, 0.12f, 1.00f);
            static ImVec4 colorActive = ImVec4(0.00f, 1.00f, 0.15f, 1.00f);
            PushStyleColor(ImGuiCol_Button, colorBase);
            PushStyleColor(ImGuiCol_ButtonActive, colorActive);
            PushStyleColor(ImGuiCol_ButtonHovered, colorHovered);
        }
        else if (cat == ButtonCategory::Destructive)
        {
            static ImVec4 colorBase = ImVec4(0.80f, 0.00f, 0.00f, 0.56f);
            static ImVec4 colorHovered = ImVec4(0.75f, 0.00f, 0.00f, 1.00f);
            static ImVec4 colorActive = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
            PushStyleColor(ImGuiCol_Button, colorBase);
            PushStyleColor(ImGuiCol_ButtonActive, colorActive);
            PushStyleColor(ImGuiCol_ButtonHovered, colorHovered);
        }
        else if (cat == ButtonCategory::Editor)
        {
            static ImVec4 colorBase = ImVec4(0.00f, 0.55f, 0.80f, 0.83f);
            static ImVec4 colorHovered = ImVec4(0.00f, 0.58f, 0.85f, 1.00f);
            static ImVec4 colorActive = ImVec4(0.00f, 0.69f, 1.00f, 1.00f);
            PushStyleColor(ImGuiCol_Button, colorBase);
            PushStyleColor(ImGuiCol_ButtonActive, colorActive);
            PushStyleColor(ImGuiCol_ButtonHovered, colorHovered);
        }

        auto ret = Button(label, size);

        if (cat != ButtonCategory::Normal)
            PopStyleColor(3);

        return ret;
    }

    bool Button(ButtonIcon cat, const char* label /*=nullptr*/, const ImVec2& size /*= ImVec2(0, 0)*/)
    {
        switch (cat)
        {
            case ButtonIcon::Yes: return Button(ButtonCategory::Normal, label ? label : "Yes", size); 
            case ButtonIcon::No: return Button(ButtonCategory::Normal, label ? label : "No", size);
            case ButtonIcon::Cancel: return Button(ButtonCategory::Normal, label ? label : "Cancel", size);
            case ButtonIcon::Accept: return Button(ButtonCategory::Constructive, label ? label : "Accept", size);
            case ButtonIcon::Add: return Button(ButtonCategory::Constructive, label ? label : "Add", size);
            case ButtonIcon::Delete: return Button(ButtonCategory::Destructive, label ? label : "Delete", size);
        }

        return Button(label ? label : "", size);
    }

    //--

    void ToggleButton(const char* str_id, bool* v)
    {
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        float height = ImGui::GetFrameHeight();
        float width = height * 1.55f;
        float radius = height * 0.50f;

        ImGui::InvisibleButton(str_id, ImVec2(width, height));
        if (ImGui::IsItemClicked())
            *v = !*v;

        float t = *v ? 1.0f : 0.0f;

        ImGuiContext& g = *GImGui;
        float ANIM_SPEED = 0.08f;
        if (g.LastActiveId == g.CurrentWindow->GetID(str_id))// && g.LastActiveIdTimer < ANIM_SPEED)
        {
            float t_anim = ImSaturate(g.LastActiveIdTimer / ANIM_SPEED);
            t = *v ? (t_anim) : (1.0f - t_anim);
        }

        ImU32 col_bg;
        if (ImGui::IsItemHovered())
            col_bg = ImGui::GetColorU32(ImLerp(ImVec4(0.78f, 0.78f, 0.78f, 1.0f), ImVec4(0.64f, 0.83f, 0.34f, 1.0f), t));
        else
            col_bg = ImGui::GetColorU32(ImLerp(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), ImVec4(0.56f, 0.83f, 0.26f, 1.0f), t));

        draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), col_bg, height * 0.5f);
        draw_list->AddCircleFilled(ImVec2(p.x + radius + t * (width - radius * 2.0f), p.y + radius), radius - 1.5f, IM_COL32(255, 255, 255, 255));
    }

    //--

    void PrepareCanvasImages(ImGuiIO& io)
    {
        if (!io.Fonts->TexID)
        {
            // get data
            unsigned char* pixels;
            int width, height, bytes_per_pixel;
            io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytes_per_pixel);

            base::image::ImageView sourceView(base::image::NATIVE_LAYOUT, base::image::PixelFormat::Uint8_Norm, bytes_per_pixel, pixels, width, height);
            auto canvasImage = base::CreateSharedPtr<base::image::Image>(sourceView);

            auto canvasImageId = IconRegistry::GetInstance().registerImage(canvasImage);
            io.Fonts->SetTexID(canvasImageId);
        }
    }

    //--

} // ImGui

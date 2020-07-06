/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: imgui #]
***/

#pragma once

namespace base
{
    namespace input
    {
        class BaseEvent;
    }
    namespace canvas
    {
        class Canvas;
    }
}

// BoomerEngine ImGui extensions
namespace ImGui
{
    //--

    // BoomerEngine integration - convert engine input event to ImGui input
    // Returns true if ImGui wants to "eat" this particular event
    IMGUI_API bool ProcessInputEvent(ImGuiContext* ctx, const base::input::BaseEvent& evt);

    // Being frame with BoomerEngine canvas
    IMGUI_API void BeginCanvasFrame(base::canvas::Canvas& c);

    // End frame and send it to canvas for rendering
    IMGUI_API void EndCanvasFrame(base::canvas::Canvas& c);

    //--
    // Internal helpers: called by functions above

    // BoomerEngine integration - push textures
    IMGUI_API void PrepareCanvasImages(ImGuiIO& io);

    // BoomerEngine integration - render collected content on canvas
    // NOTE: since it's a normal canvas operation any way additional transformation and alpha value may be provided
    // NOTE: setting transform to NULL will skip vertex transform
    IMGUI_API void RenderToCanvas(const ImDrawData* data, base::canvas::Canvas& c);

    //--

    // Add a folder to the ImGui icon search path, default is "engine/data/icons/"
    IMGUI_API void AddIconSearchPath(base::StringView<char> path);

    // Load ImGui icon by name, cached
    #undef LoadIcon // Window
    IMGUI_API ImTextureID LoadIcon(base::StringView<char> name);

    // Register image
    IMGUI_API ImTextureID RegisterImage(const base::image::ImagePtr& image);

    // Image wit automatic size
    IMGUI_API void Image(ImTextureID user_texture_id, const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));
    IMGUI_API bool ImageButton(ImTextureID user_texture_id, int frame_padding = -1, const ImVec4& bg_col = ImVec4(0, 0, 0, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));
    IMGUI_API bool ImageToggle(ImTextureID user_texture_id, bool* state, int frame_padding = -1, bool enabled=true);
    IMGUI_API bool ImageToggle(ImTextureID user_texture_id_off, ImTextureID user_texture_id_on, bool* state, int frame_padding = -1, bool enabled = true);

    // Draw image at cursor pos
    IMGUI_API ImVec2 DrawImage(ImTextureID user_texture_id, float ox = 0.0f, float oy = 0.0f, const ImVec4& tint_col = ImVec4(1, 1, 1, 1));

    // Toggle button
    IMGUI_API void ToggleButton(const char* str_id, bool* v);

    //--

    enum class Font : uint8_t
    {
        Default,
        Italic,
        Bold,
        Symbols,
        MonoType,
    };

    IMGUI_API ImFontAtlas* GetSharedFontAtlas();
    IMGUI_API ImFont* GetFont(Font font);

    //--

    IMGUI_API bool IsKeyDown(const base::input::KeyCode code);
    IMGUI_API bool IsKeyPressed(const base::input::KeyCode code, bool ctrl = false, bool shift = false, bool alt = false);
    IMGUI_API bool IsKeyReleased(const base::input::KeyCode code);

    //--

    enum class ButtonCategory : uint8_t
    {
        Normal,
        Destructive, // red, something will be deleted/removed/destroyed
        Constructive, // green, something will be created/accepted
        Editor, // blue, "open editor"
    };

    enum class ButtonIcon : uint8_t
    {
        Yes,
        No,
        Cancel,
        Accept,
        Add,
        Delete,
    };

    IMGUI_API bool Button(ButtonCategory cat, const char* label, const ImVec2& size = ImVec2(0, 0));
    IMGUI_API bool Button(ButtonIcon cat, const char* label=nullptr, const ImVec2& size = ImVec2(0, 0));

    static inline const ImVec4 COLOR_DESTRUCTIVE_BACKGROUND = ImVec4(0.29f, 0.07f, 0.07f, 1.00f);

    //--

}
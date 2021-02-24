/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: imgui #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base::input)
class BaseEvent;
END_BOOMER_NAMESPACE(base::input)

BEGIN_BOOMER_NAMESPACE(base::canvas)
class Canvas;
END_BOOMER_NAMESPACE(base::canvas)

#undef LoadIcon // Window

// BoomerEngine ImGui extensions
namespace ImGui
{
	//--

	/// canvas integration for ImGui
	class IMGUI_API ImGUICanvasHelper : public base::NoCopy
	{
	public:
		ImGUICanvasHelper();
		~ImGUICanvasHelper();

		//--

		INLINE ImGuiContext* context() const { return m_context; }

		//--

		// BoomerEngine integration - convert engine input event to ImGui input
		// Returns true if ImGui wants to "eat" this particular event
		bool processInput(const base::input::BaseEvent& evt);

		// being ImGui frame with BoomerEngine canvas
		void beginFrame(base::canvas::Canvas& c, float dt);

		// end frame and send it to canvas for rendering
		void endFrame(base::canvas::Canvas& c, const base::XForm2D& placement = base::XForm2D::IDENTITY());

		//--

		// Add a folder to the ImGui icon search path, default is "engine/data/icons/"
		void addIconSearchPath(base::StringView path);

		// Load ImGui icon by name, cached
		ImTextureID loadIcon(base::StringView name);

		// Register image
		ImTextureID registerImage(const base::image::ImagePtr& image);

		//--

	private:
		ImGuiContext* m_context = nullptr;

		base::canvas::DynamicAtlasPtr m_atlas;

		base::Array<base::StringBuf> m_searchPaths;
		base::HashMap<base::StringBuf, ImTextureID> m_iconMap;

		//--

		struct ImageUVRange
		{
			base::Vector2 uvMin;
			base::Vector2 uvScale;
		};

		base::Array<ImageUVRange> m_imageUVRanges;

		//--

		void renderToCanvas(const ImDrawData* data, base::canvas::Canvas& c, const base::XForm2D& placement);
		void prepareCanvasImages();
	};

    //--

    /*// Image wit automatic size
    IMGUI_API void Image(ImTextureID user_texture_id, const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));
    IMGUI_API bool ImageButton(ImTextureID user_texture_id, int frame_padding = -1, const ImVec4& bg_col = ImVec4(0, 0, 0, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));
    IMGUI_API bool ImageToggle(ImTextureID user_texture_id, bool* state, int frame_padding = -1, bool enabled=true);
    IMGUI_API bool ImageToggle(ImTextureID user_texture_id_off, ImTextureID user_texture_id_on, bool* state, int frame_padding = -1, bool enabled = true);

    // Draw image at cursor pos
    IMGUI_API ImVec2 DrawImage(ImTextureID user_texture_id, float ox = 0.0f, float oy = 0.0f, const ImVec4& tint_col = ImVec4(1, 1, 1, 1));*/

    // Toggle button
    IMGUI_API void ToggleButton(const char* str_id, bool* v);

	// Key states
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
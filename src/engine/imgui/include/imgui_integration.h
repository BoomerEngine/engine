/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: imgui #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()
class InputEvent;
END_BOOMER_NAMESPACE()

BEGIN_BOOMER_NAMESPACE_EX(canvas)
class Canvas;
END_BOOMER_NAMESPACE_EX(canvas)

#undef LoadIcon // Window

namespace ImGui
{

	//--

	/// canvas integration for ImGui
	class ENGINE_IMGUI_API ImGUICanvasHelper : public boomer::NoCopy
	{
	public:
		ImGUICanvasHelper();
		~ImGUICanvasHelper();

		//--

		INLINE ImGuiContext* context() const { return m_context; }

		//--

		// BoomerEngine integration - convert engine input event to ImGui input
		// Returns true if ImGui wants to "eat" this particular event
		bool processInput(const boomer::InputEvent& evt);

		// being ImGui frame with BoomerEngine canvas
		void beginFrame(boomer::canvas::Canvas& c, float dt);

		// end frame and send it to canvas for rendering
		void endFrame(boomer::canvas::Canvas& c, const boomer::XForm2D& placement = boomer::XForm2D::IDENTITY());

		//--

		// Add a folder to the ImGui icon search path, default is "engine/data/icons/"
		void addIconSearchPath(boomer::StringView path);

		// Load ImGui icon by name, cached
		ImTextureID loadIcon(boomer::StringView name);

		// Register image
		ImTextureID registerImage(const boomer::ImagePtr& image);

		//--

	private:
		ImGuiContext* m_context = nullptr;

		boomer::canvas::DynamicAtlasPtr m_atlas;

		boomer::Array<boomer::StringBuf> m_searchPaths;
		boomer::HashMap<boomer::StringBuf, ImTextureID> m_iconMap;

		//--

		struct ImageUVRange
		{
			boomer::Vector2 uvMin;
			boomer::Vector2 uvScale;
		};

		boomer::Array<ImageUVRange> m_imageUVRanges;

		//--

		void renderToCanvas(const ImDrawData* data, boomer::canvas::Canvas& c, const boomer::XForm2D& placement);
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
	IMGUI_API bool IsKeyDown(const boomer::InputKey code);
	IMGUI_API bool IsKeyPressed(const boomer::InputKey code, bool ctrl = false, bool shift = false, bool alt = false);
	IMGUI_API bool IsKeyReleased(const boomer::InputKey code);

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
	IMGUI_API bool Button(ButtonIcon cat, const char* label = nullptr, const ImVec2& size = ImVec2(0, 0));

	static inline const ImVec4 COLOR_DESTRUCTIVE_BACKGROUND = ImVec4(0.29f, 0.07f, 0.07f, 1.00f);

	//--

}
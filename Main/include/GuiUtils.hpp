#pragma once
#include "ApplicationTickable.hpp"
#include "GameConfig.hpp"
#include "Input.hpp"
#include <SDL2/SDL.h>

// NK imports
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#undef NK_IMPLEMENTATION
#ifdef EMBEDDED
#undef NK_SDL_GLES2_IMPLEMENTATION
#include "../third_party/nuklear/nuklear.h"
#include "nuklear/nuklear_sdl_gles2.h"
#else
#undef	NK_SDL_GL3_IMPLEMENTATION
#include "../third_party/nuklear/nuklear.h"
#include "nuklear/nuklear_sdl_gl3.h"
#endif

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024
#define FULL_FONT_TEXTURE_HEIGHT 32768 //needed to load all CJK glyphs

class BasicNuklearGui : public IApplicationTickable
{
public:
	BasicNuklearGui() : m_nctx() {};
	~BasicNuklearGui();
	bool Init() override;
	void Tick(float deltaTime) override;
	void Render(float deltaTime) override;
	void NKRender();
	void UpdateNuklearInput(SDL_Event evt);
    void ShutdownNuklear();
    void InitNuklearIfNeeded();
	virtual bool OnKeyPressedConsume(SDL_Scancode code) { return m_isOpen; };

protected:
	bool m_nuklearRunning = false;
	struct nk_context* m_nctx = NULL;
	std::queue<SDL_Event> m_eventQueue;
	// Are we consuming text
	bool m_isOpen = true;
	// Background screenshot
	bool m_backgroundFrame = true;
	Texture m_fromTexture;
	Mesh m_bgMesh;
};

class BasicWindow : public BasicNuklearGui
{
public:
	BasicWindow(String name) : m_name(name) {};
	void Tick(float deltaTime) override;
	void Render(float deltaTime) override;
	bool OnKeyPressedConsume(SDL_Scancode code) override;

	void EnableInputForEdit(int editWidth, int editHeight);
	void Close();

	virtual void DrawWindow() {};
	virtual void OnClose() {};

protected:
	// Configurable
	int m_windowFlag = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_TITLE | NK_WINDOW_CLOSABLE;
	String m_name;
	int m_width = 400;
	int m_height = 200;

	bool m_inEdit = false;
	bool m_isFocused = false;
};

bool nk_edit_isfocused(struct nk_context* ctx);

class BasicPrompt : public BasicWindow {
public:
	BasicPrompt(String title, String body, String submitText = "Submit")
		: BasicWindow(title), m_text(body), m_submitText(submitText) { };
	bool Init() override;
	virtual bool OnKeyPressedConsume(SDL_Scancode code) override;
	void DrawWindow() override;
	void OnClose() override;

	void Focus() { m_forceFocus = true; }

	Delegate<bool, char*> OnResult;

protected:
	String m_text;
	String m_submitText;
	char m_data[255] = { 0 };
	bool m_submitted = false;
	bool m_forceFocus = false;
	bool m_shrinkWindow = true;
};

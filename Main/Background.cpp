#include "stdafx.h"
#include "Background.hpp"
#include "Application.hpp"
#include "GameConfig.hpp"
#include "Background.hpp"
#include "Game.hpp"
#include "Track.hpp"
#include "Camera.hpp"
#include "lua.hpp"
#include "Shared/LuaBindable.hpp"

/* Background template for fullscreen effects */
class FullscreenBackground : public Background
{
public:
	~FullscreenBackground()
	{
		if (bindable)
		{
			delete bindable;
			bindable = nullptr;
		}
		if (lua)
		{
			lua_close(lua);
			lua = nullptr;
		}
	}

	virtual bool Init(bool foreground) override
	{
		fullscreenMesh = MeshGenerators::Quad(g_gl, Vector2(-1.0f), Vector2(2.0f));
		this->foreground = foreground;
		return true;
	}
	void UpdateRenderState(float deltaTime)
	{
		renderState = g_application->GetRenderStateBase();
	}
	virtual void Render(float deltaTime) override
	{
		assert(fullscreenMaterial);

		// Render a fullscreen quad
		RenderQueue rq(g_gl, renderState);
		rq.Draw(Transform(), fullscreenMesh, fullscreenMaterial, fullscreenMaterialParams);
		rq.Process();
	}

	RenderState renderState;
	Mesh fullscreenMesh;
	Material fullscreenMaterial;
	Map<String, Texture> textures;
	Texture frameBufferTexture;
	MaterialParameterSet fullscreenMaterialParams;
	float clearTransition = 0.0f;
	float offsyncTimer = 0.0f;
	float speedMult = 1.0f;
	bool foreground;
	bool hasFbTex;
	bool errored = false;
	Vector<String> defaultBGs;
	LuaBindable *bindable = nullptr;
	String folderPath;
	lua_State *lua = nullptr;
	Vector3 timing;
	Vector2 tilt;
};

class TestBackground : public FullscreenBackground
{
	virtual bool Init(bool foreground) override
	{
		if (!FullscreenBackground::Init(foreground))
			return false;

		defaultBGs = Path::GetSubDirs(Path::Normalize(
			Path::Absolute("skins/" + g_application->GetCurrentSkin() + "/backgrounds/")));

		String skin = g_gameConfig.GetString(GameConfigKeys::Skin);
		lua = luaL_newstate();

		auto openLib = [this](char* name, lua_CFunction lib)
		{
			luaL_requiref(lua, name, lib, 1);
			lua_pop(lua, 1);
		};

		//open libs
		//TODO: not sure which should be included
		openLib("_G", luaopen_base);
		openLib(LUA_LOADLIBNAME, luaopen_package);
		openLib(LUA_TABLIBNAME, luaopen_table);
		openLib(LUA_STRLIBNAME, luaopen_string);
		openLib(LUA_MATHLIBNAME, luaopen_math);

		g_application->SetLuaBindings(lua);
		game->SetInitialGameplayLua(lua);

		String matPath = "";
		String fname = foreground ? "fg" : "bg";
		String bindName = foreground ? "foreground" : "background";
		if (defaultBGs.Contains(game->GetBeatmap()->GetMapSettings().foregroundPath))
		{
			//default bg: load from skin path
			folderPath = "skins/" +
						 g_application->GetCurrentSkin() + Path::sep +
						 "backgrounds" + Path::sep +
						 game->GetBeatmap()->GetMapSettings().foregroundPath +
						 Path::sep;
			folderPath = Path::Absolute(folderPath);
		}
		else
		{
			//if skin doesn't have it, try loading from chart folder
			folderPath = game->GetMapRootPath() + Path::sep +
						 game->GetBeatmap()->GetMapSettings().foregroundPath +
						 Path::sep;
			folderPath = Path::Absolute(folderPath);
		}

		bindable = new LuaBindable(lua, bindName);
		bindable->AddFunction("LoadTexture", this, &TestBackground::LoadTexture);
		bindable->AddFunction("SetParami", this, &TestBackground::SetParami);
		bindable->AddFunction("SetParamf", this, &TestBackground::SetParamf);
		bindable->AddFunction("DrawShader", this, &TestBackground::DrawShader);
		bindable->AddFunction("GetPath", this, &TestBackground::GetPath);
		bindable->AddFunction("SetSpeedMult", this, &TestBackground::SetSpeedMult);
		bindable->AddFunction("GetTiming", this, &TestBackground::GetTiming);
		bindable->AddFunction("GetTilt", this, &TestBackground::GetTilt);
		bindable->AddFunction("GetScreenCenter", this, &TestBackground::GetScreenCenter);
		bindable->AddFunction("GetClearTransition", this, &TestBackground::GetClearTransition);
		bindable->Push();
		lua_settop(lua, 0);
		if (luaL_dofile(lua, Path::Normalize(folderPath + fname + ".lua").c_str()))
		{
			Logf("Lua error: %s", Logger::Error, lua_tostring(lua, -1));
			return false;
		}
		matPath = folderPath + fname + ".fs";

		CheckedLoad(fullscreenMaterial = LoadBackgroundMaterial(matPath));
		fullscreenMaterial->opaque = false;
		hasFbTex = fullscreenMaterial->HasUniform("fb_tex");
		return true;
	}
	virtual void Render(float deltaTime) override
	{
		if (errored)
			return;
		UpdateRenderState(deltaTime);
		game->SetGameplayLua(lua);
		const TimingPoint &tp = game->GetPlayback().GetCurrentTimingPoint();
		timing.x = game->GetPlayback().GetBeatTime();
		timing.z = game->GetPlayback().GetLastTime() / 1000.0f;
		offsyncTimer += (speedMult * deltaTime / tp.beatDuration) * 1000.0 * game->GetPlaybackSpeed();
		offsyncTimer = fmodf(offsyncTimer, 1.0f);
		timing.y = offsyncTimer;

		float clearBorder = 0.70f;
		if ((game->GetFlags() & GameFlags::Hard) != GameFlags::None)
		{
			clearBorder = 0.30f;
		}

		bool cleared = game->GetScoring().currentGauge >= clearBorder;

		if (cleared)
			clearTransition += deltaTime / tp.beatDuration * 1000;
		else
			clearTransition -= deltaTime / tp.beatDuration * 1000;

		clearTransition = Math::Clamp(clearTransition, 0.0f, 1.0f);

		Vector3 trackEndWorld = Vector3(0.0f, 25.0f, 0.0f);
		Vector2i screenCenter = game->GetCamera().GetScreenCenter();

		tilt = { game->GetCamera().GetActualRoll(), game->GetCamera().GetBackgroundSpin() };
		fullscreenMaterialParams.SetParameter("clearTransition", clearTransition);
		fullscreenMaterialParams.SetParameter("tilt", tilt);
		fullscreenMaterialParams.SetParameter("screenCenter", screenCenter);
		fullscreenMaterialParams.SetParameter("timing", timing);
		if (foreground && hasFbTex)
		{
			frameBufferTexture->SetFromFrameBuffer();
			fullscreenMaterialParams.SetParameter("fb_tex", frameBufferTexture);
		}

		if (foreground)
			lua_getglobal(lua, "render_fg");
		else
			lua_getglobal(lua, "render_bg");

		if (lua_isfunction(lua, -1))
		{
			lua_pushnumber(lua, deltaTime);
			if (lua_pcall(lua, 1, 0, 0) != 0)
			{
				Logf("Lua error: %s", Logger::Error, lua_tostring(lua, -1));
				g_gameWindow->ShowMessageBox("Lua Error", lua_tostring(lua, -1), 0);
				errored = true;
			}
		}
		lua_settop(lua, 0);
		g_application->ForceRender();
	}

	int LoadTexture(lua_State *L /*String uniformName, String filename*/)
	{
		String uniformName(luaL_checkstring(L, 2));
		String filename(luaL_checkstring(L, 3));
		filename = Path::Normalize(folderPath + Path::sep + filename);
		textures.Add(uniformName, g_application->LoadTexture(filename, true));
		return 0;
	}

	int GetTiming(lua_State* L)
	{
		lua_pushnumber(L, timing.x);
		lua_pushnumber(L, timing.y);
		lua_pushnumber(L, timing.z);
		return 3;
	}

	int GetTilt(lua_State* L)
	{
		lua_pushnumber(L, tilt.x);
		lua_pushnumber(L, tilt.y);
		return 2;
	}

	int GetScreenCenter(lua_State* L)
	{
		auto c = game->GetCamera().GetScreenCenter();
		lua_pushnumber(L, c.x);
		lua_pushnumber(L, c.y);
		return 2;
	}

	int GetClearTransition(lua_State* L)
	{
		lua_pushnumber(L, clearTransition);
		return 1;
	}

	int SetParami(lua_State *L /*String param, int v*/)
	{
		String param(luaL_checkstring(L, 2));
		int v(luaL_checkinteger(L, 3));
		fullscreenMaterialParams.SetParameter(param, v);
		return 0;
	}

	int SetParamf(lua_State *L /*String param, float v*/)
	{
		String param(luaL_checkstring(L, 2));
		float v(luaL_checknumber(L, 3));
		fullscreenMaterialParams.SetParameter(param, v);
		return 0;
	}
	int DrawShader(lua_State *L)
	{
		for (auto &texParam : textures)
		{
			fullscreenMaterialParams.SetParameter(texParam.first, texParam.second);
		}

		g_application->ForceRender();
		FullscreenBackground::Render(0);
		return 0;
	}
	int SetSpeedMult(lua_State* L)
	{
		speedMult = luaL_checknumber(L, 2);
		return 0;
	}

	int GetPath(lua_State* L)
	{
		lua_pushstring(L, *folderPath);
		return 1;
	}

	Material LoadBackgroundMaterial(const String &path)
	{
		String skin = g_gameConfig.GetString(GameConfigKeys::Skin);
		String pathV = Path::Absolute(String("skins/" + skin + "/shaders/") + "background" + ".vs");
		String pathF = Path::Absolute(path);
		String pathG = Path::Absolute(String("skins/" + skin + "/shaders/") + "background" + ".gs");
		Material ret = MaterialRes::Create(g_gl, pathV, pathF);
		// Additionally load geometry shader
		if (Path::FileExists(pathG))
		{
			Shader gshader = ShaderRes::Create(g_gl, ShaderType::Geometry, pathG);
			assert(gshader);
			ret->AssignShader(ShaderType::Geometry, gshader);
		}
		return ret;
	}

	Texture LoadBackgroundTexture(const String &path)
	{
		Texture ret = TextureRes::Create(g_gl, ImageRes::Create(path));
		return ret;
	}
};

Background *CreateBackground(class Game *game, bool foreground /* = false*/)
{
	Background *bg = new TestBackground();
	bg->game = game;
	if (!bg->Init(foreground))
	{
		delete bg;
		return nullptr;
	}
	return bg;
}
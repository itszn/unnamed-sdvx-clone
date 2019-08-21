#include "stdafx.h"
#include "MultiplayerScreen.hpp"
#include "Application.hpp"
#include "lua.hpp"
#include "archive.h"
#include "archive_entry.h"
#include "SkinHttp.hpp"
#include "GameConfig.hpp"
#include "cpr/util.h"
#include "SongSelect.hpp"
#include "SkinConfig.hpp"

#include <ctime>
#include <string>
#include <TransitionScreen.hpp>
#include <Game.hpp>

#define MULTIPLAYER_VERSION "v0.13"

// XXX probably should be moved with the songselect one to its own class file?
class TextInput
{
public:
	WString input;
	WString composition;
	uint32 backspaceCount;
	bool active = false;
	Delegate<const WString&> OnTextChanged;
	bool start_taking_input = false;

	~TextInput()
	{
		g_gameWindow->OnTextInput.RemoveAll(this);
		g_gameWindow->OnTextComposition.RemoveAll(this);
		g_gameWindow->OnKeyRepeat.RemoveAll(this);
		g_gameWindow->OnKeyPressed.RemoveAll(this);
	}

	void OnTextInput(const WString& wstr)
	{
		if (!start_taking_input)
			return;
		input += wstr;
		OnTextChanged.Call(input);
	}
	void OnTextComposition(const Graphics::TextComposition& comp)
	{
		composition = comp.composition;
	}
	void OnKeyRepeat(int32 key)
	{
		if (key == SDLK_BACKSPACE)
		{
			if (input.empty())
				backspaceCount++; // Send backspace
			else
			{
				auto it = input.end(); // Modify input string instead
				--it;
				input.erase(it);
				OnTextChanged.Call(input);
			}
		}
	}
	void OnKeyPressed(int32 key)
	{
		if (key == SDLK_v)
		{
			if (g_gameWindow->GetModifierKeys() == ModifierKeys::Ctrl)
			{
				if (g_gameWindow->GetTextComposition().composition.empty())
				{
					// Paste clipboard text into input buffer
					input += g_gameWindow->GetClipboard();
				}
			}
		}
	}
	void SetActive(bool state)
	{
		active = state;
		if (state)
		{
			start_taking_input = false;

			SDL_StartTextInput();
			g_gameWindow->OnTextInput.Add(this, &TextInput::OnTextInput);
			g_gameWindow->OnTextComposition.Add(this, &TextInput::OnTextComposition);
			g_gameWindow->OnKeyRepeat.Add(this, &TextInput::OnKeyRepeat);
			g_gameWindow->OnKeyPressed.Add(this, &TextInput::OnKeyPressed);
		}
		else
		{
			SDL_StopTextInput();
			g_gameWindow->OnTextInput.RemoveAll(this);
			g_gameWindow->OnTextComposition.RemoveAll(this);
			g_gameWindow->OnKeyRepeat.RemoveAll(this);
			g_gameWindow->OnKeyPressed.RemoveAll(this);
		}
	}
	void Reset()
	{
		backspaceCount = 0;
		input.clear();
	}
	void Tick()
	{
		// Wait until we release the start button
		if (active && !start_taking_input && !g_input.GetButton(Input::Button::BT_S))
		{
			start_taking_input = true;
		}
	}
};

MultiplayerScreen::MultiplayerScreen()
{
}

MultiplayerScreen::~MultiplayerScreen()
{
	g_input.OnButtonPressed.RemoveAll(this);
	g_input.OnButtonReleased.RemoveAll(this);
	g_gameWindow->OnMouseScroll.RemoveAll(this);
	g_gameWindow->OnMousePressed.RemoveAll(this);
	m_mapDatabase.OnMapsCleared.Clear();
	if (m_lua)
	{
		g_application->DisposeLua(m_lua);
		m_tcp.ClearState(m_lua);
	}
}

bool MultiplayerScreen::Init()
{
	m_lua = g_application->LoadScript("multiplayerscreen");
	if (m_lua == nullptr)
		return false;


	g_input.OnButtonPressed.Add(this, &MultiplayerScreen::m_OnButtonPressed);
	g_input.OnButtonReleased.Add(this, &MultiplayerScreen::m_OnButtonReleased);
	g_gameWindow->OnMouseScroll.Add(this, &MultiplayerScreen::m_OnMouseScroll);
	g_gameWindow->OnMousePressed.Add(this, &MultiplayerScreen::MousePressed);

	m_bindable = new LuaBindable(m_lua, "mpScreen");
	m_bindable->AddFunction("Exit", this, &MultiplayerScreen::lExit);
	m_bindable->AddFunction("SelectSong", this, &MultiplayerScreen::lSongSelect);
	m_bindable->AddFunction("JoinWithPassword", this, &MultiplayerScreen::lJoinWithPassword);
	m_bindable->AddFunction("JoinWithoutPassword", this, &MultiplayerScreen::lJoinWithoutPassword);
	m_bindable->AddFunction("NewRoomStep", this, &MultiplayerScreen::lNewRoomStep);
	m_bindable->AddFunction("SaveUsername", this, &MultiplayerScreen::lSaveUsername);
	
	m_bindable->Push();
	lua_settop(m_lua, 0);

	lua_pushstring(m_lua, MULTIPLAYER_VERSION);
	lua_setglobal(m_lua, "MULTIPLAYER_VERSION");

	m_screenState = MultiplayerScreenState::ROOM_LIST;
	lua_pushstring(m_lua, "roomList");
	lua_setglobal(m_lua, "screenState");
	
	// Start the map database
	m_mapDatabase.AddSearchPath(g_gameConfig.GetString(GameConfigKeys::SongFolder));
	m_mapDatabase.StartSearching();

	// Install the socket functions and call the lua init
	m_tcp.PushFunctions(m_lua);

	// Add a handler for some socket events
	m_tcp.SetTopicHandler("game.started", this, &MultiplayerScreen::m_handleStartPacket);
	m_tcp.SetTopicHandler("game.sync.start", this, &MultiplayerScreen::m_handleSyncStartPacket);
	m_tcp.SetTopicHandler("server.info", this, &MultiplayerScreen::m_handleAuthResponse);
	m_tcp.SetTopicHandler("server.rooms", this, &MultiplayerScreen::m_handleRoomList);
	m_tcp.SetTopicHandler("room.update", this, &MultiplayerScreen::m_handleRoomUpdate);
	m_tcp.SetTopicHandler("server.room.joined", this, &MultiplayerScreen::m_handleJoinRoom);
	m_tcp.SetTopicHandler("server.error", this, &MultiplayerScreen::m_handleError);
	m_tcp.SetTopicHandler("server.room.badpassword", this, &MultiplayerScreen::m_handleBadPassword);
	m_tcp.SetTopicHandler("game.allfailed", this, &MultiplayerScreen::m_handleAllFail);

	m_tcp.SetCloseHandler(this, &MultiplayerScreen::m_handleSocketClose);

	// TODO(itszn) better method for entering server and port
	String host = g_gameConfig.GetString(GameConfigKeys::MultiplayerHost);
	m_tcp.Connect(host);

	m_textInput = Ref<TextInput>(new TextInput());

	m_userName = g_gameConfig.GetString(GameConfigKeys::MultiplayerUsername);
	if (m_userName == "")
	{
		m_screenState = MultiplayerScreenState::SET_USERNAME;
		lua_pushstring(m_lua, "setUsername");
		lua_setglobal(m_lua, "screenState");
		m_textInput->Reset();
		m_textInput->SetActive(true);
	}
	else
	{
		m_authenticate();
	}

    return true;
}

void MultiplayerScreen::JoinRoomWithToken(String token)
{
	m_joinToken = token;
	// Already authed
	if (m_userId != "")
		return m_joinRoomWithToken();
}

void MultiplayerScreen::m_joinRoomWithToken()
{
	nlohmann::json joinReq;
	joinReq["topic"] = "server.room.join";
	joinReq["token"] = m_joinToken;
	m_tcp.SendJSON(joinReq);
}

void MultiplayerScreen::m_authenticate()
{
	if (m_textInput->active)
		m_textInput->SetActive(false);

	String password = g_gameConfig.GetString(GameConfigKeys::MultiplayerPassword);

	if (m_userName == "")
		m_userName = "Guest";

	nlohmann::json packet;
	packet["topic"] = "user.auth";
	packet["password"] = password;
	packet["name"] = m_userName;
	packet["version"] = MULTIPLAYER_VERSION;
	m_tcp.SendJSON(packet);
}

void MultiplayerScreen::m_handleSocketClose()
{
	// Don't exit if we are in game or selection
	if (m_suspended)
		return;
	g_application->RemoveTickable(this);
}

bool MultiplayerScreen::m_handleAllFail(nlohmann::json& packet)
{
	m_allFail = true;
	return true;
}

bool MultiplayerScreen::m_handleBadPassword(nlohmann::json& packet)
{
	lua_pushboolean(m_lua, true);
	lua_setglobal(m_lua, "passwordError");

	lua_pushnumber(m_lua, 1.0f);
	lua_setglobal(m_lua, "passwordErrorOffset");
	return true;
}

bool MultiplayerScreen::m_handleRoomList(nlohmann::json& packet)
{
	g_application->DiscordPresenceMenu("Browsing multiplayer rooms");
	return true;
}

bool MultiplayerScreen::m_handleJoinRoom(nlohmann::json& packet)
{
	if (m_textInput->active)
		m_textInput->SetActive(false);

	m_screenState = MultiplayerScreenState::IN_ROOM;
	lua_pushstring(m_lua, "inRoom");
	lua_setglobal(m_lua, "screenState");
	m_roomId = static_cast<String>(packet["room"]["id"]);
	m_joinToken = static_cast<String>(packet["room"]["join_token"]);
	g_application->DiscordPresenceMulti(m_joinToken, 1, 10);
	return true;
}

bool MultiplayerScreen::m_handleError(nlohmann::json& packet)
{
	g_gameWindow->ShowMessageBox("Multiplayer server closed", packet.value("error", ""), 0);
	
	// Fatal error, so leave this view
	m_suspended = true;
	g_application->RemoveTickable(this);
	return true;
}

// Save the unique user id the server assigns us
bool MultiplayerScreen::m_handleAuthResponse(nlohmann::json& packet)
{
	double server_version = atof(static_cast<String>(packet.value("version", "0.0")).c_str()+1);
	if (server_version < 0.12)
	{
		g_gameWindow->ShowMessageBox("Multiplayer server closed", "This version of multiplayer (" MULTIPLAYER_VERSION ") does not support this server", 0);
		// Fatal error, so leave this view
		m_suspended = true;
		g_application->RemoveTickable(this);
		return false;
	}

	g_application->DiscordPresenceMenu("Browsing multiplayer rooms");
	m_userId = static_cast<String>(packet["userid"]);
	m_scoreInterval = packet.value("refresh_rate",1000);

	// If we are waiting to join a room, join now
	if (m_joinToken != "")
		m_joinRoomWithToken();
	return true;
}

bool MultiplayerScreen::m_handleRoomUpdate(nlohmann::json& packet)
{
	int userCount = packet.at("users").size();
	m_joinToken = packet.value("join_token", "");
	g_application->DiscordPresenceMulti(m_joinToken, userCount, 10);
	m_handleSongChange(packet);
	return true;
}
bool MultiplayerScreen::m_handleSongChange(nlohmann::json& packet)
{
	// Case for no song selected yet
	if (packet["song"].is_null())
		return true;

	// Case for same song as before
	if (m_hasSelectedMap && packet["song"] == m_selectedMapShortPath)
		return true;

	// Clear jacket variable to force image reload
	lua_pushinteger(m_lua, 0);
	lua_setglobal(m_lua, "jacket");

	// Grab new song
	uint32 diff_ind = packet["diff"];
	DifficultyIndex* new_diff = m_getMapByShortPath(packet["song"], &diff_ind, packet["level"]);

	if (new_diff == nullptr)
	{
		// If we don't find it, then update lua state
		m_hasSelectedMap = false;
		m_clearLuaMap();

		// Tell the server we don't have the map
		// TODO(itszn) send less frequently?
		nlohmann::json packet;
		packet["topic"] = "user.nomap";
		m_tcp.SendJSON(packet);

		return true;
	}

	m_selfPicked = false;
	m_updateSelectedMap(new_diff->mapId, diff_ind, false);

	return true;
}

// Start the game
bool MultiplayerScreen::m_handleStartPacket(nlohmann::json& packet)
{
	if (!this->m_hasSelectedMap)
		return false;

	m_inGame = true;
	m_failed = false;
	m_allFail = false;
	m_syncState = SyncState::LOADING;

	// Grab the map from the database
	MapIndex* map = m_mapDatabase.GetMap(m_selectedMapId);
	DifficultyIndex* diff = map->difficulties[m_selectedDiffIndex];

	// Reset score time before playing
	m_lastScoreSent = 0;

	Logf("[Multiplayer] Starting game: diff_id=%d mapId=%d path=%s", Logger::Info, diff->id, diff->mapId, diff->path.c_str());

	// The server tells us if we are playing excessive or not
	bool is_hard = packet["hard"];
	bool is_mirror = packet.value("mirror", false);

	GameFlags flags;
	if (is_hard)
		flags = GameFlags::Hard;
	else
		flags = GameFlags::None;
	if (is_mirror)
		flags = flags | GameFlags::Mirror;

	// Create the game using the Create that takes the MultiplayerScreen class
	Game* game = Game::Create(this, *(diff), flags);
	if (!game)
	{
		Logf("Failed to start game", Logger::Error);
		return 0;
	}

	m_suspended = true;

	// Switch to the new tickable
	TransitionScreen* transistion = TransitionScreen::Create(game);
	g_application->AddTickable(transistion);
	return false;
}


bool MultiplayerScreen::m_handleSyncStartPacket(nlohmann::json& packet)
{
	m_syncState = SyncState::SYNCED;
	return true;
}

// Get a map from a given "short" path
DifficultyIndex* MultiplayerScreen::m_getMapByShortPath(const String& path, uint32* diff_ind, int32 level)
{
	Logf("[Multiplayer] looking up song '%s' difficulty index %u", Logger::Info, path.c_str(), *diff_ind);
	for (auto map : m_mapDatabase.FindMaps(path))
	{
		// No haxing pls
		if (map.second->difficulties.size() <= *diff_ind)
		{
			if (level != 0 || map.second->difficulties.size() == 0)
			{
				Logf("[Multiplayer] Difficulty out of range!", Logger::Warning);
				continue;
			}
			// If we couldn't find the exact song, just take any diff
			*diff_ind = 0;
		}

		// Grab the correct map and diff from the database
		DifficultyIndex* new_diff = map.second->difficulties[*diff_ind];

		// Check if this is the right map (the diff level matches up)
		if (level != 0 && new_diff->settings.level != level)
		{
			continue;
		}
		Logf("[Multiplayer] Found: diff_id=%d mapid=%d path=%s", Logger::Info, new_diff->id, new_diff->mapId, new_diff->path.c_str());
		return new_diff;
	}

	if (level != 0)
	{
		// We couldn't find an exact match, try to grab whatever song matches
		return m_getMapByShortPath(path, diff_ind, 0);
	}

	Logf("[Multiplayer] Could not find song", Logger::Warning);
	return nullptr;
}

// Set the selected map to a given map and difficulty (used by SongSelect)
void MultiplayerScreen::SetSelectedMap(MapIndex* map, DifficultyIndex* diff)
{
	const SongSelectIndex song(map, diff);



	// Get the "short" path (basically the last part of the path)
	const size_t last_slash_idx = song.GetMap()->path.find_last_of("\\/");
	std::string short_path = song.GetMap()->path.substr(last_slash_idx + 1);

	// Get the difficulty index into the selected map
	uint32 diff_index = (song.id % 10) - 1;

	// Get the actual map id
	const DifficultyIndex* new_diff = m_getMapByShortPath(short_path, &diff_index, diff->settings.level);

	if (new_diff == nullptr)
	{
		// Somehow we failed the round trip, so we can't use this map
		m_hasSelectedMap = false;
		return;
	}

	m_selfPicked = true;
	m_updateSelectedMap(new_diff->mapId, diff_index, true);

}

void MultiplayerScreen::m_changeSelectedRoom(int offset)
{
	lua_getglobal(m_lua, "change_selected_room");
	if (lua_isfunction(m_lua, -1))
	{
		lua_pushinteger(m_lua, offset);
		if (lua_pcall(m_lua, 1, 0, 0) != 0)
		{
			Logf("Lua error on set_diff: %s", Logger::Error, lua_tostring(m_lua, -1));
			g_gameWindow->ShowMessageBox("Lua Error on set_diff", lua_tostring(m_lua, -1), 0);
			assert(false);
		}
	}
	lua_settop(m_lua, 0);
	
}

void MultiplayerScreen::m_changeDifficulty(int offset)
{
	MapIndex* map = m_mapDatabase.GetMap(this->m_selectedMapId);
	int oldDiff = this->m_selectedDiffIndex;
	int newInd = this->m_selectedDiffIndex + offset;
	if (newInd < 0 || newInd >= map->difficulties.size())
	{
		return;
	}

	m_updateSelectedMap(this->m_selectedMapId, newInd, false);

	lua_getglobal(m_lua, "set_diff");
	if (lua_isfunction(m_lua, -1))
	{
		lua_pushinteger(m_lua, oldDiff + 1);
		lua_pushinteger(m_lua, newInd + 1);
		if (lua_pcall(m_lua, 2, 0, 0) != 0)
		{
			Logf("Lua error on set_diff: %s", Logger::Error, lua_tostring(m_lua, -1));
			g_gameWindow->ShowMessageBox("Lua Error on set_diff", lua_tostring(m_lua, -1), 0);
			assert(false);
		}
	}
	lua_settop(m_lua, 0);
	

}

void MultiplayerScreen::m_updateSelectedMap(int32 mapid, int32 diff_ind, bool isNew)
{
	this->m_selectedMapId = mapid;
	this->m_selectedDiffIndex = diff_ind;

	// Get the current map from the database
	MapIndex* map = m_mapDatabase.GetMap(mapid);
	DifficultyIndex* diff = map->difficulties[diff_ind];
	const BeatmapSettings& mapSettings = diff->settings;

	// Find "short" path for the selected map
	const size_t lastSlashIdx = map->path.find_last_of("\\/");
	m_selectedMapShortPath = map->path.substr(lastSlashIdx + 1);

	// Push a table of info to lua
	lua_newtable(m_lua);
	m_PushStringToTable("title", mapSettings.title.c_str());
	m_PushStringToTable("artist", mapSettings.artist.c_str());
	m_PushStringToTable("bpm", mapSettings.bpm.c_str());
	m_PushIntToTable("id", map->id);
	m_PushStringToTable("path", map->path.c_str());
	m_PushStringToTable("short_path", m_selectedMapShortPath.c_str());

	m_PushStringToTable("jacketPath", Path::Normalize(map->path + "/" + mapSettings.jacketPath).c_str());
	m_PushIntToTable("level", mapSettings.level);
	m_PushIntToTable("difficulty", mapSettings.difficulty);
	m_PushIntToTable("diff_index", diff_ind);
	lua_pushstring(m_lua, "self_picked");
	lua_pushboolean(m_lua, m_selfPicked);
	lua_settable(m_lua, -3);
	m_PushStringToTable("effector", mapSettings.effector.c_str());
	m_PushStringToTable("illustrator", mapSettings.illustrator.c_str());

	int diffIndex = 0;
	lua_pushstring(m_lua, "all_difficulties");
	lua_newtable(m_lua);
	for (auto& diff : map->difficulties)
	{
		lua_pushinteger(m_lua, ++diffIndex);
		lua_newtable(m_lua);
		const BeatmapSettings& diffSettings = diff->settings;
		m_PushIntToTable("level", diffSettings.level);
		m_PushIntToTable("id", diff->id);
		m_PushIntToTable("diff_index", diffIndex-1);
		m_PushIntToTable("difficulty", diffSettings.difficulty);
		lua_settable(m_lua, -3);
	}
	lua_settable(m_lua, -3);

	lua_setglobal(m_lua, "selected_song");

	m_hasSelectedMap = true;

	// If we selected this song ourselves, we have to tell the server about it
	if (isNew)
	{
		
		nlohmann::json packet;
		packet["topic"] = "room.setsong";
		packet["song"] = m_selectedMapShortPath;
		packet["diff"] = diff_ind;
		packet["level"] = mapSettings.level;
		m_tcp.SendJSON(packet);
	}
	else
	{
		nlohmann::json packet;
		packet["topic"] = "user.song.level";
		packet["level"] = mapSettings.level;
		m_tcp.SendJSON(packet);
	}

}

void MultiplayerScreen::m_clearLuaMap()
{
	lua_pushnil(m_lua);
	lua_setglobal(m_lua, "selected_song");
}

void MultiplayerScreen::MousePressed(MouseButton button)
{
	if (IsSuspended())
		return;

	lua_getglobal(m_lua, "mouse_pressed");
	if (lua_isfunction(m_lua, -1))
	{
		lua_pushnumber(m_lua, (int32)button);
		if (lua_pcall(m_lua, 1, 1, 0) != 0)
		{
			Logf("Lua error on mouse_pressed: %s", Logger::Error, lua_tostring(m_lua, -1));
			g_gameWindow->ShowMessageBox("Lua Error on mouse_pressed", lua_tostring(m_lua, -1), 0);
			assert(false);
		}
	}
	lua_settop(m_lua, 0);
}

void MultiplayerScreen::Tick(float deltaTime)
{
	// Tick the tcp socket even if we are suspended
	m_tcp.ProcessSocket();

	if (IsSuspended())
		return;

	m_textInput->Tick();

	lua_newtable(m_lua);
	m_PushStringToTable("text", Utility::ConvertToUTF8(m_textInput->input).c_str());
	lua_setglobal(m_lua, "textInput");

	// Lock mouse to screen when active
	if (m_screenState == MultiplayerScreenState::ROOM_LIST && 
		g_gameConfig.GetEnum<Enum_InputDevice>(GameConfigKeys::LaserInputDevice) == InputDevice::Mouse && g_gameWindow->IsActive())
	{
		if (!m_lockMouse)
			m_lockMouse = g_input.LockMouse();
	}
	else
	{
		if (m_lockMouse)
			m_lockMouse.Release();
		g_gameWindow->SetCursorVisible(true);
	}

	// Change difficulty
	if (m_hasSelectedMap)
	{
		float diff_input = g_input.GetInputLaserDir(0);
		m_advanceDiff += diff_input;
		int advanceDiffActual = (int)Math::Floor(m_advanceDiff * Math::Sign(m_advanceDiff)) * Math::Sign(m_advanceDiff);
		if (advanceDiffActual != 0)
			m_changeDifficulty(advanceDiffActual);
		m_advanceDiff -= advanceDiffActual;
	}

	// Room selection
	if (m_screenState == MultiplayerScreenState::ROOM_LIST)
	{
		float room_input = g_input.GetInputLaserDir(1);
		m_advanceRoom += room_input;
		int advanceRoomActual = (int)Math::Floor(m_advanceRoom * Math::Sign(m_advanceRoom)) * Math::Sign(m_advanceRoom);
		if (advanceRoomActual != 0)
			m_changeSelectedRoom(advanceRoomActual);
		m_advanceRoom -= advanceRoomActual;
	}
}

void MultiplayerScreen::PerformScoreTick(Scoring& scoring)
{
	if (m_failed)
		return;

	int lastScoreIndex = scoring.hitStats.size();
	if (lastScoreIndex == 0)
		return;

	lastScoreIndex--;

	HitStat* lastHit = scoring.hitStats[lastScoreIndex];
	MapTime scoreTimestamp = lastHit->time;

	if (scoreTimestamp <= m_lastScoreSent + m_scoreInterval)
		return;

	m_lastScoreSent = scoreTimestamp;

	uint32 score = scoring.CalculateCurrentScore();

	nlohmann::json packet;
	packet["topic"] = "room.score.update";
	packet["time"] = scoreTimestamp;
	packet["score"] = score;

	m_tcp.SendJSON(packet);
}

void MultiplayerScreen::SendFinalScore(Scoring& scoring, int clearState)
{
	nlohmann::json packet;
	packet["topic"] = "room.score.final";
	packet["score"] = scoring.CalculateCurrentScore();
	packet["combo"] = scoring.maxComboCounter;
	packet["clear"] = clearState;
	m_tcp.SendJSON(packet);

	// In case we exit early
	m_syncState = SyncState::SYNCED;
}

void MultiplayerScreen::Render(float deltaTime)
{
    if (IsSuspended())
		return;

	lua_getglobal(m_lua, "render");
	lua_pushnumber(m_lua, deltaTime);

	if (lua_pcall(m_lua, 1, 0, 0) != 0)
	{
		Logf("Lua error: %s", Logger::Error, lua_tostring(m_lua, -1));
		g_gameWindow->ShowMessageBox("Lua Error in render", lua_tostring(m_lua, -1), 0);
		g_application->RemoveTickable(this);
	}
}

void MultiplayerScreen::OnKeyPressed(int32 key)
{
	if (IsSuspended())
		return;

	lua_getglobal(m_lua, "key_pressed");
	if (lua_isfunction(m_lua, -1))
	{
		lua_pushnumber(m_lua, key);
		if (lua_pcall(m_lua, 1, 0, 0) != 0)
		{
			Logf("Lua error on key_pressed: %s", Logger::Error, lua_tostring(m_lua, -1));
			g_gameWindow->ShowMessageBox("Lua Error on key_pressed", lua_tostring(m_lua, -1), 0);
		}
	}
	lua_settop(m_lua, 0);

	if (key == SDLK_LEFT && m_hasSelectedMap)
	{
		m_changeDifficulty(-1);
	}
	else if (key == SDLK_RIGHT && m_hasSelectedMap)
	{
		m_changeDifficulty(1);
	}
	else if (key == SDLK_UP && m_screenState == MultiplayerScreenState::ROOM_LIST)
	{
		m_changeSelectedRoom(-1);
	}
	else if (key == SDLK_DOWN && m_screenState == MultiplayerScreenState::ROOM_LIST)
	{
		m_changeSelectedRoom(1);
	}
	else if (key == SDLK_ESCAPE)
	{
		if (m_screenState != MultiplayerScreenState::ROOM_LIST)
		{

			// Exiting from name setup
			if (m_userName == "")
			{
				m_suspended = true;
				g_application->RemoveTickable(this);
				return;
			}

			if (m_screenState == MultiplayerScreenState::IN_ROOM)
			{
				nlohmann::json packet;
				packet["topic"] = "room.leave";
				m_tcp.SendJSON(packet);
			}
			else
			{
				nlohmann::json packet;
				packet["topic"] = "server.rooms";
				m_tcp.SendJSON(packet);
			}

			if (m_textInput->active)
				m_textInput->SetActive(false);

			m_screenState = MultiplayerScreenState::ROOM_LIST;
			lua_pushstring(m_lua, "roomList");
			lua_setglobal(m_lua, "screenState");

			m_roomId = "";
			m_hasSelectedMap = false;

		}
	}
	else if (key == SDLK_RETURN)
	{
		if (m_screenState == MultiplayerScreenState::JOIN_PASSWORD) 
		{
			lJoinWithPassword(NULL);
		}
		else if (m_screenState == MultiplayerScreenState::NEW_ROOM_NAME ||
			m_screenState == MultiplayerScreenState::NEW_ROOM_PASSWORD)
		{
			lNewRoomStep(NULL);
		}
		else if (m_screenState == MultiplayerScreenState::SET_USERNAME)
		{
			lSaveUsername(NULL);
		}
	}
}



void MultiplayerScreen::OnKeyReleased(int32 key)
{
	if (IsSuspended())
		return;

	lua_getglobal(m_lua, "key_released");
	if (lua_isfunction(m_lua, -1))
	{
		lua_pushnumber(m_lua, key);
		if (lua_pcall(m_lua, 1, 0, 0) != 0)
		{
			Logf("Lua error on key_released: %s", Logger::Error, lua_tostring(m_lua, -1));
			g_gameWindow->ShowMessageBox("Lua Error on key_released", lua_tostring(m_lua, -1), 0);
		}
	}
	lua_settop(m_lua, 0);
}

void MultiplayerScreen::m_OnButtonPressed(Input::Button buttonCode)
{
	if (IsSuspended())
		return;

	if (g_gameConfig.GetEnum<Enum_InputDevice>(GameConfigKeys::ButtonInputDevice) == InputDevice::Keyboard && m_textInput->active)
		return;

	lua_getglobal(m_lua, "button_pressed");
	if (lua_isfunction(m_lua, -1))
	{
		lua_pushnumber(m_lua, (int32)buttonCode);
		if (lua_pcall(m_lua, 1, 0, 0) != 0)
		{
			Logf("Lua error on button_pressed: %s", Logger::Error, lua_tostring(m_lua, -1));
			g_gameWindow->ShowMessageBox("Lua Error on button_pressed", lua_tostring(m_lua, -1), 0);
		}
	}
	lua_settop(m_lua, 0);
}

void MultiplayerScreen::m_OnButtonReleased(Input::Button buttonCode)
{
	if (IsSuspended())
		return;

	if (g_gameConfig.GetEnum<Enum_InputDevice>(GameConfigKeys::ButtonInputDevice) == InputDevice::Keyboard && m_textInput->active)
		return;

	lua_getglobal(m_lua, "button_released");
	if (lua_isfunction(m_lua, -1))
	{
		lua_pushnumber(m_lua, (int32)buttonCode);
		if (lua_pcall(m_lua, 1, 0, 0) != 0)
		{
			Logf("Lua error on button_released: %s", Logger::Error, lua_tostring(m_lua, -1));
			g_gameWindow->ShowMessageBox("Lua Error on button_released", lua_tostring(m_lua, -1), 0);
		}
	}
	lua_settop(m_lua, 0);
}

void MultiplayerScreen::m_OnMouseScroll(int32 steps)
{
	if (IsSuspended())
		return;

	lua_getglobal(m_lua, "mouse_scroll");
	if (lua_isfunction(m_lua, -1))
	{
		lua_pushnumber(m_lua, steps);
		if (lua_pcall(m_lua, 1, 0, 0) != 0)
		{
			Logf("Lua error on advance_selection: %s", Logger::Error, lua_tostring(m_lua, -1));
			g_gameWindow->ShowMessageBox("Lua Error on mouse_scroll", lua_tostring(m_lua, -1), 0);
		}
	}
	lua_settop(m_lua, 0);
}

int MultiplayerScreen::lExit(lua_State * L)
{
	m_suspended = true;
	g_application->RemoveTickable(this);
	return 0;
}

int MultiplayerScreen::lSongSelect(lua_State* L)
{
	m_suspended = true;
	g_application->AddTickable(SongSelect::Create(this));
	return 0;
}

void MultiplayerScreen::OnRestore()
{
	m_suspended = false;

	// If we disconnected while playing or selecting wait until we get back before exiting
	if (!m_tcp.IsOpen())
	{
		m_suspended = true;
		g_application->RemoveTickable(this);
		return;
	}

	// Retrive the lobby info now that we are out of the game
	if (m_inGame)
	{
		m_inGame = false;
		m_selfPicked = false;
		nlohmann::json packet;
		packet["topic"] = "room.update.get";
		m_tcp.SendJSON(packet);
	}
}

void MultiplayerScreen::OnSuspend()
{
	m_suspended = true;
	m_mapDatabase.StopSearching();

	if (m_lockMouse)
		m_lockMouse.Release();
}

bool MultiplayerScreen::IsSyncing()
{
	return (m_syncState == SyncState::SYNCING);
}

void MultiplayerScreen::StartSync()
{
	m_syncState = SyncState::SYNCING;
	nlohmann::json packet;
	packet["topic"] = "room.sync.ready";
	m_tcp.SendJSON(packet);
}

int MultiplayerScreen::lJoinWithoutPassword(lua_State* L)
{
	m_roomToJoin = luaL_checkstring(L, 2);
	nlohmann::json packet;
	packet["topic"] = "server.room.join";
	packet["id"] = m_roomToJoin;
	m_tcp.SendJSON(packet);
	return 0;
}

int MultiplayerScreen::lSaveUsername(lua_State* L)
{
	if (m_userName != "" || m_textInput->input.length() == 0)
		return 0;

	// Update username
	m_userName = Utility::ConvertToUTF8(m_textInput->input);
	g_gameConfig.Set(GameConfigKeys::MultiplayerUsername, m_userName);
	m_textInput->SetActive(false);

	m_screenState = MultiplayerScreenState::ROOM_LIST;
	lua_pushstring(m_lua, "roomList");
	lua_setglobal(m_lua, "screenState");

	m_authenticate();
	return 0;
}

int MultiplayerScreen::lJoinWithPassword(lua_State* L)
{
	if (m_screenState == MultiplayerScreenState::ROOM_LIST)
	{
		Logf("In screen", Logger::Error);
		m_roomToJoin = luaL_checkstring(L, 2);
		m_textInput->Reset();
		m_textInput->SetActive(true);
		

		m_screenState = MultiplayerScreenState::JOIN_PASSWORD;
		lua_pushstring(m_lua, "passwordScreen");
		lua_setglobal(m_lua, "screenState");

		lua_pushboolean(m_lua, false);
		lua_setglobal(m_lua, "passwordError");
		return 0;
	}

	nlohmann::json packet;
	packet["topic"] = "server.room.join";
	packet["id"] = m_roomToJoin;
	packet["password"] = Utility::ConvertToUTF8(m_textInput->input);
	m_tcp.SendJSON(packet);

	return 0;
}

int MultiplayerScreen::lNewRoomStep(lua_State* L)
{
	if (m_screenState == MultiplayerScreenState::ROOM_LIST)
	{
		m_screenState = MultiplayerScreenState::NEW_ROOM_NAME;
		lua_pushstring(m_lua, "newRoomName");
		lua_setglobal(m_lua, "screenState");

		m_textInput->Reset();
		m_textInput->input = Utility::ConvertToWString(m_userName + "'s Room");
		m_textInput->SetActive(true);
	}
	else if (m_screenState == MultiplayerScreenState::NEW_ROOM_NAME)
	{
		if (m_textInput->input.length() == 0)
		{
			return 0;
		}
		m_newRoomName = Utility::ConvertToUTF8(m_textInput->input);
		m_textInput->Reset();

		m_screenState = MultiplayerScreenState::NEW_ROOM_PASSWORD;
		lua_pushstring(m_lua, "newRoomPassword");
		lua_setglobal(m_lua, "screenState");

	}
	else if (m_screenState == MultiplayerScreenState::NEW_ROOM_PASSWORD)
	{
		nlohmann::json packet;
		packet["topic"] = "server.room.new";
		packet["name"] = m_newRoomName;
		packet["password"] = Utility::ConvertToUTF8(m_textInput->input);
		m_tcp.SendJSON(packet);
		m_textInput->Reset();
	}
	return 0;
}
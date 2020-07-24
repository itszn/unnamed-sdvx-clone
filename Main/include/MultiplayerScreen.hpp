#pragma once
#include "ApplicationTickable.hpp"
#include "Shared/LuaBindable.hpp"
#include "Input.hpp"
#include "Shared/Thread.hpp"
#include "GameConfig.hpp"
#include "cpr/cpr.h"
#include <queue>
#include "Beatmap/MapDatabase.hpp"
#include "json.hpp"
#include "TCPSocket.hpp"
#include <Scoring.hpp>
#include "DBUpdateScreen.hpp"
#include "PreviewPlayer.hpp"

class PreviewPlayer;

enum SyncState {
	LOADING,
	SYNCING,
	SYNCED
};

enum MultiplayerScreenState {
	ROOM_LIST,
	JOIN_PASSWORD,
	IN_ROOM,
	NEW_ROOM_NAME,
	NEW_ROOM_PASSWORD,
	SET_USERNAME,
};

enum MultiplayerDataSyncType {
	BUTTON_PRESS,
	BUTTON_RELEASE,
	LASER_MOVE,
	HITSTAT,
	SCORE,
	SPEED,
	OFFSET,
};

struct MultiplayerData {
	union {
		uint8_t type;
		struct {
			uint8_t type;
			uint8_t padding[3];
			uint32_t time;
			uint8_t padding2[4];
		} timed;
		struct {
			uint8_t type;
			uint8_t padding[3];
			uint32_t time;
			uint32_t index;
		} button;
		struct {
			uint8_t type;
			uint8_t index;
			uint8_t padding[2];
			uint32_t time;
			float val;
		} laser;
		struct {
			uint8_t type;
			uint8_t index;
			uint8_t hit;
			uint8_t padding;
			uint32_t time;
			uint32_t delta;
		} hitstat;
		struct {
			uint8_t type;
			uint8_t padding[3];
			uint32_t time;
			float hispeed;
		} speed;
		struct {
			uint32_t type : 8;
			uint32_t score : 24;
			uint16_t combo;
			uint16_t max_combo;
			float gauge;
		} score;
		struct {
			uint32_t type : 8;
			int32_t chartOffset : 24;
			int32_t globalOffset;
			int32_t inputOffset;
		} offset;
	} t;
};

static_assert(sizeof(struct MultiplayerData) == sizeof(uint32_t) * 3,
	"MultiplayerData structure size is wrong");

class TextInputMultiplayer;
class ChatOverlay;

struct MultiplayerBPMInfo {
	double start;
	double min;
	double max;
	double mode;
};

class MultiplayerScreen : public IAsyncLoadableApplicationTickable
{
public:
	MultiplayerScreen();
	~MultiplayerScreen();

	bool Init() override;
	void Tick(float deltaTime) override;
	void Render(float deltaTime) override;
	void ForceRender(float deltaTime) override;

	void OnKeyPressed(SDL_Scancode code) override;
	void OnKeyReleased(SDL_Scancode code) override;
	void MousePressed(MouseButton button);

	virtual void OnSuspend();
	virtual void OnRestore();
	virtual bool AsyncLoad();
	virtual bool AsyncFinalize();
	void JoinRoomWithToken(String token);

	int lExit(struct lua_State* L);
	int lSongSelect(struct lua_State* L);
	int lSettings(struct lua_State* L);
	int lNewRoomStep(struct lua_State* L);
	int lJoinWithPassword(struct lua_State* L);
	int lJoinWithoutPassword(struct lua_State* L);
	int lSaveUsername(struct lua_State* L);

	void SetSelectedMap(FolderIndex*, ChartIndex*);

	void PerformScoreTick(Scoring& scoring, MapTime time);
	void SendFinalScore(class Game* game, int clearState);
	void GetMapBPMForSpeed(const String path, struct MultiplayerBPMInfo& info);

	Vector<nlohmann::json> const* GetFinalStats() const
	{
		return &m_finalStats;
	}
	void PerformFrameTick(MapTime time, Scoring& scoring, float hispeed);

	void AddButtonFrame(MultiplayerDataSyncType type, uint32_t time, uint32_t data);
	void AddLaserFrame(uint32_t time, int ind, float val);
	void AddHitstatFrame(ObjectState* obj, MapTime delta, bool hit);

	bool ConsumePlaybackForTick(ScoreTick* tick, MultiplayerData& out);

	void CheckPlaybackInput(MapTime time, Scoring&, float* hispeed);

	TCPSocket& GetTCP()
	{
		return m_tcp;
	}

	ChatOverlay* GetChatOverlay()
	{
		return m_chatOverlay;
	}

	String GetUserId()
	{
		return m_userId;
	}

	bool IsSynced() {
		return m_syncState == SyncState::SYNCED;
	}

	void StartSync();

	bool IsSyncing();

	void Fail() {
		m_failed = true;
	}
	
	bool HasFailed()
	{
		return m_failed;
	}

	bool InRoom()
	{
		return m_roomId != "";
	}

	const String& GetUserName()
	{
		return m_userName;
	}
	FakeInput PlaybackInput;

	bool IsStartingSoon()
	{
		return m_startingSoon;
	}

	String& getReplayId()
	{
		return m_replayId;
	}

private:
	void m_OnButtonPressed(Input::Button buttonCode);
	void m_OnButtonReleased(Input::Button buttonCode);
	void m_OnMouseScroll(int32 steps);

	bool m_handleStartPacket(nlohmann::json& packet);
	bool m_handleSyncStartPacket(nlohmann::json& packet);
	bool m_handleAuthResponse(nlohmann::json& packet);
	bool m_handleRoomList(nlohmann::json& packet);
	bool m_handleSongChange(nlohmann::json& packet);
	bool m_handleRoomUpdate(nlohmann::json& packet);
	bool m_handleJoinRoom(nlohmann::json& packet);
	bool m_handleBadPassword(nlohmann::json& packet);
	bool m_handleError(nlohmann::json& packet);
	void m_handleSocketClose();
	bool m_handleFinalStats(nlohmann::json& packet);

	void m_onDatabaseUpdateStart(int max);
	void m_onDatabaseUpdateProgress(int, int);
	void m_onDatabaseUpdateDone();


	void m_addFinalStat(nlohmann::json& data);

	bool m_handleFrameData(char* data, uint32_t length);


	void m_render(float deltaTime);

	void m_joinRoomWithToken();

	void OnSearchStatusUpdated(String status);

	void m_authenticate();

	void m_updateSelectedMap(int32 mapid, int32 diff_ind, bool is_new);
	void m_clearLuaMap();
	ChartIndex* m_getChartByShortPath(const String& path, uint32*, int32, bool);
	ChartIndex* m_getChartByHash(const String& hash, const String& path, uint32*, int32);

	void m_changeDifficulty(int offset);
	void m_changeSelectedRoom(int offset);

	void m_updatePreview(ChartIndex* diff, bool mapChanged);
	void m_stopPreview();

	// Some lua util functions
	void m_PushStringToTable(const char* name, const char* data)
	{
		lua_pushstring(m_lua, name);
		lua_pushstring(m_lua, data);
		lua_settable(m_lua, -3);
	}
	void m_PushFloatToTable(const char* name, float data)
	{
		lua_pushstring(m_lua, name);
		lua_pushnumber(m_lua, data);
		lua_settable(m_lua, -3);
	}
	void m_PushIntToTable(const char* name, int data)
	{
		lua_pushstring(m_lua, name);
		lua_pushinteger(m_lua, data);
		lua_settable(m_lua, -3);
	}

	struct lua_State* m_lua = nullptr;
	LuaBindable* m_bindable = nullptr;
	bool m_suspended = false;


	int32 m_lastScoreSent = 0;
	// TODO(itszn) have the server adjust this
	int32 m_scoreInterval = 200;

	// Unique id given to by the server on auth
	String m_userId;
	String m_roomId;

	bool m_failed = false;

	Timer m_dbUpdateTimer;
	String m_lastStatus = "aaaaaa";
	std::mutex m_statusLock;

	// Did this client pick the current song
	bool m_selfPicked = false;

	bool m_inGame = false;

	// Socket to talk to the server
	TCPSocket m_tcp;

	// Database ids of the selected map
	int32 m_selectedMapId = 0;
	String m_selectedMapShortPath;
	String m_selectedMapHash;

	// Selected index into the map's difficulties
	int32 m_selectedDiffIndex = 0;
	bool m_hasSelectedMap = false;

	// Instance of the database, used to look up songs
	MapDatabase* m_mapDatabase;

	float m_advanceDiff = 0.0f;
	float m_advanceRoom = 0.0f;
	MouseLockHandle m_lockMouse;

	SyncState m_syncState;

	MultiplayerScreenState m_screenState;

	Ref<TextInputMultiplayer> m_textInput;
	String m_roomToJoin;

	String m_joinToken;

	String m_userName;
	String m_newRoomName;

	float m_hispeed;
	float m_modSpeed;
	SpeedMods m_speedMod;
	struct MultiplayerBPMInfo m_bpm;
	float m_speedBPM;

	Vector<nlohmann::json> m_finalStats;

	DBUpdateScreen* m_dbUpdateScreen = nullptr;

	ChatOverlay* m_chatOverlay = NULL;

	PreviewPlayer m_previewPlayer;
	PreviewParams m_previewParams;

	std::queue<struct MultiplayerData> m_playbackData;
	List<struct MultiplayerData> m_hitstatData;

	Vector<struct MultiplayerData> m_multiplayerFrame;
	int32 m_frameInterval = 2000;
	int32 m_lastFrameIndex = 0;

	float m_oldLaserStates[2] = { 0.0f };

	uint32_t m_hitstatIndex = 0;
	bool m_startingSoon = false;

	String m_replayId = "";
	float m_oldHispeed = 0.0;
	int32_t m_chartCustomOff = 0;
};

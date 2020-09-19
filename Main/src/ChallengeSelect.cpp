#include "stdafx.h"
#include "ChallengeSelect.hpp"
#include "ChallengeResult.hpp"
#include "TitleScreen.hpp"
#include "Application.hpp"
#include <Shared/Profiling.hpp>
#include "Scoring.hpp"
#include "Input.hpp"
#include "Game.hpp"
#include "TransitionScreen.hpp"
#include "GameConfig.hpp"
#include "GameplaySettingsDialog.hpp"
#include "lua.hpp"
#include <iterator>
#include <mutex>
#include <unordered_set>
#include "SongSort.hpp"
#include "DBUpdateScreen.hpp"
#include "PreviewPlayer.hpp"
#include "SongFilter.hpp"
#include "ItemSelectionWheel.hpp"
#include <Audio/Audio.hpp>


// TODO Move TextInput out of songselect.cpp and add search support here
// TODO add game settings dialog

using ChalItemSelectionWheel = ItemSelectionWheel<ChallengeSelectIndex, ChallengeIndex>;
class ChallengeSelectionWheel : public ChalItemSelectionWheel
{
public:
	ChallengeSelectionWheel(IApplicationTickable* owner) : ChalItemSelectionWheel(owner)
	{
	}

	bool Init() override
	{
		ChalItemSelectionWheel::Init();
		m_lua = g_application->LoadScript("songselect/chalwheel");
		if (!m_lua)
			return false;

		lua_newtable(m_lua);
		{
			//text
			lua_pushstring(m_lua, "searchText");
			lua_pushstring(m_lua, "");
			lua_settable(m_lua, -3);
			//enabled
			lua_pushstring(m_lua, "searchInputActive");
			lua_pushboolean(m_lua, false);
			lua_settable(m_lua, -3);
		}
		lua_setglobal(m_lua, "chalwheel");
		return true;
	}

	void ReloadScript() override
	{
		g_application->ReloadScript("songselect/chalwheel", m_lua);
		m_SetLuaItemIndex();
	}

	void Render(float deltaTime) override
	{
		m_lock.lock();
		lua_getglobal(m_lua, "chalwheel");
		lua_pushstring(m_lua, "searchStatus");
		lua_pushstring(m_lua, *m_lastStatus);
		lua_settable(m_lua, -3);
		lua_setglobal(m_lua, "chalwheel");
		m_lock.unlock();

		lua_getglobal(m_lua, "render");
		lua_pushnumber(m_lua, deltaTime);
		if (lua_pcall(m_lua, 1, 0, 0) != 0)
		{
			Logf("Lua error: %s", Logger::Severity::Error, lua_tostring(m_lua, -1));
			g_gameWindow->ShowMessageBox("Lua Error", lua_tostring(m_lua, -1), 0);
			g_application->RemoveTickable(m_owner);
		}
	}

	~ChallengeSelectionWheel()
	{
		g_gameConfig.Set(GameConfigKeys::LastSelectedChal, m_currentlySelectedItemId);
		if (m_lua)
			g_application->DisposeLua(m_lua);
	}

	void ResetLuaTables()
	{
		const SortType sort = GetSortType();
		if (sort == SortType::SCORE_ASC || sort == SortType::SCORE_DESC)
			m_doSort();

		m_SetAllItems(); //for force calculation
		m_SetCurrentItems(); //for displaying the correct chal
	}

	void SetSearchFieldLua(Ref<TextInput> search) override
	{
		if (m_lua == nullptr)
			return;
		lua_getglobal(m_lua, "chalwheel");
		//text
		lua_pushstring(m_lua, "searchText");
		lua_pushstring(m_lua, (search->input).c_str());
		lua_settable(m_lua, -3);
		//enabled
		lua_pushstring(m_lua, "searchInputActive");
		lua_pushboolean(m_lua, search->active);
		lua_settable(m_lua, -3);
		//set global
		lua_setglobal(m_lua, "chalwheel");
	}

	// The delegate templater is unhappy if we use the super class function directly
	void OnSearchStatusUpdated(String status) override
	{
		ChalItemSelectionWheel::OnSearchStatusUpdated(status);
	}

	void OnItemsAdded(Vector<ChallengeIndex*> items) override
	{
		ChalItemSelectionWheel::OnItemsAdded(items);
	}

	void OnItemsRemoved(Vector<ChallengeIndex*> items) override
	{
		ChalItemSelectionWheel::OnItemsRemoved(items);
	}

	void OnItemsUpdated(Vector<ChallengeIndex*> items) override
	{
		ChalItemSelectionWheel::OnItemsUpdated(items);
	}

	void OnItemsCleared(Map<int32, ChallengeIndex*> newList) override
	{
		ChalItemSelectionWheel::OnItemsCleared(newList);
	}

private:
	// grab the actual ChallengeIndex from a given selection
	ChallengeIndex* m_getDBEntryFromItemIndex(const ChallengeSelectIndex ind) const {
		return ind.GetChallenge();
	}
	ChallengeIndex* m_getDBEntryFromItemIndex(const ChallengeSelectIndex* ind) const {
		return ind->GetChallenge();
	}

	// Set all songs into lua
	void m_SetAllItems() override
	{
		//set all songs
		m_SetLuaChals("allChallenges", m_items, false);
		lua_getglobal(m_lua, "challenges_changed");
		if (!lua_isfunction(m_lua, -1))
		{
			lua_pop(m_lua, 1);
			return;
		}
		lua_pushboolean(m_lua, true);
		if (lua_pcall(m_lua, 1, 0, 0) != 0)
		{
			Logf("Lua error on songs_chaged: %s", Logger::Severity::Error, lua_tostring(m_lua, -1));
			g_gameWindow->ShowMessageBox("Lua Error challenges_changed", lua_tostring(m_lua, -1), 0);
		}
	}

	void m_SetCurrentItems() override
	{
		m_SetLuaChals("challenges", m_SourceCollection(), true);

		lua_getglobal(m_lua, "challenges_changed");
		if (!lua_isfunction(m_lua, -1))
		{
			lua_pop(m_lua, 1);
			return;
		}
		lua_pushboolean(m_lua, false);
		if (lua_pcall(m_lua, 1, 0, 0) != 0)
		{
			Logf("Lua error on songs_chaged: %s", Logger::Severity::Error, lua_tostring(m_lua, -1));
			g_gameWindow->ShowMessageBox("Lua Error challenges_changed", lua_tostring(m_lua, -1), 0);
		}
	}

	void m_SetLuaChals(const char* key, const Map < int32, ChallengeSelectIndex> &collection, bool sorted)
	{
		lua_getglobal(m_lua, "chalwheel");
		lua_pushstring(m_lua, key);
		lua_newtable(m_lua);
		int chalIndex = 0;

		if (sorted)
		{
			// sortVec should only have the current chals in the collection
			for (auto& chalId : m_sortVec)
			{
				// Grab the chal for this sort index
				ChallengeSelectIndex chal = collection.find(chalId)->second;
				m_PushChalToLua(chal, ++chalIndex);
			}
		}
		else {
			
			for (auto& chal : collection)
			{
				m_PushChalToLua(chal.second, ++chalIndex);
			}
		}
		lua_settable(m_lua, -3);
		lua_setglobal(m_lua, "chalwheel");
	}

	void m_PushChalToLua(ChallengeSelectIndex chalsel, int index)
	{
		lua_pushinteger(m_lua, index);
		lua_newtable(m_lua);
		ChallengeIndex* chal = chalsel.GetChallenge();
		m_PushStringToTable("title", *(chal->title));
		m_PushStringToTable("requirement_text", *(chal->reqText));
		m_PushIntToTable("id", chal->id);
		m_PushIntToTable("rating", chal->clearRating);
		m_PushIntToTable("missing_chart", chal->missingChart);

		int chartIndex = 0;
		lua_pushstring(m_lua, "charts");
		lua_newtable(m_lua);
		for (auto& diff : chal->charts)
		{

			String folder_path = Path::RemoveLast(diff->path, nullptr);
			lua_pushinteger(m_lua, ++chartIndex);
			lua_newtable(m_lua);
			m_PushStringToTable("title", diff->title.c_str());
			m_PushStringToTable("artist", diff->artist.c_str());
			m_PushStringToTable("bpm", diff->bpm.c_str());
			m_PushStringToTable("jacketPath", Path::Normalize(folder_path + "/" + diff->jacket_path).c_str());
			Logf("Jacket path: %s", Logger::Severity::Info, Path::Normalize(folder_path + "/" + diff->jacket_path).c_str());

			m_PushIntToTable("level", diff->level);
			m_PushIntToTable("difficulty", diff->diff_index);
			m_PushIntToTable("id", diff->id);
			m_PushStringToTable("effector", diff->effector.c_str());
			m_PushStringToTable("illustrator", diff->illustrator.c_str());
			lua_settable(m_lua, -3);
		}
		lua_settable(m_lua, -3);

		lua_settable(m_lua, -3);
	}

	void m_OnItemSelected(ChallengeSelectIndex index) override
	{
		m_currentlySelectedItemId = index.GetChallenge()->id;
	}
};


class ChallengeSelect_Impl : public ChallengeSelect
{
private:
	Timer m_dbUpdateTimer;
	MapDatabase* m_mapDatabase;

	// Map selection wheel
	Ref<ChallengeSelectionWheel> m_selectionWheel;
	Ref<TextInput> m_searchInput;

	float m_advanceChal = 0.0f;
	float m_sensMult = 1.0f;
	MouseLockHandle m_lockMouse;
	bool m_suspended = true;
	bool m_hasRestored = false;
	bool m_previewLoaded = true;
	bool m_showScores = false;


	Map<Input::Button, float> m_timeSinceButtonPressed;
	Map<Input::Button, float> m_timeSinceButtonReleased;
	lua_State* m_lua = nullptr;

	// Select sound
	Sample m_selectSound;

	bool m_transitionedToGame = false;
	int32 m_lastChalIndex = -1;

	DBUpdateScreen* m_dbUpdateScreen = nullptr;

	ChallengeManager m_manager;
public:
	ChallengeSelect_Impl() {}

	bool AsyncLoad() override
	{
		m_selectSound = g_audio->CreateSample("audio/menu_click.wav");
		m_selectionWheel = Ref<ChallengeSelectionWheel>(new ChallengeSelectionWheel(this));
		m_mapDatabase = new MapDatabase(true);

		// Add database update hook before triggering potential update
		m_mapDatabase->OnDatabaseUpdateStarted.Add(this, &ChallengeSelect_Impl::m_onDatabaseUpdateStart);
		m_mapDatabase->OnDatabaseUpdateDone.Add(this, &ChallengeSelect_Impl::m_onDatabaseUpdateDone);
		m_mapDatabase->OnDatabaseUpdateProgress.Add(this, &ChallengeSelect_Impl::m_onDatabaseUpdateProgress);
		m_mapDatabase->SetChartUpdateBehavior(g_gameConfig.GetBool(GameConfigKeys::TransferScoresOnChartUpdate));
		m_mapDatabase->FinishInit();

		// Setup the map database
		m_mapDatabase->AddSearchPath(g_gameConfig.GetString(GameConfigKeys::SongFolder));

		return true;
	}

	void m_onDatabaseUpdateStart(int max)
	{
		m_dbUpdateScreen = new DBUpdateScreen(max);
		g_application->AddTickable(m_dbUpdateScreen);
	}

	void m_onDatabaseUpdateProgress(int current, int max)
	{
		if (!m_dbUpdateScreen)
			return;
		m_dbUpdateScreen->SetCurrent(current, max);
	}

	void m_onDatabaseUpdateDone()
	{
		if (!m_dbUpdateScreen)
			return;
		g_application->RemoveTickable(m_dbUpdateScreen);
		m_dbUpdateScreen = NULL;
	}

	bool AsyncFinalize() override
	{
		m_lua = g_application->LoadScript("songselect/background");
		if (!m_lua)
		{

		}
		g_input.OnButtonPressed.Add(this, &ChallengeSelect_Impl::m_OnButtonPressed);
		g_input.OnButtonReleased.Add(this, &ChallengeSelect_Impl::m_OnButtonReleased);
		g_gameWindow->OnMouseScroll.Add(this, &ChallengeSelect_Impl::m_OnMouseScroll);

		if (!m_selectionWheel->Init())
			return false;

		m_mapDatabase->OnChallengesAdded.Add(m_selectionWheel.get(), &ChallengeSelectionWheel::OnItemsAdded);
		m_mapDatabase->OnChallengesUpdated.Add(m_selectionWheel.get(), &ChallengeSelectionWheel::OnItemsUpdated);
		m_mapDatabase->OnChallengesCleared.Add(m_selectionWheel.get(), &ChallengeSelectionWheel::OnItemsCleared);
		m_mapDatabase->OnChallengesRemoved.Add(m_selectionWheel.get(), &ChallengeSelectionWheel::OnItemsRemoved);
		m_mapDatabase->OnSearchStatusUpdated.Add(m_selectionWheel.get(), &ChallengeSelectionWheel::OnSearchStatusUpdated);
		//m_selectionWheel->OnItemsChanged.Add(m_filterSelection.get(), &FilterSelection::OnSongsChanged);
		m_mapDatabase->StartSearching();

		// TODO add sort wheel
		m_selectionWheel->SetSort(new ChallengeSort("Title ^", false));

		m_selectionWheel->SelectItemByItemId(g_gameConfig.GetInt(GameConfigKeys::LastSelectedChal));

		m_searchInput = Ref<TextInput>(new TextInput());
		m_searchInput->OnTextChanged.Add(this, &ChallengeSelect_Impl::OnSearchTermChanged);

		g_audio->SetGlobalVolume(g_gameConfig.GetFloat(GameConfigKeys::MasterVolume));

		m_sensMult = g_gameConfig.GetFloat(GameConfigKeys::SongSelSensMult);

		return true;
	}

	bool Init() override
	{
		return true;
	}

	~ChallengeSelect_Impl()
	{
		// Clear callbacks
		if (m_mapDatabase)
		{
			m_mapDatabase->OnChallengesCleared.Clear();
			delete m_mapDatabase;
		}
		g_input.OnButtonPressed.RemoveAll(this);
		g_input.OnButtonReleased.RemoveAll(this);
		g_gameWindow->OnMouseScroll.RemoveAll(this);

		if (m_lua)
			g_application->DisposeLua(m_lua);
	}

	void OnSearchTermChanged(const String &search)
	{
		/*
		if (search.empty())
			m_filterSelection->AdvanceSelection(0);
		else
		{
			Map<int32, ChallengeIndex *> filter = m_mapDatabase->FindFolders(search);
			m_selectionWheel->SetFilter(filter);
		}
		*/
	}

	void m_OnButtonPressed(Input::Button buttonCode)
	{
		//if (m_multiplayer && m_multiplayer->GetChatOverlay()->IsOpen())
		//if (m_suspended || m_collDiag.IsActive() || m_settDiag.IsActive())
		if (m_suspended)
			return;

		if (g_gameConfig.GetEnum<Enum_InputDevice>(GameConfigKeys::ButtonInputDevice) == InputDevice::Keyboard && m_searchInput->active)
			return;

		m_timeSinceButtonPressed[buttonCode] = 0;

		//if (buttonCode == Input::Button::BT_S && !m_filterSelection->Active && !m_sortSelection->Active && !IsSuspended() && !m_transitionedToGame)
		if (buttonCode == Input::Button::BT_S && !IsSuspended() && !m_transitionedToGame)
		{
			bool autoplay = (g_gameWindow->GetModifierKeys() & ModifierKeys::Ctrl) == ModifierKeys::Ctrl;
			ChallengeIndex *folder = m_selectionWheel->GetSelection();
			if (folder)
			{
				// TODO start the chal logic
				ChallengeIndex* chal = GetCurrentSelectedChallenge();
				if (m_manager.StartChallenge(chal))
					m_transitionedToGame = true;
				/*
				Game *game = Game::Create(chart, Game::FlagsFromSettings());
				if (!game)
				{
					Log("Failed to start game", Logger::Severity::Error);
					return;
				}
				game->GetScoring().autoplay = autoplay;

				// Transition to game
				g_transition->TransitionTo(game);
				m_transitionedToGame = true;
				*/
			}
		}
		else
		{
			switch (buttonCode)
			{
			/*

			case Input::Button::FX_1:
				if (g_input.GetButton(Input::Button::FX_0))
				{
					m_settDiag.Open();
				}
				break;
			case Input::Button::FX_0:
				if (g_input.GetButton(Input::Button::FX_1))
				{
					m_settDiag.Open();
				}
				break;
			*/
			case Input::Button::BT_S:
				break;
			case Input::Button::Back:
				m_suspended = true;
				g_application->RemoveTickable(this);
				break;
			default:
				break;
			}
		}
	}

	void m_OnButtonReleased(Input::Button buttonCode)
	{
		//if (m_multiplayer && m_multiplayer->GetChatOverlay()->IsOpen())
		//if (m_suspended || m_settDiag.IsActive())
		if (m_suspended)
			return;

		if (g_gameConfig.GetEnum<Enum_InputDevice>(GameConfigKeys::ButtonInputDevice) == InputDevice::Keyboard && m_searchInput->active)
			return;

		m_timeSinceButtonReleased[buttonCode] = 0;

		/*
		switch (buttonCode)
		{
		case Input::Button::FX_0:
			if (m_timeSinceButtonPressed[Input::Button::FX_0] < m_timeSinceButtonPressed[Input::Button::FX_1] && !g_input.GetButton(Input::Button::FX_1) && !m_sortSelection->Active)
			{
				m_filterSelection->Active = !m_filterSelection->Active;
			}
			break;
		case Input::Button::FX_1:
			if (m_timeSinceButtonPressed[Input::Button::FX_1] < m_timeSinceButtonPressed[Input::Button::FX_0] && !g_input.GetButton(Input::Button::FX_0) && !m_filterSelection->Active)
			{
				m_sortSelection->Active = !m_sortSelection->Active;
			}
			break;
			default:
			break;
		}
		*/
	}

	void m_OnMouseScroll(int32 steps)
	{
		//if (m_suspended || m_collDiag.IsActive() || m_settDiag.IsActive())
		if (m_suspended)
			return;

		//if (m_multiplayer && m_multiplayer->GetChatOverlay()->IsOpen())

		/*
		if (m_sortSelection->Active)
		{
			m_sortSelection->AdvanceSelection(steps);
		}
		else if (m_filterSelection->Active)
		{
			m_filterSelection->AdvanceSelection(steps);
		}
		else
		{
		*/
			m_selectionWheel->AdvanceSelection(steps);
		//}
	}

	virtual void OnKeyPressed(SDL_Scancode code)
	{
		//if (m_multiplayer && m_multiplayer->GetChatOverlay()->OnKeyPressedConsume(code))

		//if (m_settDiag.IsActive())
		//	return;

		/*
		if (m_filterSelection->Active)
		{
			if (code == SDL_SCANCODE_DOWN)
			{
				m_filterSelection->AdvanceSelection(1);
			}
			else if (code == SDL_SCANCODE_UP)
			{
				m_filterSelection->AdvanceSelection(-1);
			}
		}
		else if (m_sortSelection->Active)
		{
			if (code == SDL_SCANCODE_DOWN)
			{
				m_sortSelection->AdvanceSelection(1);
			}
			else if (code == SDL_SCANCODE_UP)
			{
				m_sortSelection->AdvanceSelection(-1);
			}
		}
		else
		*/
		{
			if (code == SDL_SCANCODE_DOWN)
			{
				m_selectionWheel->AdvanceSelection(1);
			}
			else if (code == SDL_SCANCODE_UP)
			{
				m_selectionWheel->AdvanceSelection(-1);
			}
			else if (code == SDL_SCANCODE_PAGEDOWN)
			{
				m_selectionWheel->AdvancePage(1);
			}
			else if (code == SDL_SCANCODE_PAGEUP)
			{
				m_selectionWheel->AdvancePage(-1);
			}
			else if (code == SDL_SCANCODE_F5)
			{
				m_mapDatabase->StartSearching();
				OnSearchTermChanged(m_searchInput->input);
			}
			else if (code == SDL_SCANCODE_F2)
			{
				m_selectionWheel->SelectRandom();
			}
			else if (code == SDL_SCANCODE_F9)
			{
				m_selectionWheel->ReloadScript();
				//m_filterSelection->ReloadScript();
				g_application->ReloadScript("songselect/background", m_lua);
			}
			else if (code == SDL_SCANCODE_F12)
			{
				//Path::ShowInFileBrowser(m_selectionWheel->GetSelection()->path);
			}
			else if (code == SDL_SCANCODE_TAB)
			{
				m_searchInput->SetActive(!m_searchInput->active);
			}
			else if (code == SDL_SCANCODE_RETURN && m_searchInput->active)
			{
				m_searchInput->SetActive(false);
			}
			else if (code == SDL_SCANCODE_DELETE)
			{
				/*
				ChartIndex* chart = m_selectionWheel->GetSelectedChart();
				String name = chart->title + " [" + chart->diff_shortname + "]";
				bool res = g_gameWindow->ShowYesNoMessage("Delete chart?", "Are you sure you want to delete " + name + "\nThis will only delete "+chart->path+"\nThis cannot be undone...");
				if (!res)
					return;
				Path::Delete(chart->path);
				m_mapDatabase->StartSearching();
				OnSearchTermChanged(m_searchInput->input);
				*/
			}
		}
	}

	virtual void OnKeyReleased(SDL_Scancode code)
	{
	}

	virtual void Tick(float deltaTime) override
	{
		if (m_dbUpdateTimer.Milliseconds() > 500)
		{
			m_mapDatabase->Update();
			m_dbUpdateTimer.Restart();
		}

		// Tick navigation
		if (!IsSuspended())
		{
			TickNavigation(deltaTime);
			m_searchInput->Tick();
			m_selectionWheel->SetSearchFieldLua(m_searchInput);
			//m_settDiag.Tick(deltaTime);
		}
		//if (m_multiplayer)
		//	m_multiplayer->GetChatOverlay()->Tick(deltaTime);
	}

	virtual void Render(float deltaTime)
	{
		if (m_suspended && m_hasRestored) return;
		lua_getglobal(m_lua, "render");
		lua_pushnumber(m_lua, deltaTime);
		if (lua_pcall(m_lua, 1, 0, 0) != 0)
		{
			Logf("Lua error: %s", Logger::Severity::Error, lua_tostring(m_lua, -1));
			g_gameWindow->ShowMessageBox("Lua Error", lua_tostring(m_lua, -1), 0);
			assert(false);
		}

		m_selectionWheel->Render(deltaTime);
		//m_filterSelection->Render(deltaTime);
		//m_sortSelection->Render(deltaTime);

		//m_settDiag.Render(deltaTime);

		//if (m_multiplayer)
		//	m_multiplayer->GetChatOverlay()->Render(deltaTime);
	}

	void TickNavigation(float deltaTime)
	{
		if (!m_manager.ReturnToSelect())
		{
			return;
		}

		// Lock mouse to screen when active
		if (g_gameConfig.GetEnum<Enum_InputDevice>(GameConfigKeys::LaserInputDevice) == InputDevice::Mouse && g_gameWindow->IsActive())
		{
			if (!m_lockMouse)
				m_lockMouse = g_input.LockMouse();
		}
		else
		{
			if (m_lockMouse)
				m_lockMouse.reset();
			g_gameWindow->SetCursorVisible(true);
		}

		for (size_t i = 0; i < (size_t)Input::Button::Length; i++)
		{
			m_timeSinceButtonPressed[(Input::Button)i] += deltaTime;
			m_timeSinceButtonReleased[(Input::Button)i] += deltaTime;
		}

		/*
		if (m_settDiag.IsActive())
		{
			return;
		}
		*/

		// Song navigation using laser inputs
		/// TODO: Investigate strange behaviour further and clean up.
		float diff_input = g_input.GetInputLaserDir(0) * m_sensMult;
		float song_input = g_input.GetInputLaserDir(1) * m_sensMult;

		m_advanceChal += song_input;

		int advanceChalActual = (int)Math::Floor(m_advanceChal * Math::Sign(m_advanceChal)) * Math::Sign(m_advanceChal);

		/*
		if (m_filterSelection->Active)
		{
			if (advanceDiffActual != 0)
				m_filterSelection->AdvanceSelection(advanceDiffActual);
			if (advanceSongActual != 0)
				m_filterSelection->AdvanceSelection(advanceSongActual);
		}
		else if (m_sortSelection->Active)
		{
			if (advanceDiffActual != 0)
				m_sortSelection->AdvanceSelection(advanceDiffActual);
			if (advanceSongActual != 0)
				m_sortSelection->AdvanceSelection(advanceSongActual);
		}
		else
		*/
		{
			if (advanceChalActual != 0)
				m_selectionWheel->AdvanceSelection(advanceChalActual);
		}

		m_advanceChal -= advanceChalActual;
	}

	virtual void OnSuspend()
	{
		m_lastChalIndex = m_selectionWheel->GetCurrentItemIndex();

		m_suspended = true;
		//m_previewPlayer.Pause();
		m_mapDatabase->PauseSearching();
		if (m_lockMouse)
			m_lockMouse.reset();
	}

	virtual void OnRestore()
	{
		// NOTE: we can't trigger the next chart here bc you can't add tickables on restore
		g_application->DiscordPresenceMenu("Challenge Select");
		m_suspended = false;
		m_hasRestored = true;
		m_transitionedToGame = false;
		//m_previewPlayer.Restore();
		m_selectionWheel->ResetLuaTables();
		//TODO if the manager is going to trigger in the next tick we probably should not do this
		//     we could add a delegate for finishing the charts and then use that to restart searching
		m_mapDatabase->ResumeSearching();
		if (g_gameConfig.GetBool(GameConfigKeys::AutoResetSettings))
		{
			g_gameConfig.SetEnum<Enum_SpeedMods>(GameConfigKeys::SpeedMod, SpeedMods::XMod);
			g_gameConfig.Set(GameConfigKeys::ModSpeed, g_gameConfig.GetFloat(GameConfigKeys::AutoResetToSpeed));
			//m_filterSelection->SetFiltersByIndex(0, 0);
		}
		if (m_lastChalIndex != -1)
		{
			m_selectionWheel->SelectItemByItemIndex(m_lastChalIndex);
		}

	}

	ChallengeIndex* GetCurrentSelectedChallenge() override
	{
		return m_selectionWheel->GetSelection();
	}
	
};

ChallengeSelect* ChallengeSelect::Create()
{
	ChallengeSelect_Impl* impl = new ChallengeSelect_Impl();
	return impl;
}

ChallengeOption<bool> ChallengeManager::m_getOptionAsBool(
	nlohmann::json reqs, String name)
{
	if (!reqs.contains(name))
		return ChallengeOption<bool>::IgnoreOption();

	nlohmann::json entry = reqs[name];
	bool val;
	if (entry.is_null())
	{
		return ChallengeOption<bool>::DisableOption();
	}
	if (!entry.is_boolean())
	{
		Logf("Skipping setting '%s': not a boolean", Logger::Severity::Warning, *name);
		return ChallengeOption<bool>::IgnoreOption();
	}

	entry.get_to(val);
	return ChallengeOption<bool>(val);
}

ChallengeOption<float> ChallengeManager::m_getOptionAsFloat(
	nlohmann::json reqs, String name, float min, float max)
{
	if (!reqs.contains(name))
		return ChallengeOption<float>::IgnoreOption();

	nlohmann::json entry = reqs[name];
	float val;
	if (entry.is_null())
	{
		return ChallengeOption<float>::DisableOption();
	}
	if (entry.is_number_float())
	{
		entry.get_to(val);
	}
	if (!entry.is_number_float() || isnan(val))
	{
		Logf("Skipping setting '%s': not a number", Logger::Severity::Warning, *name);
		return ChallengeOption<float>::IgnoreOption();
	}
	if (val < min || val > max)
	{
		Logf("Skipping setting '%s': must be between [%f and %f]", Logger::Severity::Warning, *name, min, max);
		return ChallengeOption<float>::IgnoreOption();
	}

	return ChallengeOption<float>(val);
}

ChallengeOption<uint32> ChallengeManager::m_getOptionAsPositiveInteger(
	nlohmann::json reqs, String name, int64 min, int64 max)
{
	if (!reqs.contains(name))
		return ChallengeOption<uint32>::IgnoreOption();

	nlohmann::json entry = reqs[name];
	int32 val;
	if (entry.is_null())
	{
		return ChallengeOption<uint32>::DisableOption();

	}
	if (entry.is_string())
	{
		// Get the intergral from the string
		String str;
		entry.get_to(str);
		// XXX should we actually like validate the string?
		val = atoi(*str);
	}
	else if (entry.is_number_integer())
	{
		entry.get_to(val);
	}
	else
	{
		Logf("Skipping setting '%s': not a integer or string", Logger::Severity::Warning, *name);
		return ChallengeOption<uint32>::IgnoreOption();
	}
	if (val < min || val > max)
	{
		Logf("Skipping setting '%s': must be between [%u and %u]", Logger::Severity::Warning, *name, min, max);
		return ChallengeOption<uint32>::IgnoreOption();
	}

	return ChallengeOption<uint32>(val);
}

ChallengeRequirements ChallengeManager::m_processReqs(nlohmann::json req)
{
	ChallengeRequirements out;
	out.clear =          m_getOptionAsBool(req,            "clear");
	out.min_percentage = m_getOptionAsPositiveInteger(req, "min_percentage", 0,   200);
	out.min_gauge =      m_getOptionAsFloat(req,           "min_gauge",      0.0, 1.0);
	out.max_errors =     m_getOptionAsPositiveInteger(req, "max_errors");
	out.max_nears =      m_getOptionAsPositiveInteger(req, "max_nears");
	out.min_crits =      m_getOptionAsPositiveInteger(req, "min_crits");
	out.min_chain =      m_getOptionAsPositiveInteger(req, "min_chain");
	return out;
}

ChallengeOptions ChallengeManager::m_processOptions(nlohmann::json j)
{
	ChallengeOptions out;
	out.mirror =       m_getOptionAsBool(j,            "mirror");
	out.excessive =    m_getOptionAsBool(j,            "excessive_gauge");
	out.min_modspeed = m_getOptionAsPositiveInteger(j, "min_modspeed");
	out.max_modspeed = m_getOptionAsPositiveInteger(j, "max_modspeed");
	out.allow_cmod =   m_getOptionAsBool(j,            "allow_cmod");
	out.hidden_min =   m_getOptionAsFloat(j,           "hidden_min",     0.0, 1.0);
	out.sudden_min =   m_getOptionAsFloat(j,           "sudden_min",     0.0, 1.0);
	out.crit_judge =   m_getOptionAsPositiveInteger(j, "crit_judgement", 0,   46);
	out.near_judge =   m_getOptionAsPositiveInteger(j, "near_judgement", 0,   92);
	out.hold_judge =   m_getOptionAsPositiveInteger(j, "hold_judgement", 0,   138);

	// These reqs are checked at the end so we keep em as options
	// TODO only do this on the global one? (overrides don't work for these)
	out.average_percentage = m_getOptionAsPositiveInteger(j, "min_average_percentage", 0, 200);
	out.average_gauge =      m_getOptionAsFloat(j,           "min_average_gauge",      0, 1.0);
	out.average_errors =     m_getOptionAsPositiveInteger(j, "max_average_errors");
	out.max_overall_errors = m_getOptionAsPositiveInteger(j, "max_overall_errors");
	out.average_nears =      m_getOptionAsPositiveInteger(j, "max_average_nears");
	out.max_overall_nears =  m_getOptionAsPositiveInteger(j, "max_overall_nears");
	out.average_crits =      m_getOptionAsPositiveInteger(j, "min_average_crits");
	out.min_overall_crits =  m_getOptionAsPositiveInteger(j, "min_overall_crits");
	return out;
}

bool ChallengeManager::StartChallenge(ChallengeIndex* chal)
{
	assert(!m_running);

	if (chal->missingChart)
		return false;

	// Force reload from file in case it was changed
	chal->ReloadSettings();

	// Check if there are valid settings
	nlohmann::json settings = chal->GetSettings();

	// Check if the json loaded correctly
	if (settings.is_null() || settings.is_discarded())
		return false;

	m_chal = chal;
	m_running = true;
	m_finishedCurrentChart = false;
	m_failedEarly = false;
	m_seenResults = false;
	m_scores.clear();
	m_reqs.clear();
	m_opts.clear();
	m_results.clear();
	m_chartIndex = 0;
	m_chartsPlayed = 0;

	m_totalNears = 0;
	m_totalErrors = 0;
	m_totalCrits = 0;
	m_totalScore = 0;
	m_totalPercentage = 0;
	m_totalGauge = 0.0;

	m_globalReqs.Reset();
	m_globalOpts.Reset();
	if (settings.contains("global"))
	{
		m_globalReqs = m_processReqs(settings["global"]);
		m_globalOpts = m_processOptions(settings["global"]);
	}


	if (settings.contains("overrides"))
	{
		nlohmann::json overrides = settings["overrides"];

		if (m_chal->charts.size() < overrides.size())
		{
			Log("Note: more overrides than charts", Logger::Severity::Warning);
		}
		for (unsigned int i = 0; i < overrides.size() && i < m_chal->charts.size(); i++)
		{
			m_reqs.push_back(m_processReqs(overrides[i]));
			m_opts.push_back(m_processOptions(overrides[i]));
		}
	}
	for (unsigned int i=m_reqs.size(); i<m_chal->charts.size(); i++)
	{
		m_reqs.push_back(ChallengeRequirements());
		m_opts.push_back(ChallengeOptions());
	}

	return m_setupNextChart();
}

bool ChallengeManager::m_finishedAllCharts(bool passed)
{
	assert(m_chartsPlayed > 0);

	OverallChallengeResult res;

	if (!passed)
	{
		res.failString = "Not All Charts Passed Challenge Goals";
	}

	res.averagePercent = m_totalPercentage / m_chartsPlayed;
	if (passed && res.averagePercent < m_globalOpts.average_percentage.Get(0))
	{
		passed = false;
		res.failString = Utility::Sprintf("Average Percentage of %u%% < Min of %u%%",
			res.averagePercent, m_globalOpts.average_percentage.Get(0));
	}
	res.averageGauge = m_totalGauge / m_chartsPlayed;
	if (passed && res.averageGauge < m_globalOpts.average_gauge.Get(0.0))
	{
		res.failString = Utility::Sprintf("Average Guage of %d%% < Min of %d%%",
			(int)(res.averageGauge * 100), (int)(m_globalOpts.average_gauge.Get(0) * 100));
		passed = false;
	}

	res.overallErrors = m_totalErrors;
	if (passed && res.overallErrors > m_globalOpts.max_overall_errors.Get(INT_MAX))
	{
		res.failString = Utility::Sprintf("Total of %u Error > Max of %u",
			res.overallErrors, m_globalOpts.max_overall_errors.Get(INT_MAX));
		passed = false;
	}
	res.averageErrors = m_totalErrors / m_chartsPlayed;
	if (passed && res.averageErrors > m_globalOpts.average_errors.Get(INT_MAX))
	{
		res.failString = Utility::Sprintf("Average of %u Error > Max of %u",
			res.averageErrors, m_globalOpts.average_errors.Get(INT_MAX));
		passed = false;
	}

	res.overallNears = m_totalNears;
	if (passed && res.overallNears > m_globalOpts.max_overall_nears.Get(INT_MAX))
	{
		res.failString = Utility::Sprintf("Total of %u Good > Max of %u",
			res.overallNears, m_globalOpts.max_overall_nears.Get(INT_MAX));
		passed = false;
	}
	res.averageNears = m_totalNears / m_chartsPlayed;
	if (passed && res.averageNears > m_globalOpts.average_nears.Get(INT_MAX))
	{
		res.failString = Utility::Sprintf("Average of %u Good > Max of %u",
			res.averageNears, m_globalOpts.average_nears.Get(INT_MAX));
		passed = false;
	}

	res.overallCrits = m_totalCrits;
	if (passed && res.overallCrits < m_globalOpts.min_overall_crits.Get(0))
	{
		res.failString = Utility::Sprintf("Total of %u Crit < Min of %u",
			res.overallCrits, m_globalOpts.min_overall_crits.Get(INT_MAX));
		passed = false;
	}
	res.averageCrits = m_totalCrits / m_chartsPlayed;
	if (passed && res.averageCrits < m_globalOpts.average_crits.Get(0))
	{
		res.failString = Utility::Sprintf("Average of %u Crit < Min of %u",
			res.averageNears, m_globalOpts.average_crits.Get(INT_MAX));
		passed = false;
	}

	res.passed = passed;

	m_overallResults = res;

	// TODO update database if passed

	ChallengeResultScreen* resScreen = ChallengeResultScreen::Create(this);
	g_transition->TransitionTo(resScreen);

	m_seenResults = true;
	return false;
}

// Returns if it is all done or not
bool ChallengeManager::ReturnToSelect()
{
	if (!m_running)
		return true;

	if (!m_finishedCurrentChart)
		return false;

	if (m_seenResults)
	{
		// We all all done here
		m_running = false;
		m_chal = nullptr;
		m_currentChart = nullptr;
		return true;
	}

	m_chartsPlayed++;

	if (!m_passedCurrentChart || m_failedEarly)
	{
		return m_finishedAllCharts(m_passedCurrentChart);
	}

	m_chartIndex++;
	if (m_chartIndex == m_chal->charts.size())
	{
		return m_finishedAllCharts(true);
	}

	return m_setupNextChart();
}

bool ChallengeManager::m_setupNextChart()
{
	assert(m_running);

	m_passedCurrentChart = false;
	m_finishedCurrentChart = false;
	m_currentChart = m_chal->charts[m_chartIndex];
	m_currentOptions = m_globalOpts.Merge(m_opts[m_chartIndex]);

	if (m_currentOptions.min_modspeed.Get(0) > 
		m_currentOptions.max_modspeed.Get(INT_MAX))
	{
		Log("Skipping setting 'max_modspeed': must be more than 'min_modspeed'", Logger::Severity::Warning);
		m_currentOptions.max_modspeed = ChallengeOption<uint32>::IgnoreOption();
	}

	GameFlags flags;
	if (m_currentOptions.excessive.Get(false))
		flags = GameFlags::Hard;
	else
		flags = GameFlags::None;

	if (m_currentOptions.mirror.Get(false))
		flags = flags | GameFlags::Mirror;

	Game* game = Game::Create(this, m_currentChart, flags);
	if (!game)
	{
		Log("Failed to start game", Logger::Severity::Error);
		return false;
	}

	g_transition->TransitionTo(game);
	return true;
}

void ChallengeManager::ReportScore(Game* game, ClearMark clearMark)
{
	assert(m_running);

	if (clearMark == ClearMark::NotPlayed)
	{

	}

	m_finishedCurrentChart = true;
	const Scoring& scoring = game->GetScoring();

	ChallengeRequirements req = m_globalReqs.Merge(m_reqs[m_chartIndex]);
	ChallengeResult res;

	uint32 finalScore = scoring.CalculateCurrentScore();
	res.score = finalScore;
	res.flags = game->GetFlags();
	uint32 percentage = (uint32)std::max(((int)finalScore - 8000000) / 10000, 0);
	res.percent = percentage;

	res.crits = scoring.GetPerfects();
	m_totalCrits += res.crits;
	res.nears = scoring.GetGoods();
	m_totalNears += res.nears;
	res.errors = scoring.GetMisses();
	m_totalErrors += res.errors;

	m_totalScore += finalScore;
	m_totalPercentage += percentage;

	res.badge = clearMark;
	if (req.clear.Get(false))
	{
		if (clearMark >= ClearMark::NormalClear)
			req.clear.MarkPassed();
		else if (res.failString == "")
			res.failString = "Chart was not cleared";
	}

	if (req.min_percentage.HasValue())
	{
		if (percentage >= *req.min_percentage)
			req.min_percentage.MarkPassed();
		else if (res.failString == "")
			res.failString = Utility::Sprintf("Percentage of %u%% < Min of %u%%",
				percentage, *req.min_percentage);
	}

	res.gauge = scoring.currentGauge;
	if (req.min_gauge.HasValue())
	{
		if (res.gauge >= *req.min_gauge)
			req.min_gauge.MarkPassed();
		else if (res.failString == "")
			res.failString = Utility::Sprintf("Gauge of %d%% < Min of %d%%",
				(int)(res.gauge * 100), (int)(*req.min_gauge * 100));
	}

	if (req.max_errors.HasValue())
	{
		if (res.errors <= *req.max_errors)
			req.max_errors.MarkPassed();
		else if (res.failString == "")
			res.failString = Utility::Sprintf("%u Errors > Max of %u",
				res.errors, *req.max_errors);
	}

	if (req.max_nears.HasValue())
	{
		if (res.nears <= *req.max_nears)
			req.max_nears.MarkPassed();
		else if (res.failString == "")
			res.failString = Utility::Sprintf("%u Nears > Max of %u",
				res.nears, *req.max_nears);
	}

	res.maxCombo = scoring.maxComboCounter;
	if (req.min_chain.HasValue())
	{
		if (res.maxCombo >= *req.min_chain)
			req.min_chain.MarkPassed();
		else if (res.failString == "")
			res.failString = Utility::Sprintf("%u Chain < Min of %u",
				res.maxCombo, *req.min_chain);
	}

	if (req.min_crits.HasValue())
	{
		if (res.crits >= *req.min_crits)
			req.min_crits.MarkPassed();
		else if (res.failString == "")
			res.failString = Utility::Sprintf("%u Crits < Min of %u",
				res.crits, *req.min_crits);
	}

	res.passed = req.Passed();
	m_passedCurrentChart = res.passed;

	// Now check if we crossed any global failing thresholds
	if (m_totalErrors > m_globalOpts.max_overall_errors.Get(INT_MAX))
		m_failedEarly = true;
	if (m_totalNears > m_globalOpts.max_overall_nears.Get(INT_MAX))
		m_failedEarly = true;

	m_results.push_back(res);
}

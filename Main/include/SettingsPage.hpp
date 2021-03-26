#pragma once
#include "GuiUtils.hpp"

class SettingsPage
{
protected:
	SettingsPage(nk_context* nctx, const std::string_view& name) : m_nctx(nctx), m_name(name) {}

	virtual void Load() = 0;
	virtual void Save() = 0;

	virtual void RenderContents() = 0;

	class SettingTextData
	{
	public:
		SettingTextData() noexcept {}

		void Load();
		void Save();

		void Render(nk_context* nctx);
		void RenderPassword(nk_context* nctx);

	protected:
		constexpr static size_t BUFFER_SIZE = 1024;

		virtual String LoadConfig() { return ""; }
		virtual void SaveConfig(const String& value) {}

	private:

		std::array<char, BUFFER_SIZE> m_buffer;
		int m_len = 0;
	};

	class GameConfigTextData : public SettingTextData
	{
	public:
		GameConfigTextData(GameConfigKeys key) : m_key(key) {}

	protected:
		String LoadConfig() override;
		void SaveConfig(const String& value) override;

		GameConfigKeys m_key;
	};

	// Useful elements
	inline void LayoutRowDynamic(int num_columns) { LayoutRowDynamic(num_columns, static_cast<float>(m_buttonHeight)); }
	void LayoutRowDynamic(int num_columns, float height);

	inline void Separator() { Separator(m_buttonHeight); }
	inline void Separator(int height) { Separator(static_cast<float>(height)); }
	void Separator(float height);

	void Label(const std::string_view& label, enum nk_text_alignment alignment = nk_text_alignment::NK_TEXT_LEFT);

	bool ToggleInput(bool val, const std::string_view& label);
	bool ToggleSetting(GameConfigKeys key, const std::string_view& label);

	template<typename EnumClass>
	bool EnumSetting(GameConfigKeys key, const std::string_view& label)
	{
		const auto& nameMap = EnumClass::GetMap();
		Vector<const char*> names;

		for (auto it = nameMap.begin(); it != nameMap.end(); it++)
		{
			names.Add(it->second.data());
		}

		const int value = static_cast<int>(g_gameConfig.GetEnum<EnumClass>(key));
		const int newValue = SelectionInput(value, names, label);

		if (newValue != value) {
			g_gameConfig.SetEnum<EnumClass>(key, nameMap.FromString(names[newValue]));
			return true;
		}
		return false;
	}

	int SelectionInput(int val, const Vector<const char*>& options, const std::string_view& label);
	bool SelectionSetting(GameConfigKeys key, const Vector<const char*>& options, const std::string_view& label);
	bool StringSelectionSetting(GameConfigKeys key, const Vector<String>& options, const std::string_view& label);

	int IntInput(int val, const std::string_view& label, int min, int max, int step = 1, float perPixel = 1.0f);
	bool IntSetting(GameConfigKeys key, const std::string_view& label, int min, int max, int step = 1, float perPixel = 1.0f);

	float FloatInput(float val, const std::string_view& label, float min, float max, float step, float perPixel);
	bool FloatSetting(GameConfigKeys key, const std::string_view& label, float min, float max, float step = 0.01f);
	bool PercentSetting(GameConfigKeys key, const std::string_view& label);

	Color ColorInput(const Color& val, const std::string_view& label, bool& useHSV);

public:
	void Init();
	void Exit();

	void Render(const struct nk_rect& rect);

	inline const String& GetName() const { return m_name; }

protected:
	nk_context* m_nctx = nullptr;
	String m_name;

	int m_buttonHeight = 30;
	struct nk_vec2 m_comboBoxSize = nk_vec2(1050, 250);
};

class SettingsPageCollection : public BasicNuklearGui
{
public:
	bool Init() override;

	~SettingsPageCollection() override;

	void Tick(float deltaTime) override;

	void Render(float deltaTime) override;

	void OnKeyPressed(SDL_Scancode code) override;

	void Exit();

	void Reload();

protected:
	virtual void AddPages(Vector<std::unique_ptr<SettingsPage>>& pages) = 0;

private:
	bool m_forceReload = false;
	bool m_forcePortrait = false;

private:
	Vector<std::unique_ptr<SettingsPage>> m_pages;
	
	float m_pageButtonHeight = 40;

	constexpr static int PAGE_NAME_SIZE = 18;
	Vector<Ref<TextRes>> m_pageNames;

	Ref<TextRes> m_exitText = nullptr;

	size_t m_currPage = 0;

	struct nk_rect m_pageHeaderRegion;
	struct nk_rect m_pageContentRegion;

	struct nk_rect m_profileButtonRegion;
	struct nk_rect m_exitButtonRegion;

	bool m_enableSwitchPageOnHover = false;
	Vector2i m_prevMousePos;
	int m_prevMouseInd = -1;

	void InitPages();

	void RenderPages();

	void UpdatePageRegions();
	void RenderPageHeaders();
	void RenderPageContents();

	void ProcessTabHandleMouseHover(const Vector2i& mousePos);

	void OnMousePressed(MouseButton button);
	int GetPageIndFromMousePos(const Vector2i& mousePos) const;
};

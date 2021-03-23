#pragma once
#include "GuiUtils.hpp"

class SettingsPage
{
protected:
	SettingsPage(nk_context* nctx, const std::string_view& name) : m_nctx(nctx), m_name(name) {}

	virtual void Load() = 0;
	virtual void Save() = 0;

	virtual void RenderContents() = 0;

	class TextSettingData
	{
	public:
		TextSettingData(GameConfigKeys key) : m_key(key) {}

		void Load();
		void Save();

		void Render(nk_context* nctx);
		void RenderPassword(nk_context* nctx);

	protected:
		constexpr static size_t BUFFER_SIZE = 1024;

		GameConfigKeys m_key;
		std::array<char, BUFFER_SIZE> m_buffer;
		int m_len = 0;
	};

	// Useful elements
	inline void LayoutRowDynamic(int num_columns) { LayoutRowDynamic(num_columns, m_buttonHeight); }
	void LayoutRowDynamic(int num_columns, float height);

	bool ToggleSetting(GameConfigKeys key, const std::string_view& label);

	template<typename EnumClass>
	bool EnumSetting(GameConfigKeys key, const std::string_view& label)
	{
		EnumStringMap<typename EnumClass::EnumType> nameMap = EnumClass::GetMap();
		Vector<const char*> names;

		int value = (int)g_gameConfig.GetEnum<EnumClass>(key);
		const int prevValue = value;

		for (auto it = nameMap.begin(); it != nameMap.end(); it++)
		{
			names.Add(*(*it).second);
		}

		nk_label(m_nctx, label.data(), nk_text_alignment::NK_TEXT_LEFT);
		nk_combobox(m_nctx, names.data(), names.size(), &value, m_buttonHeight, m_comboBoxSize);
		if (prevValue != value) {
			g_gameConfig.SetEnum<EnumClass>(key, nameMap.FromString(names[value]));
			return true;
		}
		return false;
	}

	bool SelectionSetting(GameConfigKeys key, const Vector<const char*>& options, const std::string_view& label);
	bool StringSelectionSetting(GameConfigKeys key, const Vector<String>& options, const std::string_view& label);

	int IntInput(int val, const std::string_view& label, int min, int max, int step = 1, float perPixel = 1.0f);
	bool IntSetting(GameConfigKeys key, const std::string_view& label, int min, int max, int step = 1, float perPixel = 1.0f);

	float FloatInput(float val, const std::string_view& label, float min, float max, float step, float perPixel);
	bool FloatSetting(GameConfigKeys key, const std::string_view& label, float min, float max, float step = 0.01f);
	bool PercentSetting(GameConfigKeys key, const std::string_view& label);

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

protected:
	virtual void AddPages(Vector<std::unique_ptr<SettingsPage>>& pages) = 0;

private:
	Vector<String> m_profiles;
	String m_currentProfile;
	bool m_needsProfileReboot = false;

	void InitProfile();
	void RefreshProfile();

private:
	Vector<std::unique_ptr<SettingsPage>> m_pages;
	size_t m_currPage = 0;

	struct nk_rect m_pageHeaderRegion;
	struct nk_rect m_pageContentRegion;

	void InitPages();

	void RenderPages();

	void UpdatePageRegions();
	void RenderPageHeaders();
	void RenderPageContents();
};

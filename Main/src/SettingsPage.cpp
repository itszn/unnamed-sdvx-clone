#include "stdafx.h"
#include "SettingsPage.hpp"

#include <Shared/Files.hpp>

#include "Application.hpp"
#include "SettingsScreen.hpp"

constexpr static int NK_PROPERTY_DEFAULT = 0;

static inline void nk_sdl_text(nk_flags event)
{
	if (event & NK_EDIT_ACTIVATED)
	{
		SDL_StartTextInput();
	}
	if (event & NK_EDIT_DEACTIVATED)
	{
		SDL_StopTextInput();
	}
}

static inline int nk_get_property_state(struct nk_context* ctx, const char* name)
{
	if (!ctx || !ctx->current || !ctx->current->layout) return NK_PROPERTY_DEFAULT;
	struct nk_window* win = ctx->current;
	nk_hash hash = 0;
	if (name[0] == '#') {
		hash = nk_murmur_hash(name, (int)nk_strlen(name), win->property.seq++);
		name++; /* special number hash */
	}
	else hash = nk_murmur_hash(name, (int)nk_strlen(name), 42);

	if (win->property.active && hash == win->property.name)
		return win->property.state;
	return NK_PROPERTY_DEFAULT;
}

void SettingsPage::SettingTextData::Load()
{
	String str = LoadConfig();

	if (str.length() >= m_buffer.size())
	{
		Logf("A config has been cropped due to being too long (%d)", Logger::Severity::Error, m_len);
		m_len = static_cast<int>(m_buffer.size() - 1);
	}
	else
	{
		m_len = static_cast<int>(str.length());
	}

	std::memcpy(m_buffer.data(), str.data(), m_len + 1);
}

void SettingsPage::SettingTextData::Save()
{
	String str = String(m_buffer.data(), m_len);

	str.TrimBack('\n');
	str.TrimBack(' ');

	SaveConfig(str);
}

void SettingsPage::SettingTextData::Render(nk_context* nctx)
{
	nk_sdl_text(nk_edit_string(nctx, NK_EDIT_FIELD, m_buffer.data(), &m_len, static_cast<int>(m_buffer.size()), nk_filter_default));
}

void SettingsPage::SettingTextData::RenderPassword(nk_context* nctx)
{
	// Hack taken from https://github.com/vurtun/nuklear/blob/a9e5e7299c19b8a8831a07173211fa8752d0cc8c/demo/overview.c#L549
	const int old_len = m_len;

	std::array<char, BUFFER_SIZE> tokenBuffer;
	std::fill(tokenBuffer.begin(), tokenBuffer.begin() + m_len, '*');

	nk_sdl_text(nk_edit_string(nctx, NK_EDIT_FIELD, tokenBuffer.data(), &m_len, 1024, nk_filter_default));

	if (old_len < m_len)
	{
		std::memcpy(m_buffer.data() + old_len, tokenBuffer.data() + old_len, m_len - old_len);
	}
}

String SettingsPage::GameConfigTextData::LoadConfig()
{
	return g_gameConfig.GetString(m_key);
}

void SettingsPage::GameConfigTextData::SaveConfig(const String& value)
{
	g_gameConfig.Set(m_key, value);
}

void SettingsPage::LayoutRowDynamic(int num_columns, float height)
{
	nk_layout_row_dynamic(m_nctx, height, num_columns);
}

void SettingsPage::Separator()
{
	nk_labelf(m_nctx, nk_text_alignment::NK_TEXT_CENTERED, "_______________________");
}

void SettingsPage::Label(const std::string_view& label)
{
	nk_label(m_nctx, label.data(), nk_text_alignment::NK_TEXT_LEFT);
}

bool SettingsPage::ToggleInput(bool val, const std::string_view& label)
{
	constexpr static int TRUE_VAL = 0;
	constexpr static int FALSE_VAL = 1;

	int val_i = val ? TRUE_VAL : FALSE_VAL;
	nk_checkbox_label(m_nctx, label.data(), &val_i);

	return val_i == TRUE_VAL;
}

bool SettingsPage::ToggleSetting(GameConfigKeys key, const std::string_view& label)
{
	const bool value = g_gameConfig.GetBool(key);
	const bool newValue = ToggleInput(value, label);

	if (newValue != value)
	{
		g_gameConfig.Set(key, newValue);
		return true;
	}
	else
	{
		return false;
	}
}

int SettingsPage::SelectionInput(int val, const Vector<const char*>& options, const std::string_view& label)
{
	assert(options.size() > 0);

	nk_label(m_nctx, label.data(), nk_text_alignment::NK_TEXT_LEFT);
	nk_combobox(m_nctx, const_cast<const char**>(options.data()), static_cast<int>(options.size()), &val, m_buttonHeight, m_comboBoxSize);

	return val;
}

bool SettingsPage::SelectionSetting(GameConfigKeys key, const Vector<const char*>& options, const std::string_view& label)
{
	assert(options.size() > 0);

	const int value = g_gameConfig.GetInt(key) % options.size();
	const int newValue = SelectionInput(value, options, label);

	if (newValue != value)
	{
		g_gameConfig.Set(key, newValue);
		return true;
	}

	return false;
}

bool SettingsPage::StringSelectionSetting(GameConfigKeys key, const Vector<String>& options, const std::string_view& label)
{
	String value = g_gameConfig.GetString(key);
	int selection = 0;

	const auto stringSearch = std::find(options.begin(), options.end(), value);
	if (stringSearch != options.end())
	{
		selection = static_cast<int>(stringSearch - options.begin());
	}

	Vector<const char*> displayData;
	for (const String& s : options)
	{
		displayData.Add(s.data());
	}

	const int newSelection = SelectionInput(selection, displayData, label);

	if (newSelection != selection) {
		g_gameConfig.Set(key, options[newSelection]);
		return true;
	}
	return false;
}
	
int SettingsPage::IntInput(int val, const std::string_view& label, int min, int max, int step, float perPixel)
{
	const int oldState = nk_get_property_state(m_nctx, label.data());
	val = nk_propertyi(m_nctx, label.data(), min, val, max, step, perPixel);
	const int newState = nk_get_property_state(m_nctx, label.data());

	if (oldState != newState) {
		if (newState == NK_PROPERTY_DEFAULT)
			SDL_StopTextInput();
		else
			SDL_StartTextInput();
	}

	return val;
}

bool SettingsPage::IntSetting(GameConfigKeys key, const std::string_view& label, int min, int max, int step, float perPixel)
{
	const int value = g_gameConfig.GetInt(key);
	const int newValue = IntInput(value, label, min, max, step, perPixel);
	if (newValue != value) {
		g_gameConfig.Set(key, newValue);
		return true;
	}
	return false;
}

float SettingsPage::FloatInput(float val, const std::string_view& label, float min, float max, float step, float perPixel)
{
	const int oldState = nk_get_property_state(m_nctx, label.data());
	val = nk_propertyf(m_nctx, label.data(), min, val, max, step, perPixel);
	const int newState = nk_get_property_state(m_nctx, label.data());

	if (oldState != newState) {
		if (newState == NK_PROPERTY_DEFAULT)
			SDL_StopTextInput();
		else
			SDL_StartTextInput();
	}

	return val;
}

bool SettingsPage::FloatSetting(GameConfigKeys key, const std::string_view& label, float min, float max, float step)
{
	float value = g_gameConfig.GetFloat(key);
	const float prevValue = value;

	// nuklear supports precision only up to 2 decimal places (wtf)
	if (step >= 0.01f)
	{
		float incPerPixel = step;
		if (incPerPixel >= step / 2) incPerPixel = step * Math::Round(incPerPixel / step);

		value = FloatInput(value, label.data(), min, max, step, incPerPixel);
	}
	else
	{
		nk_labelf(m_nctx, nk_text_alignment::NK_TEXT_LEFT, label.data(), value);
		nk_slider_float(m_nctx, min, &value, max, step);
	}

	if (value != prevValue) {
		g_gameConfig.Set(key, value);
		return true;
	}

	return false;
}

bool SettingsPage::PercentSetting(GameConfigKeys key, const std::string_view& label)
{
	float value = g_gameConfig.GetFloat(key);
	const float prevValue = value;

	nk_labelf(m_nctx, nk_text_alignment::NK_TEXT_LEFT, label.data(), value * 100);
	nk_slider_float(m_nctx, 0, &value, 1, 0.005f);

	if (value != prevValue)
	{
		g_gameConfig.Set(key, value);
		return true;
	}
	else
	{
		return false;
	}
}

Color SettingsPage::ColorInput(const Color& val, const std::string_view& label, bool& useHSV) {
	LayoutRowDynamic(1);
	nk_label(m_nctx, label.data(), nk_text_alignment::NK_TEXT_LEFT);
	nk_colorf nkCol = { val.x, val.y, val.z, val.w };

	if (nk_combo_begin_color(m_nctx, nk_rgb_cf(nkCol), nk_vec2(200, 400))) {
		enum color_mode { COL_RGB, COL_HSV };

		LayoutRowDynamic(1, 120);
		nkCol = nk_color_picker(m_nctx, nkCol, NK_RGBA);

		LayoutRowDynamic(2, 25);
		useHSV = nk_option_label(m_nctx, "RGB", useHSV ? 1 : 0) == 1;
		useHSV = nk_option_label(m_nctx, "HSV", useHSV ? 0 : 1) == 0;

		LayoutRowDynamic(1, 25);
		if (useHSV == false) {
			nkCol.r = FloatInput(nkCol.r, "#R:", 0,  1.0f, 0.01f, 0.005f);
			nkCol.g = FloatInput(nkCol.g, "#G:", 0, 1.0f, 0.01f, 0.005f);
			nkCol.b = FloatInput(nkCol.b, "#B:", 0, 1.0f, 0.01f, 0.005f);
			nkCol.a = FloatInput(nkCol.a, "#A:", 0, 1.0f, 0.01f, 0.005f);
		}
		else {
			std::array<float, 4> hsva;
			nk_colorf_hsva_fv(hsva.data(), nkCol);
			hsva[0] = FloatInput(hsva[0], "#H:", 0, 1.0f, 0.01f, 0.05f);
			hsva[1] = FloatInput(hsva[1], "#S:", 0, 1.0f, 0.01f, 0.05f);
			hsva[2] = FloatInput(hsva[2], "#V:", 0, 1.0f, 0.01f, 0.05f);
			hsva[3] = FloatInput(hsva[3], "#A:", 0, 1.0f, 0.01f, 0.05f);
			nkCol = nk_hsva_colorfv(hsva.data());
		}
		nk_combo_end(m_nctx);
	}

	LayoutRowDynamic(1);

	return Color(nkCol.r, nkCol.g, nkCol.b, nkCol.a);
}

void SettingsPage::Init()
{
	Load();
}

void SettingsPage::Exit()
{
	Save();
}

void SettingsPage::Render(const struct nk_rect& rect)
{
	m_comboBoxSize.x = rect.x - 30;

	if (nk_begin(m_nctx, m_name.data(), rect, NK_WINDOW_NO_SCROLLBAR))
	{
		RenderContents();
		nk_end(m_nctx);
	}
}


bool SettingsPageCollection::Init()
{
	BasicNuklearGui::Init();
	InitPages();

	return true;
}

SettingsPageCollection::~SettingsPageCollection()
{
	for (auto& page : m_pages)
	{
		page->Exit();
	}

	g_application->ApplySettings();
}

void SettingsPageCollection::Tick(float deltaTime)
{
	if (m_needsProfileReboot)
	{
		RefreshProfile();
		return;
	}

	BasicNuklearGui::Tick(deltaTime);
}

void SettingsPageCollection::Render(float deltaTime)
{
	if (IsSuspended())
	{
		return;
	}

	RenderPages();

	BasicNuklearGui::NKRender();
}

void SettingsPageCollection::OnKeyPressed(SDL_Scancode code)
{
	if (IsSuspended())
	{
		return;
	}

	if (code == SDL_SCANCODE_ESCAPE)
	{
		Exit();
	}
}

void SettingsPageCollection::Exit()
{
	for (auto& page : m_pages)
	{
		page->Exit();
	}

	g_gameWindow->OnAnyEvent.RemoveAll(this);

	g_input.Cleanup();
	g_input.Init(*g_gameWindow);

	g_application->RemoveTickable(this);
}

void SettingsPageCollection::InitProfile()
{
	m_currentProfile = g_gameConfig.GetString(GameConfigKeys::CurrentProfileName);
	m_profiles.push_back("Main");

	Vector<FileInfo> profiles = Files::ScanFiles(Path::Absolute("profiles/"), "cfg", NULL);

	for (const auto& file : profiles)
	{
		String profileName = "";
		String unused = Path::RemoveLast(file.fullPath, &profileName);
		profileName = profileName.substr(0, profileName.length() - 4); // Remove .cfg
		m_profiles.push_back(profileName);
	}
}

void SettingsPageCollection::RefreshProfile()
{
	// TODO: just refresh pages and remove the include for SettingsScreen

	String newProfile = g_gameConfig.GetString(GameConfigKeys::CurrentProfileName);

	// Save old settings
	g_gameConfig.Set(GameConfigKeys::CurrentProfileName, m_currentProfile);
	Exit();

	g_application->ApplySettings();

	// Load in new settings
	g_application->ReloadConfig(newProfile);

	g_application->AddTickable(SettingsScreen::Create());
}

void SettingsPageCollection::InitPages()
{
	m_pages.clear();

	AddPages(m_pages);

	for (const auto& page : m_pages)
	{
		page->Init();
	}

	m_currPage = 0;
}

void SettingsPageCollection::UpdatePageRegions()
{
	const float SETTINGS_DESIRED_CONTENTS_WIDTH = g_resolution.y / 1.4f;
	const float SETTINGS_DESIRED_HEADERS_WIDTH = 120.0f;

	const float SETTINGS_WIDTH = Math::Min(SETTINGS_DESIRED_CONTENTS_WIDTH + SETTINGS_DESIRED_HEADERS_WIDTH, g_resolution.x - 5.0f);
	const float SETTINGS_CONTENTS_WIDTH = Math::Max(SETTINGS_WIDTH * 0.75f, SETTINGS_WIDTH - SETTINGS_DESIRED_HEADERS_WIDTH);
	const float SETTINGS_HEADERS_WIDTH = SETTINGS_WIDTH - SETTINGS_CONTENTS_WIDTH;

	// Better to keep current layout if there's not enough space
	if (SETTINGS_CONTENTS_WIDTH < 10.0f || SETTINGS_HEADERS_WIDTH < 10.0f)
	{
		return;
	}

	const float SETTINGS_OFFSET_X = (g_resolution.x - SETTINGS_WIDTH) / 2;
	const float SETTINGS_CONTENTS_OFFSET_X = SETTINGS_OFFSET_X + SETTINGS_HEADERS_WIDTH;

	m_pageHeaderRegion = { SETTINGS_OFFSET_X, 0, SETTINGS_HEADERS_WIDTH, (float)g_resolution.y };
	m_pageContentRegion = { SETTINGS_CONTENTS_OFFSET_X, 0, SETTINGS_CONTENTS_WIDTH, (float)g_resolution.y };
}

void SettingsPageCollection::RenderPages()
{
	UpdatePageRegions();
	RenderPageHeaders();
	RenderPageContents();
}

void SettingsPageCollection::RenderPageHeaders()
{
	if (nk_begin(m_nctx, "Pages", m_pageHeaderRegion, NK_WINDOW_NO_SCROLLBAR))
	{
		nk_layout_row_dynamic(m_nctx, 50, 1);

		for (size_t i = 0; i < m_pages.size(); ++i)
		{
			const auto& page = m_pages[i];
			const String& name = page->GetName();

			if (nk_button_text(m_nctx, name.c_str(), static_cast<int>(name.size())))
			{
				m_currPage = i;
			}
		}

		if (nk_button_label(m_nctx, "Exit")) Exit();
		nk_end(m_nctx);
	}
}

void SettingsPageCollection::RenderPageContents()
{
	if (m_currPage >= m_pages.size())
	{
		return;
	}

	m_pages[m_currPage]->Render(m_pageContentRegion);
}

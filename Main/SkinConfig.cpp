#include "stdafx.h"
#include "SkinConfig.hpp"
#include "json.hpp"
#include "fifo_map.hpp"

using namespace nlohmann;


// https://github.com/nlohmann/json/issues/485#issuecomment-333652309
// A workaround to give to use fifo_map as map, we are just ignoring the 'less' compare
template<class K, class V, class dummy_compare, class A>
using workaround_fifo_map = fifo_map<K, V, fifo_map_compare<K>, A>;

SkinConfig::SkinConfig(String skin)
{
	m_skin = skin;


	using ordered_json = basic_json<workaround_fifo_map>;

	Map<String, SkinSetting::Type> inputModeMap = {
		{ "selection", SkinSetting::Type::Selection },
		{ "bool", SkinSetting::Type::Boolean },
		{ "int", SkinSetting::Type::Integer},
		{ "float", SkinSetting::Type::Float},
		{ "label", SkinSetting::Type::Label},
		{ "text", SkinSetting::Type::Text},
	};

	File defFile;
	if(defFile.OpenRead(Path::Normalize("skins/" + skin + "/config-definitions.json")));
	{
		Buffer buf(defFile.GetSize());
		defFile.Read(buf.data(), buf.size());
		String jsonData((char*)buf.data(), buf.size());
		auto definitions = ordered_json::parse(*jsonData);
		for (auto entry : definitions.items())
		{
			SkinSetting newsetting;
      Logf("Adding skin config entry : %s", Logger::Info, entry.key());
			if (entry.key() == "separator")
			{
				newsetting.type = SkinSetting::Type::Separator;
				m_settings.Add(newsetting);
				continue;
			}
			auto values = entry.value();
			newsetting.key = entry.key();
			if (values.contains("label"))
			{
				newsetting.label = (std::string)values.at("label");
			}
			else
			{
				newsetting.label = entry.key();
			}
			newsetting.type = inputModeMap.at(values.at("type"));
			switch (newsetting.type)
			{
			case SkinSetting::Type::Selection:
				newsetting.selectionSetting.def = strdup(((std::string)values.at("default")).c_str());
				newsetting.selectionSetting.numOptions = values.at("values").size();
				newsetting.selectionSetting.options = new String[newsetting.selectionSetting.numOptions];
				for (size_t i = 0; i < newsetting.selectionSetting.numOptions; i++)
				{
					newsetting.selectionSetting.options[i] = (std::string)values.at("values").at(i);
				}
				break;

			case SkinSetting::Type::Boolean:
				newsetting.boolSetting.def = values.at("default");
				break;

			case SkinSetting::Type::Text:
				newsetting.textSetting.def = strdup(((std::string)values.at("default")).c_str());
				break;

			case SkinSetting::Type::Integer:
				newsetting.intSetting.def = values.at("default");
				newsetting.intSetting.max = values.at("max");
				newsetting.intSetting.min = values.at("min");
				break;

			case SkinSetting::Type::Float:
				newsetting.floatSetting.def = values.at("default");
				newsetting.floatSetting.max = values.at("max");
				newsetting.floatSetting.min = values.at("min");
				break;
			}
			m_settings.Add(newsetting);
		}
	}

  InitDefaults();
	Load(Path::Normalize("skins/" + skin + "/skin.cfg"));
}

SkinConfig::~SkinConfig()
{
	for (auto& it : m_keys)
	{
		m_reverseKeys.Add(it.second, it.first);
	}

	Save(Path::Normalize("skins/" + m_skin + "/skin.cfg"));
}

bool SkinConfig::IsSet(String key) const
{
	return (m_keys.Contains(key) && m_entries.Contains(m_keys.at(key)));
}

const Vector<SkinSetting>& SkinConfig::GetSettings() const
{
	return m_settings;
}

void SkinConfig::InitDefaults()
{
	for (auto& setting : m_settings)
	{
		String def;
		switch (setting.type)
		{
		case SkinSetting::Type::Selection:
			def = setting.selectionSetting.def;
			Set(setting.key, def);
			break;
		case SkinSetting::Type::Boolean:
			Set(setting.key, setting.boolSetting.def);
			break;
		case SkinSetting::Type::Integer:
			Set(setting.key, setting.intSetting.def);
			break;
		case SkinSetting::Type::Float:
			Set(setting.key, setting.floatSetting.def);
			break;
		case SkinSetting::Type::Text:
			def = setting.textSetting.def;
			Set(setting.key, def);
			break;
		}
	}
}

int32 SkinConfig::GetInt(String key) const
{
	return GetEnsure<IntConfigEntry>(key)->data;
}
float SkinConfig::GetFloat(String key) const
{
	return GetEnsure<FloatConfigEntry>(key)->data;
}
String SkinConfig::GetString(String key) const
{
	return GetEnsure<StringConfigEntry>(key)->data;
}
bool SkinConfig::GetBool(String key) const
{
	return GetEnsure<BoolConfigEntry>(key)->data;
}


void SkinConfig::Set(String key, const int32& value)
{
	int32& dst = SetEnsure<IntConfigEntry>(key)->data;
	if (dst != value)
	{
		dst = value;
		m_dirty = true;
	}
}
void SkinConfig::Set(String key, const float& value)
{
	float& dst = SetEnsure<FloatConfigEntry>(key)->data;
	if (dst != value)
	{
		dst = value;
		m_dirty = true;
	}
}
void SkinConfig::Set(String key, const bool& value)
{
	bool& dst = SetEnsure<BoolConfigEntry>(key)->data;
	if (dst != value)
	{
		dst = value;
		m_dirty = true;
	}
}
void SkinConfig::Set(String key, const String& value)
{
	String& dst = SetEnsure<StringConfigEntry>(key)->data;
	if (dst != value)
	{
		dst = value;
		m_dirty = true;
	}
}

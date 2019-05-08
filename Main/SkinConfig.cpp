#include "stdafx.h"
#include "SkinConfig.hpp"
#include "../third_party/nlohmann_json/json.hpp"

using namespace nlohmann;


SkinConfig::SkinConfig(String skin)
{
	m_skin = skin;


	Map<String, SkinSetting::Type> inputModeMap = {
		{ "selection", SkinSetting::Type::Selection },
		{ "bool", SkinSetting::Type::Bool },
		{ "int", SkinSetting::Type::Int},
		{ "float", SkinSetting::Type::Float},
	};

	File defFile;
	if(defFile.OpenRead(Path::Normalize("skins/" + skin + "/config-definitions.json")));
	{
		Buffer buf(defFile.GetSize());
		defFile.Read(buf.data(), buf.size());
		String jsonData((char*)buf.data(), buf.size());
		auto definitions = json::parse(*jsonData);
		for (auto entry : definitions.items())
		{
			SkinSetting newsetting;
			auto values = entry.value();
			newsetting.key = entry.key();
			newsetting.type = inputModeMap.at(values.at("type"));
			switch (newsetting.type)
			{
			case SkinSetting::Type::Selection:
				newsetting.selectionSetting.default = strdup(*(String)values.at("default"));
				newsetting.selectionSetting.numOptions = values.at("values").size();
				newsetting.selectionSetting.options = new String[newsetting.selectionSetting.numOptions];
				for (size_t i = 0; i < newsetting.selectionSetting.numOptions; i++)
				{
					newsetting.selectionSetting.options[i] = (String)values.at("values").at(i);
				}
				break;

			case SkinSetting::Type::Bool:
				newsetting.boolSetting.default = values.at("default");
				break;

			case SkinSetting::Type::Int:
				newsetting.intSetting.default = values.at("default");
				newsetting.intSetting.max = values.at("max");
				newsetting.intSetting.min = values.at("min");
				break;

			case SkinSetting::Type::Float:
				newsetting.floatSetting.default = values.at("default");
				newsetting.floatSetting.max = values.at("max");
				newsetting.floatSetting.min = values.at("min");
				break;
			}
			m_settings.Add(newsetting);
		}
	}

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

void SkinConfig::InitDefaults()
{
	for (auto& setting : m_settings)
	{
		String def(setting.selectionSetting.default);
		switch (setting.type)
		{
		case SkinSetting::Type::Selection:
			Set(setting.key, def);
			break;
		case SkinSetting::Type::Bool:
			Set(setting.key, setting.boolSetting.default);
			break;
		case SkinSetting::Type::Int:
			Set(setting.key, setting.intSetting.default);
			break;
		case SkinSetting::Type::Float:
			Set(setting.key, setting.floatSetting.default);
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

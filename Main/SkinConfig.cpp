#include "SkinConfig.hpp"

SkinConfig::SkinConfig(String skin)
{
}

bool SkinConfig::IsSet(String key) const
{
	return (m_keys.Contains(key) && m_entries.Contains(m_keys.at(key)));
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


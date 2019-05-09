#pragma once
#include "Shared/String.hpp"
#include "Shared/Color.hpp"

class IConfigEntry
{
public:
	enum class EntryType
	{
		Integer,
		Float,
		Boolean,
		String,
		Color,
		Enum
	};

	virtual ~IConfigEntry() = default;
	// Converts entry to string value
	virtual String ToString() const = 0;
	// Sets the current entry from a string value
	virtual void FromString(const String& str) = 0;

	virtual EntryType GetType() = 0;

	template<typename T> T* As() { return dynamic_cast<T*>(this); }
};

class IntConfigEntry : public IConfigEntry
{
public:
	int32 data;
public:
	virtual String ToString() const override;
	virtual void FromString(const String& str) override;
	EntryType GetType() override { return EntryType::Integer; };
};

class BoolConfigEntry : public IConfigEntry
{
public:
	bool data;
public:
	virtual String ToString() const override;
	virtual void FromString(const String& str) override;
	EntryType GetType() override { return EntryType::Boolean; };
};

class FloatConfigEntry : public IConfigEntry
{
public:
	float data;
public:
	virtual String ToString() const override;
	virtual void FromString(const String& str) override;
	EntryType GetType() override { return EntryType::Float; };
};

class StringConfigEntry : public IConfigEntry
{
public:
	String data;
public:
	virtual String ToString() const override;
	virtual void FromString(const String& str) override;
	EntryType GetType() override { return EntryType::String; };
};

class ColorConfigEntry : public IConfigEntry
{
public:
	Color data;
	virtual String ToString() const override;
	virtual void FromString(const String& str) override;
	EntryType GetType() override { return EntryType::Color; };
};

template<typename EnumClass>
class EnumConfigEntry : public IConfigEntry
{
public:
	typename EnumClass::EnumType data;
public:
	virtual String ToString() const override
	{
		return EnumClass::ToString(data);
	}
	virtual void FromString(const String& str) override
	{
		data = EnumClass::FromString(str);
	}
	EntryType GetType() override { return EntryType::Enum; };
};
#pragma once

/*
	An enum for common string encodings.
	
	When a new encoding must be supported, be sure to update these functions:
	- GetDisplayString
	- StringEncodingConverter::GetIConvArg
*/

enum class StringEncoding
{
	Unknown,
	UTF8, ASCII = UTF8,
	CP932, ShiftJIS = CP932,
	CP949, EUC_KR = CP949
};

constexpr const char* GetDisplayString(const StringEncoding encoding)
{
	switch (encoding)
	{
	case StringEncoding::UTF8:
		return "UTF-8";
	case StringEncoding::CP932:
		return "CP932 (or ShiftJIS)";
	case StringEncoding::CP949:
		return "CP949 (or EUC-KR)";
	case StringEncoding::Unknown:
		return "Unknown";
	default:
		return "[an unknown encoding]";
	}
}
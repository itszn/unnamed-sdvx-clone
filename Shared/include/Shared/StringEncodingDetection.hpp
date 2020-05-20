#pragma once

#include "Shared/String.hpp"

class BinaryStream;

// Encoding detection for commonly-used encodings

class StringEncodingDetector final
{
public:
	enum class Encoding
	{
		Unknown = 0,
		// Unicode
		UTF8,
		// Japanese
		ShiftJIS,
		// Korean
		CP949,
	};

	static Encoding Detect(const char* str);
	inline static Encoding Detect(String& str) { return Detect(str.c_str()); }
	static Encoding Detect(BinaryStream& stream);

protected:
	StringEncodingDetector(BinaryStream& stream) : m_stream(stream) {}

	Encoding Detect();

	bool IsValidUTF8();

	// Score: negative for invalid, lower is better
	void GetScores(int& score_shift_jis, int& score_cp949);

	BinaryStream& m_stream;

	// Max. bytes to be examined.
	constexpr static size_t MAX_READ_FOR_ENCODING_DETECTION = 64;
};
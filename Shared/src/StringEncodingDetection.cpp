#include "stdafx.h"
#include "StringEncodingDetection.hpp"

#include "Shared/String.hpp"
#include "Shared/Buffer.hpp"
#include "Shared/MemoryStream.hpp"
#include "Shared/BinaryStream.hpp"
#include "iconv.h"

/*
	Encoding detection for commonly-used encodings
	----------------------------------------------
	The heuristic works in the following way:
	* If the given bytes are valid in UTF-8, then it's considered as UTF-8.
	* If exactly one of Shift-JIS and CP949 are valid, then the valid encoding is picked.
		- PUA characters are not considered as valid.
	* If both are valid, then one with lower heuristic value is considered.
		- Heuristic value of each character is defined by the CharClass they belong.
		- Currently there is no global panelty values. It can be added though.
		- Tie is broken towards Shift-JIS.
*/

// Feel free to tweak these values
enum class CharClass
{
	INVALID = -1,
	ALNUM = 10, // 0-9, A-Z, a-z
	PUNCTUATION = 15, // ASCII symbols, 
	KANA = 20, // Full-width and half-width kana
	BASIC_HANGUL = 20, // Hangul in KS X 1001
	COMMON_KANJI = 30, // Kanji in JIS X 0208
	SYMBOLS = 40,
	COMPLEX_HANGUL = 60, // Hangul not in KS X 1001
	RARE_KANJI = 80, // Kanji not in JIS X 0208
	OTHER_CHARS = 100,
};

static inline bool IsPrintableAscii(uint8_t ch)
{
	return ch == 0x09 || ch == 0x0A || ch == 0x0D || 0x20 <= ch && ch <= 0x7E;
}

static inline CharClass GetAsciiCharClass(uint8_t ch)
{
	if (ch == 0x09 || ch == 0x0A || ch == 0x0D)
		return CharClass::PUNCTUATION;
	if (0x30 <= ch && ch <= 0x39 || 0x41 <= ch && ch <= 0x5A || 0x61 <= ch && ch <= 0x7A)
		return CharClass::ALNUM;
	if (0x20 <= ch && ch <= 0x7E)
		return CharClass::PUNCTUATION;

	return CharClass::INVALID;
}

#pragma region Util functions for Shift_JIS and CP949

class EncodingHeuristic
{
public:
	EncodingHeuristic() = default;

	inline int GetScore();
	inline bool IsValid() { return GetScore() >= 0; }
	inline bool Consume(uint8_t ch);
	inline bool Finalize();

protected:
	virtual bool RequiresSecondByte(uint8_t ch) = 0;
	virtual CharClass GetCharClass(uint16_t ch) = 0;

	inline bool Process(uint16_t ch);

	uint8_t m_hi = 0;
	int m_score = 0;
};

inline int EncodingHeuristic::GetScore()
{
	return m_score;
}

inline bool EncodingHeuristic::Consume(uint8_t ch)
{
	if (!IsValid())
	{
		return false;
	}

	if (m_hi == 0)
	{
		if (RequiresSecondByte(ch))
		{
			m_hi = ch;
			return true;
		}

		return Process(ch);
	}

	uint16_t curr = (static_cast<uint16_t>(m_hi) << 8) | ch;
	m_hi = 0;

	return Process(curr);
}

inline bool EncodingHeuristic::Process(uint16_t ch)
{
	CharClass charClass = GetCharClass(ch);
	if (charClass == CharClass::INVALID)
	{
		m_score = -1;
		return false;
	}
	else
	{
		m_score += static_cast<int>(charClass);
		return true;
	}
}

inline bool EncodingHeuristic::Finalize()
{
	if (m_hi) m_score = -1;

	return IsValid();
}

class ShiftJISHeuristic : public EncodingHeuristic
{
protected:
	bool RequiresSecondByte(uint8_t ch) override
	{
		return 0x81 <= ch && ch <= 0x9F || 0xE0 <= ch && ch <= 0xFC;
	}

	CharClass GetCharClass(uint16_t ch) override
	{
		if (ch <= 0x80) return GetAsciiCharClass(ch);
		return CharClass::OTHER_CHARS;
	}
};

class CP949Heuristic : public EncodingHeuristic
{
protected:
	bool RequiresSecondByte(uint8_t ch) override
	{
		return 0x81 <= ch && ch <= 0xC6 || 0xA1 <= ch && ch <= 0xFE;
	}

	CharClass GetCharClass(uint16_t ch) override
	{
		// KS X 1003
		if (ch < 0x80) return GetAsciiCharClass(ch);
		// KS X 1001
		return CharClass::OTHER_CHARS;
	}
};

#pragma endregion

// Detect contents of the string
StringEncodingDetector::Encoding StringEncodingDetector::Detect(const char* str)
{
	// This is inefficient, because stringBuffer copies contents of str. Copying is unnecessary.
	// A version of MemoryStream which does not use a Buffer is preferable.
	::Buffer stringBuffer(str);
	::MemoryReader memoryReader(stringBuffer);

	return Detect(memoryReader);
}

StringEncodingDetector::Encoding StringEncodingDetector::Detect(BinaryStream& stream)
{
	assert(stream.IsReading());
	size_t init_pos = stream.Tell();

	// If the size is too small, the encoding can't be detected realiably.
	if (stream.GetSize() < 2) return Encoding::Unknown;

	StringEncodingDetector detector(stream);
	Encoding result = detector.Detect();

	stream.Seek(init_pos);
	return result;
}

StringEncodingDetector::Encoding StringEncodingDetector::Detect()
{
	if (IsValidUTF8())
	{
		return Encoding::UTF8;
	}

	int shift_jis = 0;
	int cp949 = 0;

	GetScores(OUT shift_jis, OUT cp949);

	return shift_jis >= 0 ? cp949 > shift_jis ? Encoding::CP949 : Encoding::ShiftJIS :
		cp949 >= 0 ? Encoding::CP949 : Encoding::Unknown;
}

bool StringEncodingDetector::IsValidUTF8()
{
	m_stream.Seek(0);

	uint8_t remaining = 0;

	for (size_t i = 0; i < MAX_READ_FOR_ENCODING_DETECTION; i += sizeof(uint64_t))
	{
		uint64_t data = 0;
		uint64_t data_len = m_stream.Serialize(&data, sizeof(uint64_t));

		for (uint8_t j = 0; j < data_len; ++j)
		{
			uint8_t ch = static_cast<uint8_t>(data & 0xFF);

			if (remaining == 0)
			{
				if (ch < 0x80) return IsPrintableAscii(ch);
				if ((ch >> 6) == 0b10) return false;
				while ((ch & 0x0b01000000))
				{
					++remaining;
					ch <<= 1;
				}

				return true;
			}
			else
			{
				if ((ch >> 6) != 0b10) return false;
				--remaining;
			}

			data >>= 8;
		}

		if (data_len < sizeof(uint64_t))
		{
			return remaining == 0;
		}
	}
}

void StringEncodingDetector::GetScores(int& score_shift_jis, int& score_cp949)
{
	score_shift_jis = score_cp949 = 0;

	m_stream.Seek(0);

	ShiftJISHeuristic heuristic_shift_jis;
	CP949Heuristic heuristic_cp949;

	for (size_t i = 0; i < MAX_READ_FOR_ENCODING_DETECTION; i += sizeof(uint64_t))
	{
		if (!(heuristic_shift_jis.IsValid() || heuristic_cp949.IsValid())) break;

		uint64_t data = 0;
		uint64_t data_len = m_stream.Serialize(&data, sizeof(uint64_t));

		for (uint8_t j = 0; j < data_len; ++j)
		{
			uint8_t ch = static_cast<uint8_t>(data & 0xFF);
			// 0x00 is not valid for both encoding.
			if (ch == 0x00)
			{
				score_shift_jis = score_cp949 = -1;
				return;
			}

			heuristic_shift_jis.Consume(ch);
			heuristic_cp949.Consume(ch);

			data >>= 8;
		}

		if (data_len < sizeof(uint64_t))
		{
			heuristic_shift_jis.Finalize();
			heuristic_cp949.Finalize();

			break;
		}
	}

	score_shift_jis = heuristic_shift_jis.GetScore();
	score_cp949 = heuristic_cp949.GetScore();
}
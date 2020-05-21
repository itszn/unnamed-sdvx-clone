#include "stdafx.h"
#include "StringEncodingDetection.hpp"

#include "Shared/String.hpp"
#include "Shared/Buffer.hpp"
#include "Shared/MemoryStream.hpp"
#include "Shared/BinaryStream.hpp"
#include "Shared/Log.hpp"

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
	ALNUM = 10, // Full-width or half-width 0-9, A-Z, a-z
	PUNCTUATION = 15, // ASCII symbols
	KANA = 20, // Full-width or half-width kana
	HANGUL_KSX1001 = 20, // Hangul in KS X 1001
	KANJI_LEVEL_1 = 30, // Level 1 Kanji in JIS X 0208
	OTHER_CHARS = 50,
	KANJI_KSX1001 = 70, // Kanji in KS X 1001
	HANGUL_CP949 = 80, // Hangul not in KS X 1001
	KANJI_LEVEL_2 = 80, // Level 2+ Kanji in JIS X 0208
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

#pragma region Heuristics for Shift_JIS and CP949

static inline StringEncodingDetector::Encoding DecideEncodingFromHeuristicScores(const int shift_jis, const int cp949)
{
	using Encoding = StringEncodingDetector::Encoding;

	if (shift_jis >= 0)
	{
		if (cp949 >= 0 && cp949 < shift_jis)
			return Encoding::CP949;
		else
			return Encoding::ShiftJIS;
	}
	else if (cp949 >= 0)
	{
		return Encoding::CP949;
	}

	return Encoding::Unknown;
}

class EncodingHeuristic
{
public:
	EncodingHeuristic() = default;

	inline int GetScore() const;
	inline bool IsValid() const { return GetScore() >= 0; }
	inline bool Consume(const uint8_t ch);
	inline bool Finalize();

protected:
	virtual bool RequiresSecondByte(const uint8_t ch) const = 0;
	virtual CharClass GetCharClass(const uint16_t ch) const = 0;

	inline bool Process(const uint16_t ch);

	uint8_t m_hi = 0;
	int m_score = 0;
};

inline int EncodingHeuristic::GetScore() const
{
	return m_score;
}

inline bool EncodingHeuristic::Consume(const uint8_t ch)
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

inline bool EncodingHeuristic::Process(const uint16_t ch)
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

// See also:
// - https://www.sljfaq.org/afaq/encodings.html
// - https://charset.fandom.com/ko/wiki/EUC-JP
class ShiftJISHeuristic : public EncodingHeuristic
{
protected:
	bool RequiresSecondByte(const uint8_t ch) const override
	{
		return 0x81 <= ch && ch <= 0x9F || 0xE0 <= ch && ch <= 0xFC;
	}

	CharClass GetCharClass(const uint16_t ch) const override
	{
		// JIS X 0201
		if (ch <= 0x80) return GetAsciiCharClass(ch);

		if (0xA6 <= ch && ch <= 0xDD)
			return CharClass::KANA;
		if (0xA1 <= ch && ch <= 0xDF)
			return CharClass::OTHER_CHARS;

		// JIS X 0208
		if (0x84BF <= ch && ch <= 0x889E || 0x9873 <= ch && ch <= 0x989E || 0xEAA5 <= ch)
			return CharClass::INVALID;

		if (0x824F <= ch && ch <= 0x8258 || 0x8260 <= ch && ch <= 0x8279 || 0x8281 <= ch && ch <= 0x829A)
			return CharClass::ALNUM;
		if (0x829F <= ch && ch <= 0x82F1 || 0x8340 <= ch && ch <= 8396)
			return CharClass::KANA;
		if (0x889F <= ch && ch <= 0x9872)
			return CharClass::KANJI_LEVEL_1;
		if (0x989F <= ch && ch <= 0xEAA4)
			return CharClass::KANJI_LEVEL_2;

		return CharClass::OTHER_CHARS;
	}
};

// See also:
// - https://charset.fandom.com/ko/wiki/CP949
class CP949Heuristic : public EncodingHeuristic
{
private:
	CharClass GetEUCKRCharClass(const uint16_t ch) const
	{
		// Complete Hangul
		if (0xB0A1 <= ch && ch <= 0xC8FE)
			return CharClass::HANGUL_KSX1001;

		// Incomplete Hangul
		if (0xA4A1 <= ch && ch <= 0xA4FE)
			return CharClass::HANGUL_KSX1001;

		// Full-width alnum
		if (0xA3B0 <= ch && ch <= 0xA3B9 || 0xA3C1 <= ch && ch <= 0xA3DA || 0xA3E1 <= ch && ch <= 0xA3FA)
			return CharClass::ALNUM;

		// Kanji (Every kanji is regarded as rare even though some are not...)
		if (0xCAA1 <= ch && ch <= 0xFDFE)
			return CharClass::KANJI_KSX1001;
		
		// Kana
		if (0xAAA1 <= ch && ch <= 0xAAF3 || 0xABA1 <= ch && ch <= 0xABF6)
			return CharClass::KANA;

		// Invalid region
		if (0xA2E9 <= ch && ch <= 0xA2FE || 0xA6E5 <= ch && ch <= 0xA6FE || 0xACF2 <= ch)
			return CharClass::INVALID;

		return CharClass::OTHER_CHARS;
	}

protected:
	bool RequiresSecondByte(const uint8_t ch) const override
	{
		return 0x81 <= ch && ch <= 0xC6 || 0xA1 <= ch && ch <= 0xFE;
	}

	CharClass GetCharClass(const uint16_t ch) const override
	{
		// KS X 1003
		if (ch < 0x80) return GetAsciiCharClass(ch);
		if (ch <= 0xFF) return CharClass::INVALID;
		
		const uint8_t hi = static_cast<uint8_t>(ch >> 8);
		const uint8_t lo = static_cast<uint8_t>(ch & 0xFF);
		
		// KS X 1001
		if (0xA1 <= hi && hi <= 0xFE && 0xA1 <= lo && lo <= 0xFE)
		{
			return GetEUCKRCharClass(ch);
		}

		// CP949 extension
		if (lo < 0x41 || 0x5A < lo && lo < 0x61 || 0x7A < lo && lo < 0x81 || lo == 0xFF)
		{
			return CharClass::INVALID;
		}
		else if (0x8141 <= ch && ch <= 0xC652)
		{
			return CharClass::HANGUL_CP949;
		}

		return CharClass::INVALID;
	}
};

#pragma endregion

// Detect contents of the string
StringEncodingDetector::Encoding StringEncodingDetector::Detect(const char* str)
{
	// This is inefficient, because stringBuffer copies contents of str. Copying is unnecessary.
	// A version of MemoryStream which does not use a Buffer is preferable.
	Buffer stringBuffer(str);
	MemoryReader memoryReader(stringBuffer);

	return Detect(memoryReader);
}

StringEncodingDetector::Encoding StringEncodingDetector::Detect(BinaryStream& stream)
{
	assert(stream.IsReading());
	
	if (stream.GetSize() == 0)
		return Encoding::Unknown;

	size_t init_pos = stream.Tell();

	StringEncodingDetector detector(stream);
	Encoding result = detector.Detect();

	stream.Seek(init_pos);
	return result;
}

String StringEncodingDetector::ToUTF8(Encoding encoding, const char* str)
{
	switch (encoding)
	{
	case Encoding::ShiftJIS:
		return ToUTF8("SHIFT_JIS", str);
	case Encoding::CP949:
		return ToUTF8("CP949", str);
	case Encoding::Unknown:
	case Encoding::UTF8:
	default:
		return String(str);
	}
}

String StringEncodingDetector::ToUTF8(const char* encoding, const char* str)
{
	iconv_t conv_d = iconv_open("UTF-8", encoding);
	if (conv_d == (iconv_t)-1)
	{
		Logf("Error in ToUTF8: iconv_open returned -1 for encoding %s", Logger::Error, encoding);
		return String(str);
	}

	String result;
	char out_buf_arr[ICONV_BUFFER_SIZE];
	out_buf_arr[ICONV_BUFFER_SIZE - 1] = '\0';

	const char* in_buf = str;
	size_t in_buf_left = strlen(str);
	
	char* out_buf = out_buf_arr;
	size_t out_buf_left = ICONV_BUFFER_SIZE-1;

	while (iconv(conv_d, const_cast<char**>(&in_buf), &in_buf_left, &out_buf, &out_buf_left) == -1)
	{
		switch (errno)
		{
		case E2BIG:
			*out_buf = '\0';
			result += out_buf_arr;
			out_buf = out_buf_arr;
			out_buf_left = ICONV_BUFFER_SIZE - 1;
			break;
		case EINVAL:
		case EILSEQ:
		default:
			Logf("Error in ToUTF8: iconv failed with %d for encoding %s", Logger::Error, errno, encoding);
			return String(str);
			break;
		}
	}

	iconv_close(conv_d);
	return result;
}

StringEncodingDetector::Encoding StringEncodingDetector::Detect()
{
	if (IsValidUTF8())
	{
		return Encoding::UTF8;
	}

	// If the size is too small, the encoding can't be detected realiably anymore.
	if (m_stream.GetSize() < 2) return Encoding::Unknown;

	int shift_jis = 0;
	int cp949 = 0;

	GetScores(shift_jis, cp949);
	return DecideEncodingFromHeuristicScores(shift_jis, cp949);
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
				if (ch < 0x80)
				{
					if (!IsPrintableAscii(ch)) return false;
				}
				else if ((ch >> 6) == 0b10) return false;
				else while (ch & 0b01000000)
				{
					++remaining;
					ch <<= 1;
				}
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
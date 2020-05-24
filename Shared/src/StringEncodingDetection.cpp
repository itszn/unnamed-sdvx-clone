#include "stdafx.h"
#include "StringEncodingDetection.hpp"

#include <bitset>
#include <errno.h>

#include "Shared/String.hpp"
#include "Shared/Buffer.hpp"
#include "Shared/MemoryStream.hpp"
#include "Shared/BinaryStream.hpp"
#include "Shared/Log.hpp"

#include "archive.h"
#include "archive_entry.h"
#include "iconv.h"

/*
	Encoding detection for commonly-used encodings
	----------------------------------------------
	Currently, the heuristic is very simple:
		1. For each character class, a hand-chosen value is assigned.
		2. An encoding with lowest average value is chosen.

	ISO-8859-15 encoding is disabled for now, because it's practically never used, while having some false positives.
*/

using Encoding = StringEncodingDetector::Encoding;

#pragma region Definitions for Heuristics

// Feel free to tweak these values
enum class CharClass
{
	INVALID = -1,
	IGNORABLE = 0,
	ALNUM = 10, // Full-width or half-width 0-9, A-Z, a-z
	PUNCTUATION = 15, // ASCII symbols
	KANA = 20, // Full-width or half-width kana
	HANGUL_KSX1001 = 20, // Hangul in KS X 1001
	KANJI_LEVEL_1 = 30, // Level 1 Kanji in JIS X 0208
	HANGUL_UNICODE = 40, // Hangul in Unicode
	KANJI_UNICODE = 45, // Kanji in CJK Unified Ideographs block
	OTHER_CHARS = 50,
	KANJI_KSX1001 = 70, // Kanji in KS X 1001
	HANGUL_CP949 = 80, // Hangul not in KS X 1001
	KANJI_LEVEL_2 = 80, // Level 2+ Kanji in JIS X 0208
	KANJI_CP932 = 120, // Kanji in CP932
	PRIVATE_USE = 500, // Private-use characters
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

static inline Encoding DecideEncodingFromHeuristicScores(const int shift_jis, const int cp949)
{

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
	virtual Encoding GetEncoding() const { return Encoding::Unknown; }

	EncodingHeuristic(const EncodingHeuristic&) = delete;

	inline int GetScore() const { return m_score; }
	inline size_t GetCount() const { return m_count; }

	inline bool IsValid() const { return GetScore() >= 0; }

	virtual bool Consume(const uint8_t ch) = 0;
	virtual bool Finalize() = 0;

protected:
	virtual CharClass GetCharClass(const uint16_t ch) const = 0;
	inline bool Process(const uint16_t ch)
	{
		return Process(GetCharClass(ch));
	}

	inline bool Process(CharClass charClass)
	{
		if (charClass == CharClass::INVALID)
		{
			MarkInvalid();
			return false;
		}
		else
		{
			m_score += static_cast<int>(charClass);
			++m_count;

			return true;
		}
	}

	inline void MarkInvalid() { m_score = -1; }

	size_t m_count = 0;
	int m_score = 0;
};

class NullHeuristic : public EncodingHeuristic
{
public:
	NullHeuristic() : EncodingHeuristic() { MarkInvalid(); }

	bool Consume(const uint8_t ch) override { return false; }
	bool Finalize() override { return false; }

protected:
	CharClass GetCharClass(const uint16_t ch) const { return CharClass::INVALID; }
};

class TwoByteEncodingHeuristic : public EncodingHeuristic
{
public:
	bool Consume(const uint8_t ch) override;
	bool Finalize() override;

protected:
	virtual bool RequiresSecondByte(const uint8_t ch) const = 0;

	uint8_t m_hi = 0;
};

bool TwoByteEncodingHeuristic::Consume(const uint8_t ch)
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

inline bool TwoByteEncodingHeuristic::Finalize()
{
	if (m_hi) MarkInvalid();

	return IsValid();
}

#pragma endregion

#pragma region Heuristics

class UTF8Heuristic : public EncodingHeuristic
{
public:
	Encoding GetEncoding() const override { return Encoding::UTF8; }

	bool Consume(const uint8_t ch) override
	{
		if (m_remaining == 0)
		{
			if (ch < 0x80) return Process(ch);
			if ((ch >> 6) == 0b10)
			{
				MarkInvalid();
				return false;
			}
			if (ch < 0xE0)
			{
				m_remaining = 1;
				m_currChar = ch & 0x1F;
			}
			else if (ch < 0xF0)
			{
				m_remaining = 2;
				m_currChar = ch & 0x0F;
			}
			else if (ch < 0xF8)
			{
				m_remaining = 3;
				m_currChar = ch & 0x07;
			}
			else
			{
				MarkInvalid();
				return false;
			}
		}
		else
		{
			if ((ch >> 6) != 0b10)
			{
				MarkInvalid();
				return false;
			}

			m_currChar = ((m_currChar << 6) | (ch & 0x3F));
			if (--m_remaining == 0)
			{
				if(m_currChar < 0x10000) return Process(static_cast<uint16_t>(m_currChar));
				else if (m_currChar >= 0x110000)
				{
					MarkInvalid();
					return false;
				}
				else
				{
					Process(CharClass::OTHER_CHARS);
					return true;
				}
			}
		}

		return true;
	}

	bool Finalize() override
	{
		if (m_remaining) MarkInvalid();

		return IsValid();
	}

protected:
	CharClass GetCharClass(const uint16_t ch) const override
	{
		if (ch < 0x80)
			return GetAsciiCharClass(static_cast<uint8_t>(ch));

		if (0x3040 <= ch && ch <= 0x30FF)
			return CharClass::KANA;

		if (0x1100 <= ch && ch <= 0x11FF || 0xAC00 <= ch && ch <= 0xD7AF)
			return CharClass::HANGUL_UNICODE;

		if (0x4E00 <= ch && ch <= 0x9FFF)
			return CharClass::KANJI_UNICODE;

		// PUA
		if (0xE000 <= ch && ch <= 0xF8FF)
			return CharClass::PRIVATE_USE;

		// BOM
		if (ch == 0xFEFF)
			return CharClass::IGNORABLE;

		if (0xFF10 <= ch && ch <= 0xFF19 || 0xFF21 <= ch && ch <= 0xFF3A || 0xFF41 <= ch && ch <= 0xFF5A)
			return CharClass::ALNUM;
		
		// Half-width characters
		if (0xFF66 <= ch && ch <= 0xFF9D)
			return CharClass::KANA;
		
		if (0xFFA1 <= ch && ch <= 0xFFDC)
			return CharClass::HANGUL_UNICODE;

		return CharClass::OTHER_CHARS;

	}

	uint32_t m_currChar = 0;
	unsigned int m_remaining = 0;
};

class ISO8859Heuristic : public EncodingHeuristic
{
public:
	Encoding GetEncoding() const override { return Encoding::ISO8859; }

	bool Consume(const uint8_t ch) override
	{
		return Process(ch);
	}
	bool Finalize() override
	{
		return IsValid();
	}

protected:
	CharClass GetCharClass(const uint16_t ch) const override
	{
		if (ch < 0x80) return GetAsciiCharClass(static_cast<uint8_t>(ch));

		if (ch < 0xA0) return CharClass::INVALID;
		return CharClass::OTHER_CHARS;
	}
};

// See also:
// - https://www.sljfaq.org/afaq/encodings.html
// - https://charset.fandom.com/ko/wiki/CP932
class ShiftJISHeuristic : public TwoByteEncodingHeuristic
{
public:
	Encoding GetEncoding() const override { return Encoding::ShiftJIS; }

protected:
	bool RequiresSecondByte(const uint8_t ch) const override
	{
		return 0x81 <= ch && ch <= 0x9F || 0xE0 <= ch && ch <= 0xFC;
	}

	CharClass GetCharClass(const uint16_t ch) const override
	{
		// JIS X 0201
		if (ch <= 0x80) return GetAsciiCharClass(static_cast<uint8_t>(ch));

		if (0xA6 <= ch && ch <= 0xDD)
			return CharClass::KANA;
		if (0xA1 <= ch && ch <= 0xDF)
			return CharClass::OTHER_CHARS;

		// CP932 extension
		if (0x8740 <= ch && ch <= 0x879C)
			return CharClass::OTHER_CHARS;
		if (0xED40 <= ch && ch <= 0xEEFC || 0xFA40 <= ch && ch <= 0xFC4B)
			return CharClass::KANJI_CP932;
		if (0xF040 <= ch && ch <= 0xF9FC)
			return CharClass::PRIVATE_USE;

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
class CP949Heuristic : public TwoByteEncodingHeuristic
{
public:
	Encoding GetEncoding() const override { return Encoding::CP949; }

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

		// PUA
		if (0xC9A1 <= ch && ch <= 0xC9FE || 0xFEA1 <= ch && ch <= 0xFEFE)
			return CharClass::PRIVATE_USE;

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
		if (ch < 0x80) return GetAsciiCharClass(static_cast<uint8_t>(ch));
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

class EncodingHeuristics
{
public:
	EncodingHeuristics() = default;

	inline EncodingHeuristic& GetHeuristic(Encoding encoding)
	{
		assert(encoding != Encoding::Unknown);

		switch (encoding)
		{
		case Encoding::UTF8:
			return utf8;
		case Encoding::ISO8859:
			return iso8859;
		case Encoding::ShiftJIS:
			return shift_jis;
		case Encoding::CP949:
			return cp949;
		default:
			Logf("EncodingHeuristics::GetHeuristic called for invalid argument %d", Logger::Error, encoding);
			assert(false);
			return utf8;
		}
	}

	inline Encoding GetBest() const
	{
		Encoding result = Encoding::Unknown;
		int minScore = -1;
		size_t minCount = 0;

		CheckBest(result, minScore, minCount, utf8);
		CheckBest(result, minScore, minCount, iso8859);
		CheckBest(result, minScore, minCount, shift_jis);
		CheckBest(result, minScore, minCount, cp949);

		return result;
	}

	inline void FeedInputs(const char* c)
	{
		while (*c)
		{
			utf8.IsValid() && utf8.Consume(*c);
			iso8859.IsValid() && iso8859.Consume(*c);
			shift_jis.IsValid() && shift_jis.Consume(*c);
			cp949.IsValid() && cp949.Consume(*c);
			++c;
		}

		utf8.Finalize();
		iso8859.Finalize();
		shift_jis.Finalize();
		cp949.Finalize();
	}

	inline static void CheckBest(Encoding& result, int& minScore, size_t& minCount, const EncodingHeuristic& heuristic)
	{
		if (!heuristic.IsValid()) return;

		if (minScore == -1 || heuristic.GetScore() * minCount < minScore * heuristic.GetCount())
		{
			result = heuristic.GetEncoding();
			minScore = heuristic.GetScore();
			minCount = heuristic.GetCount();
		}
	}

protected:
	UTF8Heuristic utf8;
	NullHeuristic iso8859; // ISO-8859 is disabled for now
	ShiftJISHeuristic shift_jis;
	CP949Heuristic cp949;
};

// Detect contents of the string
Encoding StringEncodingDetector::Detect(const char* str, const Option& option)
{
	// This is inefficient, because stringBuffer copies contents of str. Copying is unnecessary.
	// A version of MemoryStream which does not use a Buffer is preferable.
	Buffer stringBuffer(str);
	MemoryReader memoryReader(stringBuffer);

	return Detect(memoryReader, option);
}

Encoding StringEncodingDetector::Detect(BinaryStream& stream, const Option& option)
{
	assert(stream.IsReading());
	
	if (stream.GetSize() == 0)
		return Encoding::Unknown;

	size_t init_pos = stream.Tell();

	StringEncodingDetector detector(stream);
	Encoding result = detector.Detect(option);

	stream.Seek(init_pos);
	return result;
}

Encoding StringEncodingDetector::DetectArchive(const Buffer& buffer, const Option& option)
{
	struct archive* a = archive_read_new();
	if (a == nullptr)
	{
		return Encoding::Unknown;
	}

	archive_read_support_filter_all(a);
	archive_read_support_format_all(a);

	if (archive_read_open_memory(a, buffer.data(), buffer.size()) != ARCHIVE_OK)
	{
		archive_read_free(a);
		return Encoding::Unknown;
	}

	EncodingHeuristics heuristics;

	struct archive_entry* entry = nullptr;

	while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
	{
		if (const char* entryName = archive_entry_pathname(entry))
		{
			heuristics.FeedInputs(entryName);
		}
		else
		{
			archive_read_free(a);
			return Encoding::Unknown;
		}

		archive_read_data_skip(a);
	}

	archive_read_free(a);

	for (const Encoding encoding : option.assumptions)
	{
		if (heuristics.GetHeuristic(encoding).IsValid())
		{
			return encoding;
		}
	}

	return heuristics.GetBest();
}

String StringEncodingDetector::ToUTF8(Encoding encoding, const char* str, const size_t str_len)
{
	switch (encoding)
	{
	case Encoding::ISO8859:
		return ToUTF8("ISO-8859-25", str, str_len);
	case Encoding::ShiftJIS:
		return ToUTF8("CP932", str, str_len);
	case Encoding::CP949:
		return ToUTF8("CP949", str, str_len);
	case Encoding::Unknown:
	case Encoding::UTF8:
	default:
		return String(str);
	}
}

String StringEncodingDetector::ToUTF8(const char* encoding, const char* str, const size_t str_len)
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
	const char* in_buf_prev = in_buf;
	size_t in_buf_left = str_len;
	
	char* out_buf = out_buf_arr;
	size_t out_buf_left = ICONV_BUFFER_SIZE-1;

	while (iconv(conv_d, const_cast<char**>(&in_buf), &in_buf_left, &out_buf, &out_buf_left) == -1)
	{
		// errno doesn't seem to be realible on Windows (set to 0 or 9 instead of E2BIG)
		const int err = errno;

		if (in_buf != in_buf_prev)
		{
			in_buf_prev = in_buf;

			*out_buf = '\0';
			result += out_buf_arr;
			out_buf = out_buf_arr;
			out_buf_left = ICONV_BUFFER_SIZE - 1;

			continue;
		}

		Logf("Error in ToUTF8: iconv failed with %d for encoding %s", Logger::Error, err, encoding);
		iconv_close(conv_d);
		return String(str);
	}

	iconv_close(conv_d);

	*out_buf = '\0';
	result.append(out_buf_arr);

	return result;
}

String StringEncodingDetector::PathnameToUTF8(Encoding encoding, struct archive_entry* entry)
{
	if (encoding != Encoding::Unknown)
	{
		if (const char* pathname = archive_entry_pathname(entry))
		{
			return StringEncodingDetector::ToUTF8(encoding, pathname);
		}
	}
	
	if (const wchar_t* pathname_w = archive_entry_pathname_w(entry))
	{
		return Utility::ConvertToUTF8(pathname_w);
	}

	return String();
}

Encoding StringEncodingDetector::Detect(const Option& option)
{
	if (m_stream.GetSize() == 0)
	{
		if (option.assumptions.empty())
			return Encoding::Unknown;
		else
			return option.assumptions.front();
	}

	EncodingHeuristics heuristics;

	// First, check assumptions
	for (const Encoding encoding : option.assumptions)
	{
		if (encoding == Encoding::Unknown)
			return encoding;

		EncodingHeuristic& heuristic = heuristics.GetHeuristic(encoding);
		FeedInput(heuristic, option.maxLookahead);

		if (heuristic.IsValid())
			return encoding;
	}

	// Feed input for the rest.
	Encoding result = Encoding::Unknown;
	int minScore = -1;
	size_t minCount = 0;

	for (Encoding encoding = static_cast<Encoding>(0); encoding != Encoding::MAX_VALUE;
		encoding = static_cast<Encoding>(static_cast<int>(encoding)+1))
	{
		EncodingHeuristic& heuristic = heuristics.GetHeuristic(encoding);
		FeedInput(heuristic, option.maxLookahead);

		EncodingHeuristics::CheckBest(result, minScore, minCount, heuristic);
	}

	return result;
}

void StringEncodingDetector::ResetStream()
{
	m_stream.Seek(0);
}

uint64_t StringEncodingDetector::Read(uint64_t& data)
{
	return m_stream.Serialize(&data, sizeof(data));
}
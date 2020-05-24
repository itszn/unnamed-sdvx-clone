#pragma once

#include "Shared/String.hpp"

class BinaryStream;
class Buffer;

class StringEncodingDetector;

// Encoding detection for commonly-used encodings

class StringEncodingDetector final
{
public:
	enum class Encoding
	{
		Unknown = -1,
		// Unicode
		UTF8 = 0,
		// ISO 8859-15
		ISO8859,
		// Japanese
		CP932,
		ShiftJIS = CP932,
		// Korean
		CP949,
		MAX_VALUE
	};

	struct Option
	{
		Option() = default;

		// 0: process the whole input
		size_t maxLookahead = 64;

		// Assume these encodings unless proven invalid.
		// The encodings will be checked sequentially.
		std::vector<Encoding> assumptions = { Encoding::UTF8 };
	};

	static Encoding Detect(const char* str, const Option& option);
	inline static Encoding Detect(const char* str) { return Detect(str, Option()); }

	inline static Encoding Detect(String& str, const Option& option) { return Detect(str.c_str(), option); }
	inline static Encoding Detect(String& str) { return Detect(str, Option()); }

	static Encoding Detect(BinaryStream& stream, const Option& option);
	inline static Encoding Detect(BinaryStream& stream) { return Detect(stream, Option()); }

	static Encoding DetectArchive(const Buffer& buffer, const Option& option);
	inline static Encoding DetectArchive(const Buffer& buffer) { return DetectArchive(buffer, Option()); }

	static String ToUTF8(Encoding encoding, const char* str, size_t str_len);
	static String ToUTF8(const char* encoding, const char* str, size_t str_len);

	inline static String ToUTF8(Encoding encoding, const char* str)
	{
		return ToUTF8(encoding, str, strlen(str));
	}
	inline static String ToUTF8(Encoding encoding, const String& str)
	{
		return ToUTF8(encoding, str.c_str(), str.size());
	}
	inline static String ToUTF8(const char* encoding, const char* str)
	{
		return ToUTF8(encoding, str, strlen(str));
	}
	inline static String ToUTF8(const char* encoding, const String& str)
	{
		return ToUTF8(encoding, str.c_str(), str.size());
	}
	static String PathnameToUTF8(Encoding encoding, struct archive_entry* entry);

	inline static constexpr const char* ToString(Encoding encoding);

protected:
	StringEncodingDetector(BinaryStream& stream) : m_stream(stream) {}
	Encoding Detect(const Option& option);

	template<class Heuristic>
	void FeedInput(Heuristic& heuristic, const size_t maxLookahead);

	void ResetStream();
	uint64_t Read(uint64_t& data);

protected:
	BinaryStream& m_stream;

	// Size of the buffer for iconv
	constexpr static size_t ICONV_BUFFER_SIZE = 64;
};

inline constexpr const char* StringEncodingDetector::ToString(Encoding encoding)
{
	switch (encoding)
	{
	case Encoding::UTF8:
		return "utf-8";
	case Encoding::ISO8859:
		return "iso-8859-15";
	case Encoding::CP932:
		return "cp932";
	case Encoding::CP949:
		return "cp949";
	case Encoding::Unknown:
		return "unknown";
	default:
		return "an unknown encoding";
	}
}

template<class Heuristic>
inline void StringEncodingDetector::FeedInput(Heuristic& heuristic, const size_t maxLookahead)
{
	if (!heuristic.IsValid())
		return;

	ResetStream();
	for (size_t i = 0; maxLookahead == 0 || i < maxLookahead; i += sizeof(uint64_t))
	{
		uint64_t data = 0;
		const uint64_t data_len = Read(data);

		for (uint8_t j = 0; j < data_len; ++j)
		{
			if (!heuristic.Consume(static_cast<uint8_t>(data & 0xFF)))
				return;
			data >>= 8;
		}

		if (data_len < sizeof(uint64_t))
		{
			heuristic.Finalize();
			return;
		}
	}
}

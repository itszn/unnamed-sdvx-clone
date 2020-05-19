#include "stdafx.h"
#include "String.hpp"

#include "Shared/Buffer.hpp"
#include "Shared/MemoryStream.hpp"
#include "Shared/BinaryStream.hpp"
#include "iconv.h"

/*
	Encoding detection for commonly-used encodings
*/

namespace Utility
{
	// Max. bytes to be examined.
	constexpr size_t MAX_READ_FOR_ENCODING_DETECTION = 64;

	// Internal class for detecting invalid sequences 
	class StringEncodingValidator
	{
	protected:
		virtual void Initialize() {}
		virtual bool Finalize() { return true; }

		virtual bool Consume(uint8_t c) = 0;

		static bool IsPrintableAscii(uint8_t c)
		{
			return c == 0x09 || c == 0x0A || c == 0x0D || 0x20 <= c && c <= 0x7E;
		}

	public:
		// Returns that whether the stream contains valid character sequences.
		// PUA characters are not necessarily considered as valid characters.
		virtual bool Validate(::BinaryStream& stream, const size_t MAX_READ = MAX_READ_FOR_ENCODING_DETECTION)
		{
			Initialize();

			for (size_t i = 0; i < MAX_READ; i += sizeof(uint64_t))
			{
				uint64_t data = 0;
				size_t read = stream.Serialize(&data, sizeof(uint64_t));

				for (size_t j = 0; j < read; ++j)
				{
					if (Consume(static_cast<uint8_t>(data & 0xFF)) == false)
						return false;

					data >>= 8;
				}

				if (read < sizeof(uint64_t))
					return Finalize();
			}

			return true;
		}
	};

	class UTF8Validator : public StringEncodingValidator
	{
		uint8_t m_remaining = 0;

		void Initialize() override { m_remaining = 0; }
		bool Finalize() override { return m_remaining == 0; }

		bool Consume(uint8_t c) override
		{
			if (m_remaining == 0)
			{
				// 0b0_______
				if ((c & 0b1'0000000) == 0) return IsPrintableAscii(c);
				
				// 0b10______
				if ((c & 0b01'000000) == 0) return false;

				while ((c & 0b01000000))
				{
					++m_remaining;
					c <<= 1;
				}

				return true;
			}
			else
			{
				--m_remaining;

				return (c & 0b11'000000) == 0b10'000000;
			}
		}
	};

	class DoubleByteValidator : public StringEncodingValidator
	{
		uint8_t m_firstByte = 0;

		void Initialize() override { m_firstByte = 0; }
		bool Finalize() override { return m_firstByte == 0; }

		virtual bool Consume(uint8_t hi, uint8_t lo) = 0;
		virtual bool RequiresSecondByte(uint8_t c) { return false; }

		bool Consume(uint8_t c) override
		{
			if (m_firstByte)
			{
				uint8_t hi = m_firstByte;
				m_firstByte = 0;

				return Consume(hi, c);
			}

			if (RequiresSecondByte(c))
			{
				m_firstByte = c;
				return true;
			}
			else
			{
				Consume(0, c);
			}
		}
	};

	class ShiftJISValidator : public DoubleByteValidator
	{
		bool RequiresSecondByte(uint8_t c) override
		{
			return 0x81 <= c && c <= 0x9F || 0xE0 <= c && c <= 0xFC;
		}

		bool Consume(uint8_t hi, uint8_t lo) override
		{
			if (hi == 0)
			{
				return IsPrintableAscii(lo) || 0xA1 <= lo && lo <= 0xDF;
			}
			else
			{
				return 0x40 <= lo && lo <= 0xFC && lo != 0x7F;
			}
		}
	};

	class CP949Validator : public DoubleByteValidator
	{
		bool RequiresSecondByte(uint8_t c) override
		{
			return 0x81 <= c && c <= 0xC6 || 0xA1 <= c && c <= 0xFE;
		}

		bool Consume(uint8_t hi, uint8_t lo) override
		{
			if (hi == 0)
			{
				return IsPrintableAscii(lo);
			}

			// TODO: make the check finer
			return true;
		}
	};

	// Detect contents of the string
	StringEncoding DetectEncoding(const char* str)
	{
		// This is inefficient, because stringBuffer copies contents of str. Copying is unnecessary.
		// A version of MemoryStream which does not use a Buffer is preferable.
		::Buffer stringBuffer(str);
		::MemoryReader memoryReader(stringBuffer);

		return DetectEncoding(memoryReader);
	}

	StringEncoding DetectEncoding(::BinaryStream& stream)
	{
		assert(stream.IsReading());
		size_t size = stream.GetSize();
		size_t init_pos = stream.Tell();
		stream.Seek(0);

		// If the size is too small, the encoding can't be detected.
		if (size < 2) return StringEncoding::Unknown;

		StringEncoding result = StringEncoding::Unknown;

		if (UTF8Validator().Validate(stream))
			result = StringEncoding::UTF8;
		else if (stream.Seek(0), ShiftJISValidator().Validate(stream))
			result = StringEncoding::ShiftJIS;
		else if (stream.Seek(0), CP949Validator().Validate(stream))
			result = StringEncoding::CP949;

		stream.Seek(init_pos);
		return result;
	}
}
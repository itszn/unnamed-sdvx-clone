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

		// If the first three bytes are BOM, 

		stream.Seek(init_pos);
		return StringEncoding::Unknown;
	}
}
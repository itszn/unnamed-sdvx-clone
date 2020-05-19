#include "stdafx.h"
#include "String.hpp"

#include "BinaryStream.hpp"
#include "iconv.h"

/*
	Encoding detection for commonly-used encodings
*/

namespace Utility
{
	// Detect contents of the buffer
	StringEncoding DetectEncoding(const char* buffer, const uint32 size)
	{
		return StringEncoding::Unknown;
	}

	StringEncoding DetectEncoding(BinaryStream& stream)
	{
		return StringEncoding::Unknown;
	}
}
#include "stdafx.h"
#include "StringEncodingHeuristic.hpp"

static inline CharClass GetAsciiCharClass(uint8_t ch)
{
	if (0x30 <= ch && ch <= 0x39 || 0x41 <= ch && ch <= 0x5A || 0x61 <= ch && ch <= 0x7A)
		return CharClass::ALNUM;
	else if (ch == 0x09 || ch == 0x0A || ch == 0x0D || 0x20 <= ch && ch <= 0x7E)
		return CharClass::ASCII_OTHER_CHARS;
	else
		return CharClass::INVALID;
}

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

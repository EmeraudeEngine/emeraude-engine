/*
 * src/Libs/Hash/MD5.cpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2026 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
 *
 * Emeraude-Engine is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * Emeraude-Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Emeraude-Engine; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Complete project and additional information can be found at :
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "MD5.hpp"

/* STL inclusions. */
#include <cstring>

namespace EmEn::Libs::Hash
{
	constexpr auto S11 = 7;
	constexpr auto S12 = 12;
	constexpr auto S13 = 17;
	constexpr auto S14 = 22;
	constexpr auto S21 = 5;
	constexpr auto S22 = 9;
	constexpr auto S23 = 14;
	constexpr auto S24 = 20;
	constexpr auto S31 = 4;
	constexpr auto S32 = 11;
	constexpr auto S33 = 16;
	constexpr auto S34 = 23;
	constexpr auto S41 = 6;
	constexpr auto S42 = 10;
	constexpr auto S43 = 15;
	constexpr auto S44 = 21;

	void
	MD5::decode (uint32_t * output, const uint8_t * input, size_t length) noexcept
	{
		for ( size_t i = 0, j = 0; j < length; i++, j += 4 )
		{
			output[i] = static_cast< uint32_t >(input[j]) | (static_cast< uint32_t >(input[j + 1]) << 8) | (static_cast< uint32_t >(input[j + 2]) << 16) | (static_cast< uint32_t >(input[j + 3]) << 24);
		}
	}

	void
	MD5::encode (uint8_t * output, const uint32_t * input, size_t length) noexcept
	{
		for ( size_t i = 0, j = 0; j < length; i++, j += 4 )
		{
			output[j] = input[i] & 0xff;
			output[j+1] = (input[i] >> 8) & 0xff;
			output[j+2] = (input[i] >> 16) & 0xff;
			output[j+3] = (input[i] >> 24) & 0xff;
		}
	}

	void
	MD5::transform (const uint8_t * block) noexcept
	{
		std::array< uint32_t, 16 > hash{0};

		auto saltA = m_states[0];
		auto saltB = m_states[1];
		auto saltC = m_states[2];
		auto saltD = m_states[3];

		MD5::decode(hash.data(), block, MD5::BlockSize);

		/* Round 1 */
		MD5::FF(saltA, saltB, saltC, saltD, hash[ 0], S11, 0xd76aa478); /* 1 */
		MD5::FF(saltD, saltA, saltB, saltC, hash[ 1], S12, 0xe8c7b756); /* 2 */
		MD5::FF(saltC, saltD, saltA, saltB, hash[ 2], S13, 0x242070db); /* 3 */
		MD5::FF(saltB, saltC, saltD, saltA, hash[ 3], S14, 0xc1bdceee); /* 4 */
		MD5::FF(saltA, saltB, saltC, saltD, hash[ 4], S11, 0xf57c0faf); /* 5 */
		MD5::FF(saltD, saltA, saltB, saltC, hash[ 5], S12, 0x4787c62a); /* 6 */
		MD5::FF(saltC, saltD, saltA, saltB, hash[ 6], S13, 0xa8304613); /* 7 */
		MD5::FF(saltB, saltC, saltD, saltA, hash[ 7], S14, 0xfd469501); /* 8 */
		MD5::FF(saltA, saltB, saltC, saltD, hash[ 8], S11, 0x698098d8); /* 9 */
		MD5::FF(saltD, saltA, saltB, saltC, hash[ 9], S12, 0x8b44f7af); /* 10 */
		MD5::FF(saltC, saltD, saltA, saltB, hash[10], S13, 0xffff5bb1); /* 11 */
		MD5::FF(saltB, saltC, saltD, saltA, hash[11], S14, 0x895cd7be); /* 12 */
		MD5::FF(saltA, saltB, saltC, saltD, hash[12], S11, 0x6b901122); /* 13 */
		MD5::FF(saltD, saltA, saltB, saltC, hash[13], S12, 0xfd987193); /* 14 */
		MD5::FF(saltC, saltD, saltA, saltB, hash[14], S13, 0xa679438e); /* 15 */
		MD5::FF(saltB, saltC, saltD, saltA, hash[15], S14, 0x49b40821); /* 16 */

		/* Round 2 */
		MD5::GG(saltA, saltB, saltC, saltD, hash[ 1], S21, 0xf61e2562); /* 17 */
		MD5::GG(saltD, saltA, saltB, saltC, hash[ 6], S22, 0xc040b340); /* 18 */
		MD5::GG(saltC, saltD, saltA, saltB, hash[11], S23, 0x265e5a51); /* 19 */
		MD5::GG(saltB, saltC, saltD, saltA, hash[ 0], S24, 0xe9b6c7aa); /* 20 */
		MD5::GG(saltA, saltB, saltC, saltD, hash[ 5], S21, 0xd62f105d); /* 21 */
		MD5::GG(saltD, saltA, saltB, saltC, hash[10], S22, 0x2441453); /* 22 */
		MD5::GG(saltC, saltD, saltA, saltB, hash[15], S23, 0xd8a1e681); /* 23 */
		MD5::GG(saltB, saltC, saltD, saltA, hash[ 4], S24, 0xe7d3fbc8); /* 24 */
		MD5::GG(saltA, saltB, saltC, saltD, hash[ 9], S21, 0x21e1cde6); /* 25 */
		MD5::GG(saltD, saltA, saltB, saltC, hash[14], S22, 0xc33707d6); /* 26 */
		MD5::GG(saltC, saltD, saltA, saltB, hash[ 3], S23, 0xf4d50d87); /* 27 */
		MD5::GG(saltB, saltC, saltD, saltA, hash[ 8], S24, 0x455a14ed); /* 28 */
		MD5::GG(saltA, saltB, saltC, saltD, hash[13], S21, 0xa9e3e905); /* 29 */
		MD5::GG(saltD, saltA, saltB, saltC, hash[ 2], S22, 0xfcefa3f8); /* 30 */
		MD5::GG(saltC, saltD, saltA, saltB, hash[ 7], S23, 0x676f02d9); /* 31 */
		MD5::GG(saltB, saltC, saltD, saltA, hash[12], S24, 0x8d2a4c8a); /* 32 */

		/* Round 3 */
		MD5::HH(saltA, saltB, saltC, saltD, hash[ 5], S31, 0xfffa3942); /* 33 */
		MD5::HH(saltD, saltA, saltB, saltC, hash[ 8], S32, 0x8771f681); /* 34 */
		MD5::HH(saltC, saltD, saltA, saltB, hash[11], S33, 0x6d9d6122); /* 35 */
		MD5::HH(saltB, saltC, saltD, saltA, hash[14], S34, 0xfde5380c); /* 36 */
		MD5::HH(saltA, saltB, saltC, saltD, hash[ 1], S31, 0xa4beea44); /* 37 */
		MD5::HH(saltD, saltA, saltB, saltC, hash[ 4], S32, 0x4bdecfa9); /* 38 */
		MD5::HH(saltC, saltD, saltA, saltB, hash[ 7], S33, 0xf6bb4b60); /* 39 */
		MD5::HH(saltB, saltC, saltD, saltA, hash[10], S34, 0xbebfbc70); /* 40 */
		MD5::HH(saltA, saltB, saltC, saltD, hash[13], S31, 0x289b7ec6); /* 41 */
		MD5::HH(saltD, saltA, saltB, saltC, hash[ 0], S32, 0xeaa127fa); /* 42 */
		MD5::HH(saltC, saltD, saltA, saltB, hash[ 3], S33, 0xd4ef3085); /* 43 */
		MD5::HH(saltB, saltC, saltD, saltA, hash[ 6], S34, 0x4881d05); /* 44 */
		MD5::HH(saltA, saltB, saltC, saltD, hash[ 9], S31, 0xd9d4d039); /* 45 */
		MD5::HH(saltD, saltA, saltB, saltC, hash[12], S32, 0xe6db99e5); /* 46 */
		MD5::HH(saltC, saltD, saltA, saltB, hash[15], S33, 0x1fa27cf8); /* 47 */
		MD5::HH(saltB, saltC, saltD, saltA, hash[ 2], S34, 0xc4ac5665); /* 48 */

		/* Round 4 */
		MD5::II(saltA, saltB, saltC, saltD, hash[ 0], S41, 0xf4292244); /* 49 */
		MD5::II(saltD, saltA, saltB, saltC, hash[ 7], S42, 0x432aff97); /* 50 */
		MD5::II(saltC, saltD, saltA, saltB, hash[14], S43, 0xab9423a7); /* 51 */
		MD5::II(saltB, saltC, saltD, saltA, hash[ 5], S44, 0xfc93a039); /* 52 */
		MD5::II(saltA, saltB, saltC, saltD, hash[12], S41, 0x655b59c3); /* 53 */
		MD5::II(saltD, saltA, saltB, saltC, hash[ 3], S42, 0x8f0ccc92); /* 54 */
		MD5::II(saltC, saltD, saltA, saltB, hash[10], S43, 0xffeff47d); /* 55 */
		MD5::II(saltB, saltC, saltD, saltA, hash[ 1], S44, 0x85845dd1); /* 56 */
		MD5::II(saltA, saltB, saltC, saltD, hash[ 8], S41, 0x6fa87e4f); /* 57 */
		MD5::II(saltD, saltA, saltB, saltC, hash[15], S42, 0xfe2ce6e0); /* 58 */
		MD5::II(saltC, saltD, saltA, saltB, hash[ 6], S43, 0xa3014314); /* 59 */
		MD5::II(saltB, saltC, saltD, saltA, hash[13], S44, 0x4e0811a1); /* 60 */
		MD5::II(saltA, saltB, saltC, saltD, hash[ 4], S41, 0xf7537e82); /* 61 */
		MD5::II(saltD, saltA, saltB, saltC, hash[11], S42, 0xbd3af235); /* 62 */
		MD5::II(saltC, saltD, saltA, saltB, hash[ 2], S43, 0x2ad7d2bb); /* 63 */
		MD5::II(saltB, saltC, saltD, saltA, hash[ 9], S44, 0xeb86d391); /* 64 */

		m_states[0] += saltA;
		m_states[1] += saltB;
		m_states[2] += saltC;
		m_states[3] += saltD;

		/* Zero-ize sensitive information. */
		memset(hash.data(), 0, sizeof(hash));
	}

	void
	MD5::update (const uint8_t * message, size_t length) noexcept
	{
		/* Compute number of bytes mod 64. */
		size_t index = m_count[0] / 8 % MD5::BlockSize;

		/* Update number of bits. */
		m_count[0] += static_cast< unsigned int >(length << 3);

		if ( m_count[0] < (length << 3) )
		{
			m_count[1]++;
		}

		m_count[1] += static_cast< unsigned int >(length >> 29);

		/* Number of bytes we need to fill in buffer. */
		const size_t firstPart = 64 - index;

		size_t msgIndex = 0;

		/* Transform as many times as possible. */
		if ( length >= firstPart )
		{
			/* Fill buffer first, transform. */
			memcpy(&m_buffer[index], message, firstPart);

			this->transform(m_buffer.data());

			/* Transform chunks of block size (64 bytes). */
			for (msgIndex = firstPart; msgIndex + MD5::BlockSize <= length; msgIndex += MD5::BlockSize )
			{
				this->transform(&message[msgIndex]);
			}

			index = 0;
		}
		else
		{
			msgIndex = 0;
		}

		// buffer remaining input
		memcpy(&m_buffer[index], &message[msgIndex], length - msgIndex);
	}

	void
	MD5::final (std::array< uint8_t, 16 > & digest) noexcept
	{
		static std::array< uint8_t, MD5::BlockSize > padding{
			0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
		};

		/* Save number of bits. */
		std::array< uint8_t, 8 > bits{0};

		MD5::encode(bits.data(), m_count.data(), bits.size());

		/* Pad out to 56 mod 64. */
		const uint32_t index = m_count[0] / 8 % MD5::BlockSize;
		const uint32_t padLen = (index < 56) ? (56 - index) : (120 - index);

		this->update(padding.data(), padLen);

		/* Append length (before padding). */
		this->update(bits.data(), bits.size());

		/* Store state in digest. */
		MD5::encode(digest.data(), m_states.data(), digest.size());

		/* Zero-ize sensitive information. */
		m_buffer.fill(0);
	}

	void
	MD5::reset () noexcept
	{
		m_buffer.fill(0);
		m_count.fill(0);

		m_states[0] = InitA;
		m_states[1] = InitB;
		m_states[2] = InitC;
		m_states[3] = InitD;
	}
}

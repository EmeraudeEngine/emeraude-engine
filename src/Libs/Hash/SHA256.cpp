/*
 * src/Libs/Hash/SHA256.cpp
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

#include "SHA256.hpp"

/* STL inclusions. */
#include <cstring>
#include <fstream>

#define SHA2_SHFR(x, n)	(x >> n)
#define SHA2_ROTR(x, n)   ((x >> n) | (x << ((sizeof(x) << 3) - n)))
#define SHA2_ROTL(x, n)   ((x << n) | (x >> ((sizeof(x) << 3) - n)))
#define SHA2_CH(x, y, z)  ((x & y) ^ (~x & z))
#define SHA2_MAJ(x, y, z) ((x & y) ^ (x & z) ^ (y & z))
#define SHA256_F1(x) (SHA2_ROTR(x,  2) ^ SHA2_ROTR(x, 13) ^ SHA2_ROTR(x, 22))
#define SHA256_F2(x) (SHA2_ROTR(x,  6) ^ SHA2_ROTR(x, 11) ^ SHA2_ROTR(x, 25))
#define SHA256_F3(x) (SHA2_ROTR(x,  7) ^ SHA2_ROTR(x, 18) ^ SHA2_SHFR(x,  3))
#define SHA256_F4(x) (SHA2_ROTR(x, 17) ^ SHA2_ROTR(x, 19) ^ SHA2_SHFR(x, 10))
#define SHA2_UNPACK32(x, str)				 \
{											 \
	*((str) + 3) = (uint8_t) ((x)	  );	   \
	*((str) + 2) = (uint8_t) ((x) >>  8);	   \
	*((str) + 1) = (uint8_t) ((x) >> 16);	   \
	*((str) + 0) = (uint8_t) ((x) >> 24);	   \
}
#define SHA2_PACK32(str, x)				   \
{											 \
	*(x) =   ((uint32_t) *((str) + 3)	  )	\
		   | ((uint32_t) *((str) + 2) <<  8)	\
		   | ((uint32_t) *((str) + 1) << 16)	\
		   | ((uint32_t) *((str) + 0) << 24);   \
}

namespace EmEn::Libs::Hash
{
	const std::array< uint32_t, 64 > SHA256::sha256_k{
		0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
		0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
		0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
		0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
		0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
		0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
		0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
		0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
		0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
		0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
		0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
		0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
		0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
		0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
		0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
		0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
	};

	void
	SHA256::transform (const uint8_t * message, size_t length) noexcept
	{
		std::array< uint32_t, 64 > w{0};
		std::array< uint32_t, 8 > wv{0};

		for ( size_t i = 0; i < length; i++ )
		{
			const auto * subBlock = message + (i << 6);

			for ( size_t j = 0; j < 16; j++ )
			{
				SHA2_PACK32(&subBlock[j << 2], &w[j]);
			}

			for ( size_t j = 16; j < w.size(); j++ )
			{
				w[j] = SHA256_F4(w[j - 2]) + w[j - 7] + SHA256_F3(w[j - 15]) + w[j - 16];
			}

			for ( size_t j = 0; j < wv.size(); j++ )
			{
				wv[j] = m_h[j];
			}

			for ( size_t j = 0; j < w.size(); j++ )
			{
				auto t1 = wv[7] + SHA256_F2(wv[4]) + SHA2_CH(wv[4], wv[5], wv[6]) + sha256_k[j] + w[j];
				auto t2 = SHA256_F1(wv[0]) + SHA2_MAJ(wv[0], wv[1], wv[2]);

				wv[7] = wv[6];
				wv[6] = wv[5];
				wv[5] = wv[4];
				wv[4] = wv[3] + t1;
				wv[3] = wv[2];
				wv[2] = wv[1];
				wv[1] = wv[0];
				wv[0] = t1 + t2;
			}

			for ( size_t j = 0; j < wv.size(); j++ )
			{
				m_h[j] += wv[j];
			}
		}
	}

	void
	SHA256::reset () noexcept
	{
		m_length = 0;
		m_totalLength = 0;

		m_h[0] = 0x6a09e667;
		m_h[1] = 0xbb67ae85;
		m_h[2] = 0x3c6ef372;
		m_h[3] = 0xa54ff53a;
		m_h[4] = 0x510e527f;
		m_h[5] = 0x9b05688c;
		m_h[6] = 0x1f83d9ab;
		m_h[7] = 0x5be0cd19;
	}

	void
	SHA256::update (const uint8_t * message, size_t length) noexcept
	{
		const auto temporaryLength = SHA256::BlockSize - m_length;
		auto remainingLength = length < temporaryLength ? length : temporaryLength;

		memcpy(&m_block[m_length], message, remainingLength);

		if ( m_length + length < SHA256::BlockSize )
		{
			m_length += length;

			return;
		}

		this->transform(m_block.data(), 1);

		auto newLength = length - remainingLength;
		auto blockSize = newLength / SHA256::BlockSize;
		const auto * shiftedMessage = message + remainingLength;

		this->transform(shiftedMessage, blockSize);

		remainingLength = newLength % SHA256::BlockSize;

		memcpy(m_block.data(), &shiftedMessage[blockSize << 6], remainingLength);

		m_length = remainingLength;
		m_totalLength += (blockSize + 1) << 6;
	}

	void
	SHA256::final (std::array< uint8_t, 32 > & digest) noexcept
	{
		auto blockSize = 1 + static_cast< int >((SHA256::BlockSize - 9) < (m_length % SHA256::BlockSize));
		auto len_b = (m_totalLength + m_length) << 3;
		auto pm_length = blockSize << 6;

		memset(m_block.data() + m_length, 0, pm_length - m_length);

		m_block[m_length] = 0x80;

		SHA2_UNPACK32(len_b, m_block.data() + pm_length - 4);

		this->transform(m_block.data(), blockSize);

		for ( auto i = 0; i < 8; i++ )
		{
			SHA2_UNPACK32(m_h[i], &digest[i << 2]);
		}
	}
}

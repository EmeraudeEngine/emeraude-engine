/*
 * src/Libs/Hash/MD5.hpp
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

#pragma once

/* STL inclusions. */
#include <array>
#include <cstddef>
#include <cstdint>

namespace EmEn::Libs::Hash
{
	/**
	 * @brief The MD5 class.
	 * @note http://www.zedwood.com/article/cpp-md5-function
	 */
	class MD5 final
	{
		public:

			static constexpr auto HashLength = 32UL;

			/**
			 * @brief Default constructor
			 */
			MD5 () noexcept = default;

			/**
			 * @brief MD5 block processLogics operation. Continues an MD5 message-digest
			 * operation, processing another message block.
			 * @param message
			 * @param length
			 * @return void
			 */
			void update (const uint8_t * message, size_t length) noexcept;

			/**
			 * @brief finalize
			 * @param digest
			 * @return void
			 */
			void final (std::array< uint8_t, 16 > & digest) noexcept;

			/**
			 * @brief reset
			 * @return void
			 */
			void reset () noexcept;

		private:

			static constexpr auto BlockSize = 64UL;

			static constexpr uint32_t InitA = 0x67452301;
			static constexpr uint32_t InitB = 0xefcdab89;
			static constexpr uint32_t InitC = 0x98badcfe;
			static constexpr uint32_t InitD = 0x10325476;

			/**
			 * @brief Applies MD5 algo on a block
			 * @param block
			 * @return void
			 */
			void transform (const uint8_t * block) noexcept;

			/**
			 * @brief Encodes input (uint8_t) into output (uint32_t). Assumes len is a multiple of 4.
			 * @param output
			 * @param input
			 * @param length
			 * @return void
			 */
			static void decode (uint32_t * output, const uint8_t * input, size_t length) noexcept;

			/**
			 * @brief Encodes input (uint32_t) into output (uint8_t). Assumes len is a multiple of 4.
			 * @param output
			 * @param input
			 * @param length
			 * @return void
			 */
			static void encode (uint8_t * output, const uint32_t * input, size_t length) noexcept;

			/**
			 * @brief F
			 * @param x
			 * @param y
			 * @param z
			 * @return uint32_t
			 */
			static
			uint32_t
			F (uint32_t x, uint32_t y, uint32_t z) noexcept
			{
				return (x & y) | (~x & z);
			}

			/**
			 * @brief G
			 * @param x
			 * @param y
			 * @param z
			 * @return uint32_t
			 */
			static
			uint32_t
			G (uint32_t x, uint32_t y, uint32_t z) noexcept
			{
				return (x & z) | (y & ~z);
			}

			/**
			 * @brief H
			 * @param x
			 * @param y
			 * @param z
			 * @return uint32_t
			 */
			static
			uint32_t
			H (uint32_t x, uint32_t y, uint32_t z) noexcept
			{
				return x ^ y ^ z;
			}

			/**
			 * @brief I
			 * @param x
			 * @param y
			 * @param z
			 * @return uint32_t
			 */
			static
			uint32_t
			I (uint32_t x, uint32_t y, uint32_t z) noexcept
			{
				return y ^ (x | ~z);
			}

			/**
			 * @brief rotate_left
			 * @param x
			 * @param n
			 * @return uint32_t
			 */
			static
			uint32_t
			rotate_left (uint32_t x, uint32_t n) noexcept
			{
				return (x << n) | (x >> (32 - n));
			}

			/**
			 * @brief FF
			 * @param a
			 * @param b
			 * @param c
			 * @param d
			 * @param x
			 * @param s
			 * @param ac
			 * @return void
			 */
			static
			void
			FF (uint32_t & a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) noexcept
			{
				a = MD5::rotate_left(a+ MD5::F(b,c,d) + x + ac, s) + b;
			}

			/**
			 * @brief GG
			 * @param a
			 * @param b
			 * @param c
			 * @param d
			 * @param x
			 * @param s
			 * @param ac
			 * @return void
			 */
			static
			void
			GG (uint32_t & a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) noexcept
			{
				a = MD5::rotate_left(a + MD5::G(b,c,d) + x + ac, s) + b;
			}

			/**
			 * @brief HH
			 * @param a
			 * @param b
			 * @param c
			 * @param d
			 * @param x
			 * @param s
			 * @param ac
			 * @return void
			 */
			static
			void
			HH (uint32_t & a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) noexcept
			{
				a = MD5::rotate_left(a + MD5::H(b,c,d) + x + ac, s) + b;
			}

			/**
			 * @brief II
			 * @param a
			 * @param b
			 * @param c
			 * @param d
			 * @param x
			 * @param s
			 * @param ac
			 * @return void
			 */
			static
			void
			II (uint32_t & a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) noexcept
			{
				a = MD5::rotate_left(a + MD5::I(b,c,d) + x + ac, s) + b;
			}

			std::array< uint8_t, BlockSize > m_buffer{};
			std::array< uint32_t, 2 > m_count{0, 0};
			std::array< uint32_t, 4 > m_states{InitA, InitB, InitC, InitD};
	};
}

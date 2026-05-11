/*
 * src/Libs/Hash/CRC32.cpp
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

#include "CRC32.hpp"

namespace EmEn::Libs::Hash
{
	namespace
	{
		/** @brief 256-entry lookup table for the reflected polynomial 0xEDB88320 (IEEE 802.3 / zlib / PNG). */
		constexpr auto Table = []() noexcept {
			constexpr uint32_t Polynomial{0xEDB88320};

			std::array< uint32_t, 256 > table{};

			for ( uint32_t i = 0; i < 256; ++i )
			{
				uint32_t crc = i;

				for ( uint32_t j = 0; j < 8; ++j )
				{
					crc = ((crc & 1U) != 0U) ? ((crc >> 1) ^ Polynomial) : (crc >> 1);
				}

				table[i] = crc;
			}

			return table;
		}();
	}

	void
	CRC32::update (const uint8_t * message, size_t length) noexcept
	{
		uint32_t crc = m_state;

		for ( size_t i = 0; i < length; ++i )
		{
			crc = Table[(crc ^ message[i]) & 0xFFU] ^ (crc >> 8);
		}

		m_state = crc;
	}

	void
	CRC32::final (std::array< uint8_t, 4 > & digest) noexcept
	{
		const uint32_t result = m_state ^ 0xFFFFFFFF;

		digest[0] = static_cast< uint8_t >((result >> 24) & 0xFFU);
		digest[1] = static_cast< uint8_t >((result >> 16) & 0xFFU);
		digest[2] = static_cast< uint8_t >((result >>  8) & 0xFFU);
		digest[3] = static_cast< uint8_t >( result        & 0xFFU);
	}

	uint32_t
	CRC32::value () const noexcept
	{
		return m_state ^ 0xFFFFFFFF;
	}

	void
	CRC32::reset () noexcept
	{
		m_state = 0xFFFFFFFF;
	}
}
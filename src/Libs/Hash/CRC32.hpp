/*
 * src/Libs/Hash/CRC32.hpp
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
	 * @brief The CRC32 class.
	 * @note Standard CRC-32 (IEEE 802.3 / zlib / PNG / gzip) using the reflected polynomial 0xEDB88320.
	 */
	class CRC32 final
	{
		public:

			static constexpr auto HashLength = 8UL;

			/**
			 * @brief Default constructor.
			 */
			CRC32 () noexcept = default;

			/**
			 * @brief Continues a CRC32 computation, processing another message block.
			 * @param message A pointer to the input data.
			 * @param length The size of the input data in bytes.
			 * @return void
			 */
			void update (const uint8_t * message, size_t length) noexcept;

			/**
			 * @brief Finalizes the CRC32 computation and writes the 4-byte digest in big-endian order.
			 * @param digest A reference to a 4-byte array receiving the result.
			 * @return void
			 */
			void final (std::array< uint8_t, 4 > & digest) noexcept;

			/**
			 * @brief Returns the current CRC32 value (post-finalization XOR applied).
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t value () const noexcept;

			/**
			 * @brief Resets the internal state to start a new computation.
			 * @return void
			 */
			void reset () noexcept;

		private:

			uint32_t m_state{0xFFFFFFFF};
	};
}
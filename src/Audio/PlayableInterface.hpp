/*
 * src/Audio/PlayableInterface.hpp
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
#include <memory>

/* Local inclusions. */
#include "Buffer.hpp"

namespace EmEn::Audio
{
	/**
	 * @brief Playable interface class.
	 */
	class PlayableInterface
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			PlayableInterface (const PlayableInterface & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			PlayableInterface (PlayableInterface && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return PlayableInterface &
			 */
			PlayableInterface & operator= (const PlayableInterface & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return PlayableInterface &
			 */
			PlayableInterface & operator= (PlayableInterface && copy) noexcept = default;

			/**
			 * @brief Destructs the playable interface.
			 */
			virtual ~PlayableInterface () = default;

			/**
			 * @brief Returns the number of chunks.
			 * @return size_t
			 */
			[[nodiscard]]
			virtual size_t streamable () const noexcept = 0;

			/**
			 * @brief Returns the buffer smart pointer.
			 * @param bufferIndex The index of buffer if it's streamable. Default 0.
			 * @return std::shared_ptr< const Buffer >
			 */
			[[nodiscard]]
			virtual std::shared_ptr< const Buffer > buffer (size_t bufferIndex = 0) const noexcept = 0;

		protected:

			/**
			 * @brief Constructs a playable interface.
			 */
			PlayableInterface () noexcept = default;
	};
}

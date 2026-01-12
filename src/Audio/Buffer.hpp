/*
 * src/Audio/Buffer.hpp
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

/* Local inclusions for inheritances. */
#include "AbstractObject.hpp"

/* Local inclusions for usages.*/
#include "Libs/WaveFactory/Wave.hpp"

namespace EmEn::Audio
{
	/**
	 * @brief The Buffer class.
	 * @extends EmEn::Audio::AbstractObject
	 */
	class Buffer final : public AbstractObject
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"AudioBuffer"};

			/**
			 * @brief Constructs a buffer.
			 */
			Buffer () noexcept;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Buffer (const Buffer & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Buffer (Buffer && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			Buffer & operator= (const Buffer & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			Buffer & operator= (Buffer && copy) noexcept = default;

			/**
			 *  @brief Destructs the buffer.
			 */
			~Buffer () override;

			/** @copydoc EmEn::Audio::AbstractObject::isCreated() */
			[[nodiscard]]
			bool
			isCreated () const noexcept override
			{
				if ( this->identifier() <= 0 )
				{
					return false;
				}

				return alIsBuffer(this->identifier()) == AL_TRUE;
			}

			/**
			 * @brief Copies data to the buffer.
			 * @param wave A reference to wave.
			 * @param chunkIndex The index of the chunk being copied. Default 0.
			 * @param length The length of the chunk. Default 0.
			 * @return bool
			 */
			[[nodiscard]]
			bool feedData (const Libs::WaveFactory::Wave< short int > & wave, size_t chunkIndex = 0, size_t length = 0) noexcept;

			/**
			 * @brief Returns the frequency of the buffer.
			 * @return int
			 */
			[[nodiscard]]
			int frequency () const noexcept;

			/**
			 * @brief Returns the depth of the buffer.
			 * @return int
			 */
			[[nodiscard]]
			int bits () const noexcept;

			/**
			 * @brief Returns the number of channel of the buffer.
			 * @return int
			 */
			[[nodiscard]]
			int channels () const noexcept;

			/**
			 * @brief Returns the size in bytes of the buffer.
			 * @return int
			 */
			[[nodiscard]]
			int size () const noexcept;
	};
}

/*
 * src/Audio/Filters/HighPass.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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
#include "Abstract.hpp"

namespace EmEn::Audio::Filters
{
	/**
	 * @brief The high pass filter.
	 * @extends EmEn::Audio::Filters::Abstract This is a filter.
	 */
	class HighPass final : public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"HighPass"};

			/**
			 * @brief Constructs a high pass filter.
			 */
			HighPass () noexcept;

			/** @copydoc EmEn::Audio::Filters::Abstract::resetProperties() */
			void resetProperties () noexcept override;

			/**
			 * @brief Sets the gain.
			 * @param value A value between 0 and 1.
			 */
			void setGain (float value) noexcept;

			/**
			 * @brief Sets the gain at low frequency.
			 * @param value A value between 0 and 1.
			 */
			void setGainLF (float value) noexcept;

			/**
			 * @brief Returns the gain value.
			 * @return float
			 */
			[[nodiscard]]
			float gain () const noexcept;

			/**
			 * @brief Returns the gain at low frequency value.
			 * @return float
			 */
			[[nodiscard]]
			float gainLF () const noexcept;
	};
}

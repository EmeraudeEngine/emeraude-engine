/*
 * src/Audio/Effects/Echo.hpp
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

namespace EmEn::Audio::Effects
{
	/**
	 * @brief Echo effect.
	 * @extends EmEn::Audio::Effects::Abstract This is an effect.
	 */
	class Echo final : public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Echo"};

			/**
			 * @brief Constructs an echo effect.
			 */
			Echo () noexcept;

			/** @copydoc EmEn::Audio::Effects::Abstract::resetProperties() */
			void resetProperties () noexcept override;

			/**
			 * @brief Sets the delay.
			 * @param value A value between 0 and 0.207.
			 */
			void setDelay (float value) noexcept;

			/**
			 * @brief Sets the low rate delay.
			 * @param value A value between 0 and 0.404.
			 */
			void setLRDelay (float value) noexcept;

			/**
			 * @brief Sets the damping.
			 * @param value A value between 0 and 0.99.
			 */
			void setDamping (float value) noexcept;

			/**
			 * @brief Sets the feed back.
			 * @param value A value between 0 and 1.
			 */
			void setFeedBack (float value) noexcept;

			/**
			 * @brief Sets the spread.
			 * @param value A value between -1 and 1.
			 */
			void setSpread (float value) noexcept;

			/**
			 * @brief Returns the delay value.
			 * @return float
			 */
			[[nodiscard]]
			float delay () const noexcept;

			/**
			 * @brief Returns the low rate delay value.
			 * @return float
			 */
			[[nodiscard]]
			float LRDelay () const noexcept;

			/**
			 * @brief Returns the damping value.
			 * @return float
			 */
			[[nodiscard]]
			float damping () const noexcept;

			/**
			 * @brief Returns the feed back value.
			 * @return float
			 */
			[[nodiscard]]
			float feedBack () const noexcept;

			/**
			 * @brief Returns the spread value.
			 * @return float
			 */
			[[nodiscard]]
			float spread () const noexcept;
	};
}

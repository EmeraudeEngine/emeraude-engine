/*
 * src/Audio/Effects/Distortion.hpp
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
	 * @brief Distortion effect.
	 * @extends EmEn::Audio::Effects::Abstract This is an effect.
	 */
	class Distortion final : public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Distortion"};

			/**
			 * @brief Constructs a distortion effect.
			 */
			Distortion () noexcept;

			/** @copydoc EmEn::Audio::Effects::Abstract::resetProperties() */
			void resetProperties () noexcept override;

			/**
			 * @brief Sets the edge.
			 * @param value A value between 0 and 1.
			 */
			void setEdge (float value) noexcept;

			/**
			 * @brief Sets the gain.
			 * @param value A value between 0.01 and 1.
			 */
			void setGain (float value) noexcept;

			/**
			 * @brief Sets the low pass cut-off.
			 * @param value A value between 80 and 24000.
			 */
			void setLowPassCutOff (float value) noexcept;

			/**
			 * @brief Sets the equalizer center.
			 * @param value A value between 80 and 24000.
			 */
			void setEQCenter (float value) noexcept;

			/**
			 * @brief Sets the equalizer bandwidth.
			 * @param value A value between 80 and 24000.
			 */
			void setEQBandWidth (float value) noexcept;

			/**
			 * @brief Returns the edge value.
			 * @return float
			 */
			[[nodiscard]]
			float edge () const noexcept;

			/**
			 * @brief Returns the gain value.
			 * @return float
			 */
			[[nodiscard]]
			float gain () const noexcept;

			/**
			 * @brief Returns the low pass cut-off value.
			 * @return float
			 */
			[[nodiscard]]
			float lowPassCutOff () const noexcept;

			/**
			 * @brief Returns the equalizer center value.
			 * @return float
			 */
			[[nodiscard]]
			float EQCenter () const noexcept;

			/**
			 * @brief Returns the equalizer bandwidth value.
			 * @return float
			 */
			[[nodiscard]]
			float EQBandWidth () const noexcept;
	};
}

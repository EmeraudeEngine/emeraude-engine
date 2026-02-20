/*
 * src/Scenes/LensEffects/Dithering.hpp
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
#include <algorithm>

/* Local inclusions for inheritances. */
#include "Saphir/FramebufferEffectInterface.hpp"

namespace EmEn::Scenes::LensEffects
{
	/**
	 * @brief The dithering lens effect class.
	 * @extends EmEn::Saphir::FramebufferEffectInterface This is a framebuffer effect.
	 *
	 * Applies ordered Bayer dithering to simulate the limited color depth
	 * of 8-bit and 16-bit era hardware. Uses a 4x4 Bayer matrix by default.
	 */
	class Dithering final : public Saphir::FramebufferEffectInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Dithering"};

			/**
			 * @brief Constructs a dithering lens effect.
			 * @param intensity The dithering strength in range [0, 1]. Default 0.15.
			 */
			explicit
			Dithering (float intensity = 0.15F) noexcept
				: m_intensity{std::clamp(intensity, 0.0F, 1.0F)}
			{

			}

			/** @copydoc EmEn::Saphir::FramebufferEffectInterface::generateFragmentShaderCode() */
			[[nodiscard]]
			bool generateFragmentShaderCode (Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept override;

			/**
			 * @brief Sets the dithering intensity.
			 * @param intensity Value in range [0, 1].
			 * @return void
			 */
			void
			setIntensity (float intensity) noexcept
			{
				m_intensity = std::clamp(intensity, 0.0F, 1.0F);
			}

			/**
			 * @brief Returns the dithering intensity.
			 * @return float
			 */
			[[nodiscard]]
			float
			intensity () const noexcept
			{
				return m_intensity;
			}

			/**
			 * @brief Sets the Bayer matrix size.
			 * @param size Accepted values: 2, 4, 8. Default 4.
			 * @return void
			 */
			void setMatrixSize (int size) noexcept;

			/**
			 * @brief Returns the Bayer matrix size.
			 * @return int
			 */
			[[nodiscard]]
			int
			matrixSize () const noexcept
			{
				return m_matrixSize;
			}

		private:

			/** @copydoc EmEn::Saphir::FramebufferEffectInterface::requestScreenSize() */
			[[nodiscard]]
			bool
			requestScreenSize () const noexcept override
			{
				return true;
			}

			float m_intensity{0.15F};
			int m_matrixSize{4};
	};
}

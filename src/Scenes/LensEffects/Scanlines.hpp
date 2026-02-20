/*
 * src/Scenes/LensEffects/Scanlines.hpp
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
	 * @brief The CRT scanlines lens effect class.
	 * @extends EmEn::Saphir::FramebufferEffectInterface This is a framebuffer effect.
	 *
	 * Simulates the horizontal scanlines visible on CRT monitors,
	 * where alternating bright and dark lines create the characteristic
	 * raster scan pattern.
	 */
	class Scanlines final : public Saphir::FramebufferEffectInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Scanlines"};

			/**
			 * @brief Constructs a scanlines lens effect.
			 * @param intensity The darkness of gaps between scanlines (0 = off, 1 = full black). Default 0.3.
			 */
			explicit
			Scanlines (float intensity = 0.3F) noexcept
				: m_intensity{std::clamp(intensity, 0.0F, 1.0F)}
			{

			}

			/** @copydoc EmEn::Saphir::FramebufferEffectInterface::generateFragmentShaderCode() */
			[[nodiscard]]
			bool generateFragmentShaderCode (Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept override;

			/**
			 * @brief Sets the scanline gap darkness intensity.
			 * @param intensity Value in range [0, 1].
			 * @return void
			 */
			void
			setIntensity (float intensity) noexcept
			{
				m_intensity = std::clamp(intensity, 0.0F, 1.0F);
			}

			/**
			 * @brief Returns the scanline gap darkness intensity.
			 * @return float
			 */
			[[nodiscard]]
			float
			intensity () const noexcept
			{
				return m_intensity;
			}

			/**
			 * @brief Sets the width of the bright scanline in pixels.
			 * @param width Value > 0. Default 1.0.
			 * @return void
			 */
			void setLineWidth (float width) noexcept;

			/**
			 * @brief Returns the bright scanline width.
			 * @return float
			 */
			[[nodiscard]]
			float
			lineWidth () const noexcept
			{
				return m_lineWidth;
			}

			/**
			 * @brief Sets the width of the dark gap between scanlines in pixels.
			 * @param width Value > 0. Default 1.0.
			 * @return void
			 */
			void setGapWidth (float width) noexcept;

			/**
			 * @brief Returns the dark gap width.
			 * @return float
			 */
			[[nodiscard]]
			float
			gapWidth () const noexcept
			{
				return m_gapWidth;
			}

		private:

			/** @copydoc EmEn::Saphir::FramebufferEffectInterface::requestScreenSize() */
			[[nodiscard]]
			bool
			requestScreenSize () const noexcept override
			{
				return true;
			}

			float m_intensity{0.3F};
			float m_lineWidth{1.0F};
			float m_gapWidth{1.0F};
	};
}

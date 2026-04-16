/*
 * src/Graphics/Effects/Lens/VHSAberration.hpp
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
#include "Graphics/DirectPostProcessEffect.hpp"

namespace EmEn::Graphics::Effects::Lens
{
	/**
	 * @brief The VHS tracking aberration lens effect class.
	 * @extends EmEn::Graphics::DirectPostProcessEffect This is a framebuffer effect.
	 *
	 * Simulates the iconic VHS tape tracking error: a band of horizontal
	 * displacement and luminance noise that scrolls near the bottom of the frame,
	 * characteristic of worn analog videotapes from the 1980s.
	 */
	class VHSAberration final : public DirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VHSAberration"};

			/**
			 * @brief Constructs a VHS tracking aberration effect.
			 * @param intensity The overall effect intensity (0.0 = off, 1.0 = full). Default 0.5.
			 */
			explicit
			VHSAberration (float intensity = 0.5F) noexcept
				: m_intensity{std::clamp(intensity, 0.0F, 1.0F)}
			{

			}

			/** @copydoc EmEn::Graphics::DirectPostProcessEffect::generateFragmentShaderCode() */
			[[nodiscard]]
			bool generateFragmentShaderCode (Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept override;

			/**
			 * @brief Sets the overall effect intensity.
			 * @param intensity Value in range [0, 1].
			 * @return void
			 */
			void
			setIntensity (float intensity) noexcept
			{
				m_intensity = std::clamp(intensity, 0.0F, 1.0F);
			}

			/**
			 * @brief Returns the overall effect intensity.
			 * @return float
			 */
			[[nodiscard]]
			float
			intensity () const noexcept
			{
				return m_intensity;
			}

			/**
			 * @brief Sets the height of the tracking error band as a fraction of screen height.
			 * @param height Value in range (0, 1]. Default 0.08.
			 * @return void
			 */
			void setBandHeight (float height) noexcept;

			/**
			 * @brief Returns the tracking error band height.
			 * @return float
			 */
			[[nodiscard]]
			float
			bandHeight () const noexcept
			{
				return m_bandHeight;
			}

			/**
			 * @brief Sets the horizontal displacement magnitude in UV space.
			 * @param displacement Value >= 0. Default 0.02.
			 * @return void
			 */
			void setDisplacement (float displacement) noexcept;

			/**
			 * @brief Returns the horizontal displacement magnitude.
			 * @return float
			 */
			[[nodiscard]]
			float
			displacement () const noexcept
			{
				return m_displacement;
			}

			/**
			 * @brief Sets the vertical scroll speed of the tracking band.
			 * @param speed Scroll speed multiplier. Default 0.15.
			 * @return void
			 */
			void
			setScrollSpeed (float speed) noexcept
			{
				m_scrollSpeed = speed;
			}

			/**
			 * @brief Returns the vertical scroll speed.
			 * @return float
			 */
			[[nodiscard]]
			float
			scrollSpeed () const noexcept
			{
				return m_scrollSpeed;
			}

			/**
			 * @brief Enables or disables the head switching noise bar at the bottom of the frame.
			 * @note Simulates the bright distorted horizontal bar caused by VCR head drum switching.
			 * @param enabled True to enable, false to disable. Default disabled.
			 * @return void
			 */
			void
			enableHeadSwitching (bool enabled) noexcept
			{
				m_headSwitchingEnabled = enabled;
			}

			/**
			 * @brief Returns whether head switching noise is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			headSwitchingEnabled () const noexcept
			{
				return m_headSwitchingEnabled;
			}

			/**
			 * @brief Sets the height of the head switching bar as a fraction of screen height.
			 * @param height Value in range (0, 1]. Default 0.025.
			 * @return void
			 */
			void setHeadSwitchHeight (float height) noexcept;

			/**
			 * @brief Returns the head switching bar height.
			 * @return float
			 */
			[[nodiscard]]
			float
			headSwitchHeight () const noexcept
			{
				return m_headSwitchHeight;
			}

			/**
			 * @brief Sets the brightness intensity of the head switching bar.
			 * @param brightness Value in range [0, 1]. Default 0.7.
			 * @return void
			 */
			void
			setHeadSwitchBrightness (float brightness) noexcept
			{
				m_headSwitchBrightness = std::clamp(brightness, 0.0F, 1.0F);
			}

			/**
			 * @brief Returns the head switching bar brightness.
			 * @return float
			 */
			[[nodiscard]]
			float
			headSwitchBrightness () const noexcept
			{
				return m_headSwitchBrightness;
			}

		private:

			/** @copydoc EmEn::Graphics::DirectPostProcessEffect::requestScreenSize() */
			[[nodiscard]]
			bool
			requestScreenSize () const noexcept override
			{
				return true;
			}

			float m_intensity{0.5F};
			float m_bandHeight{0.08F};
			float m_displacement{0.02F};
			float m_scrollSpeed{0.15F};
			bool m_headSwitchingEnabled{false};
			float m_headSwitchHeight{0.025F};
			float m_headSwitchBrightness{0.7F};
	};
}

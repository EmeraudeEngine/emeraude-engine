/*
 * src/Scenes/EffectsToolkit.hpp
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
#include <cstdint>
#include <memory>

/* Local inclusions for usages. */
#include "Libs/PixelFactory/Color.hpp"
#include "Saphir/FramebufferEffectInterface.hpp"
#include "Scenes/Component/PointLight.hpp"
#include "Scenes/Component/SphericalPushModifier.hpp"
#include "Scenes/Node.hpp"

namespace EmEn::Scenes
{
	/**
	 * @brief Utility class providing ready-made visual effects for scenes and post-processing presets.
	 */
	class EffectsToolkit final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"EffectsToolkit"};

			/** Default constructor. */
			EffectsToolkit () noexcept = delete;

			/**
			 * @brief Creates a flashing point light.
			 * @param node A reference to a scene node.
			 * @param color A reference to a color.
			 * @param radius The point light radius.
			 * @param intensity The point light initial intensity.
			 * @param duration The animation duration in milliseconds.
			 * @return std::shared_ptr< Component::PointLight >
			 */
			static std::shared_ptr< Component::PointLight > createFlashEffect (Node & node, const Libs::PixelFactory::Color< float > & color, float radius, float intensity, uint32_t duration) noexcept;

			/**
			 * @brief Creates a temporary spherical push force.
			 * @param node A reference to a scene node.
			 * @param radius The push limit radius.
			 * @param maxMagnitude The force of the push.
			 * @param duration The animation duration in milliseconds.
			 * @return std::shared_ptr< Component::SphericalPushModifier >
			 */
			static std::shared_ptr< Component::SphericalPushModifier > createBlowEffect (Node & node, float radius, float maxMagnitude, uint32_t duration) noexcept;

			/**
			 * @brief Creates a Hitchcock Psycho 1960 film noir post-processing preset.
			 * @note Film grain + high contrast B&W + vignetting.
			 * @return Saphir::FramebufferEffectsList
			 */
			static Saphir::FramebufferEffectsList hitchcock60s () noexcept;

			/**
			 * @brief Creates a warm cinematic golden hour post-processing preset.
			 * @note Color grading (warm hue, punchy contrast, vibrant saturation) + soft vignetting.
			 * @return Saphir::FramebufferEffectsList
			 */
			static Saphir::FramebufferEffectsList goldenHour () noexcept;

			/**
			 * @brief Creates a clean signal on a CRT television post-processing preset.
			 * @note Pure CRT display artifacts only (scanlines, phosphor, barrel distortion). No signal degradation.
			 * @return Saphir::FramebufferEffectsList
			 */
			static Saphir::FramebufferEffectsList analog80s () noexcept;

			/**
			 * @brief Creates a VHS signal displayed on a CRT television post-processing preset.
			 * @note K7 tape artifacts + CRT display (scanlines, phosphor, barrel distortion).
			 * @return Saphir::FramebufferEffectsList
			 */
			static Saphir::FramebufferEffectsList VHSToAnalog80s () noexcept;

			/**
			 * @brief Creates a poor satellite signal displayed on a CRT television post-processing preset.
			 * @note Signal ghosting + chroma bleeding + CRT display artifacts. No VHS tape artifacts.
			 * @return Saphir::FramebufferEffectsList
			 */
			static Saphir::FramebufferEffectsList SatelliteToAnalog80s () noexcept;

			/**
			 * @brief Creates a VHS signal displayed on a modern screen post-processing preset.
			 * @note K7 tape artifacts (VHS aberration, chroma bleeding) + washed-out color grading + film grain.
			 * @return Saphir::FramebufferEffectsList
			 */
			static Saphir::FramebufferEffectsList VHSToPureSignal () noexcept;

			/**
			 * @brief Creates a poor satellite signal displayed on a modern screen post-processing preset.
			 * @note Signal ghosting + chroma bleeding. No CRT or VHS artifacts.
			 * @return Saphir::FramebufferEffectsList
			 */
			static Saphir::FramebufferEffectsList SatelliteToPureSignal () noexcept;

		/**
		 * @brief Creates a retro 8-bit post-processing preset.
		 * @note Pixelation + color quantization + Bayer dithering + scanlines + phosphor bloom + vignetting.
		 * @return Saphir::FramebufferEffectsList
		 */
		static Saphir::FramebufferEffectsList retro8Bits () noexcept;
	};
}
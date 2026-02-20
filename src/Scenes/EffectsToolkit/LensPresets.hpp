/*
 * src/Scenes/EffectsToolkit/LensPresets.hpp
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

/* Local inclusions for usages. */
#include "Graphics/DirectPostProcessEffect.hpp"

namespace EmEn::Scenes::EffectsToolkit::LensPresets
{
	/**
	 * @brief Creates a Hitchcock Psycho 1960 film noir post-processing preset.
	 * @note Film grain + high contrast B&W + vignetting.
	 * @return std::vector< std::shared_ptr< Graphics::DirectPostProcessEffect > >
	 */
	std::vector< std::shared_ptr< Graphics::DirectPostProcessEffect > > Hitchcock60s () noexcept;

	/**
	 * @brief Creates a warm cinematic golden hour post-processing preset.
	 * @note Color grading (warm hue, punchy contrast, vibrant saturation) + soft vignetting.
	 * @return std::vector< std::shared_ptr< Graphics::DirectPostProcessEffect > >
	 */
	std::vector< std::shared_ptr< Graphics::DirectPostProcessEffect > > GoldenHour () noexcept;

	/**
	 * @brief Creates a cool twilight blue hour post-processing preset.
	 * @note Color grading (cool blue shift, soft contrast, muted saturation) + deeper vignetting.
	 * @return std::vector< std::shared_ptr< Graphics::DirectPostProcessEffect > >
	 */
	std::vector< std::shared_ptr< Graphics::DirectPostProcessEffect > > BlueHour () noexcept;

	/**
	 * @brief Creates a clean signal on a CRT television post-processing preset.
	 * @note Pure CRT display artifacts only (scanlines, phosphor, barrel distortion). No signal degradation.
	 * @return std::vector< std::shared_ptr< Graphics::DirectPostProcessEffect > >
	 */
	std::vector< std::shared_ptr< Graphics::DirectPostProcessEffect > > Analog80s () noexcept;

	/**
	 * @brief Creates a VHS signal displayed on a CRT television post-processing preset.
	 * @note K7 tape artifacts + CRT display (scanlines, phosphor, barrel distortion).
	 * @return std::vector< std::shared_ptr< Graphics::DirectPostProcessEffect > >
	 */
	std::vector< std::shared_ptr< Graphics::DirectPostProcessEffect > > VHSToAnalog80s () noexcept;

	/**
	 * @brief Creates a poor satellite signal displayed on a CRT television post-processing preset.
	 * @note Signal ghosting + chroma bleeding + CRT display artifacts. No VHS tape artifacts.
	 * @return std::vector< std::shared_ptr< Graphics::DirectPostProcessEffect > >
	 */
	std::vector< std::shared_ptr< Graphics::DirectPostProcessEffect > > SatelliteToAnalog80s () noexcept;

	/**
	 * @brief Creates a VHS signal displayed on a modern screen post-processing preset.
	 * @note K7 tape artifacts (VHS aberration, chroma bleeding) + washed-out color grading + film grain.
	 * @return std::vector< std::shared_ptr< Graphics::DirectPostProcessEffect > >
	 */
	std::vector< std::shared_ptr< Graphics::DirectPostProcessEffect > > VHSToPureSignal () noexcept;

	/**
	 * @brief Creates a poor satellite signal displayed on a modern screen post-processing preset.
	 * @note Signal ghosting + chroma bleeding. No CRT or VHS artifacts.
	 * @return std::vector< std::shared_ptr< Graphics::DirectPostProcessEffect > >
	 */
	std::vector< std::shared_ptr< Graphics::DirectPostProcessEffect > > SatelliteToPureSignal () noexcept;

	/**
	 * @brief Creates a retro 8-bit post-processing preset.
	 * @note Pixelation + color quantization + Bayer dithering + scanlines + phosphor bloom + vignetting.
	 * @return std::vector< std::shared_ptr< Graphics::DirectPostProcessEffect > >
	 */
	std::vector< std::shared_ptr< Graphics::DirectPostProcessEffect > > Retro8Bits () noexcept;;
}

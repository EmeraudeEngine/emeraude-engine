/*
 * src/Graphics/MoonSurfacePhotometry.hpp
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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

/* Local inclusions for usages. */
#include "Math/Vector.hpp"
#include "PixelFactory/Pixmap.hpp"

namespace EmEn::Graphics
{
	/**
	 * @brief The photometry of a moon's SURFACE map, measured once from its pixels: the mean the sky material divides
	 * the map by, and how much brighter or darker than a uniform sphere the textured disc is for every position of the
	 * star seen from the moon — what makes the moon's LIGHT the integral of the disc the sky draws (graphics doc 42
	 * § "The moons", owner decision 2026-10-25).
	 * @note The map is equirectangular: 0° longitude at its centre, the face toward the planet, east to the right, north
	 * up (Material::SkyAtmosphereResource::setMoonSurface()). The sky draws that face toward the observer always (no
	 * libration), so the disc's brightness depends on the star's position alone: the SUB-SOLAR point, the point of the
	 * surface under the star, in the frame subSolarPoint() builds like the sky shader does.
	 * @note The default object is a UNIFORM surface: mean 1, relative brightness 1 everywhere.
	 * @note Immutable once measured: the sky material measures it on a loader thread before its creation, the
	 * celestial clock reads it on the logic thread after the sky resource is loaded (its status publishes it).
	 */
	class EMEN_API MoonSurfacePhotometry final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"MoonSurfacePhotometry"};

			/** @brief The table's steps per half turn: 36, a 5° step in the sub-solar longitude and latitude. */
			static constexpr size_t StepsPerHalfTurn{36};

			/** @brief The table's longitudes, −180° to 180° both included. */
			static constexpr size_t LongitudeCount{(2 * StepsPerHalfTurn) + 1};

			/** @brief The table's latitudes, −90° to 90° both included. */
			static constexpr size_t LatitudeCount{StepsPerHalfTurn + 1};

			/** @brief The measurement grid's width: the map box-filtered to at most this many cells across, and half as
			 * many down (a ratio of two disc integrals needs no finer cell than 1.4°). */
			static constexpr size_t GridWidth{256};

			/**
			 * @brief The point of a moon's surface under its star, in its map's frame.
			 */
			struct SubSolarPoint
			{
				/** @brief The longitude, in radians, east positive: 0 faces the planet. */
				float longitude{0.0F};
				/** @brief The latitude, in radians, north positive. */
				float latitude{0.0F};
			};

			/**
			 * @brief Where a moon stands, for its sub-solar point: unit world vectors.
			 */
			struct MoonGeometry
			{
				/** @brief The direction toward the star. */
				Base::Math::Vector< 3, float > starDirection{0.0F, 1.0F, 0.0F};
				/** @brief The direction toward the moon. */
				Base::Math::Vector< 3, float > moonDirection{0.0F, 1.0F, 0.0F};
				/** @brief The pole of the moon's orbit. */
				Base::Math::Vector< 3, float > pole{0.0F, 1.0F, 0.0F};
			};

			/**
			 * @brief Measures a surface map.
			 * @note About 2 700 disc integrals over the near side of the measurement grid (tens of milliseconds, once):
			 * call it off the frame loop.
			 * @param map A reference to the map's pixels (grey, RGB or RGBA, one byte per channel).
			 * @param sRGB True when the pixels are sRGB-encoded (decoded to linear light before any mean).
			 * @return std::optional< MoonSurfacePhotometry > Nothing for an invalid map or a near side without light.
			 */
			[[nodiscard]]
			static std::optional< MoonSurfacePhotometry > measure (const Base::PixelFactory::Pixmap< uint8_t > & map, bool sRGB) noexcept;

			/**
			 * @brief Returns the mean luminance, in linear light, of a map over the WHOLE sphere (each texel weighted by
			 * cos(latitude), its area): the calibration of a turning body's surface (a planet's), whose integrated light
			 * the magnitude already gives.
			 * @param map A reference to the map's pixels (grey, RGB or RGBA, one byte per channel; equirectangular).
			 * @param sRGB True when the pixels are sRGB-encoded.
			 * @return std::optional< float > Nothing for an invalid or black map.
			 */
			[[nodiscard]]
			static std::optional< float > sphereMean (const Base::PixelFactory::Pixmap< uint8_t > & map, bool sRGB) noexcept;

			/**
			 * @brief Returns the sub-solar point of a moon: the frame the sky shader draws its map in (the face toward the
			 * observer, north along the orbit's pole) — keep both in step (SkyAtmosphereResource, the moons' block).
			 * @note A pole along the line of sight falls back to the shader's substitute (world +Y, or +X when the moon is
			 * near the vertical).
			 * @param geometry The star's and the moon's directions and the orbit's pole.
			 * @return SubSolarPoint The origin for a degenerate frame.
			 */
			[[nodiscard]]
			static SubSolarPoint subSolarPoint (const MoonGeometry & geometry) noexcept;

			/**
			 * @brief Returns the mean luminance, in linear light, of the map's NEAR side as the full moon shows it: the
			 * reflectance weighted by its projected area, cos²(latitude) × cos(longitude) over |longitude| < 90°.
			 * @note At full the lunar reflectance is uniform over the disc, so this mean is what the full moon's
			 * illuminance sees: the sky material divides the map by it, and the textured disc integrated at full is the
			 * moon's full illuminance exactly.
			 * @return float
			 */
			[[nodiscard]]
			float
			nearSideMean () const noexcept
			{
				return m_nearSideMean;
			}

			/**
			 * @brief Returns the textured disc's integrated brightness relative to a uniform disc of the same near-side
			 * mean, for a sub-solar point: 1 at full, below 1 when the lit part is darker than the mean (the maria), above
			 * when brighter. The moon's light is the uniform phase law × this.
			 * @note Bilinear in the 5° table; the longitude wraps, the latitude is clamped. 1 for a non-finite point.
			 * @param point The sub-solar point (subSolarPoint()).
			 * @return float
			 */
			[[nodiscard]]
			float relativeBrightness (const SubSolarPoint & point) const noexcept;

		private:

			/**
			 * @brief Returns the table of a uniform surface: 1 everywhere.
			 * @return std::array< float, LongitudeCount * LatitudeCount >
			 */
			[[nodiscard]]
			static constexpr
			std::array< float, LongitudeCount * LatitudeCount >
			uniformTable () noexcept
			{
				std::array< float, LongitudeCount * LatitudeCount > table{};

				for ( auto & value : table )
				{
					value = 1.0F;
				}

				return table;
			}

			/* [latitude][longitude], latitude −90° first, longitude −180° first. */
			std::array< float, LongitudeCount * LatitudeCount > m_table{uniformTable()};
			float m_nearSideMean{1.0F};
	};
}

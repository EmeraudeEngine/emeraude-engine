/*
 * src/Graphics/MoonSurfacePhotometry.cpp
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

#include "MoonSurfacePhotometry.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>
#include <numbers>
#include <span>
#include <utility>
#include <vector>

/* Local inclusions. */
#include "Math/Astronomy.hpp"
#include "PixelFactory/Reduction.hpp"

namespace EmEn::Graphics
{
	using namespace Base;

	namespace
	{
		/**
		 * @brief A cell of the measurement grid: its normal in the moon's frame and its share of the near side.
		 */
		struct Cell
		{
			/** @brief The normal's component toward the observer: the cos of the emission, μ. */
			double toward{0.0};
			double east{0.0};
			double north{0.0};
			/** @brief The projected area: cos(latitude) (the cell's area on the sphere) × μ. */
			double weight{0.0};
			/** @brief The mean luminance of its texels, in linear light. */
			double luminance{0.0};
		};

		/**
		 * @brief Reads a map's texels as luminance in linear light.
		 * @note Holds a view of the map's pixels: it lives inside MoonSurfacePhotometry::measure(), while the map does.
		 */
		class TexelReader final
		{
			public:

				/**
				 * @brief Constructs a reader.
				 * @param map A reference to a VALID map (it outlives the reader).
				 * @param sRGB True when the pixels are sRGB-encoded.
				 */
				TexelReader (const PixelFactory::Pixmap< uint8_t > & map, bool sRGB) noexcept
					: m_data{map.data()},
					m_width{static_cast< size_t >(map.width())},
					m_height{static_cast< size_t >(map.height())},
					m_stride{static_cast< size_t >(map.colorCount())}
				{
					size_t value = 0;

					for ( auto & decoded : m_decode )
					{
						const auto encoded = static_cast< float >(value) / 255.0F;

						decoded = static_cast< double >(sRGB ? PixelFactory::sRGBToLinear(encoded) : encoded);
						++value;
					}
				}

				[[nodiscard]]
				size_t
				width () const noexcept
				{
					return m_width;
				}

				[[nodiscard]]
				size_t
				height () const noexcept
				{
					return m_height;
				}

				/**
				 * @brief Returns a texel's luminance (Rec. 709 weights; a grey map's value).
				 * @pre row < height(), column < width().
				 * @param row The texel's row.
				 * @param column The texel's column.
				 * @return double
				 */
				[[nodiscard]]
				double
				luminance (size_t row, size_t column) const noexcept
				{
					const auto pixel = m_data.subspan(((row * m_width) + column) * m_stride, m_stride);
					const std::span< const double > decode{m_decode};

					if ( m_stride < 3 )
					{
						return decode[pixel[0]];
					}

					return (0.2126 * decode[pixel[0]]) + (0.7152 * decode[pixel[1]]) + (0.0722 * decode[pixel[2]]);
				}

			private:

				std::span< const uint8_t > m_data;
				std::array< double, 256 > m_decode{};
				size_t m_width;
				size_t m_height;
				size_t m_stride;
		};

		/**
		 * @brief Returns the latitude of a row's centre, in radians, north first.
		 * @param index The row.
		 * @param count The row count.
		 * @return double
		 */
		[[nodiscard]]
		double
		latitudeOf (size_t index, size_t count) noexcept
		{
			return (0.5 - ((static_cast< double >(index) + 0.5) / static_cast< double >(count))) * std::numbers::pi;
		}

		/**
		 * @brief Returns the longitude of a column's centre, in radians, 0 at the map's centre, east to the right.
		 * @param index The column.
		 * @param count The column count.
		 * @return double
		 */
		[[nodiscard]]
		double
		longitudeOf (size_t index, size_t count) noexcept
		{
			return (((static_cast< double >(index) + 0.5) / static_cast< double >(count)) - 0.5) * 2.0 * std::numbers::pi;
		}

		/**
		 * @brief Returns the near side's mean luminance at the map's own resolution, weighted by cos²(latitude) ×
		 * cos(longitude): the sky material's calibration.
		 * @param texels A reference to the reader.
		 * @return std::optional< double > Nothing for a near side without light.
		 */
		[[nodiscard]]
		std::optional< double >
		measureNearSideMean (const TexelReader & texels) noexcept
		{
			double weighted = 0.0;
			double weights = 0.0;

			for ( size_t row = 0; row < texels.height(); ++row )
			{
				const auto cosLatitude = std::cos(latitudeOf(row, texels.height()));

				for ( size_t column = 0; column < texels.width(); ++column )
				{
					const auto longitude = longitudeOf(column, texels.width());

					/* The near side only: the hemisphere a full moon shows. */
					if ( std::abs(longitude) < 0.5 * std::numbers::pi )
					{
						const auto weight = cosLatitude * cosLatitude * std::cos(longitude);

						weighted += weight * texels.luminance(row, column);
						weights += weight;
					}
				}
			}

			const auto mean = weights > 0.0 ? weighted / weights : 0.0;

			if ( !(mean > 1e-4) || !std::isfinite(mean) )
			{
				return std::nullopt;
			}

			return mean;
		}

		/**
		 * @brief Returns the mean luminance of a block of texels.
		 * @pre The block lies inside the map.
		 * @param texels A reference to the reader.
		 * @param rows The block's rows, [first, last).
		 * @param columns The block's columns, [first, last).
		 * @return double 0 for an empty block.
		 */
		[[nodiscard]]
		double
		blockMean (const TexelReader & texels, std::pair< size_t, size_t > rows, std::pair< size_t, size_t > columns) noexcept
		{
			double sum = 0.0;

			for ( auto row = rows.first; row < rows.second; ++row )
			{
				for ( auto column = columns.first; column < columns.second; ++column )
				{
					sum += texels.luminance(row, column);
				}
			}

			const auto count = static_cast< double >((rows.second - rows.first) * (columns.second - columns.first));

			return count > 0.0 ? sum / count : 0.0;
		}

		/**
		 * @brief Returns the measurement grid: the near side box-filtered to at most GridWidth × GridWidth / 2 cells.
		 * @param texels A reference to the reader.
		 * @return std::vector< Cell >
		 */
		[[nodiscard]]
		std::vector< Cell >
		measurementGrid (const TexelReader & texels) noexcept
		{
			const auto columns = std::min(texels.width(), MoonSurfacePhotometry::GridWidth);
			const auto rows = std::min(texels.height(), MoonSurfacePhotometry::GridWidth / 2);
			std::vector< Cell > cells;
			cells.reserve(rows * columns / 2);

			for ( size_t row = 0; row < rows; ++row )
			{
				const auto latitude = latitudeOf(row, rows);
				const std::pair texelRows{(row * texels.height()) / rows, ((row + 1) * texels.height()) / rows};

				for ( size_t column = 0; column < columns; ++column )
				{
					const auto longitude = longitudeOf(column, columns);

					if ( std::abs(longitude) >= 0.5 * std::numbers::pi )
					{
						continue;
					}

					const std::pair texelColumns{(column * texels.width()) / columns, ((column + 1) * texels.width()) / columns};
					const auto toward = std::cos(latitude) * std::cos(longitude);

					cells.push_back(Cell{
						.toward = toward,
						.east = std::cos(latitude) * std::sin(longitude),
						.north = std::sin(latitude),
						.weight = std::cos(latitude) * toward,
						.luminance = blockMean(texels, texelRows, texelColumns)
					});
				}
			}

			return cells;
		}
	}

	std::optional< MoonSurfacePhotometry >
	MoonSurfacePhotometry::measure (const PixelFactory::Pixmap< uint8_t > & map, bool sRGB) noexcept
	{
		if ( !map.isValid() )
		{
			return std::nullopt;
		}

		const TexelReader texels{map, sRGB};
		const auto mean = measureNearSideMean(texels);

		if ( !mean.has_value() )
		{
			return std::nullopt;
		}

		const auto cells = measurementGrid(texels);
		double fullTextured = 0.0;
		double fullUniform = 0.0;

		for ( const auto & cell : cells )
		{
			fullTextured += cell.weight * cell.luminance;
			fullUniform += cell.weight;
		}

		/* The grid's own full-phase mean: a ratio of two sums over the same cells is exactly 1 at full. */
		if ( !(fullUniform > 0.0) || !(fullTextured > 0.0) )
		{
			return std::nullopt;
		}

		MoonSurfacePhotometry photometry;
		photometry.m_nearSideMean = static_cast< float >(mean.value());

		const auto gridMean = fullTextured / fullUniform;
		const auto step = std::numbers::pi / static_cast< double >(StepsPerHalfTurn);
		/* Latitude-major, longitude −180° first: the indices below are < LatitudeCount × LongitudeCount. */
		const std::span table{photometry.m_table};

		/* Per sub-solar point: the disc integral of the textured surface over the uniform one. The lunar reflectance is
		 * the Lommel-Seeliger kernel × factors of the phase angle alone (Math::Astronomy::lunarReflectance()): uniform
		 * over the disc, they cancel in the ratio. */
		for ( size_t latitudeIndex = 0; latitudeIndex < LatitudeCount; ++latitudeIndex )
		{
			const auto latitude = (static_cast< double >(latitudeIndex) * step) - (0.5 * std::numbers::pi);

			for ( size_t longitudeIndex = 0; longitudeIndex < LongitudeCount; ++longitudeIndex )
			{
				const auto longitude = (static_cast< double >(longitudeIndex) * step) - std::numbers::pi;
				const auto starToward = std::cos(latitude) * std::cos(longitude);
				const auto starEast = std::cos(latitude) * std::sin(longitude);
				const auto starNorth = std::sin(latitude);
				double textured = 0.0;
				double uniform = 0.0;

				for ( const auto & cell : cells )
				{
					const auto kernel = Math::Astronomy::lommelSeeligerKernel((cell.toward * starToward) + (cell.east * starEast) + (cell.north * starNorth), cell.toward);

					textured += kernel * cell.weight * cell.luminance;
					uniform += kernel * cell.weight;
				}

				/* A new moon shows no lit surface: nothing to weigh, the uniform value. */
				const auto relative = uniform > 1e-9 * fullUniform ? textured / (uniform * gridMean) : 1.0;

				table[(latitudeIndex * LongitudeCount) + longitudeIndex] = std::isfinite(relative) ? static_cast< float >(relative) : 1.0F;
			}
		}

		return photometry;
	}

	MoonSurfacePhotometry::SubSolarPoint
	MoonSurfacePhotometry::subSolarPoint (const MoonGeometry & geometry) noexcept
	{
		using Axis = std::array< double, 3 >;

		const auto toAxis = [] (const Math::Vector< 3, float > & vector) {
			return Axis{static_cast< double >(vector.x()), static_cast< double >(vector.y()), static_cast< double >(vector.z())};
		};
		const auto dot = [] (const Axis & lhs, const Axis & rhs) {
			const auto & [lx, ly, lz] = lhs;
			const auto & [rx, ry, rz] = rhs;

			return (lx * rx) + (ly * ry) + (lz * rz);
		};

		/* The sky shader's frame: toward the observer, north along the pole (made perpendicular), east = north × toward. */
		const auto moon = toAxis(geometry.moonDirection);
		const Axis toward{-moon[0], -moon[1], -moon[2]};
		const auto perpendicular = [&] (const Axis & axis) {
			const auto along = dot(axis, toward);

			return Axis{axis[0] - (along * toward[0]), axis[1] - (along * toward[1]), axis[2] - (along * toward[2])};
		};

		auto north = perpendicular(toAxis(geometry.pole));

		if ( dot(north, north) < 1e-8 )
		{
			north = perpendicular(std::abs(toward[1]) < 0.9 ? Axis{0.0, 1.0, 0.0} : Axis{1.0, 0.0, 0.0});
		}

		const auto length = std::sqrt(dot(north, north));

		if ( !(length > 0.0) || !std::isfinite(length) )
		{
			return {};
		}

		north = {north[0] / length, north[1] / length, north[2] / length};

		const Axis east{
			(north[1] * toward[2]) - (north[2] * toward[1]),
			(north[2] * toward[0]) - (north[0] * toward[2]),
			(north[0] * toward[1]) - (north[1] * toward[0])
		};
		const auto star = toAxis(geometry.starDirection);

		return {
			.longitude = static_cast< float >(std::atan2(dot(star, east), dot(star, toward))),
			.latitude = static_cast< float >(std::asin(std::clamp(dot(star, north), -1.0, 1.0)))
		};
	}

	float
	MoonSurfacePhotometry::relativeBrightness (const SubSolarPoint & point) const noexcept
	{
		if ( !std::isfinite(point.longitude) || !std::isfinite(point.latitude) )
		{
			return 1.0F;
		}

		const auto step = std::numbers::pi / static_cast< double >(StepsPerHalfTurn);
		/* The longitude wrapped to [−π, π], the latitude clamped to [−π/2, π/2]: both positions lie in the table. */
		const auto longitude = std::remainder(static_cast< double >(point.longitude), 2.0 * std::numbers::pi);
		const auto latitude = std::clamp(static_cast< double >(point.latitude), -0.5 * std::numbers::pi, 0.5 * std::numbers::pi);
		const auto longitudePosition = std::clamp((longitude + std::numbers::pi) / step, 0.0, static_cast< double >(LongitudeCount - 1));
		const auto latitudePosition = std::clamp((latitude + (0.5 * std::numbers::pi)) / step, 0.0, static_cast< double >(LatitudeCount - 1));
		const auto longitudeIndex = std::min(static_cast< size_t >(longitudePosition), LongitudeCount - 2);
		const auto latitudeIndex = std::min(static_cast< size_t >(latitudePosition), LatitudeCount - 2);
		const auto longitudeFraction = longitudePosition - static_cast< double >(longitudeIndex);
		const auto latitudeFraction = latitudePosition - static_cast< double >(latitudeIndex);
		/* Indices proven in range above: longitude < LongitudeCount − 1, latitude < LatitudeCount − 1, each + 1 inside. */
		const std::span table{m_table};
		const auto at = [table] (size_t latitudeAt, size_t longitudeAt) {
			return static_cast< double >(table[(latitudeAt * LongitudeCount) + longitudeAt]);
		};
		const auto south = (at(latitudeIndex, longitudeIndex) * (1.0 - longitudeFraction)) + (at(latitudeIndex, longitudeIndex + 1) * longitudeFraction);
		const auto northward = (at(latitudeIndex + 1, longitudeIndex) * (1.0 - longitudeFraction)) + (at(latitudeIndex + 1, longitudeIndex + 1) * longitudeFraction);

		return static_cast< float >((south * (1.0 - latitudeFraction)) + (northward * latitudeFraction));
	}
}

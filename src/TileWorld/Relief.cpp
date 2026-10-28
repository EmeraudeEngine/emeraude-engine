/*
 * src/TileWorld/Relief.cpp
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

#include "Relief.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

/* Local inclusions. */
#include "Algorithms/PerlinNoise.hpp"
#include "BlobTrait.hpp"
#include "Layout.hpp"
#include "TileWorld/TileInterface.hpp"
#include "TileWorld/Types.hpp"
#include "Tracer.hpp"

namespace EmEn::TileWorld
{
	namespace
	{
		constexpr auto TracerTag{"TileWorld::Relief"};

		/** @brief The largest amplitude generate() accepts, metres. */
		constexpr float MaxAmplitude{1000.0F};

		/** @brief The largest grade a slope rule may state (10 = a 1000 % slope: anything goes). */
		constexpr float MaxGrade{10.0F};

		/** @brief One rule between two corners: |h[a] − h[b]| ≤ bound. */
		struct EdgeConstraint
		{
			size_t a{0};
			size_t b{0};
			float bound{0.0F};
		};

		/** @brief The axis a tile's rule follows in the WORLD frame (a quarter turn swaps the two axes). */
		[[nodiscard]]
		SlopeAxis
		worldAxis (SlopeAxis axis, Rotation rotation) noexcept
		{
			if ( axis == SlopeAxis::None || (static_cast< uint8_t >(rotation) % 2U) == 0 )
			{
				return axis;
			}

			return axis == SlopeAxis::NorthSouth ? SlopeAxis::EastWest : SlopeAxis::NorthSouth;
		}

		/**
		 * @brief The rules of every cell: four edges each (the shared ones twice, the stricter bound wins by itself).
		 * @return std::optional< std::vector< EdgeConstraint > > Nothing when a cell is empty or states an invalid rule
		 * (logged).
		 */
		[[nodiscard]]
		std::optional< std::vector< EdgeConstraint > >
		collectConstraints (const Layout & layout) noexcept
		{
			std::vector< EdgeConstraint > constraints;
			constraints.reserve(static_cast< size_t >(layout.columns()) * layout.rows() * 4U);

			const auto cornerColumns = static_cast< size_t >(layout.columns()) + 1;
			const auto tileSize = layout.tileSize();

			for ( size_t index = 0; index < static_cast< size_t >(layout.columns()) * layout.rows(); ++index )
			{
				const auto column = static_cast< uint32_t >(index % layout.columns());
				const auto row = static_cast< uint32_t >(index / layout.columns());
				const auto * cell = layout.cell(column, row);

				if ( cell == nullptr || cell->tile == nullptr )
				{
					TraceError{TracerTag} << "constrain(): the cell (" << column << ", " << row << ") holds no tile (validate the layout first) !";

					return std::nullopt;
				}

				const auto rule = cell->tile->slopeRule();
				const auto valid = std::isfinite(rule.maxGrade) && std::isfinite(rule.crossGrade) && rule.maxGrade >= 0.0F && rule.crossGrade >= 0.0F && rule.maxGrade <= MaxGrade && rule.crossGrade <= MaxGrade;

				if ( !valid )
				{
					TraceError{TracerTag} << "constrain(): the tile '" << cell->tile->name() << "' states an invalid slope rule (" << rule.maxGrade << ", " << rule.crossGrade << ") !";

					return std::nullopt;
				}

				const auto northWest = (static_cast< size_t >(row) * cornerColumns) + column;
				const auto northEast = northWest + 1;
				const auto southWest = northWest + cornerColumns;
				const auto southEast = southWest + 1;
				const auto axis = worldAxis(rule.axis, cell->rotation);
				/* Along Z, the north and south edges (running along X) are the cross-section, and the reverse. */
				const auto northSouthEdges = (axis == SlopeAxis::NorthSouth ? rule.crossGrade : rule.maxGrade) * tileSize;
				const auto eastWestEdges = (axis == SlopeAxis::EastWest ? rule.crossGrade : rule.maxGrade) * tileSize;

				constraints.push_back({.a = northWest, .b = northEast, .bound = northSouthEdges});
				constraints.push_back({.a = southWest, .b = southEast, .bound = northSouthEdges});
				constraints.push_back({.a = northWest, .b = southWest, .bound = eastWestEdges});
				constraints.push_back({.a = northEast, .b = southEast, .bound = eastWestEdges});
			}

			return constraints;
		}

		/**
		 * @brief One sweep of cyclic projections, in a fixed order: each violated rule moves both corners by half the
		 * excess.
		 * @return float The largest excess met (0 when every rule held).
		 */
		[[nodiscard]]
		float
		projectionSweep (const std::vector< EdgeConstraint > & constraints, std::vector< float > & heights) noexcept
		{
			float worst = 0.0F;

			for ( const auto & constraint : constraints )
			{
				auto & heightA = *std::next(heights.begin(), static_cast< std::ptrdiff_t >(constraint.a));
				auto & heightB = *std::next(heights.begin(), static_cast< std::ptrdiff_t >(constraint.b));
				const auto difference = heightA - heightB;
				const auto excess = std::abs(difference) - constraint.bound;

				if ( excess <= 0.0F )
				{
					continue;
				}

				worst = std::max(worst, excess);

				const auto shift = difference > 0.0F ? excess * 0.5F : -excess * 0.5F;
				heightA -= shift;
				heightB += shift;
			}

			return worst;
		}
	}

	bool
	Relief::generate (const Layout & layout, const ReliefParameters & parameters) noexcept
	{
		if ( layout.columns() == 0 || layout.rows() == 0 )
		{
			TraceError{TracerTag} << "generate(): the layout is empty !";

			return false;
		}

		if ( !std::isfinite(parameters.amplitude) || parameters.amplitude < 0.0F || parameters.amplitude > MaxAmplitude || !std::isfinite(parameters.wavelength) || parameters.wavelength <= 0.0F )
		{
			TraceError{TracerTag} << "generate(): refused (amplitude " << parameters.amplitude << " out of 0 to " << MaxAmplitude << ", or wavelength " << parameters.wavelength << " not > 0) !";

			return false;
		}

		m_cornerColumns = layout.columns() + 1;
		m_cornerRows = layout.rows() + 1;
		m_heights.assign(static_cast< size_t >(m_cornerColumns) * m_cornerRows, 0.0F);

		if ( parameters.amplitude == 0.0F )
		{
			return true;
		}

		Base::Algorithms::PerlinNoise< float > noise{parameters.seed};
		const auto tileSize = layout.tileSize();
		const auto halfColumns = static_cast< float >(layout.columns()) * 0.5F;
		const auto halfRows = static_cast< float >(layout.rows()) * 0.5F;

		for ( uint32_t row = 0; row < m_cornerRows; ++row )
		{
			for ( uint32_t column = 0; column < m_cornerColumns; ++column )
			{
				const auto x = ((static_cast< float >(column) - halfColumns) * tileSize) / parameters.wavelength;
				const auto z = ((static_cast< float >(row) - halfRows) * tileSize) / parameters.wavelength;

				/* Two octaves, weights 2/3 and 1/3; the third coordinate off the lattice (Perlin is 0.5 on it). */
				const auto value = ((2.0F * noise.generate(x, z, 0.37F)) + noise.generate(2.0F * x, 2.0F * z, 0.71F)) / 3.0F;

				m_heights[(static_cast< size_t >(row) * m_cornerColumns) + column] = parameters.amplitude * ((2.0F * value) - 1.0F);
			}
		}

		return true;
	}

	bool
	Relief::flatten (const Layout & layout, float height) noexcept
	{
		if ( layout.columns() == 0 || layout.rows() == 0 || !std::isfinite(height) )
		{
			TraceError{TracerTag} << "flatten(): the layout is empty or the height is not finite !";

			return false;
		}

		m_cornerColumns = layout.columns() + 1;
		m_cornerRows = layout.rows() + 1;
		m_heights.assign(static_cast< size_t >(m_cornerColumns) * m_cornerRows, height);

		return true;
	}

	ReliefReport
	Relief::constrain (const Layout & layout) noexcept
	{
		ReliefReport report{};
		report.largestViolation = std::numeric_limits< float >::infinity();

		if ( m_cornerColumns != layout.columns() + 1 || m_cornerRows != layout.rows() + 1 || m_heights.empty() )
		{
			TraceError{TracerTag} << "constrain(): the relief is not sized for this layout !";

			return report;
		}

		const auto constraints = collectConstraints(layout);

		if ( !constraints.has_value() )
		{
			return report;
		}

		for ( uint32_t sweep = 0; sweep < MaxSweeps; ++sweep )
		{
			report.sweeps = sweep + 1;
			report.largestViolation = projectionSweep(constraints.value(), m_heights);

			if ( report.largestViolation <= Tolerance )
			{
				report.converged = true;

				break;
			}
		}

		return report;
	}

	std::optional< ReliefReport >
	Relief::solve (const Layout & layout, const ReliefParameters & parameters) noexcept
	{
		auto attempt = parameters;
		uint32_t attempts = 0;

		for ( uint32_t retry = 0; retry <= MaxRetries; ++retry )
		{
			if ( !this->generate(layout, attempt) )
			{
				return std::nullopt;
			}

			auto report = this->constrain(layout);
			++attempts;

			report.amplitude = attempt.amplitude;
			report.attempts = attempts;

			if ( report.converged )
			{
				return report;
			}

			TraceWarning{TracerTag} << "solve(): amplitude " << attempt.amplitude << " m left a " << report.largestViolation << " m violation after " << report.sweeps << " sweeps.";

			attempt.amplitude *= 0.5F;
		}

		if ( !this->flatten(layout) )
		{
			return std::nullopt;
		}

		auto report = this->constrain(layout);
		report.amplitude = 0.0F;
		report.attempts = attempts + 1;

		TraceWarning{TracerTag} << "solve(): no amplitude met the slope rules after " << MaxRetries << " halvings; the relief is flat.";

		return report;
	}

	bool
	Relief::setHeight (uint32_t cornerColumn, uint32_t cornerRow, float height) noexcept
	{
		if ( cornerColumn >= m_cornerColumns || cornerRow >= m_cornerRows || !std::isfinite(height) )
		{
			TraceError{TracerTag} << "setHeight(" << cornerColumn << ", " << cornerRow << ", " << height << "): refused (outside the lattice or not finite) !";

			return false;
		}

		m_heights[(static_cast< size_t >(cornerRow) * m_cornerColumns) + cornerColumn] = height;

		return true;
	}

	float
	Relief::height (uint32_t cornerColumn, uint32_t cornerRow) const noexcept
	{
		if ( cornerColumn >= m_cornerColumns || cornerRow >= m_cornerRows )
		{
			return 0.0F;
		}

		return m_heights[(static_cast< size_t >(cornerRow) * m_cornerColumns) + cornerColumn];
	}

	CornerHeights
	Relief::cellHeights (uint32_t column, uint32_t row) const noexcept
	{
		return {
			this->height(column, row),
			this->height(column + 1, row),
			this->height(column + 1, row + 1),
			this->height(column, row + 1)
		};
	}

	std::optional< float >
	Relief::surfaceHeight (const Layout & layout, const Base::Math::Vector< 3, float > & position) const noexcept
	{
		const auto x = position[Base::Math::X];
		const auto z = position[Base::Math::Z];

		if ( m_cornerColumns != layout.columns() + 1 || m_cornerRows != layout.rows() + 1 )
		{
			return std::nullopt;
		}

		const auto tileSize = layout.tileSize();
		/* The grid is centred on the origin: the cell coordinates of the point, fractional. */
		const auto column = (x / tileSize) + (static_cast< float >(layout.columns()) * 0.5F);
		const auto row = (z / tileSize) + (static_cast< float >(layout.rows()) * 0.5F);

		if ( !std::isfinite(column) || !std::isfinite(row) || column < 0.0F || row < 0.0F || column >= static_cast< float >(layout.columns()) || row >= static_cast< float >(layout.rows()) )
		{
			return std::nullopt;
		}

		const auto cellColumn = static_cast< uint32_t >(column);
		const auto cellRow = static_cast< uint32_t >(row);
		const auto u = column - static_cast< float >(cellColumn);
		const auto v = row - static_cast< float >(cellRow);
		const auto [northWest, northEast, southEast, southWest] = this->cellHeights(cellColumn, cellRow);

		return (northWest * (1.0F - u) * (1.0F - v)) + (northEast * u * (1.0F - v)) + (southEast * u * v) + (southWest * (1.0F - u) * v);
	}

	std::pair< float, float >
	Relief::range () const noexcept
	{
		if ( m_heights.empty() )
		{
			return {0.0F, 0.0F};
		}

		const auto [lowest, highest] = std::ranges::minmax_element(m_heights);

		return {*lowest, *highest};
	}
}

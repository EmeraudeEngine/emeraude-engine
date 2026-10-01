/*
 * src/Scenes/GroundTriangles.hpp
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

/* STL inclusions. */
#include <cstddef>
#include <cstdint>

/* Local inclusions for usages. */
#include "Math/Space3D/AACuboid.hpp"
#include "Math/Space3D/Triangle.hpp"
#include "VertexFactory/Grid.hpp"
#include "GroundLevelInterface.hpp"

namespace EmEn::Scenes
{
	/**
	 * @brief Visits the triangles of a ground grid under a world region (the shared body of the grounds'
	 * GroundLevelInterface::visitTriangles()).
	 * @note The feature id is (cellZ × cells per side + cellX) × 2 + half: stable while the grid does not change.
	 * @param grid A reference to the ground's grid (its rendered triangulation, Grid::forEachTriangleInRegion()).
	 * @param worldRegion A reference to the region; only its X and Z extents count.
	 * @param visitor A reference to the visitor.
	 * @return size_t The number of triangles visited.
	 */
	inline
	size_t
	visitGridTriangles (const Base::VertexFactory::Grid< float > & grid, const Base::Math::Space3D::AACuboid< float > & worldRegion, GroundTriangleVisitor & visitor) noexcept
	{
		const auto & minimum = worldRegion.minimum();
		const auto & maximum = worldRegion.maximum();
		const auto cellsPerSide = static_cast< uint32_t >(grid.squaredQuadCount());

		return grid.forEachTriangleInRegion(minimum[Base::Math::X], minimum[Base::Math::Z], maximum[Base::Math::X], maximum[Base::Math::Z], [&visitor, cellsPerSide] (const auto & pointA, const auto & pointB, const auto & pointC, auto cellX, auto cellZ, uint8_t half) {
			const auto featureId = (((static_cast< uint32_t >(cellZ) * cellsPerSide) + static_cast< uint32_t >(cellX)) * 2U) + half;

			visitor.onTriangle(Base::Math::Space3D::Triangle< float >{pointA, pointB, pointC}, featureId);
		});
	}
}

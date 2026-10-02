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

	/**
	 * @brief Answers the height of a ground triangle's surface over a world X/Z, when that point lies in its footprint.
	 * @note The exact surface the physics collides, unlike GroundLevelInterface::getLevelAt() (bilinear over a cell:
	 * several metres off on a steep 1 m cell).
	 * @param triangle A reference to the triangle.
	 * @param positionX The world X.
	 * @param positionZ The world Z.
	 * @param height A reference to the height written.
	 * @return bool False when the point is outside the footprint or the triangle is vertical.
	 */
	[[nodiscard]]
	inline
	bool
	surfaceHeightOver (const Base::Math::Space3D::Triangle< float > & triangle, float positionX, float positionZ, float & height) noexcept
	{
		using Base::Math::X;
		using Base::Math::Y;
		using Base::Math::Z;

		const auto & pointA = triangle.pointA();
		const auto & pointB = triangle.pointB();
		const auto & pointC = triangle.pointC();

		const auto side = [positionX, positionZ] (const Base::Math::Vector< 3, float > & from, const Base::Math::Vector< 3, float > & to) {
			return ((to[X] - from[X]) * (positionZ - from[Z])) - ((to[Z] - from[Z]) * (positionX - from[X]));
		};

		const auto sideAB = side(pointA, pointB);
		const auto sideBC = side(pointB, pointC);
		const auto sideCA = side(pointC, pointA);

		/* Inside for either winding. */
		const bool inside = (sideAB >= 0.0F && sideBC >= 0.0F && sideCA >= 0.0F) || (sideAB <= 0.0F && sideBC <= 0.0F && sideCA <= 0.0F);

		if ( !inside )
		{
			return false;
		}

		const auto normal = Base::Math::Vector< 3, float >::crossProduct(pointB - pointA, pointC - pointA);

		if ( normal[Y] == 0.0F )
		{
			return false;
		}

		height = pointA[Y] - (((normal[X] * (positionX - pointA[X])) + (normal[Z] * (positionZ - pointA[Z]))) / normal[Y]);

		return true;
	}
}


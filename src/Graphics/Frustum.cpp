/*
 * src/Graphics/Frustum.cpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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

#include "Frustum.hpp"

/* Local inclusions. */
#include "Libs/Math/Matrix.hpp"

namespace EmEn::Graphics
{
	using namespace Libs;
	using namespace Libs::Math;

	void
	Frustum::update (const Matrix< 4, float > & viewProjectionMatrix) noexcept
	{
		// Extract frustum planes from view-projection matrix using Gribb-Hartmann method.
		// The matrix is in column-major format (OpenGL style).
		// Each plane equation is: ax + by + cz + d = 0

		// Left plane: row3 + row0
		{
			const auto a = viewProjectionMatrix[3] + viewProjectionMatrix[0];
			const auto b = viewProjectionMatrix[7] + viewProjectionMatrix[4];
			const auto c = viewProjectionMatrix[11] + viewProjectionMatrix[8];
			const auto d = viewProjectionMatrix[15] + viewProjectionMatrix[12];
			const auto length = std::sqrt(a * a + b * b + c * c);
			m_planes[Left] = Plane< float >(Vector< 3, float >(a / length, b / length, c / length), d / length);
		}

		// Right plane: row3 - row0
		{
			const auto a = viewProjectionMatrix[3] - viewProjectionMatrix[0];
			const auto b = viewProjectionMatrix[7] - viewProjectionMatrix[4];
			const auto c = viewProjectionMatrix[11] - viewProjectionMatrix[8];
			const auto d = viewProjectionMatrix[15] - viewProjectionMatrix[12];
			const auto length = std::sqrt(a * a + b * b + c * c);
			m_planes[Right] = Plane< float >(Vector< 3, float >(a / length, b / length, c / length), d / length);
		}

		// Bottom plane: row3 + row1
		{
			const auto a = viewProjectionMatrix[3] + viewProjectionMatrix[1];
			const auto b = viewProjectionMatrix[7] + viewProjectionMatrix[5];
			const auto c = viewProjectionMatrix[11] + viewProjectionMatrix[9];
			const auto d = viewProjectionMatrix[15] + viewProjectionMatrix[13];
			const auto length = std::sqrt(a * a + b * b + c * c);
			m_planes[Bottom] = Plane< float >(Vector< 3, float >(a / length, b / length, c / length), d / length);
		}

		// Top plane: row3 - row1
		{
			const auto a = viewProjectionMatrix[3] - viewProjectionMatrix[1];
			const auto b = viewProjectionMatrix[7] - viewProjectionMatrix[5];
			const auto c = viewProjectionMatrix[11] - viewProjectionMatrix[9];
			const auto d = viewProjectionMatrix[15] - viewProjectionMatrix[13];
			const auto length = std::sqrt(a * a + b * b + c * c);
			m_planes[Top] = Plane< float >(Vector< 3, float >(a / length, b / length, c / length), d / length);
		}

		// Near plane: row3 + row2
		{
			const auto a = viewProjectionMatrix[3] + viewProjectionMatrix[2];
			const auto b = viewProjectionMatrix[7] + viewProjectionMatrix[6];
			const auto c = viewProjectionMatrix[11] + viewProjectionMatrix[10];
			const auto d = viewProjectionMatrix[15] + viewProjectionMatrix[14];
			const auto length = std::sqrt(a * a + b * b + c * c);
			m_planes[Near] = Plane< float >(Vector< 3, float >(a / length, b / length, c / length), d / length);
		}

		// Far plane: row3 - row2
		{
			const auto a = viewProjectionMatrix[3] - viewProjectionMatrix[2];
			const auto b = viewProjectionMatrix[7] - viewProjectionMatrix[6];
			const auto c = viewProjectionMatrix[11] - viewProjectionMatrix[10];
			const auto d = viewProjectionMatrix[15] - viewProjectionMatrix[14];
			const auto length = std::sqrt(a * a + b * b + c * c);
			m_planes[Far] = Plane< float >(Vector< 3, float >(a / length, b / length, c / length), d / length);
		}
	}

	bool
	Frustum::isSeeing (const Vector< 3, float > & point) const noexcept
	{
		// A point is visible if it's on the positive side of all frustum planes.
		// The signed distance is positive when the point is on the side of the normal.
		for ( const auto & plane : m_planes )
		{
			if ( plane.getSignedDistanceTo(point) < 0.0F )
			{
				return false;
			}
		}

		return true;
	}

	bool
	Frustum::isSeeing (const Space3D::Sphere< float > & sphere) const noexcept
	{
		// A sphere is visible if its center is within radius distance from all planes.
		// For each plane, we check if: signedDistance(center) >= -radius
		// This means the sphere intersects or is inside the frustum.
		for ( const auto & plane : m_planes )
		{
			if ( plane.getSignedDistanceTo(sphere.position()) < -sphere.radius() )
			{
				return false;
			}
		}

		return true;
	}

	bool
	Frustum::isSeeing (const Space3D::AACuboid< float > & aabb) const noexcept
	{
		// For each plane, we test the "p-vertex" (the corner closest to the plane).
		// The p-vertex is chosen based on the plane's normal direction:
		// - If normal component is positive, use maximum; otherwise use minimum.
		// If the p-vertex is outside (negative side), the whole AABB is outside.
		for ( const auto & plane : m_planes )
		{
			// Compute the p-vertex (positive vertex) based on plane normal
			Vector< 3, float > pVertex;
			pVertex[X] = plane.normal()[X] >= 0.0F ? aabb.maximum()[X] : aabb.minimum()[X];
			pVertex[Y] = plane.normal()[Y] >= 0.0F ? aabb.maximum()[Y] : aabb.minimum()[Y];
			pVertex[Z] = plane.normal()[Z] >= 0.0F ? aabb.maximum()[Z] : aabb.minimum()[Z];

			// If the p-vertex is on the negative side of the plane, the AABB is outside
			if ( plane.getSignedDistanceTo(pVertex) < 0.0F )
			{
				return false;
			}
		}

		return true;
	}
}

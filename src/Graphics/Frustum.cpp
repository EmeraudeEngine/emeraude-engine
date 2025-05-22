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

/* STL inclusions. */
#include <ostream>
#include <string>

/* Local inclusions. */
#include "Libs/Math/Matrix.hpp"

namespace EmEn::Graphics
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;

	bool Frustum::s_enableFrustumTest{true};

	void
	Frustum::updateProperties (float /*farDistance*/) noexcept
	{
		/* FIXME: Empty function */
	}

	void
	Frustum::update (const Matrix< 4, float > & viewProjectionMatrix) noexcept
	{
	    const auto rowA = viewProjectionMatrix.row(0); // X
	    const auto rowB = viewProjectionMatrix.row(1); // Y
	    const auto rowC = viewProjectionMatrix.row(2); // Z
	    const auto rowD = viewProjectionMatrix.row(3); // W

	    auto createPlane = [] (const Vector< 4, float > & planeVec) -> Plane< float >
		{
	        const auto normal3D = planeVec.toVector3();
	        const auto magnitude = normal3D.length();

	    	if ( magnitude < std::numeric_limits< float >::epsilon() * 100.0F )
	    	{
	            return {};
	        }

	        const auto normalizedNormal = normal3D / magnitude;
	        const auto normalizedDistance = planeVec[3] / magnitude;

	        return {normalizedNormal, normalizedDistance};
	    };

	    // Left Plane (w' + x' >= 0)
	    m_planes[Left] = createPlane(rowD + rowA);

	    // Right Plane (w' - x' >= 0)
	    m_planes[Right] = createPlane(rowD - rowA);

	    // Bottom Plane (w' - y' >= 0) --- +Y = Down
	    m_planes[Bottom] = createPlane(rowD - rowB);

	    // Top Plane (w' + y' >= 0) --- +Y = Down
	    m_planes[Top] = createPlane(rowD + rowB);

	    // Near Plane (z' >= 0) --- Z in [0, 1]
	    m_planes[Near] = createPlane(rowC);

	    // Far Plane (w' - z' >= 0) --- Z in [0, 1]
	    m_planes[Far] = createPlane(rowD - rowC);
	}

	Frustum::Result
	Frustum::isCollidingWith (const Vector< 3, float > & point) const noexcept
	{
		if ( !s_enableFrustumTest )
		{
			return Result::Inside;
		}

		for ( const auto & plane : m_planes )
		{
			const auto & normal = plane.normal();

			/* Dot product */
			const auto coordX = normal[X] * point[X];
			const auto coordY = normal[Y] * point[Y];
			const auto coordZ = normal[Z] * point[Z];

			if ( coordX + coordY + coordZ + plane.distance() <= 0.0F )
			{
				return Result::Outside;
			}
		}

		return Result::Inside;
	}

	Frustum::Result
	Frustum::isCollidingWith (const Space3D::Sphere< float > & sphere) const noexcept
	{
		if ( !s_enableFrustumTest )
		{
			return Result::Inside;
		}

		const auto & position = sphere.position();

		const auto radius = sphere.radius();

		for ( const auto & plane : m_planes )
		{
			const auto & normal = plane.normal();

			/* Dot product */
			const auto coordX = normal[X] * position[X];
			const auto coordY = normal[Y] * position[Y];
			const auto coordZ = normal[Z] * position[Z];

			const auto distance = coordX + coordY + coordZ + plane.distance();

			if ( distance < -radius )
			{
				return Result::Outside;
			}

			if ( distance < radius )
			{
				return Result::Intersect;
			}
		}

		return Result::Inside;
	}

	Frustum::Result
	Frustum::isCollidingWith (const Space3D::AACuboid< float > & box) const noexcept
	{
		if ( !s_enableFrustumTest )
		{
			return Result::Inside;
		}

		const auto & max = box.maximum();
		const auto & min = box.minimum();

		/* centroid */
		{
			const auto midpoint = Vector< 3, float >::midPoint(max, min);

			if ( this->isCollidingWith(Vector< 3, float >(midpoint[X], midpoint[Y], midpoint[Z])) == Result::Inside )
			{
				return Result::Intersect;
			}
		}

		if ( this->isCollidingWith(Vector< 3, float >(max[X], max[Y], max[Z])) == Result::Inside )
		{
			return Result::Intersect;
		}

		if ( this->isCollidingWith(Vector< 3, float >(max[X], max[Y], min[Z])) == Result::Inside )
		{
			return Result::Intersect;
		}

		if ( this->isCollidingWith(Vector< 3, float >(max[X], min[Y], max[Z])) == Result::Inside )
		{
			return Result::Intersect;
		}

		if ( this->isCollidingWith(Vector< 3, float >(max[X], min[Y], min[Z])) == Result::Inside )
		{
			return Result::Intersect;
		}

		if ( this->isCollidingWith(Vector< 3, float >(min[X], max[Y], max[Z])) == Result::Inside )
		{
			return Result::Intersect;
		}

		if ( this->isCollidingWith(Vector< 3, float >(min[X], max[Y], min[Z])) == Result::Inside )
		{
			return Result::Intersect;
		}

		if ( this->isCollidingWith(Vector< 3, float >(min[X], min[Y], max[Z])) == Result::Inside )
		{
			return Result::Intersect;
		}

		if ( this->isCollidingWith(Vector< 3, float >(min[X], min[Y], min[Z])) == Result::Inside )
		{
			return Result::Intersect;
		}

		return Result::Outside;
	}

	Frustum::Result
	Frustum::isCollidingWith (float coordX, float coordY, float coordZ, float size) const noexcept
	{
		if ( !s_enableFrustumTest )
		{
			return Result::Inside;
		}

		for ( const auto & plane : m_planes )
		{
			const auto & normal = plane.normal();

			const auto posX = normal[X] * (coordX + size);
			const auto negX = normal[X] * (coordX - size);

			const auto posY = normal[Y] * (coordY + size);
			const auto negY = normal[Y] * (coordY - size);

			const auto posZ = normal[Z] * (coordZ + size);
			const auto negZ = normal[Z] * (coordZ - size);

			if ( negX +  negY +  negZ + plane.distance() > 0.0F )
			{
				continue;
			}

			if ( posX +  negY +  negZ + plane.distance() > 0.0F )
			{
				continue;
			}

			if ( negX +  posY +  negZ + plane.distance() > 0.0F )
			{
				continue;
			}

			if ( posX +  posY +  negZ + plane.distance() > 0.0F )
			{
				continue;
			}

			if ( negX +  negY +  posZ + plane.distance() > 0.0F )
			{
				continue;
			}

			if ( posX +  negY +  posZ + plane.distance() > 0.0F )
			{
				continue;
			}

			if ( negX +  posY +  posZ + plane.distance() > 0.0F )
			{
				continue;
			}

			if ( posX +  posY +  posZ + plane.distance() > 0.0F )
			{
				continue;
			}

			return Result::Outside;
		}

		return Result::Inside;
	}

	std::ostream &
	operator<< (std::ostream & out, const Frustum & obj)
	{
		return out << "Frustum data :" "\n"
			"Right " << obj.m_planes[Frustum::Right] <<
			"Left " << obj.m_planes[Frustum::Left] <<
			"Bottom " << obj.m_planes[Frustum::Bottom] <<
			"Top " << obj.m_planes[Frustum::Top] <<
			"Far " << obj.m_planes[Frustum::Far] <<
			"Near " << obj.m_planes[Frustum::Near];
	}

	std::string
	to_string (const Frustum & obj)
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}

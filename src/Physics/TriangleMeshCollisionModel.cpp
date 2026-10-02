/*
 * src/Physics/TriangleMeshCollisionModel.cpp
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

#include "TriangleMeshCollisionModel.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace EmEn::Physics
{
	using namespace Base::Math;
	using namespace Base::Math::Space3D;

	namespace
	{
		/** @brief Below this absolute scale a frame axis is degenerate (the mesh would be flattened). */
		constexpr float MeshMinimumScale{1.0e-6F};
	}

	TriangleMeshCollisionModel::TriangleMeshCollisionModel (std::shared_ptr< const TriangleMesh< float > > mesh, bool twoSided) noexcept
		: m_mesh{std::move(mesh)},
		m_twoSided{twoSided}
	{
		/* A null or empty mesh would make every query read nothing: keep an empty one, never a null. */
		if ( m_mesh == nullptr )
		{
			m_mesh = std::make_shared< const TriangleMesh< float > >();
		}
	}

	TriangleMeshCollisionModel::~TriangleMeshCollisionModel () = default;

	std::unique_ptr< TriangleMeshCollisionModel >
	TriangleMeshCollisionModel::fromShape (const Base::VertexFactory::Shape< float > & shape, bool twoSided, float activeEdgeCosine) noexcept
	{
		const auto & shapeVertices = shape.vertices();

		std::vector< Vector< 3, float > > positions;
		std::vector< uint32_t > indices;

		positions.reserve(shapeVertices.size());
		indices.reserve(shape.triangles().size() * 3);

		for ( const auto & vertex : shapeVertices )
		{
			positions.push_back(vertex.position());
		}

		for ( const auto & triangle : shape.triangles() )
		{
			const auto a = triangle.vertexIndex(0);
			auto b = triangle.vertexIndex(1);
			auto c = triangle.vertexIndex(2);

			if ( a >= shapeVertices.size() || b >= shapeVertices.size() || c >= shapeVertices.size() )
			{
				return nullptr;
			}

			/* The front face is the one the author's normals point out of. */
			const auto winding = Vector< 3, float >::crossProduct(positions[b] - positions[a], positions[c] - positions[a]);
			const auto authored = shapeVertices[a].normal() + shapeVertices[b].normal() + shapeVertices[c].normal();

			if ( Vector< 3, float >::dotProduct(winding, authored) < 0.0F )
			{
				std::swap(b, c);
			}

			indices.push_back(a);
			indices.push_back(b);
			indices.push_back(c);
		}

		auto mesh = std::make_shared< TriangleMesh< float > >();

		if ( !mesh->build(positions, indices, activeEdgeCosine) )
		{
			return nullptr;
		}

		return std::make_unique< TriangleMeshCollisionModel >(std::move(mesh), twoSided);
	}

	AACuboid< float >
	TriangleMeshCollisionModel::getAABB () const noexcept
	{
		if ( m_mesh->empty() )
		{
			return {};
		}

		const Vector< 3, float > padding{BoundsPadding, BoundsPadding, BoundsPadding};

		return AACuboid< float >{m_mesh->maximum() + padding, m_mesh->minimum() - padding};
	}

	AACuboid< float >
	TriangleMeshCollisionModel::getAABB (const CartesianFrame< float > & worldFrame) const noexcept
	{
		const auto transform = WorldTransform::of(worldFrame);

		if ( m_mesh->empty() || !transform.valid )
		{
			return {};
		}

		const auto & lowest = m_mesh->minimum();
		const auto & highest = m_mesh->maximum();
		Vector< 3, float > minimum;
		Vector< 3, float > maximum;

		for ( uint32_t corner = 0; corner < 8; ++corner )
		{
			const Vector< 3, float > local{(corner & 1U) != 0 ? highest[X] : lowest[X], (corner & 2U) != 0 ? highest[Y] : lowest[Y], (corner & 4U) != 0 ? highest[Z] : lowest[Z]};
			const auto world = transform.toWorld(local);

			for ( size_t axis = 0; axis < 3; ++axis )
			{
				minimum[axis] = corner == 0 ? world[axis] : std::min(minimum[axis], world[axis]);
				maximum[axis] = corner == 0 ? world[axis] : std::max(maximum[axis], world[axis]);
			}
		}

		const Vector< 3, float > padding{BoundsPadding, BoundsPadding, BoundsPadding};

		return AACuboid< float >{maximum + padding, minimum - padding};
	}

	float
	TriangleMeshCollisionModel::getRadius () const noexcept
	{
		if ( m_mesh->empty() )
		{
			return 0.0F;
		}

		return (m_mesh->maximum() - m_mesh->minimum()).length() * 0.5F;
	}

	TriangleMeshCollisionModel::WorldTransform
	TriangleMeshCollisionModel::WorldTransform::of (const CartesianFrame< float > & frame) noexcept
	{
		WorldTransform transform;

		transform.rotation = frame.getRotationMatrix3();
		transform.position = frame.position();
		transform.scaling = frame.scalingFactor();

		for ( size_t axis = 0; axis < 3; ++axis )
		{
			const auto scale = transform.scaling[axis];

			if ( !std::isfinite(scale) || std::abs(scale) < MeshMinimumScale )
			{
				return transform;
			}

			transform.inverseScaling[axis] = 1.0F / scale;
		}

		transform.mirrored = (transform.scaling[X] * transform.scaling[Y] * transform.scaling[Z]) < 0.0F;
		transform.valid = true;

		return transform;
	}

	void
	TriangleMeshCollisionModel::WorldTransform::localBounds (const Vector< 3, float > & worldMinimum, const Vector< 3, float > & worldMaximum, Vector< 3, float > & localMinimum, Vector< 3, float > & localMaximum) const noexcept
	{
		/* The rotation is orthonormal: its transpose takes a world offset back. */
		auto inverseRotation = rotation;

		inverseRotation.transpose();

		for ( uint32_t corner = 0; corner < 8; ++corner )
		{
			const Vector< 3, float > world{(corner & 1U) != 0 ? worldMaximum[X] : worldMinimum[X], (corner & 2U) != 0 ? worldMaximum[Y] : worldMinimum[Y], (corner & 4U) != 0 ? worldMaximum[Z] : worldMinimum[Z]};
			const auto unrotated = inverseRotation * (world - position);
			const Vector< 3, float > local{unrotated[X] * inverseScaling[X], unrotated[Y] * inverseScaling[Y], unrotated[Z] * inverseScaling[Z]};

			for ( size_t axis = 0; axis < 3; ++axis )
			{
				localMinimum[axis] = corner == 0 ? local[axis] : std::min(localMinimum[axis], local[axis]);
				localMaximum[axis] = corner == 0 ? local[axis] : std::max(localMaximum[axis], local[axis]);
			}
		}
	}
}

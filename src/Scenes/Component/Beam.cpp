/*
 * src/Scenes/Component/Beam.cpp
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

#include "Beam.hpp"

/* STL inclusions. */
#include <algorithm>
#include <atomic>
#include <cmath>

/* Local inclusions. */
#include "Constants.hpp"
#include "Graphics/Geometry/ResourceGenerator.hpp"
#include "Graphics/RasterizationOptions.hpp"
#include "Graphics/Renderable/MeshResource.hpp"
#include "Resources/Manager.hpp"
#include "Scenes/AbstractEntity.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes::Component
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Graphics;

	namespace
	{
		/** @brief Numbers every beam: each one owns its material, so each one needs its own resource names. */
		std::atomic< uint32_t > s_beamCount{0};
	}

	Beam::Beam (const std::string & componentName, const AbstractEntity & parentEntity, Resources::Manager & resources, uint32_t segmentCount) noexcept
		: Abstract{componentName, parentEntity},
		m_segmentCount{std::max(segmentCount, 1U)}
	{
		const auto resourceName = "Beam#" + std::to_string(s_beamCount.fetch_add(1)) + '/' + componentName;

		m_material = resources.container< Material::BeamResource >()->getOrCreateResourceSync(resourceName, [] (Material::BeamResource & material) {
			return material.setManualLoadSuccess(true);
		});

		if ( m_material == nullptr )
		{
			TraceError{ClassId} << "Unable to create the material of the beam '" << componentName << "' !";

			return;
		}

		const Geometry::ResourceGenerator generator{resources, Geometry::None};

		const auto strip = generator.beamStrip(m_segmentCount);

		if ( strip == nullptr )
		{
			TraceError{ClassId} << "Unable to create the strip geometry of the beam '" << componentName << "' !";

			return;
		}

		/* The ribbon faces the eye from either side of its strip: no culling. */
		const auto material = m_material;

		const auto renderable = resources.container< Renderable::MeshResource >()->getOrCreateResourceSync(resourceName, [strip, material] (Renderable::MeshResource & meshResource) {
			return meshResource.load(strip, material, RasterizationOptions{PolygonMode::Fill, CullingMode::None});
		});

		if ( renderable == nullptr )
		{
			TraceError{ClassId} << "Unable to create the renderable of the beam '" << componentName << "' !";

			return;
		}

		m_renderableInstance = std::make_shared< RenderableInstance::Unique >(renderable, RenderableInstance::lightingFlags(RenderableInstance::Lighting::Unlit));

		/* An emissive overlay: it adds light, occludes nothing, casts and receives nothing, and stays out of the TLAS
		 * (Material::BeamResource). */
		m_renderableInstance->disableDepthWrite(true);
		m_renderableInstance->disableShadowCasting();
		m_renderableInstance->disableShadowReceiving();
		m_renderableInstance->disableRayTracing();

		/* Every slot starts at the initial placement: nothing renders this instance before it is linked. */
		m_segmentMatrix = this->computeSegmentMatrix();

		for ( uint32_t slot = 0; slot < RenderStateSlotCount; ++slot )
		{
			m_renderableInstance->publishTransformationMatrix(slot, m_segmentMatrix);
		}

		this->updateBounds();
	}

	void
	Beam::setEndTarget (const std::shared_ptr< const AbstractEntity > & target, const Vector< 3, float > & offset) noexcept
	{
		m_endTarget = target;
		m_endTargetOffset = offset;
	}

	void
	Beam::processLogics (const Scene & /*scene*/) noexcept
	{
		if ( m_renderableInstance == nullptr )
		{
			return;
		}

		/* The followed end: the target's point in the world, brought into this entity's space. */
		if ( const auto target = m_endTarget.lock() )
		{
			const auto targetPoint = target->getWorldCoordinates().getModelMatrix() * Vector< 4, float >{m_endTargetOffset[X], m_endTargetOffset[Y], m_endTargetOffset[Z], 1.0F};
			const auto localPoint = this->getWorldCoordinates().getModelMatrix().inverse() * targetPoint;

			m_end = {localPoint[X], localPoint[Y], localPoint[Z]};
		}

		m_segmentMatrix = this->computeSegmentMatrix();

		this->updateBounds();
	}

	void
	Beam::publishStateForRendering (uint32_t writeStateIndex) noexcept
	{
		if ( m_renderableInstance == nullptr )
		{
			return;
		}

		m_renderableInstance->publishTransformationMatrix(writeStateIndex, m_segmentMatrix);
	}

	Matrix< 4, float >
	Beam::computeSegmentMatrix () const noexcept
	{
		const auto direction = m_end - m_start;
		const auto length = direction.length();

		const Vector< 4, float > origin{m_start[X], m_start[Y], m_start[Z], 1.0F};

		/* Collapsed: a zero x column, which the vertex stage draws as nothing (BeamGLSL beamCorner()). */
		if ( !m_enabled || length < 1.0e-6F )
		{
			return {Vector< 4, float >{}, Vector< 4, float >{0.0F, 1.0F, 0.0F, 0.0F}, Vector< 4, float >{0.0F, 0.0F, 1.0F, 0.0F}, origin};
		}

		/* Two unit directions across the beam, the arc's axes. */
		const auto forward = direction / length;
		const auto reference = std::abs(forward[Y]) < 0.99F ? Vector< 3, float >::positiveY() : Vector< 3, float >::positiveX();
		const auto across = Vector< 3, float >::crossProduct(forward, reference).normalized();
		const auto over = Vector< 3, float >::crossProduct(forward, across);

		return {
			Vector< 4, float >{direction[X], direction[Y], direction[Z], 0.0F},
			Vector< 4, float >{across[X], across[Y], across[Z], 0.0F},
			Vector< 4, float >{over[X], over[Y], over[Z], 0.0F},
			origin
		};
	}

	void
	Beam::updateBounds () noexcept
	{
		/* The ribbon never leaves the segment by more than its half width plus the arc amplitude. The one-pixel clamp
		 * of a far beam widens it by a pixel, which the culling of a line segment does not need. */
		const auto margin = m_material != nullptr ? m_material->halfWidth() + m_material->arcAmplitude() : 0.0F;

		const Vector< 3, float > minimum{
			std::min(m_start[X], m_end[X]) - margin,
			std::min(m_start[Y], m_end[Y]) - margin,
			std::min(m_start[Z], m_end[Z]) - margin
		};

		const Vector< 3, float > maximum{
			std::max(m_start[X], m_end[X]) + margin,
			std::max(m_start[Y], m_end[Y]) + margin,
			std::max(m_start[Z], m_end[Z]) + margin
		};

		if ( m_boundingBox.isValid() && m_boundingBox.minimum() == minimum && m_boundingBox.maximum() == maximum )
		{
			return;
		}

		m_boundingBox.set(maximum, minimum);
		m_boundingSphere = Space3D::Sphere< float >{(m_end - m_start).length() * 0.5F + margin, (m_start + m_end) * 0.5F};

		/* The entity refreshes its extents (culling, octree). */
		this->notify(ComponentBoundariesModified);
	}
}

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

/* Local inclusions. */
#include "Graphics/Geometry/PulledVertexResource.hpp"
#include "Graphics/RasterizationOptions.hpp"
#include "Graphics/Renderable/MeshResource.hpp"
#include "Math/CurveTessellation.hpp"
#include "Resources/Manager.hpp"
#include "Saphir/BeamGLSL.hpp"
#include "Scenes/AbstractEntity.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes::Component
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Graphics;

	namespace
	{
		/** @brief Numbers every beam: each one owns its material and its geometry, so each one needs its own names. */
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

		/* No vertex buffer: the ribbon is pulled from the stations (Saphir BeamGLSL). Its capacity grows with them. */
		m_geometry = resources.container< Geometry::PulledVertexResource >()->getOrCreateResourceSync(resourceName, [] (Geometry::PulledVertexResource & geometry) {
			return geometry.load(Topology::TriangleList, 0);
		});

		if ( m_geometry == nullptr )
		{
			TraceError{ClassId} << "Unable to create the geometry of the beam '" << componentName << "' !";

			return;
		}

		/* The ribbon faces the eye from either side: no culling. */
		const auto geometry = m_geometry;
		const auto material = m_material;

		const auto renderable = resources.container< Renderable::MeshResource >()->getOrCreateResourceSync(resourceName, [geometry, material] (Renderable::MeshResource & meshResource) {
			return meshResource.load(geometry, material, RasterizationOptions{PolygonMode::Fill, CullingMode::None});
		});

		if ( renderable == nullptr )
		{
			TraceError{ClassId} << "Unable to create the renderable of the beam '" << componentName << "' !";

			return;
		}

		m_pathPoints = std::make_shared< RenderableInstance::PathPoints >();

		m_renderableInstance = std::make_shared< RenderableInstance::Unique >(renderable, RenderableInstance::lightingFlags(RenderableInstance::Lighting::Unlit));
		/* Before any scene links it: the render thread reads it without a lock. */
		m_renderableInstance->setPathPoints(m_pathPoints);

		/* An emissive overlay: it adds light, occludes nothing, casts and receives nothing, and stays out of the TLAS
		 * (Material::BeamResource). */
		m_renderableInstance->disableDepthWrite(true);
		m_renderableInstance->disableShadowCasting();
		m_renderableInstance->disableShadowReceiving();
		m_renderableInstance->disableRayTracing();

		const std::array< Vector< 3, float >, 2 > straight{Vector< 3, float >{0.0F, 0.0F, 0.0F}, Vector< 3, float >{0.0F, 0.0F, 1.0F}};

		m_curve.setPolyline(std::span< const Vector< 3, float > >{straight});

		this->rebuild();
	}

	void
	Beam::setStart (const Vector< 3, float > & position) noexcept
	{
		m_curve.setFirstPoint(position);

		this->rebuild();
	}

	void
	Beam::setEnd (const Vector< 3, float > & position) noexcept
	{
		m_endTarget.reset();
		m_curve.setLastPoint(position);

		this->rebuild();
	}

	void
	Beam::setEndTarget (const std::shared_ptr< const AbstractEntity > & target, const Vector< 3, float > & offset) noexcept
	{
		m_endTarget = target;
		m_endTargetOffset = offset;
	}

	void
	Beam::setPolyline (std::span< const Vector< 3, float > > points, bool closed) noexcept
	{
		m_curve.setPolyline(points, closed);

		this->rebuild();
	}

	void
	Beam::setBezierPath (const BSpline< 3, float > & path) noexcept
	{
		m_curve.setBezierPath(path);

		this->rebuild();
	}

	void
	Beam::setUniformBSpline (std::span< const Vector< 3, float > > controlPoints, bool closed) noexcept
	{
		m_curve.setUniformBSpline(controlPoints, closed);

		this->rebuild();
	}

	void
	Beam::setCatmullRom (std::span< const Vector< 3, float > > points, bool closed, float alpha) noexcept
	{
		m_curve.setCatmullRom(points, closed, alpha);

		this->rebuild();
	}

	void
	Beam::setTolerance (float tolerance) noexcept
	{
		m_tolerance = std::max(tolerance, 1.0e-5F);

		this->rebuild();
	}

	void
	Beam::setEnabled (bool state) noexcept
	{
		if ( m_enabled == state )
		{
			return;
		}

		m_enabled = state;

		/* A hidden beam publishes no station: nothing is drawn. */
		++m_version;
	}

	void
	Beam::processLogics (const Scene & /*scene*/) noexcept
	{
		if ( m_renderableInstance == nullptr )
		{
			return;
		}

		/* The followed end: the target's point in the world, brought into this entity's space — re-tessellated only when
		 * it moved. */
		if ( const auto target = m_endTarget.lock() )
		{
			const auto targetPoint = target->getWorldCoordinates().getModelMatrix() * Vector< 4, float >{m_endTargetOffset[X], m_endTargetOffset[Y], m_endTargetOffset[Z], 1.0F};
			const auto localPoint = this->getWorldCoordinates().getModelMatrix().inverse() * targetPoint;
			const Vector< 3, float > end{localPoint[X], localPoint[Y], localPoint[Z]};

			if ( !m_curve.points().empty() && m_curve.points().back() != end )
			{
				m_curve.setLastPoint(end);

				this->rebuild();
			}
		}

		/* The width and the arc amplitude are the material's: its setters cannot tell the beam. */
		this->updateBounds();
	}

	void
	Beam::publishStateForRendering (uint32_t writeStateIndex) noexcept
	{
		if ( m_pathPoints == nullptr )
		{
			return;
		}

		const auto slot = writeStateIndex % RenderStateSlotCount;

		/* Only a slot that has not received this version yet: a still beam costs nothing per tick (its arc moves on the
		 * GPU, with the scene clock). */
		if ( m_publishedVersions[slot] == m_version )
		{
			return;
		}

		static const std::vector< Vector< 4, float > > Nothing{};

		/* A beam has no debug overlay. */
		m_pathPoints->publish(slot, m_enabled ? m_stations : Nothing, RenderableInstance::PathPoints::DebugLook{});
		m_publishedVersions[slot] = m_version;
	}

	void
	Beam::rebuild () noexcept
	{
		const auto polyline = m_curve.tessellate(m_tolerance);

		m_stations.clear();
		m_length = 0.0F;

		if ( polyline.size() >= 2 )
		{
			/* Every corner kept, at least m_segmentCount pieces over the whole beam: regular stations for the arc. */
			const auto points = CurveTessellation::subdivided(std::span< const Vector< 3, float > >{polyline}, m_segmentCount);
			const auto normals = CurveTessellation::rotationMinimizingNormals(std::span< const Vector< 3, float > >{points});

			std::vector< float > arcLengths(points.size(), 0.0F);

			for ( size_t index = 1; index < points.size(); ++index )
			{
				arcLengths[index] = arcLengths[index - 1] + (points[index] - points[index - 1]).length();
			}

			m_length = arcLengths.back();

			if ( m_length > 0.0F )
			{
				m_stations.reserve(points.size() * RecordsPerStation);

				for ( size_t index = 0; index < points.size(); ++index )
				{
					/* t is the NORMALIZED arc length: the arc's envelope sin(π t) pins both ends, its noise runs along it. */
					m_stations.emplace_back(points[index][X], points[index][Y], points[index][Z], arcLengths[index] / m_length);
					m_stations.emplace_back(normals[index][X], normals[index][Y], normals[index][Z], 0.0F);
				}

				m_stationsMinimum = points.front();
				m_stationsMaximum = points.front();

				for ( const auto & point : points )
				{
					for ( const auto axis : {X, Y, Z} )
					{
						m_stationsMinimum[axis] = std::min(m_stationsMinimum[axis], point[axis]);
						m_stationsMaximum[axis] = std::max(m_stationsMaximum[axis], point[axis]);
					}
				}

				/* The draw must cover the stations before they are published (the capacity only grows). */
				if ( m_geometry != nullptr )
				{
					m_geometry->setVertexCapacity(static_cast< uint32_t >(points.size() - 1) * Saphir::BeamGLSL::VerticesPerSegment);
				}
			}
		}

		++m_version;

		this->updateBounds();
	}

	void
	Beam::updateBounds () noexcept
	{
		if ( m_stations.empty() )
		{
			return;
		}

		/* The ribbon never leaves its centre line by more than its half width plus the arc amplitude. The one-pixel clamp
		 * of a far beam widens it by a pixel, which the culling does not need. A millimetre keeps a flat beam's box valid. */
		const auto margin = std::max(m_material != nullptr ? m_material->halfWidth() + m_material->arcAmplitude() : 0.0F, 0.001F);
		const Vector< 3, float > padding{margin, margin, margin};
		const auto minimum = m_stationsMinimum - padding;
		const auto maximum = m_stationsMaximum + padding;

		if ( m_boundingBox.isValid() && m_boundingBox.minimum() == minimum && m_boundingBox.maximum() == maximum )
		{
			return;
		}

		m_boundingBox.set(maximum, minimum);
		m_boundingSphere = Space3D::Sphere< float >{(maximum - minimum).length() * 0.5F, (minimum + maximum) * 0.5F};

		/* The entity refreshes its extents (culling, octree). */
		this->notify(ComponentBoundariesModified);
	}
}

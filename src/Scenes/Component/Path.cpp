/*
 * src/Scenes/Component/Path.cpp
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

#include "Path.hpp"

/* STL inclusions. */
#include <algorithm>
#include <atomic>

/* Local inclusions. */
#include "Graphics/Geometry/PulledVertexResource.hpp"
#include "Graphics/RasterizationOptions.hpp"
#include "Graphics/Renderable/MeshResource.hpp"
#include "Resources/Manager.hpp"
#include "Saphir/PathGLSL.hpp"
#include "Scenes/AbstractEntity.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes::Component
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Graphics;

	namespace
	{
		/** @brief Numbers every path: each one owns its material and its geometry, so each one needs its own names. */
		std::atomic< uint32_t > s_pathCount{0};
	}

	Path::Path (const std::string & componentName, const AbstractEntity & parentEntity, Resources::Manager & resources) noexcept
		: Abstract{componentName, parentEntity}
	{
		const auto resourceName = "Path#" + std::to_string(s_pathCount.fetch_add(1)) + '/' + componentName;

		m_material = resources.container< Material::PathResource >()->getOrCreateResourceSync(resourceName, [] (Material::PathResource & material) {
			return material.setManualLoadSuccess(true);
		});

		if ( m_material == nullptr )
		{
			TraceError{ClassId} << "Unable to create the material of the path '" << componentName << "' !";

			return;
		}

		/* No vertex buffer: the ribbon is pulled from the points (Saphir PathGLSL). Its capacity grows with them. */
		m_geometry = resources.container< Geometry::PulledVertexResource >()->getOrCreateResourceSync(resourceName, [] (Geometry::PulledVertexResource & geometry) {
			return geometry.load(Topology::TriangleList, 0);
		});

		if ( m_geometry == nullptr )
		{
			TraceError{ClassId} << "Unable to create the geometry of the path '" << componentName << "' !";

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
			TraceError{ClassId} << "Unable to create the renderable of the path '" << componentName << "' !";

			return;
		}

		m_pathPoints = std::make_shared< RenderableInstance::PathPoints >();

		m_renderableInstance = std::make_shared< RenderableInstance::Unique >(renderable, RenderableInstance::lightingFlags(RenderableInstance::Lighting::Unlit));
		/* Before any scene links it: the render thread reads it without a lock. */
		m_renderableInstance->setPathPoints(m_pathPoints);

		/* A drawing, not a shading surface: no shadow either way, out of the TLAS (no vertex data to build a BLAS). */
		m_renderableInstance->disableShadowCasting();
		m_renderableInstance->disableShadowReceiving();
		m_renderableInstance->disableRayTracing();
	}

	void
	Path::setPolyline (std::span< const Vector< 3, float > > points, bool closed) noexcept
	{
		if ( !this->acceptsFinite("setPolyline", points) )
		{
			return;
		}

		m_curve.setPolyline(points, closed);

		this->rebuild();
	}

	void
	Path::setBezierPath (const BSpline< 3, float > & path) noexcept
	{
		m_curve.setBezierPath(path);

		this->rebuild();
	}

	void
	Path::setUniformBSpline (std::span< const Vector< 3, float > > controlPoints, bool closed) noexcept
	{
		if ( !this->acceptsFinite("setUniformBSpline", controlPoints) )
		{
			return;
		}

		m_curve.setUniformBSpline(controlPoints, closed);

		this->rebuild();
	}

	void
	Path::setCatmullRom (std::span< const Vector< 3, float > > points, bool closed, float alpha) noexcept
	{
		if ( !this->acceptsFinite("setCatmullRom", points, alpha) )
		{
			return;
		}

		m_curve.setCatmullRom(points, closed, alpha);

		this->rebuild();
	}

	void
	Path::setTolerance (float tolerance) noexcept
	{
		if ( !this->acceptsFinite("setTolerance", tolerance) )
		{
			return;
		}

		m_tolerance = std::max(tolerance, 1.0e-5F);

		this->rebuild();
	}

	void
	Path::setEnabled (bool state) noexcept
	{
		if ( m_enabled == state )
		{
			return;
		}

		m_enabled = state;

		/* A hidden path publishes no point: its vertices all collapse. */
		++m_version;
	}

	void
	Path::setDebugMode (bool state) noexcept
	{
		if ( m_debugMode == state )
		{
			return;
		}

		m_debugMode = state;

		++m_version;
	}

	void
	Path::setDebugColor (const PixelFactory::Color< float > & color) noexcept
	{
		if ( !this->acceptsFinite("setDebugColor", color) )
		{
			return;
		}

		m_debugColor = color;

		++m_version;
	}

	void
	Path::rebuild () noexcept
	{
		const auto points = m_curve.tessellate(m_tolerance);

		/* The arc length rides in w (round caps' distances, future dashes). */
		m_polyline.clear();
		m_polyline.reserve(points.size());

		float arcLength = 0.0F;

		for ( size_t index = 0; index < points.size(); ++index )
		{
			if ( index > 0 )
			{
				arcLength += (points[index] - points[index - 1]).length();
			}

			m_polyline.emplace_back(points[index][X], points[index][Y], points[index][Z], arcLength);
		}

		/* The draw must cover the points before they are published (the capacity only grows). */
		if ( m_geometry != nullptr && m_polyline.size() > 1 )
		{
			m_geometry->setVertexCapacity(static_cast< uint32_t >(m_polyline.size() - 1) * Saphir::PathGLSL::VerticesPerSegment);
		}

		++m_version;

		this->updateBounds();
	}

	void
	Path::publishStateForRendering (uint32_t writeStateIndex) noexcept
	{
		if ( m_pathPoints == nullptr )
		{
			return;
		}

		const auto slot = writeStateIndex % RenderStateSlotCount;

		/* Only a slot that has not received this version yet: an unchanged path costs nothing per tick. */
		if ( m_publishedVersions[slot] == m_version )
		{
			return;
		}

		static const std::vector< Vector< 4, float > > Nothing{};

		/* The debug look: the material's width and joins, the debug colour. */
		RenderableInstance::PathPoints::DebugLook debug;
		debug.enabled = m_debugMode;
		debug.color = {m_debugColor.red(), m_debugColor.green(), m_debugColor.blue(), m_debugColor.alpha()};

		if ( m_material != nullptr )
		{
			debug.style = {m_material->halfWidth(), m_material->isWidthInPixels() ? 1.0F : 0.0F, m_material->areJoinsRound() ? 1.0F : 0.0F, m_material->miterLimit()};
		}

		m_pathPoints->publish(slot, m_enabled ? m_polyline : Nothing, debug);
		m_publishedVersions[slot] = m_version;
	}

	void
	Path::updateBounds () noexcept
	{
		if ( m_polyline.empty() )
		{
			return;
		}

		/* A width in entity units widens the bounds; a width in pixels is far below a culling margin. */
		const auto margin = m_material != nullptr && !m_material->isWidthInPixels() ? m_material->halfWidth() : 0.0F;

		Vector< 3, float > minimum{m_polyline.front()[X], m_polyline.front()[Y], m_polyline.front()[Z]};
		Vector< 3, float > maximum = minimum;

		for ( const auto & point : m_polyline )
		{
			for ( const auto axis : {X, Y, Z} )
			{
				minimum[axis] = std::min(minimum[axis], point[axis]);
				maximum[axis] = std::max(maximum[axis], point[axis]);
			}
		}

		/* A flat path (all its points in a plane) would have an empty box: the margin, or a millimetre, keeps it valid. */
		const auto padding = std::max(margin, 0.001F);

		minimum -= Vector< 3, float >{padding, padding, padding};
		maximum += Vector< 3, float >{padding, padding, padding};

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

/*
 * src/Scenes/Editor/Gizmo/Rotate.cpp
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "Rotate.hpp"

/* Local inclusions. */
#include "Graphics/Geometry/Helpers.hpp"
#include "Graphics/Geometry/ResourceGenerator.hpp"
#include "Graphics/Geometry/IndexedVertexResource.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/RenderTarget/Abstract.hpp"
#include "Graphics/ViewMatricesInterface.hpp"
#include "Graphics/Types.hpp"
#include "Resources/Manager.hpp"
#include "Saphir/Generator/GizmoRendering.hpp"
#include "Saphir/Program.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/GraphicsPipeline.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/Framebuffer.hpp"
#include "Libs/Math/Space3D/Intersections/SegmentSphere.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes::Editor::Gizmo
{
	using namespace Libs::Math;
	using namespace Libs::Math::Space3D;
	using namespace Graphics;

	bool
	Rotate::create (Renderer & renderer, Resources::Manager & resourceManager, const std::shared_ptr< const RenderTarget::Abstract > & renderTarget) noexcept
	{
		if ( m_created )
		{
			return true;
		}

		/* NOTE: Three torus rings via ResourceGenerator. Pure flat RGB.
		 * Default torus lies in XZ plane (ring around Y axis).
		 * Rotate to get rings around X and Z axes. */
		{
			Geometry::ResourceGenerator gen{resourceManager, Geometry::EnableVertexColor};

			/* X ring (Red): rotate torus 90° around Z so it lies in YZ plane (ring around X). */
			gen.parameters().setGlobalVertexColor({1.0F, 0.0F, 0.0F, 1.0F});
			gen.parameters().setTransformMatrix(Matrix< 4, float >::rotation(Radian(QuartRevolution< float >), 0.0F, 0.0F, 1.0F));
			m_subGeometries[RingX] = gen.torus(RingMajorRadius, RingMinorRadius, 32, 8, "+GizmoRotateX");

			/* Y ring (Green): no rotation, default XZ plane (ring around Y). */
			gen.parameters().setGlobalVertexColor({0.0F, 1.0F, 0.0F, 1.0F});
			gen.parameters().setTransformMatrix(Matrix< 4, float >::identity());
			m_subGeometries[RingY] = gen.torus(RingMajorRadius, RingMinorRadius, 32, 8, "+GizmoRotateY");

			/* Z ring (Blue): rotate torus 90° around X so it lies in XY plane (ring around Z). */
			gen.parameters().setGlobalVertexColor({0.0F, 0.0F, 1.0F, 1.0F});
			gen.parameters().setTransformMatrix(Matrix< 4, float >::rotation(Radian(QuartRevolution< float >), 1.0F, 0.0F, 0.0F));
			m_subGeometries[RingZ] = gen.torus(RingMajorRadius, RingMinorRadius, 32, 8, "+GizmoRotateZ");
		}

		for ( size_t i = 0; i < SubElementCount; ++i )
		{
			if ( m_subGeometries[i] == nullptr )
			{
				Tracer::error(ClassId, "Failed to create gizmo ring geometry !");

				return false;
			}
		}

		/* NOTE: Generate shader program (reuse same GizmoRendering pipeline). */
		{
			Saphir::Generator::GizmoRendering gizmoGenerator{renderTarget, Topology::TriangleList, Geometry::EnableVertexColor};

			if ( const auto * overlayFB = renderer.overlayFramebuffer(); overlayFB != nullptr )
			{
				gizmoGenerator.setPipelineFramebuffer(overlayFB);
			}

			if ( !gizmoGenerator.generateShaderProgram(renderer) )
			{
				Tracer::error(ClassId, "Failed to generate gizmo shader program !");

				return false;
			}

			m_program = gizmoGenerator.shaderProgram();
		}

		if ( m_program == nullptr || m_program->graphicsPipeline() == nullptr )
		{
			Tracer::error(ClassId, "Gizmo shader program is incomplete !");

			return false;
		}

		m_created = true;

		Tracer::info(ClassId, "Rotation gizmo created.");

		return true;
	}

	void
	Rotate::destroy () noexcept
	{
		for ( auto & subGeometry : m_subGeometries )
		{
			subGeometry.reset();
		}

		Abstract::destroy();
	}

	AxisID
	Rotate::hitTest (const Segment< float > & ray) const noexcept
	{
		if ( !m_created )
		{
			return AxisID::None;
		}

		const auto & pos = m_worldFrame.position();
		const float s = m_screenScale;

		/* NOTE: Hit-test for rings: test intersection with a thin spherical shell.
		 * If the ray hits near the ring radius, it's a hit on that ring.
		 * We use two spheres: outer and inner, and check if the hit is in the shell. */
		const float outerRadius = (RingMajorRadius + RingMinorRadius * 4.0F) * s;
		const float innerRadius = (RingMajorRadius - RingMinorRadius * 4.0F) * s;

		/* NOTE: First check if the ray intersects the outer sphere at all. */
		const Sphere< float > outerSphere{outerRadius, pos};

		if ( !isIntersecting(ray, outerSphere) )
		{
			return AxisID::None;
		}

		/* NOTE: For each ring, check if the hit point is near the ring plane.
		 * A ring around axis A lies in the plane perpendicular to A.
		 * The hit point's distance to the plane (along A) should be small. */
		AxisID closestAxis = AxisID::None;
		float closestPlaneDist = std::numeric_limits< float >::max();

		/* NOTE: Compute intersection point with outer sphere. */
		const auto rayDir = (ray.endPoint() - ray.startPoint()).normalize();
		const auto toCenter = pos - ray.startPoint();
		const float tca = Vector< 3, float >::dotProduct(toCenter, rayDir);
		const auto projection = ray.startPoint() + rayDir * tca;
		const auto hitApprox = projection; /* Approximate hit point on sphere surface. */

		const auto localHit = hitApprox - pos;
		const float distFromCenter = localHit.length();

		/* NOTE: Check if we're in the ring shell (between inner and outer radius). */
		if ( distFromCenter < innerRadius || distFromCenter > outerRadius )
		{
			return AxisID::None;
		}

		/* NOTE: X ring: lies in YZ plane. Distance to plane = |localHit.x|. */
		{
			const float planeDist = std::abs(localHit[0]) / s;

			if ( planeDist < RingMinorRadius * 6.0F && planeDist < closestPlaneDist )
			{
				closestPlaneDist = planeDist;
				closestAxis = AxisID::X;
			}
		}

		/* NOTE: Y ring: lies in XZ plane. Distance to plane = |localHit.y|. */
		{
			const float planeDist = std::abs(localHit[1]) / s;

			if ( planeDist < RingMinorRadius * 6.0F && planeDist < closestPlaneDist )
			{
				closestPlaneDist = planeDist;
				closestAxis = AxisID::Y;
			}
		}

		/* NOTE: Z ring: lies in XY plane. Distance to plane = |localHit.z|. */
		{
			const float planeDist = std::abs(localHit[2]) / s;

			if ( planeDist < RingMinorRadius * 6.0F && planeDist < closestPlaneDist )
			{
				closestPlaneDist = planeDist;
				closestAxis = AxisID::Z;
			}
		}

		return closestAxis;
	}

	void
	Rotate::renderSubElement (const Vulkan::CommandBuffer & commandBuffer, const ViewMatricesInterface & viewMatrices, size_t subElementIndex, const Matrix< 4, float > & axisRotation, bool isHighlighted) const noexcept
	{
		const auto & geometry = m_subGeometries[subElementIndex];

		if ( geometry == nullptr || !geometry->isCreated() )
		{
			return;
		}

		const auto & pipelineLayout = m_program->pipelineLayout();

		commandBuffer.bind(*geometry, 0);

		const float finalScale = isHighlighted ? (m_screenScale * HighlightScale) : m_screenScale;

		const auto rotationMatrix = Matrix< 4, float >::rotation(m_worldFrame.rightVector(), m_worldFrame.downwardVector(), m_worldFrame.backwardVector());

		const auto modelMatrix = Matrix< 4, float >::translation(m_worldFrame.position()) * rotationMatrix * Matrix< 4, float >::scaling(finalScale) * axisRotation;

		const auto & projMatrix = viewMatrices.projectionMatrix();
		const auto & viewMatrix = viewMatrices.viewMatrix(false, 0);
		const auto mvp = projMatrix * viewMatrix * modelMatrix;

		constexpr uint32_t Matrix4Floats{16};
		constexpr uint32_t MatrixBytes{Matrix4Floats * sizeof(float)};

		std::array< float, Matrix4Floats + 2 > buffer{};
		std::memcpy(buffer.data(), mvp.data(), MatrixBytes);
		buffer[Matrix4Floats] = 0.0F;
		buffer[Matrix4Floats + 1] = isHighlighted ? HighlightBrightness : 1.0F;

		vkCmdPushConstants(
			commandBuffer.handle(),
			pipelineLayout->handle(),
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			MatrixBytes + sizeof(float) * 2,
			buffer.data()
		);

		const auto indexRange = geometry->subGeometryRange(0);
		commandBuffer.drawIndexed(indexRange[0], indexRange[1], 1);
	}

	void
	Rotate::render (const Vulkan::CommandBuffer & commandBuffer, const ViewMatricesInterface & viewMatrices) const noexcept
	{
		if ( !m_created || m_program == nullptr )
		{
			return;
		}

		const auto & pipeline = m_program->graphicsPipeline();

		if ( pipeline == nullptr )
		{
			return;
		}

		commandBuffer.bind(*pipeline);

		const bool hlX = (m_highlightedAxis == AxisID::X);
		const bool hlY = (m_highlightedAxis == AxisID::Y);
		const bool hlZ = (m_highlightedAxis == AxisID::Z);

		this->renderSubElement(commandBuffer, viewMatrices, RingX, Matrix< 4, float >::identity(), hlX);
		this->renderSubElement(commandBuffer, viewMatrices, RingY, Matrix< 4, float >::identity(), hlY);
		this->renderSubElement(commandBuffer, viewMatrices, RingZ, Matrix< 4, float >::identity(), hlZ);
	}
}

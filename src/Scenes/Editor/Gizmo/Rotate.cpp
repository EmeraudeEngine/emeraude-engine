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
#include "Libs/Math/Space3D/Intersections/SegmentCuboid.hpp"
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
			m_subGeometries[RingX] = gen.torus(RingMajorRadius, RingMinorRadius, 16, 32, "+GizmoRotateX");

			/* Y ring (Green): no rotation, default XZ plane (ring around Y). */
			gen.parameters().setGlobalVertexColor({0.0F, 1.0F, 0.0F, 1.0F});
			gen.parameters().setTransformMatrix(Matrix< 4, float >::identity());
			m_subGeometries[RingY] = gen.torus(RingMajorRadius, RingMinorRadius, 16, 32, "+GizmoRotateY");

			/* Z ring (Blue): rotate torus 90° around X so it lies in XY plane (ring around Z). */
			gen.parameters().setGlobalVertexColor({0.0F, 0.0F, 1.0F, 1.0F});
			gen.parameters().setTransformMatrix(Matrix< 4, float >::rotation(Radian(QuartRevolution< float >), 1.0F, 0.0F, 0.0F));
			m_subGeometries[RingZ] = gen.torus(RingMajorRadius, RingMinorRadius, 16, 32, "+GizmoRotateZ");
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
		const float r = RingMajorRadius * s;
		const float thickness = s * 0.12F;

		AxisID closestAxis = AxisID::None;
		float closestDistance = std::numeric_limits< float >::max();

		/* NOTE: World-space AABBs for each ring.
		 * Each ring is a flat box: wide on the ring plane, thin on the ring axis. */
		const std::array< std::pair< AACuboid< float >, AxisID >, 3 > rings{{
			/* X ring: lies in YZ plane, thin in X. */
			{AACuboid< float >{
				pos + Vector< 3, float >{thickness, r, r},
				pos + Vector< 3, float >{-thickness, -r, -r}
			}, AxisID::X},
			/* Y ring: lies in XZ plane, thin in Y. */
			{AACuboid< float >{
				pos + Vector< 3, float >{r, thickness, r},
				pos + Vector< 3, float >{-r, -thickness, -r}
			}, AxisID::Y},
			/* Z ring: lies in XY plane, thin in Z. */
			{AACuboid< float >{
				pos + Vector< 3, float >{r, r, thickness},
				pos + Vector< 3, float >{-r, -r, -thickness}
			}, AxisID::Z}
		}};

		for ( const auto & [box, axisId] : rings )
		{
			Point< float > hitPoint;

			if ( isIntersecting(ray, box, hitPoint) )
			{
				const float distance = (hitPoint - ray.startPoint()).length();

				if ( distance < closestDistance )
				{
					closestDistance = distance;
					closestAxis = axisId;
				}
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

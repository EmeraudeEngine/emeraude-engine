/*
 * src/Scenes/Editor/Gizmo/Translate.cpp
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

#include "Translate.hpp"

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
	Translate::create (Renderer & renderer, Resources::Manager & resourceManager, const std::shared_ptr< const RenderTarget::Abstract > & renderTarget) noexcept
	{
		if ( m_created )
		{
			return true;
		}

		/* NOTE: Create individual arrow geometries with per-axis colors. */
		{
			Geometry::ResourceGenerator gen{resourceManager, Geometry::EnableVertexColor};

			/* NOTE: Arrows point in NEGATIVE axis directions to compensate for the
			 * view/projection pipeline inversion. Visually, this makes the arrows
			 * align with the compass reference spheres (R=X+, G=Y+, B=Z+). */
			gen.parameters().setGlobalVertexColor({1.0F, 0.0F, 0.0F, 1.0F});
			m_subGeometries[ArrowX] = gen.arrow(AxisLength, PointTo::NegativeX, "+EditorGizmoArrowX");

			gen.parameters().setGlobalVertexColor({0.0F, 1.0F, 0.0F, 1.0F});
			m_subGeometries[ArrowY] = gen.arrow(AxisLength, PointTo::NegativeY, "+EditorGizmoArrowY");

			gen.parameters().setGlobalVertexColor({0.0F, 0.0F, 1.0F, 1.0F});
			m_subGeometries[ArrowZ] = gen.arrow(AxisLength, PointTo::NegativeZ, "+EditorGizmoArrowZ");

			gen.parameters().setGlobalVertexColor({1.0F, 1.0F, 1.0F, 1.0F});
			m_subGeometries[CenterSphere] = gen.sphere(CenterRadius, 8, 6, "+EditorGizmoCenter");
		}

		for ( size_t i = 0; i < SubElementCount; ++i )
		{
			if ( m_subGeometries[i] == nullptr )
			{
				Tracer::error(ClassId, "Failed to create gizmo sub-geometry !");

				return false;
			}
		}

		/* NOTE: Generate the gizmo shader program and pipeline. */
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

		Tracer::info(ClassId, "Translation gizmo created.");

		return true;
	}

	void
	Translate::destroy () noexcept
	{
		for ( auto & subGeometry : m_subGeometries )
		{
			subGeometry.reset();
		}

		Abstract::destroy();
	}

	AxisID
	Translate::hitTest (const Segment< float > & ray) const noexcept
	{
		if ( !m_created )
		{
			return AxisID::None;
		}

		const auto & pos = m_worldFrame.position();
		const float s = m_screenScale;
		const float r = s * 0.15F;

		AxisID closestAxis = AxisID::None;
		float closestDistance = std::numeric_limits< float >::max();

		/* NOTE: Build world-space AABBs for each axis directly from gizmo position + scale.
		 * Arrows use NegativeX/Y/Z, so they extend in negative directions from center.
		 * No local-space transformation — test the world ray directly. */

		const std::array< std::pair< AACuboid< float >, AxisID >, 3 > axes{{
			/* X axis: extends in -X from gizmo center. */
			{AACuboid< float >{
				pos + Vector< 3, float >{-CenterRadius * s, r, r},
				pos + Vector< 3, float >{-AxisLength * s, -r, -r}
			}, AxisID::X},
			/* Y axis: extends in -Y from gizmo center. */
			{AACuboid< float >{
				pos + Vector< 3, float >{r, -CenterRadius * s, r},
				pos + Vector< 3, float >{-r, -AxisLength * s, -r}
			}, AxisID::Y},
			/* Z axis: extends in -Z from gizmo center. */
			{AACuboid< float >{
				pos + Vector< 3, float >{r, r, -CenterRadius * s},
				pos + Vector< 3, float >{-r, -r, -AxisLength * s}
			}, AxisID::Z}
		}};

		for ( const auto & [box, axisId] : axes )
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
	Translate::renderSubElement (const Vulkan::CommandBuffer & commandBuffer, const ViewMatricesInterface & viewMatrices, size_t subElementIndex, const Matrix< 4, float > & axisRotation, bool isHighlighted) const noexcept
	{
		const auto & geometry = m_subGeometries[subElementIndex];

		if ( geometry == nullptr )
		{
			return;
		}

		const auto & pipelineLayout = m_program->pipelineLayout();

		/* NOTE: Bind the sub-element geometry. */
		commandBuffer.bind(*geometry, 0);

		/* NOTE: Compute model matrix: position + entity rotation (local mode) + scale. */
		const float finalScale = isHighlighted ? (m_screenScale * HighlightScale) : m_screenScale;

		/* NOTE: Use the full rotation from the entity's CartesianFrame.
		 * In World mode, m_worldFrame has identity rotation (set by Manager).
		 * In Local mode, m_worldFrame carries the entity's orientation. */
		const auto rotationMatrix = Matrix< 4, float >::rotation(m_worldFrame.rightVector(), m_worldFrame.downwardVector(), m_worldFrame.backwardVector());

		const auto modelMatrix = Matrix< 4, float >::translation(m_worldFrame.position()) * rotationMatrix * Matrix< 4, float >::scaling(finalScale) * axisRotation;

		/* NOTE: Compute MVP. */
		const auto & projMatrix = viewMatrices.projectionMatrix();
		const auto & viewMatrix = viewMatrices.viewMatrix(false, 0);
		const auto mvp = projMatrix * viewMatrix * modelMatrix;

		/* NOTE: Push MVP + frameIndex + highlightFactor (72 bytes). */
		constexpr uint32_t Matrix4Floats{16};
		constexpr uint32_t MatrixBytes{Matrix4Floats * sizeof(float)};

		std::array< float, Matrix4Floats + 2 > buffer{};
		std::memcpy(buffer.data(), mvp.data(), MatrixBytes);
		buffer[Matrix4Floats] = 0.0F; /* frameIndex */
		buffer[Matrix4Floats + 1] = isHighlighted ? HighlightBrightness : 1.0F; /* highlightFactor */

		vkCmdPushConstants(
			commandBuffer.handle(),
			pipelineLayout->handle(),
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			MatrixBytes + sizeof(float) * 2,
			buffer.data()
		);

		/* NOTE: Draw the sub-element. */
		const auto indexRange = geometry->subGeometryRange(0);
		commandBuffer.drawIndexed(indexRange[0], indexRange[1], 1);
	}

	void
	Translate::render (const Vulkan::CommandBuffer & commandBuffer, const ViewMatricesInterface & viewMatrices) const noexcept
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

		/* NOTE: Bind the gizmo pipeline once for all sub-elements. */
		commandBuffer.bind(*pipeline);

		/* NOTE: Render each sub-element with its own model matrix and highlight state. */
		this->renderSubElement(commandBuffer, viewMatrices, ArrowX, Matrix< 4, float >::identity(), m_highlightedAxis == AxisID::X);
		this->renderSubElement(commandBuffer, viewMatrices, ArrowY, Matrix< 4, float >::identity(), m_highlightedAxis == AxisID::Y);
		this->renderSubElement(commandBuffer, viewMatrices, ArrowZ, Matrix< 4, float >::identity(), m_highlightedAxis == AxisID::Z);
		this->renderSubElement(commandBuffer, viewMatrices, CenterSphere, Matrix< 4, float >::identity(), false);
	}
}

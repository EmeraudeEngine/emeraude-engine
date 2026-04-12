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
#include "Graphics/Geometry/IndexedVertexResource.hpp"
#include "Graphics/Geometry/ResourceGenerator.hpp"
#include "Libs/VertexFactory/ShapeAssembler.hpp"
#include "Libs/VertexFactory/ShapeGenerator.hpp"
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

		/* NOTE: Build arrows via ResourceGenerator (cylinder + cone), no sphere.
		 * Two sub-elements per axis (shaft + tip) for pure flat RGB color.
		 * Default geometry is built along -Y. Tip is offset to end of shaft. */
		{
			Geometry::ResourceGenerator gen{resourceManager, Geometry::EnableVertexColor};

			constexpr float shaftRadius{0.02F};
			constexpr float tipRadius{0.06F};
			constexpr float gap{0.1F};
			constexpr float shaftLength{AxisLength * 0.65F};
			constexpr float tipLength{AxisLength * 0.25F};
			const auto shaftOffset = Matrix< 4, float >::translation(0.0F, -gap, 0.0F);
			const auto tipOffset = Matrix< 4, float >::translation(0.0F, -(gap + shaftLength), 0.0F);

			/* X axis (Red): rotate -90° around Z to point in -X. */
			const auto rotX = Matrix< 4, float >::rotation(Radian(-QuartRevolution< float >), 0.0F, 0.0F, 1.0F);
			gen.parameters().setGlobalVertexColor({1.0F, 0.0F, 0.0F, 1.0F});
			gen.parameters().setTransformMatrix(rotX * shaftOffset);
			m_subGeometries[ShaftX] = gen.cylinder(shaftRadius, shaftRadius, shaftLength, 8, 1, {}, "+GizmoShaftX");
			gen.parameters().setTransformMatrix(rotX * tipOffset);
			m_subGeometries[TipX] = gen.cone(tipRadius, tipLength, 8, 1, {}, "+GizmoTipX");

			/* Y axis (Green): no rotation, default -Y direction. */
			gen.parameters().setGlobalVertexColor({0.0F, 1.0F, 0.0F, 1.0F});
			gen.parameters().setTransformMatrix(shaftOffset);
			m_subGeometries[ShaftY] = gen.cylinder(shaftRadius, shaftRadius, shaftLength, 8, 1, {}, "+GizmoShaftY");
			gen.parameters().setTransformMatrix(tipOffset);
			m_subGeometries[TipY] = gen.cone(tipRadius, tipLength, 8, 1, {}, "+GizmoTipY");

			/* Z axis (Blue): rotate +90° around X to point in -Z. */
			const auto rotZ = Matrix< 4, float >::rotation(Radian(QuartRevolution< float >), 1.0F, 0.0F, 0.0F);
			gen.parameters().setGlobalVertexColor({0.0F, 0.0F, 1.0F, 1.0F});
			gen.parameters().setTransformMatrix(rotZ * shaftOffset);
			m_subGeometries[ShaftZ] = gen.cylinder(shaftRadius, shaftRadius, shaftLength, 8, 1, {}, "+GizmoShaftZ");
			gen.parameters().setTransformMatrix(rotZ * tipOffset);
			m_subGeometries[TipZ] = gen.cone(tipRadius, tipLength, 8, 1, {}, "+GizmoTipZ");
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

		/* NOTE: Render shaft + tip per axis. */
		const bool hlX = (m_highlightedAxis == AxisID::X);
		const bool hlY = (m_highlightedAxis == AxisID::Y);
		const bool hlZ = (m_highlightedAxis == AxisID::Z);

		this->renderSubElement(commandBuffer, viewMatrices, ShaftX, Matrix< 4, float >::identity(), hlX);
		this->renderSubElement(commandBuffer, viewMatrices, TipX, Matrix< 4, float >::identity(), hlX);
		this->renderSubElement(commandBuffer, viewMatrices, ShaftY, Matrix< 4, float >::identity(), hlY);
		this->renderSubElement(commandBuffer, viewMatrices, TipY, Matrix< 4, float >::identity(), hlY);
		this->renderSubElement(commandBuffer, viewMatrices, ShaftZ, Matrix< 4, float >::identity(), hlZ);
		this->renderSubElement(commandBuffer, viewMatrices, TipZ, Matrix< 4, float >::identity(), hlZ);
	}
}

/*
 * src/Scenes/Editor/Gizmo/Scale.cpp
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

#include "Scale.hpp"

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
	Scale::create (Renderer & renderer, Resources::Manager & resourceManager, const std::shared_ptr< const RenderTarget::Abstract > & renderTarget) noexcept
	{
		if ( m_created )
		{
			return true;
		}

		{
			Geometry::ResourceGenerator gen{resourceManager, Geometry::EnableVertexColor};

			const auto shaftOffset = Matrix< 4, float >::translation(0.0F, -Gap, 0.0F);
			const auto cubeOffset = Matrix< 4, float >::translation(0.0F, -(Gap + ShaftLength), 0.0F);

			/* X axis (Red) */
			const auto rotX = Matrix< 4, float >::rotation(Radian(-QuartRevolution< float >), 0.0F, 0.0F, 1.0F);
			gen.parameters().setGlobalVertexColor({1.0F, 0.0F, 0.0F, 1.0F});
			gen.parameters().setTransformMatrix(rotX * shaftOffset);
			m_subGeometries[ShaftX] = gen.cylinder(ShaftRadius, ShaftRadius, ShaftLength, 8, 1, {}, "+GizmoScaleShaftX");
			gen.parameters().setTransformMatrix(rotX * cubeOffset);
			m_subGeometries[CubeX] = gen.cube(EndCubeSize, "+GizmoScaleCubeX");

			/* Y axis (Green) */
			gen.parameters().setGlobalVertexColor({0.0F, 1.0F, 0.0F, 1.0F});
			gen.parameters().setTransformMatrix(shaftOffset);
			m_subGeometries[ShaftY] = gen.cylinder(ShaftRadius, ShaftRadius, ShaftLength, 8, 1, {}, "+GizmoScaleShaftY");
			gen.parameters().setTransformMatrix(cubeOffset);
			m_subGeometries[CubeY] = gen.cube(EndCubeSize, "+GizmoScaleCubeY");

			/* Z axis (Blue) */
			const auto rotZ = Matrix< 4, float >::rotation(Radian(QuartRevolution< float >), 1.0F, 0.0F, 0.0F);
			gen.parameters().setGlobalVertexColor({0.0F, 0.0F, 1.0F, 1.0F});
			gen.parameters().setTransformMatrix(rotZ * shaftOffset);
			m_subGeometries[ShaftZ] = gen.cylinder(ShaftRadius, ShaftRadius, ShaftLength, 8, 1, {}, "+GizmoScaleShaftZ");
			gen.parameters().setTransformMatrix(rotZ * cubeOffset);
			m_subGeometries[CubeZ] = gen.cube(EndCubeSize, "+GizmoScaleCubeZ");

			/* Center cube (Gray) — uniform scale. */
			gen.parameters().setGlobalVertexColor({0.5F, 0.5F, 0.5F, 1.0F});
			gen.parameters().setTransformMatrix(Matrix< 4, float >::identity());
			m_subGeometries[CenterCube] = gen.cube(CenterCubeSize, "+GizmoScaleCenter");
		}

		for ( size_t i = 0; i < SubElementCount; ++i )
		{
			if ( m_subGeometries[i] == nullptr )
			{
				Tracer::error(ClassId, "Failed to create gizmo scale geometry !");

				return false;
			}
		}

		/* NOTE: Generate shader program. */
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

		Tracer::info(ClassId, "Scale gizmo created.");

		return true;
	}

	void
	Scale::destroy () noexcept
	{
		for ( auto & subGeometry : m_subGeometries )
		{
			subGeometry.reset();
		}

		Abstract::destroy();
	}

	AxisID
	Scale::hitTest (const Segment< float > & ray) const noexcept
	{
		if ( !m_created )
		{
			return AxisID::None;
		}

		const auto & pos = m_worldFrame.position();
		const float s = m_screenScale;
		const float r = s * 0.15F;
		const float axisEnd = (Gap + ShaftLength + EndCubeSize) * s;
		const float centerR = Gap * s;

		AxisID closestAxis = AxisID::None;
		float closestDistance = std::numeric_limits< float >::max();

		/* NOTE: Center cube first (uniform scale) — small box at center. */
		{
			const AACuboid< float > centerBox{
				pos + Vector< 3, float >{centerR, centerR, centerR},
				pos + Vector< 3, float >{-centerR, -centerR, -centerR}
			};

			Point< float > hitPoint;

			if ( isIntersecting(ray, centerBox, hitPoint) )
			{
				return AxisID::All;
			}
		}

		/* NOTE: Axis AABBs (same as Translate). */
		const std::array< std::pair< AACuboid< float >, AxisID >, 3 > axes{{
			{AACuboid< float >{
				pos + Vector< 3, float >{-Gap * s, r, r},
				pos + Vector< 3, float >{-axisEnd, -r, -r}
			}, AxisID::X},
			{AACuboid< float >{
				pos + Vector< 3, float >{r, -Gap * s, r},
				pos + Vector< 3, float >{-r, -axisEnd, -r}
			}, AxisID::Y},
			{AACuboid< float >{
				pos + Vector< 3, float >{r, r, -Gap * s},
				pos + Vector< 3, float >{-r, -r, -axisEnd}
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
	Scale::renderSubElement (const Vulkan::CommandBuffer & commandBuffer, const ViewMatricesInterface & viewMatrices, size_t subElementIndex, const Matrix< 4, float > & axisRotation, bool isHighlighted) const noexcept
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
	Scale::render (const Vulkan::CommandBuffer & commandBuffer, const ViewMatricesInterface & viewMatrices) const noexcept
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

		const bool hlAll = (m_highlightedAxis == AxisID::All);
		const bool hlX = (m_highlightedAxis == AxisID::X) || hlAll;
		const bool hlY = (m_highlightedAxis == AxisID::Y) || hlAll;
		const bool hlZ = (m_highlightedAxis == AxisID::Z) || hlAll;

		this->renderSubElement(commandBuffer, viewMatrices, ShaftX, Matrix< 4, float >::identity(), hlX);
		this->renderSubElement(commandBuffer, viewMatrices, CubeX, Matrix< 4, float >::identity(), hlX);
		this->renderSubElement(commandBuffer, viewMatrices, ShaftY, Matrix< 4, float >::identity(), hlY);
		this->renderSubElement(commandBuffer, viewMatrices, CubeY, Matrix< 4, float >::identity(), hlY);
		this->renderSubElement(commandBuffer, viewMatrices, ShaftZ, Matrix< 4, float >::identity(), hlZ);
		this->renderSubElement(commandBuffer, viewMatrices, CubeZ, Matrix< 4, float >::identity(), hlZ);
		this->renderSubElement(commandBuffer, viewMatrices, CenterCube, Matrix< 4, float >::identity(), hlAll);
	}
}

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

		/* NOTE: Create the axis geometry (3 colored arrows + origin sphere). */
		m_geometry = Geometry::ResourceGenerator{resourceManager, Geometry::EnableVertexColor}.axis(AxisLength, "+EditorGizmoTranslate");

		if ( m_geometry == nullptr )
		{
			Tracer::error(ClassId, "Failed to create gizmo geometry !");

			return false;
		}

		/* NOTE: Generate the gizmo shader program and pipeline.
		 * The geometry uses position + vertex color (EnableVertexColor = 1 << 4). */
		{
			Saphir::Generator::GizmoRendering generator{renderTarget, Topology::TriangleList, Geometry::EnableVertexColor};

			/* NOTE: Use the post-process framebuffer for pipeline compatibility
			 * (the gizmo renders in the post-process pass, not the scene pass). */
			if ( const auto * overlayFB = renderer.overlayFramebuffer(); overlayFB != nullptr )
			{
				generator.setPipelineFramebuffer(overlayFB);
			}

			if ( !generator.generateShaderProgram(renderer) )
			{
				Tracer::error(ClassId, "Failed to generate gizmo shader program !");

				return false;
			}

			m_program = generator.shaderProgram();
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

	AxisID
	Translate::hitTest (const Segment< float > & ray) const noexcept
	{
		if ( !m_created )
		{
			return AxisID::None;
		}

		/* NOTE: Transform the ray into gizmo local space by applying the inverse
		 * of the gizmo's world transform and scale. */
		const float invScale = (m_screenScale > 0.001F) ? (1.0F / m_screenScale) : 1.0F;
		const auto & gizmoPos = m_worldFrame.position();

		/* NOTE: Build local-space ray by translating and scaling. */
		const auto localStart = (ray.startPoint() - gizmoPos) * invScale;
		const auto localEnd = (ray.endPoint() - gizmoPos) * invScale;
		const Segment< float > localRay{localStart, localEnd};

		/* NOTE: Test against each axis as an elongated AABB (bounding box around the arrow). */
		const float halfRadius = HitRadius;

		AxisID closestAxis = AxisID::None;
		float closestDistance = std::numeric_limits< float >::max();

		/* X axis: extends along positive X. */
		{
			const AACuboid< float > xBox{
				Vector< 3, float >{0.0F, -halfRadius, -halfRadius},
				Vector< 3, float >{AxisLength, halfRadius, halfRadius}
			};

			Point< float > hitPoint;

			if ( isIntersecting(localRay, xBox, hitPoint) )
			{
				const float distance = hitPoint.length();

				if ( distance < closestDistance )
				{
					closestDistance = distance;
					closestAxis = AxisID::X;
				}
			}
		}

		/* Y axis: extends along positive Y (down in Vulkan convention). */
		{
			const AACuboid< float > yBox{
				Vector< 3, float >{-halfRadius, 0.0F, -halfRadius},
				Vector< 3, float >{halfRadius, AxisLength, halfRadius}
			};

			Point< float > hitPoint;

			if ( isIntersecting(localRay, yBox, hitPoint) )
			{
				const float distance = hitPoint.length();

				if ( distance < closestDistance )
				{
					closestDistance = distance;
					closestAxis = AxisID::Y;
				}
			}
		}

		/* Z axis: extends along positive Z (back in Vulkan convention). */
		{
			const AACuboid< float > zBox{
				Vector< 3, float >{-halfRadius, -halfRadius, 0.0F},
				Vector< 3, float >{halfRadius, halfRadius, AxisLength}
			};

			Point< float > hitPoint;

			if ( isIntersecting(localRay, zBox, hitPoint) )
			{
				const float distance = hitPoint.length();

				if ( distance < closestDistance )
				{
					closestDistance = distance;
					closestAxis = AxisID::Z;
				}
			}
		}

		return closestAxis;
	}

	void
	Translate::render (const Vulkan::CommandBuffer & commandBuffer, const ViewMatricesInterface & viewMatrices) const noexcept
	{
		if ( !m_created || m_program == nullptr || m_geometry == nullptr )
		{
			return;
		}

		const auto & pipeline = m_program->graphicsPipeline();
		const auto & pipelineLayout = m_program->pipelineLayout();

		if ( pipeline == nullptr || pipelineLayout == nullptr )
		{
			return;
		}

		/* NOTE: Bind the gizmo graphics pipeline. */
		commandBuffer.bind(*pipeline);

		/* NOTE: Bind the geometry (VBO + IBO). */
		commandBuffer.bind(*m_geometry, 0);

		/* NOTE: Compute the model matrix: position + uniform scale for constant screen size. */
		const auto modelMatrix = Matrix< 4, float >::translation(m_worldFrame.position()) * Matrix< 4, float >::scaling(m_screenScale);

		/* NOTE: Compute MVP. */
		const auto & projMatrix = viewMatrices.projectionMatrix();
		const auto & viewMatrix = viewMatrices.viewMatrix(false, 0);
		const auto mvp = projMatrix * viewMatrix * modelMatrix;

		/* NOTE: Push MVP + frameIndex (68 bytes). */
		constexpr uint32_t Matrix4Floats{16};
		constexpr uint32_t MatrixBytes{Matrix4Floats * sizeof(float)};

		std::array< float, Matrix4Floats + 1 > buffer{};
		std::memcpy(buffer.data(), mvp.data(), MatrixBytes);
		buffer[Matrix4Floats] = 0.0F; /* frameIndex — not used by gizmo. */

		vkCmdPushConstants(
			commandBuffer.handle(),
			pipelineLayout->handle(),
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			MatrixBytes + sizeof(float),
			buffer.data()
		);

		/* NOTE: Draw the full geometry. */
		const auto indexRange = m_geometry->subGeometryRange(0);
		commandBuffer.drawIndexed(indexRange[0], indexRange[1], 1);
	}
}

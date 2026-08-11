/*
 * src/Scenes/Debug/Compass.cpp
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

#include "Compass.hpp"

/* STL inclusions. */
#include <cstring>

/* Local inclusions. */
#include "Graphics/Geometry/Interface.hpp"
#include "Graphics/Geometry/ResourceGenerator.hpp"
#include "Graphics/Geometry/Types.hpp"
#include "Graphics/RenderTarget/Abstract.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/Types.hpp"
#include "Graphics/ViewMatricesInterface.hpp"
#include "Math/Matrix.hpp"
#include "Math/Vector.hpp"
#include "PixelFactory/Color.hpp"
#include "Resources/Manager.hpp"
#include "Saphir/Generator/GizmoRendering.hpp"
#include "Saphir/Program.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/Framebuffer.hpp"
#include "Vulkan/GraphicsPipeline.hpp"
#include "Vulkan/PipelineLayout.hpp"

namespace EmEn::Scenes::Debug
{
	using namespace Base::Math;
	using namespace Base::PixelFactory;
	using namespace Graphics;

	namespace
	{
		/** @brief One landmark per signed world axis.
		 * @note Bright primaries on the positive axes, their complements on the negative ones:
		 * R=X+, G=Y+, B=Z+, Cyan=X-, Magenta=Y-, Yellow=Z-. Documented in the project AGENTS.md,
		 * so these pairings are a CONTRACT with the reader — never reshuffle them. */
		struct Landmark
		{
			const char * name = nullptr;
			Vector< 3, float > direction;
			Color< float > color;
		};

		constexpr std::array< Landmark, 6 > Landmarks{{
			{.name = "+CompassSphereXPositive", .direction = Vector< 3, float >::positiveX(), .color = Red},
			{.name = "+CompassSphereYPositive", .direction = Vector< 3, float >::positiveY(), .color = Green},
			{.name = "+CompassSphereZPositive", .direction = Vector< 3, float >::positiveZ(), .color = Blue},
			{.name = "+CompassSphereXNegative", .direction = Vector< 3, float >::negativeX(), .color = Cyan},
			{.name = "+CompassSphereYNegative", .direction = Vector< 3, float >::negativeY(), .color = Magenta},
			{.name = "+CompassSphereZNegative", .direction = Vector< 3, float >::negativeZ(), .color = Yellow}
		}};
	}

	bool
	Compass::create (Renderer & renderer, Resources::Manager & resourceManager, const std::shared_ptr< const RenderTarget::Abstract > & renderTarget) noexcept
	{
		if ( m_created )
		{
			return true;
		}

		/* NOTE: One sphere per axis, its color baked into the vertex attributes. No normal
		 * attribute is generated: the compass is never lit, so a normal would be dead weight
		 * in the vertex buffer AND would not match the pipeline's vertex input layout below. */
		{
			Geometry::ResourceGenerator generator{resourceManager, Geometry::EnableVertexColor};

			for ( size_t axisIndex = 0; axisIndex < AxisCount; ++axisIndex )
			{
				const auto & landmark = Landmarks[axisIndex];

				generator.parameters().setGlobalVertexColor(landmark.color);

				m_geometries[axisIndex] = generator.sphere(SphereRadius, SphereSlices, SphereStacks, landmark.name);

				if ( m_geometries[axisIndex] == nullptr )
				{
					Tracer::error(ClassId, "Unable to generate a compass sphere geometry !");

					this->destroy();

					return false;
				}
			}
		}

		/* NOTE: The pipeline is built against the OVERLAY framebuffer, not the scene one: this is
		 * what makes the compass recordable after the post-process chain. Reusing the gizmo
		 * generator is deliberate — same need (unlit vertex-colored debug geometry, no depth test,
		 * no culling), same contract, one shader to maintain. */
		{
			Saphir::Generator::GizmoRendering generator{renderTarget, Topology::TriangleList, Geometry::EnableVertexColor};

			if ( const auto * overlayFramebuffer = renderer.overlayFramebuffer(); overlayFramebuffer != nullptr )
			{
				generator.setPipelineFramebuffer(overlayFramebuffer);
			}

			if ( !generator.generateShaderProgram(renderer) )
			{
				Tracer::error(ClassId, "Unable to generate the compass shader program !");

				this->destroy();

				return false;
			}

			m_program = generator.shaderProgram();
		}

		if ( m_program == nullptr || m_program->graphicsPipeline() == nullptr )
		{
			Tracer::error(ClassId, "The compass shader program is incomplete !");

			this->destroy();

			return false;
		}

		m_created = true;

		Tracer::info(ClassId, "Debug compass created.");

		return true;
	}

	void
	Compass::destroy () noexcept
	{
		for ( auto & geometry : m_geometries )
		{
			geometry.reset();
		}

		m_program.reset();

		m_created = false;
	}

	void
	Compass::render (const Vulkan::CommandBuffer & commandBuffer, const ViewMatricesInterface & viewMatrices) const noexcept
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

		const auto & pipelineLayout = m_program->pipelineLayout();
		const auto & projectionMatrix = viewMatrices.projectionMatrix();

		/* NOTE: INFINITY view (translation dropped): the compass states directions, not places.
		 * With the regular view matrix the spheres would drift out of frame as soon as the camera
		 * leaves the origin, which is precisely when their reading matters. */
		const auto & viewMatrix = viewMatrices.viewMatrix(true, 0);
		const auto viewProjectionMatrix = projectionMatrix * viewMatrix;

		constexpr uint32_t Matrix4Floats{16};
		constexpr uint32_t MatrixBytes{Matrix4Floats * sizeof(float)};

		for ( size_t axisIndex = 0; axisIndex < AxisCount; ++axisIndex )
		{
			const auto & geometry = m_geometries[axisIndex];

			if ( geometry == nullptr || !geometry->isCreated() )
			{
				continue;
			}

			commandBuffer.bind(*geometry, 0);

			const auto modelMatrix = Matrix< 4, float >::translation(Landmarks[axisIndex].direction * AxisDistance);
			const auto modelViewProjectionMatrix = viewProjectionMatrix * modelMatrix;

			/* NOTE: Same push constant block as the gizmos: MVP (64B) + frameIndex (4B) +
			 * highlightFactor (4B). The highlight factor stays at 1.0 so the fragment stage
			 * outputs the authored color untouched — the whole point of this class. */
			std::array< float, Matrix4Floats + 2 > pushConstants{};
			std::memcpy(pushConstants.data(), modelViewProjectionMatrix.data(), MatrixBytes);
			pushConstants[Matrix4Floats] = 0.0F;
			pushConstants[Matrix4Floats + 1] = 1.0F;

			vkCmdPushConstants(
				commandBuffer.handle(),
				pipelineLayout->handle(),
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				MatrixBytes + (sizeof(float) * 2),
				pushConstants.data()
			);

			const auto indexRange = geometry->subGeometryRange(0);

			commandBuffer.drawIndexed(indexRange[0], indexRange[1], 1);
		}
	}
}

/*
 * src/Scenes/Debug/Compass.hpp
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

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

/* Forward declarations. */
namespace EmEn
{
	namespace Resources
	{
		class Manager;
	}

	namespace Graphics
	{
		class Renderer;
		class ViewMatricesInterface;

		namespace Geometry
		{
			class Interface;
		}

		namespace RenderTarget
		{
			class Abstract;
		}
	}

	namespace Vulkan
	{
		class CommandBuffer;
	}

	namespace Saphir
	{
		class Program;
	}
}

namespace EmEn::Scenes::Debug
{
	/**
	 * @brief Orientation compass: six colored spheres marking the world axis directions.
	 *
	 * ⚠️ THIS IS A MEASUREMENT INSTRUMENT, NOT SCENE CONTENT. Its whole purpose is to state
	 * the world axes unambiguously, so its colors must be READ AS AUTHORED — a compass whose
	 * red is darker than its green no longer identifies anything.
	 *
	 * That is why it does NOT live in the scene graph and is NOT drawn in the scene's HDR pass.
	 * The HDR pass is an ABSOLUTE-LUMINANCE buffer: everything written there is multiplied by
	 * the camera exposure in `ToneMapping` (`hdrColor *= exposure`, plus the auto-exposure
	 * factor). An exposure calibrated for a few thousand nits crushes a 1.0 vertex color to
	 * black, so an LDR debug color cannot survive that pass whatever its lighting state — the
	 * former implementation was unlit already and still went dark.
	 *
	 * The compass is therefore recorded AFTER the post-process chain, in the same pass and with
	 * the same contract as the editor gizmos (see `Graphics::Renderer::renderFrame()`): its
	 * pipeline is compiled against `Renderer::overlayFramebuffer()`, depth test and write are
	 * disabled, and no culling is applied. Exposure, bloom, tone mapping, TAA and every other
	 * post effect are therefore structurally unable to reach it.
	 *
	 * The spheres are drawn through the INFINITY view matrix (translation dropped), so the
	 * compass keeps a constant angular size and never drifts with the camera position.
	 */
	class EMEN_API Compass final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"DebugCompass"};

			/**
			 * @brief Constructs a compass. GPU resources are created by create().
			 */
			Compass () noexcept = default;

			/** @brief Deleted copy/move: the instance owns GPU resources. */
			Compass (const Compass &) noexcept = delete;
			Compass (Compass &&) noexcept = delete;
			Compass & operator= (const Compass &) noexcept = delete;
			Compass & operator= (Compass &&) noexcept = delete;

			/**
			 * @brief Destructs the compass, releasing its GPU resources.
			 */
			~Compass () = default;

			/**
			 * @brief Creates the compass GPU resources (geometries, shader program, pipeline).
			 * @param renderer A reference to the graphics renderer.
			 * @param resourceManager A reference to the resource manager.
			 * @param renderTarget A reference to a shared pointer to the render target for pipeline compatibility.
			 * @return bool
			 */
			[[nodiscard]]
			bool create (Graphics::Renderer & renderer, Resources::Manager & resourceManager, const std::shared_ptr< const Graphics::RenderTarget::Abstract > & renderTarget) noexcept;

			/**
			 * @brief Destroys the compass GPU resources.
			 * @return void
			 */
			void destroy () noexcept;

			/**
			 * @brief Records the compass draw calls.
			 * @warning This MUST be called after the post-process chain has been executed, inside
			 * the render pass that owns the overlay framebuffer. Recording it in the scene pass
			 * would put the spheres back under the exposure multiply this class exists to escape.
			 * @param commandBuffer A reference to the command buffer.
			 * @param viewMatrices A reference to the view matrices of the camera being rendered.
			 * @return void
			 */
			void render (const Vulkan::CommandBuffer & commandBuffer, const Graphics::ViewMatricesInterface & viewMatrices) const noexcept;

			/**
			 * @brief Returns whether the GPU resources are created.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isCreated () const noexcept
			{
				return m_created;
			}

		private:

			/** @brief One sphere per signed world axis: +X, +Y, +Z, -X, -Y, -Z. */
			static constexpr size_t AxisCount{6};

			/** @brief Sphere radius, in world units. */
			static constexpr float SphereRadius{8.0F};

			/** @brief Distance of each sphere from the origin, in world units.
			 * @note Only the ratio with SphereRadius matters: the infinity view drops the
			 * translation, so this pair fixes the apparent angular size of the spheres. */
			static constexpr float AxisDistance{100.0F};

			/** @brief Sphere tessellation. */
			static constexpr uint32_t SphereSlices{16};
			static constexpr uint32_t SphereStacks{16};

			/** @brief Geometry of each axis sphere, colors baked in the vertex attributes. */
			std::array< std::shared_ptr< Graphics::Geometry::Interface >, AxisCount > m_geometries{};

			/** @brief The shader program and its pipeline, built for the overlay framebuffer. */
			std::shared_ptr< Saphir::Program > m_program;

			/** @brief Whether GPU resources are created. */
			bool m_created{false};
	};
}

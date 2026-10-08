/*
 * src/Graphics/PathDebugOverlay.hpp
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
#include <cstdint>
#include <memory>
#include <vector>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

namespace EmEn
{
	namespace Vulkan
	{
		class CommandBuffer;
		class DescriptorSet;
		class DescriptorSetLayout;
		class Framebuffer;
		class GraphicsPipeline;
		class PipelineLayout;
		class ShaderStorageBufferObject;
	}

	namespace Scenes
	{
		class SceneInstanceTransforms;
	}
}

namespace EmEn::Graphics
{
	class Renderer;
	class ViewMatricesInterface;

	/**
	 * @brief Draws the paths in DEBUG mode (Scenes::Component::Path::setDebugMode()): ALWAYS ON TOP, after the tone
	 * mapping, in their display colour — translucent if their opacity says so, neither exposed nor smeared by the TAA.
	 * @note The same ribbon as the scene program (Saphir::PathGLSL::rawFunctions(): one code, two shaders): the vertices
	 * are pulled from this overlay's own per-frame SSBO — {view-projection, eye, pixel} then the points in WORLD space —
	 * filled from the list the primary view staged (Scenes::SceneInstanceTransforms::debugPaths()). One draw of
	 * 9 × (points − 1) vertices per path, no vertex buffer.
	 * @note No depth test: what is hidden is drawn too. Round joins of a TRANSLUCENT path overlap at each point (the two
	 * capsules): the joint reads darker — accepted for a debug drawing.
	 * @note Owned by the Renderer; internal-target frames (Renderer::renderFrameWithInternal()), like the outline.
	 */
	class EMEN_API PathDebugOverlay final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"PathDebugOverlay"};

			/**
			 * @brief Constructs the overlay.
			 * @param renderer A reference to the renderer.
			 */
			explicit PathDebugOverlay (Renderer & renderer) noexcept;

			/** @brief Deleted copy and move. */
			PathDebugOverlay (const PathDebugOverlay & copy) noexcept = delete;
			PathDebugOverlay (PathDebugOverlay && copy) noexcept = delete;
			PathDebugOverlay & operator= (const PathDebugOverlay & copy) noexcept = delete;
			PathDebugOverlay & operator= (PathDebugOverlay && copy) noexcept = delete;

			/** @brief Destructs the overlay. */
			~PathDebugOverlay ();

			/**
			 * @brief Draws the frame's debug paths.
			 * @note Inside the final composite render pass, after the direct (tone mapping) effects.
			 * @param commandBuffer A reference to the frame's command buffer.
			 * @param framebuffer A reference to the composite pass framebuffer (its render pass seals the pipeline).
			 * @param width The composite width, in pixels.
			 * @param height The composite height, in pixels.
			 * @param instanceTransforms A reference to the scene's instance transforms (the staged debug paths).
			 * @param mainViewMatrices A reference to the main camera's view matrices.
			 * @param readStateIndex The render state slot the frame draws (the camera of the staged points).
			 * @param sRGBTarget Whether the composite target encodes sRGB on write (the colour is then written linear).
			 */
			void record (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::Framebuffer & framebuffer, uint32_t width, uint32_t height, const Scenes::SceneInstanceTransforms & instanceTransforms, const ViewMatricesInterface & mainViewMatrices, uint32_t readStateIndex, bool sRGBTarget) noexcept;

			/**
			 * @brief Releases the GPU resources.
			 */
			void destroy () noexcept;

		private:

			/**
			 * @brief Creates the descriptor set layout, the per-frame buffers and sets, and the pipeline layout.
			 * @return bool
			 */
			[[nodiscard]]
			bool createSharedResources () noexcept;

			/**
			 * @brief Creates the pipeline against the composite render pass.
			 * @param framebuffer A reference to the composite framebuffer.
			 * @param width The composite width.
			 * @param height The composite height.
			 * @return bool
			 */
			[[nodiscard]]
			bool createPipeline (const Vulkan::Framebuffer & framebuffer, uint32_t width, uint32_t height) noexcept;

			/**
			 * @brief Uploads the frame's data into the frame's buffer, growing it when needed.
			 * @param frameIndex The frame in flight.
			 * @param data The bytes.
			 * @return bool
			 */
			[[nodiscard]]
			bool upload (uint32_t frameIndex, const std::vector< float > & data) noexcept;

			Renderer & m_renderer;
			std::shared_ptr< Vulkan::DescriptorSetLayout > m_descriptorSetLayout;
			std::vector< std::unique_ptr< Vulkan::ShaderStorageBufferObject > > m_buffers;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_descriptorSets;
			std::shared_ptr< Vulkan::PipelineLayout > m_pipelineLayout;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_pipeline;
			/** @brief Reused staging storage (render thread only). */
			std::vector< float > m_staging;
			/** @brief The render pass the pipeline was sealed against: a recreated swap chain brings a new one. */
			VkRenderPass m_pipelineRenderPass{VK_NULL_HANDLE};
			bool m_sharedResourcesCreated{false};
	};
}

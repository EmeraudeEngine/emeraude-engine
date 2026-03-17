/*
 * src/Graphics/IndirectPostProcessEffect.hpp
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

#pragma once

/* STL inclusions. */
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

/* Local inclusions for inheritances. */
#include "PostProcessEffect.hpp"

/* Local inclusions for usages. */
#include "Vulkan/DescriptorSet.hpp"
#include "PostProcessor.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Vulkan
	{
		class CommandBuffer;
		class DescriptorSetLayout;
		class GraphicsPipeline;
		class PipelineLayout;
		class ShaderModule;
		class TextureInterface;
	}

	namespace Scenes
	{
		class LightSet;
	}

	namespace Graphics
	{
		class IntermediateRenderTarget;
		class Renderer;
	}
}

namespace EmEn::Graphics
{
	/**
	 * @brief Abstract interface for multi-pass post-processing effects.
	 * @note Each effect manages its own intermediate render targets, render passes, and pipelines.
	 * Effects receive an input texture and produce an output texture, forming a chain.
	 * Provides shared infrastructure for fullscreen pass rendering (pipeline creation,
	 * pass recording, vertex shader, descriptor set layouts) to eliminate duplication.
	 * @extends EmEn::Graphics::PostProcessEffect This is a post-process effect.
	 */
	class IndirectPostProcessEffect : public PostProcessEffect
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			IndirectPostProcessEffect (const IndirectPostProcessEffect & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			IndirectPostProcessEffect (IndirectPostProcessEffect && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return IndirectPostProcessEffect &
			 */
			IndirectPostProcessEffect & operator= (const IndirectPostProcessEffect & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return IndirectPostProcessEffect &
			 */
			IndirectPostProcessEffect & operator= (IndirectPostProcessEffect && copy) noexcept = delete;

			/**
			 * @brief Destructs the indirect post-process effect.
			 */
			~IndirectPostProcessEffect () override = default;

			/**
			 * @brief Returns whether this effect requires a depth buffer input.
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			requiresDepth () const noexcept
			{
				return false;
			}

			/**
			 * @brief Returns whether this effect requires HDR input data.
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			requiresHDR () const noexcept
			{
				return false;
			}

			/**
			 * @brief Returns whether this effect requires a normals buffer input.
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			requiresNormals () const noexcept
			{
				return false;
			}

			/**
			 * @brief Returns whether this effect requires a material properties buffer input.
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			requiresMaterialProperties () const noexcept
			{
				return false;
			}

			/**
			 * @brief Returns whether this effect requires a velocity buffer input.
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			requiresVelocity () const noexcept
			{
				return false;
			}

			/**
			 * @brief Returns whether this effect requires ray tracing (TLAS) to function.
			 * @note Effects returning true are skipped when RT is not available on the device.
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			requiresRayTracing () const noexcept
			{
				return false;
			}

			/**
			 * @brief Returns whether this effect requires the scene light set to function.
			 * @note Effects returning true are skipped when no main directional light is available.
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			requiresLightSet () const noexcept
			{
				return false;
			}

			/**
			 * @brief Creates GPU resources for this effect.
			 * @param width The framebuffer width.
			 * @param height The framebuffer height.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool create (uint32_t width, uint32_t height) noexcept = 0;

			/**
			 * @brief Destroys GPU resources for this effect.
			 * @return void
			 */
			virtual void destroy () noexcept = 0;

			/**
			 * @brief Recreates GPU resources after a resize.
			 * @note Default implementation calls destroy() then create(). Override only if
			 * the effect needs partial recreation (e.g., keeping adaptation state).
			 * @param width The new framebuffer width.
			 * @param height The new framebuffer height.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool resize (uint32_t width, uint32_t height) noexcept;

			/**
			 * @brief Executes the effect for the current frame.
			 * @note Called outside any active render pass. The effect manages its own render passes.
			 * @param commandBuffer A reference to the active command buffer.
			 * @param inputColor The input color texture to process.
			 * @param inputDepth The input depth texture (maybe nullptr if not available).
			 * @param inputNormals The input normals texture (maybe nullptr if not available).
			 * @param inputMaterialProperties The input material properties texture (maybe nullptr if not available).
			 * @param lightSet The scene light set for lighting queries (maybe nullptr if not available).
			 * @param constants The current post-processing push constants.
			 * @return const Vulkan::TextureInterface & The output texture to pass to the next effect.
			 */
			[[nodiscard]]
			virtual const Vulkan::TextureInterface & execute (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::TextureInterface & inputColor, const Vulkan::TextureInterface * inputDepth, const Vulkan::TextureInterface * inputNormals, const Vulkan::TextureInterface * inputMaterialProperties, const Scenes::LightSet * lightSet, const PostProcessor::PushConstants & constants) noexcept = 0;

		protected:

			/**
			 * @brief Constructs an indirect post-process effect.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit
			IndirectPostProcessEffect (Renderer & renderer) noexcept
				: m_renderer{renderer}
			{

			}

			[[nodiscard]]
			Renderer &
			renderer () noexcept
			{
				return m_renderer;
			}

			[[nodiscard]]
			const Renderer &
			renderer () const noexcept
			{
				return m_renderer;
			}

			/* ---- Shared fullscreen pass infrastructure ---- */

			/**
			 * @brief The fullscreen triangle vertex shader source shared by all effects.
			 * @note Uses gl_VertexIndex to generate a full-screen triangle (3 vertices, no VBO).
			 */
			static constexpr auto FullscreenVertexShaderSource = R"GLSL(
#version 450

layout(location = 0) out vec2 vUV;

void main()
{
	vUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(vUV * 2.0 - 1.0, 0.0, 1.0);
}
)GLSL";

			/**
			 * @brief Returns the compiled fullscreen vertex shader module (cached by ShaderManager).
			 * @return std::shared_ptr< Vulkan::ShaderModule >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::ShaderModule > getFullscreenVertexShader () const noexcept;

			/**
			 * @brief Creates a standard fullscreen graphics pipeline.
			 * @note Configures: empty vertex input, triangle list, dynamic viewport/scissor,
			 * no culling, no depth test, no blending, single color attachment, RGBA write mask.
			 * @param tracerTag The tracer tag for debug identification.
			 * @param name The pipeline name for debug identification.
			 * @param vertexModule The vertex shader module.
			 * @param fragmentModule The fragment shader module.
			 * @param pipelineLayout The pipeline layout.
			 * @param target The intermediate render target defining the render pass.
			 * @return std::shared_ptr< Vulkan::GraphicsPipeline >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::GraphicsPipeline > createFullscreenPipeline (const char * tracerTag, const std::string & name, const std::shared_ptr< Vulkan::ShaderModule > & vertexModule, const std::shared_ptr< Vulkan::ShaderModule > & fragmentModule, const std::shared_ptr< Vulkan::PipelineLayout > & pipelineLayout, const IntermediateRenderTarget & target) const noexcept;

			/**
			 * @brief Records a fullscreen pass into a command buffer.
			 * @note Performs: beginRenderPass, bind pipeline, set viewport/scissor,
			 * push constants, bind descriptor set, draw(3,1), endRenderPass.
			 * @param commandBuffer A reference to the active command buffer.
			 * @param target The intermediate render target to render into.
			 * @param pipeline The graphics pipeline to use.
			 * @param pipelineLayout The pipeline layout for push constants and descriptor binding.
			 * @param descriptorSet The descriptor set to bind.
			 * @param pushConstants Pointer to the push constants data.
			 * @param pushConstantsSize Size of the push constants data in bytes.
			 * @return void
			 */
			static void recordFullscreenPass (const Vulkan::CommandBuffer & commandBuffer, const IntermediateRenderTarget & target, const Vulkan::GraphicsPipeline & pipeline, const Vulkan::PipelineLayout & pipelineLayout, const Vulkan::DescriptorSet & descriptorSet, const void * pushConstants, uint32_t pushConstantsSize) noexcept;

			/* ---- Shared descriptor set layout helpers ---- */

			/**
			 * @brief Returns a shared descriptor set layout with N combined image samplers.
			 * @param samplerCount The number of combined image samplers (1, 2, 3, etc.).
			 * @return std::shared_ptr< Vulkan::DescriptorSetLayout >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::DescriptorSetLayout > getInputLayout (uint32_t samplerCount) const noexcept;

			/**
			 * @brief Allocates per-frame descriptor sets (one per frame-in-flight).
			 * @param layout The descriptor set layout.
			 * @param classId The class identifier for debug tracing.
			 * @param baseName The base name for descriptor set identification.
			 * @return std::vector< std::unique_ptr< Vulkan::DescriptorSet > >
			 */
			[[nodiscard]]
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > createPerFrameDescriptorSets (const std::shared_ptr< Vulkan::DescriptorSetLayout > & layout, const char * classId, const std::string & baseName) const noexcept;

		private:

			Renderer & m_renderer;
	};
}

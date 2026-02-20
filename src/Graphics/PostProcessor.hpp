/*
 * src/Graphics/PostProcessor.hpp
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
#include <memory>
#include <vector>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"

/* Local inclusions for usages. */
#include "RenderTarget/Abstract.hpp"
#include "Saphir/FramebufferEffectInterface.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Saphir
	{
		class Program;
	}

	namespace Vulkan
	{
		class CommandBuffer;
		class DescriptorSet;
		class DescriptorSetLayout;
		class LayoutManager;
	}

	namespace Graphics
	{
		class GrabPass;
		class PostProcessEffect;
		class Renderer;

		namespace Geometry
		{
			class IndexedVertexResource;
		}
	}
}

namespace EmEn::Graphics
{
	/**
	 * @brief The post-processor service to apply fullscreen effects on the final render.
	 * @extends EmEn::ServiceInterface The post-processor is a renderer sub-service.
	 */
	class PostProcessor final : public ServiceInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"PostProcessorService"};

			/* GLSL variables. */
			static constexpr auto Fragment{"em_Fragment"};

			/**
			 * @brief Push constants matching the GLSL pcPostProcessing layout.
			 */
			struct PushConstants
			{
				float frameWidth;
				float frameHeight;
				float time;
				float nearPlane;
				float farPlane;
				float tanHalfFovY;
			};

			/**
			 * @brief Constructs the post-processor service.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit PostProcessor (Renderer & renderer) noexcept;

			/**
			 * @brief Configures the post-processor over a render-target.
			 * @param renderTarget A reference to a render-target.
			 * @return bool
			 */
			[[nodiscard]]
			bool configure (const std::shared_ptr< RenderTarget::Abstract > & renderTarget) noexcept;

			/**
			 * @brief Sets a list of post-effects.
			 * @param effectsList The list of shader effects.
			 * @return void
			 */
			void setEffectsList (const Saphir::FramebufferEffectsList & effectsList) noexcept;

			/**
			 *
			 *
			 * @brief Clears every effect.
			 * @return void
			 */
			void clearEffects () noexcept;

			/**
			 * @brief Returns the current effects list.
			 * @return const Saphir::FramebufferEffectsList &
			 */
			[[nodiscard]]
			const Saphir::FramebufferEffectsList &
			effectsList () const noexcept
			{
				return m_effectsList;
			}

			/**
			 * @brief Returns whether the post-processor has active effects.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasEffects () const noexcept
			{
				return !m_effectsList.empty();
			}

			/**
			 * @brief Enables or disables the post-processor.
			 * @param state The desired enabled state.
			 * @return void
			 */
			void
			enable (bool state) noexcept
			{
				m_enabled = state;
			}

			/**
			 * @brief Returns whether the post-processor is enabled and ready to render.
			 * @note All cancellation conditions are centralized here for simplicity.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isEnabled () const noexcept
			{
				return m_enabled && this->usable();
			}

			/**
			 * @brief Records the blit from the swap chain color image into the post-processor's own grab pass.
			 * @note Must be called between render pass 1 and render pass 2, outside any active render pass.
			 * @param commandBuffer A reference to the active command buffer.
			 * @return void
			 */
			void recordBlit (const Vulkan::CommandBuffer & commandBuffer) const noexcept;

			/**
			 * @brief Renders post-processing effects into the current command buffer.
			 * @param commandBuffer A reference to the active command buffer.
			 * @return void
			 */
			void render (const Vulkan::CommandBuffer & commandBuffer) const noexcept;

			/**
			 * @brief Executes the multi-pass effect chain outside of any active render pass.
			 * @note Must be called after recordBlit() and before the RP2 restart.
			 * Each effect in the chain receives the output of the previous one.
			 * After execution, the descriptor set is updated to point to the chain output.
			 * @param commandBuffer A reference to the active command buffer.
			 * @return void
			 */
			void executeEffectChain (const Vulkan::CommandBuffer & commandBuffer) noexcept;

			/**
			 * @brief Enables or disables HDR mode for the post-processing pipeline.
			 * @note When HDR is enabled, the grab pass uses R16G16B16A16_SFLOAT format
			 * and the blit from the swap chain uses vkCmdBlitImage for format conversion.
			 * @param state True to enable HDR, false for standard 8-bit pipeline.
			 * @return void
			 */
			void
			enableHDR (bool state) noexcept
			{
				m_hdrEnabled = state;
			}

			/**
			 * @brief Returns whether HDR mode is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isHDREnabled () const noexcept
			{
				return m_hdrEnabled;
			}

			/**
			 * @brief Sets the multi-pass effect chain.
			 * @param effectChain The chain of post-process effects to execute.
			 * @return void
			 */
			void
			setEffectChain (const std::vector< std::shared_ptr< PostProcessEffect > > & effectChain) noexcept
			{
				m_effectChain = effectChain;
			}

			/**
			 * @brief Returns the current effect chain.
			 * @return const std::vector< std::shared_ptr< PostProcessEffect > > &
			 */
			[[nodiscard]]
			const std::vector< std::shared_ptr< PostProcessEffect > > &
			effectChain () const noexcept
			{
				return m_effectChain;
			}

			/**
			 * @brief Returns whether the multi-pass effect chain has any effects.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasEffectChain () const noexcept
			{
				return !m_effectChain.empty();
			}

			/**
			 * @brief Updates the near and far plane values for depth-based effects.
			 * @param nearPlane The camera near plane distance.
			 * @param farPlane The camera far plane distance.
			 * @return void
			 */
			void
			setClipPlanes (float nearPlane, float farPlane) noexcept
			{
				m_nearPlane = nearPlane;
				m_farPlane = farPlane;
			}

			/**
			 * @brief Returns or creates the descriptor set layout for post-processing.
			 * @param layoutManager A reference to the Vulkan layout manager.
			 * @return std::shared_ptr< Vulkan::DescriptorSetLayout >
			 */
			[[nodiscard]]
			static std::shared_ptr< Vulkan::DescriptorSetLayout > getDescriptorSetLayout (Vulkan::LayoutManager & layoutManager) noexcept;

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			Renderer & m_renderer;
			std::unique_ptr< GrabPass > m_grabPass;
			std::shared_ptr< Saphir::Program > m_program;
			std::shared_ptr< Geometry::IndexedVertexResource > m_quadGeometry;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_descriptorSets;
			Saphir::FramebufferEffectsList m_effectsList;
			std::vector< std::shared_ptr< PostProcessEffect > > m_effectChain;
			float m_nearPlane{0.1F};
			float m_farPlane{1000.0F};
			bool m_enabled{false};
			bool m_hdrEnabled{false};
	};
}

/*
 * src/Graphics/Renderer.hpp
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

/* STL inclusions */
#include <memory>
#include <unordered_map>

/* Local inclusions. */
#include "StaticVector.hpp"
#include "Graphics/RenderTarget/Abstract.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	namespace Sync
	{
		class Fence;
		class Semaphore;
	}

	class CommandPool;
}

namespace EmEn::Graphics
{
	/**
	 * @brief Declares the scope of one renderer frame.
	 */
	class RendererFrameScope final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"RendererFrameScope"};

			/**
			 * @brief Constructs a render frame scope.
			 */
			RendererFrameScope () noexcept = default;

			/**
			 * @brief Initializes the command pool and the command buffer.
			 * @param device A reference to the graphics device smart pointer.
			 * @param frameIndex The frame index.
			 * @return bool
			 */
			[[nodiscard]]
			bool initialize (const std::shared_ptr< Vulkan::Device > & device, uint32_t frameIndex) noexcept;

			/**
			 * @brief Declares a semaphore to be waited on by a later submission.
			 * @note The semaphore is added to exactly ONE list (primary or secondary), never both.
			 * Primary semaphores (shadow maps) are consumed by render-to-textures, then forwarded
			 * to secondary via promotePrimaryToSecondary(). Secondary semaphores are waited on
			 * by the final frame submission. Binary semaphores can only be waited on once per signal.
			 * @param semaphore A reference to a semaphore smart pointer.
			 * @param primary True for shadow map semaphores (consumed by RTTs), false for RTT semaphores.
			 * @return void
			 */
			void declareSemaphore (const std::shared_ptr< Vulkan::Sync::Semaphore > & semaphore, bool primary) noexcept;

			/**
			 * @brief Returns the command pool smart pointer.
			 * @return std::shared_ptr< Vulkan::CommandPool >
			 */
			[[nodiscard]]
			const std::shared_ptr< Vulkan::CommandPool > &
			commandPool () const noexcept
			{
				return m_commandPool;
			}

			/**
			 * @brief Returns the command buffer smart pointer for a render-target.
			 * @param renderTarget A pointer to a render target.
			 * @return std::shared_ptr< Vulkan::CommandPool >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::CommandBuffer > getCommandBuffer (const RenderTarget::Abstract * renderTarget) noexcept;

			/**
			 * @brief Returns the frame index.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			frameIndex () const noexcept
			{
				return m_frameIndex;
			}

			/**
			 * @brief Returns primary semaphores ready to use with vkQueueSubmit().
			 * @return Base::Storage< VkSemaphore, 16 > &
			 */
			[[nodiscard]]
			Base::StaticVector< VkSemaphore, 16 > &
			primarySemaphores () noexcept
			{
				return m_primarySemaphores;
			}

			/**
			 * @brief Returns secondary semaphores ready to use with vkQueueSubmit().
			 * @return Base::Storage< VkSemaphore, 16 > &
			 */
			[[nodiscard]]
			Base::StaticVector< VkSemaphore, 16 > &
			secondarySemaphores () noexcept
			{
				return m_secondarySemaphores;
			}

			/**
			 * @brief Forwards unconsumed primary semaphores to the secondary list.
			 * @note Call this after render-to-textures to ensure shadow map semaphores
			 * are waited on by the final submit when no RTT consumed them.
			 * @return void
			 */
			void promotePrimaryToSecondary () noexcept;

			/**
			 * @brief Returns the in-flight fence.
			 * @return Vulkan::Sync::Fence *
			 */
			[[nodiscard]]
			Vulkan::Sync::Fence *
			inFlightFence () const noexcept
			{
				return m_inFlightFence.get();
			}

			/**
			 * @brief Returns the image available semaphore.
			 * @return Vulkan::Sync::Semaphore *
			 */
			[[nodiscard]]
			Vulkan::Sync::Semaphore *
			imageAvailableSemaphore () const noexcept
			{
				return m_imageAvailableSemaphore.get();
			}

			/**
			 * @brief Returns the image finished semaphore.
			 * @return Vulkan::Sync::Semaphore *
			 */
			[[nodiscard]]
			Vulkan::Sync::Semaphore *
			renderFinishedSemaphore () const noexcept
			{
				return m_renderFinishedSemaphore.get();
			}

			/**
			 * @brief Clears all command buffers and semaphores for a next frame usage.
			 * @return bool
			 */
			bool prepareForNewFrame () noexcept;

		private:

			/**
			 * @brief Returns the frame name.
			 * @param frameIndex The number of the frame.
			 * @return std::string
			 */
			[[nodiscard]]
			static
			std::string
			getFrameName (uint32_t frameIndex) noexcept
			{
				return "Frame" + std::to_string(frameIndex);
			}

			std::shared_ptr< Vulkan::CommandPool > m_commandPool;
			std::unordered_map< const RenderTarget::Abstract *, std::shared_ptr< Vulkan::CommandBuffer > > m_commandBuffers;
			Base::StaticVector< VkSemaphore, 16 > m_primarySemaphores;
			Base::StaticVector< VkSemaphore, 16 > m_secondarySemaphores;
			/* Synchronization. */
			std::unique_ptr< Vulkan::Sync::Fence > m_inFlightFence;
			std::unique_ptr< Vulkan::Sync::Semaphore > m_imageAvailableSemaphore;
			std::unique_ptr< Vulkan::Sync::Semaphore > m_renderFinishedSemaphore;
			uint32_t m_frameIndex{0};
	};
}

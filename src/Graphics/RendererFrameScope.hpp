#pragma once

/* STL inclusions. */
#include <cstdint>
#include <memory>
#include <vector>
#include <string>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

/* Forward declarations. */
namespace EmEn::Vulkan
{
	namespace Sync
	{
		class Semaphore;
	}

	class Device;
	class CommandPool;
	class CommandBuffer;
}

namespace EmEn::Graphics
{
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
			 * @brief Declares semaphore to wait.
			 * @param semaphore A reference to a semaphore smart pointer.
			 * @param primary Is a primary resource to wait?
			 * @return void
			 */
			void declareSemaphore (const std::shared_ptr< Vulkan::Sync::Semaphore > & semaphore, bool primary) noexcept;

			/**
			 * @brief Returns the command pool smart pointer.
			 * @return std::shared_ptr< Vulkan::CommandPool >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::CommandPool >
			commandPool () const noexcept
			{
				return m_commandPool;
			}

			/**
			 * @brief Returns the command buffer smart pointer.
			 * @return std::shared_ptr< Vulkan::CommandPool >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::CommandBuffer >
			commandBuffer () const noexcept
			{
				return m_commandBuffer;
			}

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
			 * @return std::vector< VkSemaphore > &
			 */
			[[nodiscard]]
			std::vector< VkSemaphore > &
			primarySemaphores () noexcept
			{
				return m_primarySemaphores;
			}

			/**
			 * @brief Returns secondary semaphores ready to use with vkQueueSubmit().
			 * @return std::vector< VkSemaphore > &
			 */
			[[nodiscard]]
			std::vector< VkSemaphore > &
			secondarySemaphores () noexcept
			{
				return m_secondarySemaphores;
			}

			/**
			 * @brief Clears all semaphores for a next frame usage.
			 * @return void
			 */
			void
			clearSemaphores () noexcept
			{
				m_primarySemaphores.clear();
				m_secondarySemaphores.clear();
			}

		private:

			/**
			 * @brief Returns the frame name.
			 * @param frameIndex The number of the frame.
			 * @return std::string
			 */
			[[nodiscard]]
			static std::string getFrameName (uint32_t frameIndex) noexcept;

			std::shared_ptr< Vulkan::CommandPool > m_commandPool;
			std::shared_ptr< Vulkan::CommandBuffer > m_commandBuffer;
			std::vector< VkSemaphore > m_primarySemaphores;
			std::vector< VkSemaphore > m_secondarySemaphores;
			uint32_t m_frameIndex{0};
	};
}

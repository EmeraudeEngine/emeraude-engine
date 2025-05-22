/*
 * src/Vulkan/CommandBuffer.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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
#include <array>
#include <cstdint>
#include <cstddef>
#include <memory>
#include <vector>

/* Local inclusions for inheritances. */
#include "AbstractObject.hpp"

/* Local inclusions for usages. */
#include "Libs/PixelFactory/Color.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Vulkan
	{
		namespace Sync
		{
			class Event;
			class MemoryBarrier;
			class BufferMemoryBarrier;
			class ImageMemoryBarrier;
		}

		class GraphicsPipeline;
		class ComputePipeline;
		class DescriptorSet;
		class PipelineLayout;
		class Framebuffer;
		class VertexBufferObject;
		class IndexBufferObject;
		class CommandPool;
		class Buffer;
		class Image;
	}

	namespace Graphics::Geometry
	{
		class Interface;
	}
}

namespace EmEn::Vulkan
{
	/**
	 * @brief The command buffer wrapper class
	 * @extends EmEn::Vulkan::AbstractObject This object will use the command pool to get the device.
	 */
	class CommandBuffer final : public AbstractObject
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanCommandBuffer"};

			/**
			 * @brief Constructs a command buffer.
			 * @param commandPool The reference to the command pool smart pointer.
			 * @param primary Set command as primary or secondary.
			 */
			explicit CommandBuffer (const std::shared_ptr< CommandPool > & commandPool, bool primary = true) noexcept;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			CommandBuffer (const CommandBuffer & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			CommandBuffer (CommandBuffer && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			CommandBuffer & operator= (const CommandBuffer & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			CommandBuffer & operator= (CommandBuffer && copy) noexcept = default;

			/**
			 * @brief Destructs the command buffer.
			 */
			~CommandBuffer () override;

			/**
			 * @brief Returns the command buffer vulkan handle.
			 * @return VkCommandBuffer
			 */
			[[nodiscard]]
			VkCommandBuffer
			handle () const noexcept
			{
				return m_handle;
			}

			/**
			 * @brief Returns the responsible command pool smart pointer.
			 * @return std::shared_ptr< CommandPool >
			 */
			[[nodiscard]]
			std::shared_ptr< CommandPool >
			commandPool () const noexcept
			{
				return m_commandPool;
			}

			/**
			 * @brief Returns whether the buffer level is primary.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isBufferLevelPrimary () const noexcept
			{
				return m_primaryLevel;
			}

			/**
			 * @brief Begins registering commands.
			 * @note Flags descriptions
			 *  - VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: The command buffer will be rerecorded right after executing it once.
			 *  - VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT: This is a secondary command buffer that will be entirely within a single render pass.
			 *  - VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT: The command buffer can be resubmitted while it is also already pending execution.
			 * @param usage A Vulkan flag for the command buffer usage.
			 * @return bool
			 */
			[[nodiscard]]
			bool begin (VkCommandBufferUsageFlagBits usage) const noexcept;

			/**
			 * @brief Ends registering commands.
			 * @return bool
			 */
			[[nodiscard]]
			bool end () const noexcept;

			/**
			 * @brief Reset the command buffer.
			 * @param flags The reset flags. Default none.
			 * @return bool
			 */
			[[nodiscard]]
			bool reset (VkCommandBufferResetFlags flags = 0) const noexcept;

			/**
			 * @brief Registers a render pass begin.
			 * @param framebuffer A reference to a framebuffer.
			 * @param renderArea The render area.
			 * @param clearValues The framebuffer clear values.
			 * @param subpassContents
			 * @return void
			 */
			void beginRenderPass (const Framebuffer & framebuffer, const VkRect2D & renderArea, const std::array< VkClearValue, 2 > & clearValues, VkSubpassContents subpassContents) const noexcept;

			/**
			 * @brief Registers a render pass end.
			 * @return void
			 */
			void endRenderPass () const noexcept;

			/**
			 * @brief Registers an update buffer command.
			 * @param buffer A reference to the buffer.
			 * @param dstOffset The byte offset into the buffer to execute updating, and must be a multiple of 4.
			 * @param dataSize The number of bytes to update, and must be a multiple of 4.
			 * @param pData A pointer to the source data for the buffer processLogics, and must be at least dataSize bytes in size.
			 * @return void
			 */
			void update (const Buffer & buffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void * pData) const noexcept;

			/**
			 * @brief Registers a fill buffer command.
			 * @param buffer A reference to the buffer.
			 * @param dstOffset The byte offset into the buffer at which to execute filling, and must be a multiple of 4.
			 * @param size The number of bytes to fill, and must be either a multiple of 4, or VK_WHOLE_SIZE to fill the range from offset to the end of the buffer. If VK_WHOLE_SIZE is used and the remaining size of the buffer is not a multiple of 4, then the nearest smaller multiple is used.
			 * @param data The 4-byte word written repeatedly to the buffer to fill size bytes of data. The data word is written to memory according to the host endianness.
			 * @return void
			 */
			void fill (const Buffer & buffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data) const noexcept;

			/**
			 * @brief Registers a buffer to buffer copy command.
			 * @deprecated This must be done the transferManager !
			 * @param src A reference to the buffer.
			 * @param dst A reference to the buffer.
			 * @param srcOffset The source buffer start for reading. Default 0.
			 * @param dstOffset The destination buffer start for writing. default 0.
			 * @param size The size of copy. Default the whole source buffer.
			 * @return void
			 */
			void copy (const Buffer & src, const Buffer & dst, VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0, VkDeviceSize size = VK_WHOLE_SIZE) const noexcept;

			/**
			 * @brief Registers a image to image copy command.
			 * @deprecated This must be done the transferManager !
			 * @param src A reference to the image.
			 * @param dst A reference to the image.
			 * @return void
			 */
			void copy (const Image & src, const Image & dst) const noexcept;

			/**
			 * @brief Registers a buffer to image copy command.
			 * @deprecated This must be done the transferManager !
			 * @param src A reference to the buffer.
			 * @param dst A reference to the image.
			 * @param srcOffset The source buffer start for reading. Default 0.
			 * @return void
			 */
			void copy (const Buffer & src, const Image & dst, VkDeviceSize srcOffset = 0) const noexcept;

			/**
			 * @brief Registers an image to buffer copy command.
			 * @deprecated This must be done the transferManager !
			 * @param src A reference to the image.
			 * @param dst A reference to the buffer.
			 * @return void
			 */
			void copy (const Image & src, const Buffer & dst) const noexcept;

			/**
			 * @brief Registers an image to image blit command.
			 * @deprecated This must be done the transferManager !
			 * @param src A reference to the image.
			 * @param dst A reference to the buffer.
			 * @return void
			 */
			void blit (const Image & src, const Image & dst) const noexcept;

			/**
			 * @brief Clears the color part of the image.
			 * @param image A reference to a command buffer.
			 * @param imageLayout Specifies the current layout of the image subresource ranges to be cleared, and must be VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR, VK_IMAGE_LAYOUT_GENERAL or VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL.
			 * @param color A reference to a color. Default black.
			 * @return void
			 */
			void clearColor (const Image & image, VkImageLayout imageLayout, const Libs::PixelFactory::Color< float > & color = {}) const noexcept;

			/**
			 * @brief Clears the depth/stencil part of the image.
			 * @param image A reference to a command buffer.
			 * @param imageLayout Specifies the current layout of the image subresource ranges to be cleared, and must be VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR, VK_IMAGE_LAYOUT_GENERAL or VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL.
			 * @return void
			 */
			void clearDepthStencil (const Image & image, VkImageLayout imageLayout) const noexcept;

			/**
			 * @brief Set a pipeline barrier. Full version.
			 * @param memoryBarriers A reference to a vector of memory barriers.
			 * @param bufferMemoryBarriers A reference to a vector of buffer memory barriers.
			 * @param imageMemoryBarriers A reference to a vector of image memory barriers.
			 * @param srcStageMask A bitmask of VkPipelineStageFlagBits specifying the source stages.
			 * @param dstStageMask A bitmask of VkPipelineStageFlagBits specifying the destination stages.
			 * @param dependencyFlags
			 * @return void
			 */
			void pipelineBarrier (const std::vector< VkMemoryBarrier > & memoryBarriers, const std::vector< VkBufferMemoryBarrier > & bufferMemoryBarriers, const std::vector< VkImageMemoryBarrier > & imageMemoryBarriers, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags = 0) const noexcept;

			/**
			 * @brief Set a pipeline memory barrier.
			 * @param memoryBarriers A reference to a vector of memory barriers.
			 * @param srcStageMask A bitmask of VkPipelineStageFlagBits specifying the source stages.
			 * @param dstStageMask A bitmask of VkPipelineStageFlagBits specifying the destination stages.
			 * @param dependencyFlags
			 * @return void
			 */
			void pipelineBarrier (const std::vector< VkMemoryBarrier > & memoryBarriers, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags = 0) const noexcept;

			/**
			 * @brief Set a pipeline buffer memory barrier.
			 * @param bufferMemoryBarriers A reference to a vector of buffer memory barriers.
			 * @param srcStageMask A bitmask of VkPipelineStageFlagBits specifying the source stages.
			 * @param dstStageMask A bitmask of VkPipelineStageFlagBits specifying the destination stages.
			 * @param dependencyFlags
			 * @return void
			 */
			void pipelineBarrier (const std::vector< VkBufferMemoryBarrier > & bufferMemoryBarriers, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags = 0) const noexcept;

			/**
			 * @brief Set a pipeline image memory barrier.
			 * @param imageMemoryBarriers A reference to a vector of image memory barriers.
			 * @param srcStageMask A bitmask of VkPipelineStageFlagBits specifying the source stages.
			 * @param dstStageMask A bitmask of VkPipelineStageFlagBits specifying the destination stages.
			 * @param dependencyFlags
			 * @return void
			 */
			void pipelineBarrier (const std::vector< VkImageMemoryBarrier > & imageMemoryBarriers, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags = 0) const noexcept;

			/**
			 * @brief Set a pipeline memory barrier.
			 * @param memoryBarrier A reference to memory barrier.
			 * @param srcStageMask A bitmask of VkPipelineStageFlagBits specifying the source stages.
			 * @param dstStageMask A bitmask of VkPipelineStageFlagBits specifying the destination stages.
			 * @param dependencyFlags
			 * @return void
			 */
			void pipelineBarrier (const Sync::MemoryBarrier & memoryBarrier, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags = 0) const noexcept;

			/**
			 * @brief Set a pipeline buffer memory barrier.
			 * @param bufferMemoryBarrier A reference to buffer memory barrier.
			 * @param srcStageMask A bitmask of VkPipelineStageFlagBits specifying the source stages.
			 * @param dstStageMask A bitmask of VkPipelineStageFlagBits specifying the destination stages.
			 * @param dependencyFlags
			 * @return void
			 */
			void pipelineBarrier (const Sync::BufferMemoryBarrier & bufferMemoryBarrier, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags = 0) const noexcept;

			/**
			 * @brief Set a pipeline image memory barrier.
			 * @param imageMemoryBarrier A reference to an image memory barrier.
			 * @param srcStageMask A bitmask of VkPipelineStageFlagBits specifying the source stages.
			 * @param dstStageMask A bitmask of VkPipelineStageFlagBits specifying the destination stages.
			 * @param dependencyFlags
			 * @return void
			 */
			void pipelineBarrier (const Sync::ImageMemoryBarrier & imageMemoryBarrier, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags = 0) const noexcept;

			/**
			 * @brief Sets the event status in a command buffer.
			 * @param event A reference to a command buffer smart pointer.
			 * @param flags A pipeline stage flags.
			 * @return void
			 */
			void setEvent (const Sync::Event & event, VkPipelineStageFlags flags) const noexcept;

			/**
			 * @brief Resets the event status in a command buffer.
			 * @param event A reference to a command buffer smart pointer.
			 * @param flags A pipeline stage flags.
			 * @return void
			 */
			void resetEvent (const Sync::Event & event, VkPipelineStageFlags flags) const noexcept;

			/**
			 * @brief Waits for events in command.
			 * @param events A reference to event list.
			 * @param srcFlags A pipeline source stage flags.
			 * @param dstFlags A pipeline destination stage flags.
			 * @param memoryBarriers A reference to memory barrier list. Default none.
			 * @param bufferMemoryBarriers A reference to buffer memory barrier list. Default none.
			 * @param imageMemoryBarriers A reference to image memory barrier list. Default none.
			 * @return void
			 */
			void waitEvents (const std::vector< VkEvent > & events, VkPipelineStageFlags srcFlags, VkPipelineStageFlags dstFlags, const std::vector< VkMemoryBarrier > & memoryBarriers = {}, const std::vector< VkBufferMemoryBarrier > & bufferMemoryBarriers = {}, const std::vector< VkImageMemoryBarrier > & imageMemoryBarriers = {}) const noexcept;

			/**
			 * @brief Binds a graphics pipeline.
			 * @param graphicsPipeline A reference to a graphics pipeline.
			 * @return void
			 */
			void bind (const GraphicsPipeline & graphicsPipeline) const noexcept;

			/**
			 * @brief Binds a compute pipeline.
			 * @param computePipeline A reference to a compute pipeline.
			 * @return void
			 */
			void bind (const ComputePipeline & computePipeline) const noexcept;

			/**
			 * @brief Binds a single vertex buffer objects.
			 * @param vertexBufferObject A reference to a VBO.
			 * @param offset The starting point to read the VBO. Default 0.
			 * @return void
			 */
			void bind (const VertexBufferObject & vertexBufferObject, VkDeviceSize offset = 0) const noexcept;

			/**
			 * @brief Binds an index buffer objects.
			 * @param indexBufferObject A reference to a IBO.
			 * @param offset The starting point to read the IBO. Default 0.
			 * @param indexType The data type of index. Default unsigned int.
			 * @return void
			 */
			void bind (const IndexBufferObject & indexBufferObject, VkDeviceSize offset = 0, VkIndexType indexType = VK_INDEX_TYPE_UINT32) const noexcept;

			/**
			 * @brief Binds a single descriptor set.
			 * @param descriptorSet A reference to a descriptor set.
			 * @param pipelineLayout A reference to a pipeline layout.
			 * @param bindPoint The target binding point in the pipeline.
			 * @param firstSet The first set.
			 * @return void
			 */
			void bind (const DescriptorSet & descriptorSet, const PipelineLayout & pipelineLayout, VkPipelineBindPoint bindPoint, uint32_t firstSet) const noexcept;

			/**
			 * @brief Binds a single descriptor set.
			 * @param descriptorSet A reference to a descriptor set.
			 * @param pipelineLayout A reference to a pipeline layout.
			 * @param bindPoint The target binding point in the pipeline.
			 * @param firstSet The first set. Default 0.
			 * @param dynamicOffset ??? TODO: Define it
			 * @return void
			 */
			void bind (const DescriptorSet & descriptorSet, const PipelineLayout & pipelineLayout, VkPipelineBindPoint bindPoint, uint32_t firstSet, uint32_t dynamicOffset) const noexcept;

			/**
			 * @brief Binds a single geometry.
			 * @param geometry A reference to the geometry.
			 * @param subGeometryIndex A sub geometry layer index being drawn. Default 0.
			 * @return void
			 */
			void bind (const Graphics::Geometry::Interface & geometry, uint32_t subGeometryIndex = 0) const noexcept;

			/**
			 * @brief Binds a single geometry using a model vertex buffer object for location.
			 * @param geometry A reference to the geometry.
			 * @param modelVBO A reference to a vertex buffer object.
			 * @param subGeometryIndex A sub geometry layer index being drawn. Default 0.
			 * @param modelVBOOffset The offset in the model vertex buffer object. Default 0.
			 * @return void
			 */
			void bind (const Graphics::Geometry::Interface & geometry, const VertexBufferObject & modelVBO, uint32_t subGeometryIndex = 0, VkDeviceSize modelVBOOffset = 0) const noexcept;

			/**
			 * @brief Registers a draw command.
			 * @param geometry A reference to the geometry.
			 * @param subGeometryIndex A sub geometry layer index being drawn. Default 0.
			 * @param instanceCount The number of instance. Default 1.
			 * @return void
			 */
			void draw (const Graphics::Geometry::Interface & geometry, uint32_t subGeometryIndex = 0, uint32_t instanceCount = 1) const noexcept;

		private:

			VkCommandBuffer m_handle{VK_NULL_HANDLE};
			std::shared_ptr< CommandPool > m_commandPool;
			bool m_primaryLevel;
	};
}

/*
 * src/Vulkan/CommandBuffer.cpp
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

#include "CommandBuffer.hpp"

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <array>
#include <memory>
#include <vector>

/* Local inclusions. */
#include "Graphics/Geometry/Interface.hpp"
#include "Sync/Event.hpp"
#include "Sync/MemoryBarrier.hpp"
#include "Sync/ImageMemoryBarrier.hpp"
#include "Sync/BufferMemoryBarrier.hpp"
#include "CommandPool.hpp"
#include "PipelineLayout.hpp"
#include "DescriptorSet.hpp"
#include "IndexBufferObject.hpp"
#include "ComputePipeline.hpp"
#include "GraphicsPipeline.hpp"
#include "Buffer.hpp"
#include "Utility.hpp"
#include "VertexBufferObject.hpp"
#include "Framebuffer.hpp"
#include "Image.hpp"
#include "RenderPass.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace EmEn::Libs;

	CommandBuffer::CommandBuffer (const std::shared_ptr< CommandPool > & commandPool, bool primaryLevel) noexcept
		: m_commandPool(commandPool), m_primaryLevel(primaryLevel)
	{
		if constexpr ( IsDebug )
		{
			if ( commandPool == nullptr || !commandPool->isCreated() )
			{
				Tracer::error(ClassId, "Command pool is null or not created to allocate this command buffer !");

				return;
			}
		}

		m_handle = m_commandPool->allocateCommandBuffer(primaryLevel);

		if ( m_handle == VK_NULL_HANDLE )
		{
			return;
		}

		this->setCreated();
	}

	CommandBuffer::~CommandBuffer ()
	{
		if ( m_commandPool == nullptr || !m_commandPool->isCreated() )
		{
			Tracer::error(ClassId, "No or uninitialized command pool to destroy this command buffer !");

			return;
		}

		if ( m_handle != VK_NULL_HANDLE )
		{
			m_commandPool->freeCommandBuffer(m_handle);

			m_handle = VK_NULL_HANDLE;
		}

		this->setDestroyed();
	}

	bool
	CommandBuffer::begin (VkCommandBufferUsageFlagBits usage) const noexcept
	{
		if ( !this->isCreated() )
		{
			Tracer::error(ClassId, "The command buffer is not created !");

			return false;
		}

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = usage;
		beginInfo.pInheritanceInfo = nullptr;

		const auto result = vkBeginCommandBuffer(m_handle, &beginInfo);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to begin record to the command buffer " << m_handle << " : " << vkResultToCString(result) << " !";

			return false;
		}

		return true;
	}

	bool
	CommandBuffer::end () const noexcept
	{
		if ( !this->isCreated() )
		{
			Tracer::error(ClassId, "The command buffer is not created !");

			return false;
		}

		const auto result = vkEndCommandBuffer(m_handle);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to end record to the command buffer " << m_handle << " : " << vkResultToCString(result) << " !";

			return false;
		}

		return true;
	}

	bool
	CommandBuffer::reset (VkCommandBufferResetFlags flags) const noexcept
	{
		if ( !this->isCreated() )
		{
			Tracer::error(ClassId, "The command buffer is not created !");

			return false;
		}

		const auto result = vkResetCommandBuffer(m_handle, flags);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to reset the command buffer " << m_handle << " : " << vkResultToCString(result) << " !";

			return false;
		}

		return true;
	}

	void
	CommandBuffer::beginRenderPass (const Framebuffer & framebuffer, const VkRect2D & renderArea, const std::array< VkClearValue, 2 > & clearValues, VkSubpassContents subpassContents) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !framebuffer.isCreated() )
			{
				Tracer::error(ClassId, "The framebuffer is not created !");

				return;
			}

			if ( !this->isCreated() )
			{
				Tracer::error(ClassId, "The command buffer is not created !");

				return;
			}
		}

		VkRenderPassBeginInfo renderPassBeginInfo{};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = nullptr;
		renderPassBeginInfo.renderPass = framebuffer.renderPass()->handle();
		renderPassBeginInfo.framebuffer = framebuffer.handle();
		renderPassBeginInfo.renderArea= renderArea;
		renderPassBeginInfo.clearValueCount = static_cast< uint32_t >(clearValues.size());
		renderPassBeginInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(m_handle, &renderPassBeginInfo, subpassContents);
	}

	void
	CommandBuffer::endRenderPass () const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !this->isCreated() )
			{
				Tracer::error(ClassId, "The command buffer is not created !");

				return;
			}
		}

		vkCmdEndRenderPass(m_handle);
	}

	void
	CommandBuffer::update (const Buffer & buffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void * pData) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !buffer.isCreated() )
			{
				Tracer::error(ClassId, "The buffer is not created.");

				return;
			}

			if ( !this->isCreated() )
			{
				TraceError{ClassId} <<
					"The command buffer is not created !" "\n"
					"Unable to update the buffer " << buffer.handle();

				return;
			}
		}

		vkCmdUpdateBuffer(
			m_handle,
			buffer.handle(),
			dstOffset, dataSize, pData
		);
	}

	void
	CommandBuffer::fill (const Buffer & buffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !buffer.isCreated() )
			{
				Tracer::error(ClassId, "The buffer is not created.");

				return;
			}

			if ( !this->isCreated() )
			{
				TraceError{ClassId} <<
					"The command buffer is not created !" "\n"
					"Unable to fill the buffer " << buffer.handle();

				return;
			}
		}

		vkCmdFillBuffer(
			m_handle,
			buffer.handle(),
			dstOffset, size, data
		);
	}

	void
	CommandBuffer::copy (const Buffer & src, const Buffer & dst, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !src.isCreated() )
			{
				Tracer::error(ClassId, "The source buffer is not created.");

				return;
			}

			if ( !dst.isCreated() )
			{
				Tracer::error(ClassId, "The destination buffer is not created.");

				return;
			}

			if ( !this->isCreated() )
			{
				TraceError{ClassId} <<
					"The command buffer is not created !" "\n"
					"Unable to copy buffer " << src.handle() << " to buffer " << dst.handle();

				return;
			}
		}

		VkBufferCopy bufferCopy{};
		bufferCopy.srcOffset = srcOffset;
		bufferCopy.dstOffset = dstOffset;
		bufferCopy.size = size;

		vkCmdCopyBuffer(
			m_handle,
			src.handle(),
			dst.handle(),
			1, &bufferCopy
		);
	}

	void
	CommandBuffer::copy (const Image & src, const Image & dst) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !src.isCreated() )
			{
				Tracer::error(ClassId, "The source image is not created.");

				return;
			}

			if ( !dst.isCreated() )
			{
				Tracer::error(ClassId, "The destination image is not created.");

				return;
			}

			if ( !this->isCreated() )
			{
				TraceError{ClassId} <<
					"The command buffer is not created !" "\n"
					"Unable to copy image " << src.handle() << " to image " << dst.handle();

				return;
			}
		}

		/* FIXME: Check parameters and what to expose for a correct copy. */
		VkImageCopy imageCopy{};
		imageCopy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopy.srcSubresource.mipLevel = 0;
		imageCopy.srcSubresource.baseArrayLayer = 0;
		imageCopy.srcSubresource.layerCount = src.createInfo().arrayLayers;
		imageCopy.srcOffset = {.x=0, .y=0, .z=0};
		imageCopy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopy.dstSubresource.mipLevel = 0;
		imageCopy.dstSubresource.baseArrayLayer = 0;
		imageCopy.dstSubresource.layerCount = dst.createInfo().arrayLayers;
		imageCopy.dstOffset = {.x=0, .y=0, .z=0};
		/* FIXME: Use the source image extent for now. */
		imageCopy.extent = src.createInfo().extent; //

		/* FIXME: Check the image layout consistency with the operation. */
		vkCmdCopyImage(
			m_handle,
			src.handle(), src.createInfo().initialLayout,
			dst.handle(), dst.createInfo().initialLayout,
			1, &imageCopy
		);
	}

	void
	CommandBuffer::copy (const Buffer & src, const Image & dst, VkDeviceSize /*srcOffset*/) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !src.isCreated() )
			{
				Tracer::error(ClassId, "The source buffer is not created.");

				return;
			}

			if ( !dst.isCreated() )
			{
				Tracer::error(ClassId, "The destination image is not created.");

				return;
			}

			if ( !this->isCreated() )
			{
				TraceError{ClassId} <<
					"The command buffer is not created !" "\n"
					"Unable to copy buffer " << src.handle() << " to image " << dst.handle();

				return;
			}
		}

		constexpr VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		const auto baseWidth = dst.createInfo().extent.width;
		const auto baseHeight = dst.createInfo().extent.height;
		const auto pixelBytes = dst.pixelBytes();
		const auto layerCount = dst.createInfo().arrayLayers;

		for ( uint32_t layerIndex = 0; layerIndex < layerCount; layerIndex++ )
		{
			const uint32_t layerOffset = layerIndex * (baseWidth * baseHeight * pixelBytes);

			/* FIXME: Check parameters and what to expose for a correct copy. */
			VkBufferImageCopy bufferImageCopy{};
			bufferImageCopy.bufferOffset = layerOffset;
			bufferImageCopy.bufferRowLength = 0;
			bufferImageCopy.bufferImageHeight = 0;
			bufferImageCopy.imageSubresource.aspectMask = aspectMask;
			bufferImageCopy.imageSubresource.mipLevel = 0;
			bufferImageCopy.imageSubresource.baseArrayLayer = layerIndex;
			bufferImageCopy.imageSubresource.layerCount = 1;
			bufferImageCopy.imageOffset.x = 0;
			bufferImageCopy.imageOffset.y = 0;
			bufferImageCopy.imageOffset.z = 0;
			bufferImageCopy.imageExtent.width = baseWidth;
			bufferImageCopy.imageExtent.height = baseHeight;
			bufferImageCopy.imageExtent.depth = 1;

			vkCmdCopyBufferToImage(
				m_handle,
				src.handle(),
				dst.handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &bufferImageCopy
			);
		}
	}

	void
	CommandBuffer::copy (const Image & src, const Buffer & dst) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !src.isCreated() )
			{
				Tracer::error(ClassId, "The source image is not created.");

				return;
			}

			if ( !dst.isCreated() )
			{
				Tracer::error(ClassId, "The destination buffer is not created.");

				return;
			}

			if ( !this->isCreated() )
			{
				TraceError{ClassId} <<
					"The command buffer is not created !" "\n"
					"Unable to copy image " << src.handle() << " to buffer " << dst.handle();

				return;
			}
		}

		/* FIXME: Check parameters and what to expose for a correct copy. */
		VkBufferImageCopy bufferImageCopy{};
		bufferImageCopy.bufferOffset = 0;
		bufferImageCopy.bufferRowLength = 0;
		bufferImageCopy.bufferImageHeight = 0;
		bufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferImageCopy.imageSubresource.mipLevel = 0;
		bufferImageCopy.imageSubresource.baseArrayLayer = 0;
		bufferImageCopy.imageSubresource.layerCount = src.createInfo().arrayLayers;
		bufferImageCopy.imageOffset = {.x=0, .y=0, .z=0};
		bufferImageCopy.imageExtent = src.createInfo().extent;

		/* FIXME: Check the image layout consistency with the operation. */
		vkCmdCopyImageToBuffer(
			m_handle,
			src.handle(), src.createInfo().initialLayout,
			dst.handle(),
			1, &bufferImageCopy
		);
	}

	void
	CommandBuffer::blit (const Image & src, const Image & dst) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !src.isCreated() )
			{
				Tracer::error(ClassId, "The source image is not created.");

				return;
			}

			if ( !dst.isCreated() )
			{
				Tracer::error(ClassId, "The destination image is not created.");

				return;
			}

			if ( !this->isCreated() )
			{
				TraceError{ClassId} <<
					"The command buffer is not created !" "\n"
					"Unable to blit image " << src.handle() << " to image " << dst.handle();

				return;
			}
		}

		/* FIXME: Pass parameters ! */
		VkImageBlit imageBlit{};
		imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlit.srcSubresource.mipLevel = 0;
		imageBlit.srcSubresource.baseArrayLayer = 0;
		imageBlit.srcSubresource.layerCount = src.createInfo().arrayLayers;
		imageBlit.srcOffsets[0] = {.x=0, .y=0, .z=0};
		imageBlit.srcOffsets[1] = {.x=0, .y=0, .z=0};
		imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlit.dstSubresource.mipLevel = 0;
		imageBlit.dstSubresource.baseArrayLayer = 0;
		imageBlit.dstSubresource.layerCount = dst.createInfo().arrayLayers;
		imageBlit.dstOffsets[0] = {.x=0, .y=0, .z=0};
		imageBlit.dstOffsets[1] = {.x=0, .y=0, .z=0};

		vkCmdBlitImage(
			m_handle,
			src.handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			dst.handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &imageBlit,
			VK_FILTER_LINEAR // VK_FILTER_CUBIC_EXT
		);
	}

	void
	CommandBuffer::clearColor (const Image & image, VkImageLayout imageLayout, const PixelFactory::Color< float > & color) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !image.isCreated() )
			{
				Tracer::error(ClassId, "The image is not created.");

				return;
			}

			if ( !this->isCreated() )
			{
				TraceError{ClassId} <<
					"The command buffer is not created !" "\n"
					"Unable to clear the color of image " << image.handle();

				return;
			}
		}

		/* FIXME: Check if the image is a color one. */
		VkClearColorValue colorValue{};
		colorValue.float32[0] = color.red();
		colorValue.float32[1] = color.green();
		colorValue.float32[2] = color.blue();
		colorValue.float32[3] = color.alpha();

		/* NOTE: The whole image as color for now. */
		VkImageSubresourceRange subresourceRange{};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = image.createInfo().mipLevels;
		subresourceRange.baseArrayLayer = 0;
		subresourceRange.layerCount = image.createInfo().arrayLayers;

		vkCmdClearColorImage(
			m_handle,
			image.handle(), imageLayout,
			&colorValue,
			1, &subresourceRange
		);
	}

	void
	CommandBuffer::clearDepthStencil (const Image & image, VkImageLayout imageLayout) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !image.isCreated() )
			{
				Tracer::error(ClassId, "The image is not created.");

				return;
			}

			if ( !this->isCreated() )
			{
				TraceError{ClassId} <<
					"The command buffer is not created !" "\n"
					"Unable to clear the depth and the stencil of image " << image.handle();

				return;
			}
		}

		/* FIXME: Check if the image is a color one. */
		/* FIXME: Set the possibility to choose the depth, or stencil and the value. */

		constexpr VkClearDepthStencilValue depthStencilValue{};

		/* NOTE: The whole image as depth/stencil for now. */
		VkImageSubresourceRange subresourceRange{};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = image.createInfo().mipLevels;
		subresourceRange.baseArrayLayer = 0;
		subresourceRange.layerCount = image.createInfo().arrayLayers;

		vkCmdClearDepthStencilImage(
			m_handle,
			image.handle(), imageLayout,
			&depthStencilValue,
			1, &subresourceRange
		);
	}

	void
	CommandBuffer::pipelineBarrier (const std::vector< VkMemoryBarrier > & memoryBarriers, const std::vector< VkBufferMemoryBarrier > & bufferMemoryBarriers, const std::vector< VkImageMemoryBarrier > & imageMemoryBarriers, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !this->isCreated() )
			{
				Tracer::error(ClassId, "The command buffer is not created !");

				return;
			}

			if ( memoryBarriers.empty() && bufferMemoryBarriers.empty() && imageMemoryBarriers.empty() )
			{
				Tracer::warning(ClassId, "There is no memory barrier in the list !");

				return;
			}
		}

		vkCmdPipelineBarrier(
			m_handle,
			srcStageMask,
			dstStageMask,
			dependencyFlags,
			static_cast< uint32_t >(memoryBarriers.size()), memoryBarriers.data(),
			static_cast< uint32_t >(bufferMemoryBarriers.size()), bufferMemoryBarriers.data(),
			static_cast< uint32_t >(imageMemoryBarriers.size()), imageMemoryBarriers.data()
		);
	}

	void
	CommandBuffer::pipelineBarrier (const std::vector< VkMemoryBarrier > & memoryBarriers, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !this->isCreated() )
			{
				Tracer::error(ClassId, "The command buffer is not created !");

				return;
			}

			if ( memoryBarriers.empty() )
			{
				Tracer::warning(ClassId, "There is no memory barrier in the list !");

				return;
			}
		}

		vkCmdPipelineBarrier(
			m_handle,
			srcStageMask,
			dstStageMask,
			dependencyFlags,
			static_cast< uint32_t >(memoryBarriers.size()), memoryBarriers.data(),
			0, VK_NULL_HANDLE,
			0, VK_NULL_HANDLE
		);
	}

	void
	CommandBuffer::pipelineBarrier (const std::vector< VkBufferMemoryBarrier > & bufferMemoryBarriers, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !this->isCreated() )
			{
				Tracer::error(ClassId, "The command buffer is not created !");

				return;
			}

			if ( bufferMemoryBarriers.empty() )
			{
				Tracer::warning(ClassId, "There is no buffer memory barrier in the list !");

				return;
			}
		}

		vkCmdPipelineBarrier(
			m_handle,
			srcStageMask,
			dstStageMask,
			dependencyFlags,
			0, VK_NULL_HANDLE,
			static_cast< uint32_t >(bufferMemoryBarriers.size()), bufferMemoryBarriers.data(),
			0, VK_NULL_HANDLE
		);
	}

	void
	CommandBuffer::pipelineBarrier (const std::vector< VkImageMemoryBarrier > & imageMemoryBarriers, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !this->isCreated() )
			{
				Tracer::error(ClassId, "The command buffer is not created !");

				return;
			}

			if ( imageMemoryBarriers.empty() )
			{
				Tracer::warning(ClassId, "There is no image memory barrier in the list !");

				return;
			}
		}

		vkCmdPipelineBarrier(
			m_handle,
			srcStageMask,
			dstStageMask,
			dependencyFlags,
			0, VK_NULL_HANDLE,
			0, VK_NULL_HANDLE,
			static_cast< uint32_t >(imageMemoryBarriers.size()), imageMemoryBarriers.data()
		);
	}

	void
	CommandBuffer::pipelineBarrier (const Sync::MemoryBarrier & memoryBarrier, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !this->isCreated() )
			{
				Tracer::error(ClassId, "The command buffer is not created !");

				return;
			}
		}

		vkCmdPipelineBarrier(
			m_handle,
			srcStageMask,
			dstStageMask,
			dependencyFlags,
			1, &memoryBarrier.get(),
			0, VK_NULL_HANDLE,
			0, VK_NULL_HANDLE
		);
	}

	void
	CommandBuffer::pipelineBarrier (const Sync::BufferMemoryBarrier & bufferMemoryBarrier, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !this->isCreated() )
			{
				Tracer::error(ClassId, "The command buffer is not created !");

				return;
			}
		}

		vkCmdPipelineBarrier(
			m_handle,
			srcStageMask,
			dstStageMask,
			dependencyFlags,
			0, VK_NULL_HANDLE,
			1, &bufferMemoryBarrier.get(),
			0, VK_NULL_HANDLE
		);
	}

	void
	CommandBuffer::pipelineBarrier (const Sync::ImageMemoryBarrier & imageMemoryBarrier, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !this->isCreated() )
			{
				Tracer::error(ClassId, "The command buffer is not created !");

				return;
			}
		}

		vkCmdPipelineBarrier(
			m_handle,
			srcStageMask,
			dstStageMask,
			dependencyFlags,
			0, VK_NULL_HANDLE,
			0, VK_NULL_HANDLE,
			1, &imageMemoryBarrier.get()
		);
	}

	void
	CommandBuffer::setEvent (const Sync::Event & event, VkPipelineStageFlags flags) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !event.isCreated() )
			{
				Tracer::error(ClassId, "The event is not created !");

				return;
			}

			if ( !this->isCreated() )
			{
				TraceError{ClassId} <<
					"The command buffer is not created !" "\n"
					"Unable to set event " << event.handle();

				return;
			}
		}

		vkCmdSetEvent(
			m_handle,
			event.handle(),
			flags
		);
	}

	void
	CommandBuffer::resetEvent (const Sync::Event & event, VkPipelineStageFlags flags) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !event.isCreated() )
			{
				Tracer::error(ClassId, "The event is not created !");

				return;
			}

			if ( !this->isCreated() )
			{
				TraceError{ClassId} <<
					"The command buffer is not created !" "\n"
					"Unable to reset event " << event.handle();

				return;
			}
		}

		vkCmdResetEvent(
			m_handle,
			event.handle(),
			flags
		);
	}

	void
	CommandBuffer::waitEvents (const std::vector< VkEvent > & events, VkPipelineStageFlags srcFlags, VkPipelineStageFlags dstFlags, const std::vector< VkMemoryBarrier > & memoryBarriers, const std::vector< VkBufferMemoryBarrier > & bufferMemoryBarriers, const std::vector< VkImageMemoryBarrier > & imageMemoryBarriers) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !this->isCreated() )
			{
				Tracer::error(ClassId, "The command buffer is not created !");

				return;
			}

			if ( events.empty() )
			{
				Tracer::warning(ClassId, "There is no event in the list !");

				return;
			}

			if ( events.empty() && memoryBarriers.empty() && bufferMemoryBarriers.empty() && imageMemoryBarriers.empty() )
			{
				Tracer::warning(ClassId, "There is no memory barrier in the list !");

				return;
			}
		}

		vkCmdWaitEvents(
			m_handle,
			static_cast< uint32_t >(events.size()), events.data(),
			srcFlags, dstFlags,
			static_cast< uint32_t >(memoryBarriers.size()), memoryBarriers.data(),
			static_cast< uint32_t >(bufferMemoryBarriers.size()), bufferMemoryBarriers.data(),
			static_cast< uint32_t >(imageMemoryBarriers.size()), imageMemoryBarriers.data()
		);
	}

	void
	CommandBuffer::bind (const GraphicsPipeline & graphicsPipeline) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !graphicsPipeline.isCreated() )
			{
				Tracer::error(ClassId, "The graphics pipeline is not created !");

				return;
			}

			if ( !this->isCreated() )
			{
				TraceError{ClassId} <<
					"The command buffer is not created !" "\n"
					"Unable to bind graphics pipeline " << graphicsPipeline.handle();

				return;
			}
		}

		vkCmdBindPipeline(
			m_handle,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			graphicsPipeline.handle()
		);
	}

	void
	CommandBuffer::bind (const ComputePipeline & computePipeline) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !computePipeline.isCreated() )
			{
				Tracer::error(ClassId, "The compute pipeline is not created !");

				return;
			}

			if ( !this->isCreated() )
			{
				TraceError{ClassId} <<
					"The command buffer is not created !" "\n"
					"Unable to bind compute pipeline " << computePipeline.handle();

				return;
			}
		}

		vkCmdBindPipeline(
			m_handle,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			computePipeline.handle()
		);
	}

	void
	CommandBuffer::bind (const VertexBufferObject & vertexBufferObject, VkDeviceSize offset) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !vertexBufferObject.isCreated() )
			{
				Tracer::error(ClassId, "The vertex buffer object is not created !");

				return;
			}

			if ( !this->isCreated() )
			{
				TraceError{ClassId} <<
					"The command buffer is not created !" "\n"
					"Unable to bind vertex buffer object " << vertexBufferObject.handle();

				return;
			}
		}

		constexpr uint32_t firstBinding = 0;

		const std::array< VkBuffer, 1 > vertexBuffers{
			vertexBufferObject.handle()
		};

		const std::array< VkDeviceSize, 1 > offsets{
			offset
		};

		vkCmdBindVertexBuffers(
			m_handle,
			firstBinding,
			static_cast< uint32_t >(vertexBuffers.size()), vertexBuffers.data(), offsets.data()
		);
	}

	void
	CommandBuffer::bind (const IndexBufferObject & indexBufferObject, VkDeviceSize offset, VkIndexType indexType) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !indexBufferObject.isCreated() )
			{
				Tracer::error(ClassId, "The index buffer object is not created !");

				return;
			}

			if ( !this->isCreated() )
			{
				TraceError{ClassId} <<
					"The command buffer is not created !" "\n"
					"Unable to bind index buffer object " << indexBufferObject.handle();

				return;
			}
		}

		vkCmdBindIndexBuffer(
			m_handle,
			indexBufferObject.handle(),
			offset,
			indexType
		);
	}

	void
	CommandBuffer::bind (const DescriptorSet & descriptorSet, const PipelineLayout & pipelineLayout, VkPipelineBindPoint bindPoint, uint32_t firstSet) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !pipelineLayout.isCreated() )
			{
				Tracer::error(ClassId, "The pipeline layout is not created !");

				return;
			}

			if ( !descriptorSet.isCreated() )
			{
				Tracer::error(ClassId, "The descriptor set is not created !");

				return;
			}

			if ( !this->isCreated() )
			{
				TraceError{ClassId} <<
					"The command buffer is not created !" "\n"
					"Unable to bind descriptor set " << descriptorSet.handle() << " with pipeline layout " << pipelineLayout.handle();

				return;
			}
		}

		auto * descriptorSetHandle = descriptorSet.handle();

		vkCmdBindDescriptorSets(
			m_handle,
			bindPoint,
			pipelineLayout.handle(),
			firstSet,
			1, &descriptorSetHandle,
			0, nullptr
		);
	}

	void
	CommandBuffer::bind (const DescriptorSet & descriptorSet, const PipelineLayout & pipelineLayout, VkPipelineBindPoint bindPoint, uint32_t firstSet, uint32_t dynamicOffset) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !pipelineLayout.isCreated() )
			{
				Tracer::error(ClassId, "The pipeline layout is not created !");

				return;
			}

			if ( !descriptorSet.isCreated() )
			{
				Tracer::error(ClassId, "The descriptor set is not created !");

				return;
			}

			if ( !this->isCreated() )
			{
				TraceError{ClassId} <<
					"The command buffer is not created !" "\n"
					"Unable to bind descriptor set " << descriptorSet.handle() << " with pipeline layout " << pipelineLayout.handle();

				return;
			}
		}

		auto * descriptorSetHandle = descriptorSet.handle();

		vkCmdBindDescriptorSets(
			m_handle,
			bindPoint,
			pipelineLayout.handle(),
			firstSet,
			1, &descriptorSetHandle,
			1, &dynamicOffset
		);
	}

	void
	CommandBuffer::bind (const Graphics::Geometry::Interface & geometry, uint32_t /*subGeometryIndex*/) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !this->isCreated() )
			{
				TraceError{ClassId} <<
					"The command buffer is not created !" "\n"
					"Unable to bind geometry '" << geometry.name() << "'";

				return;
			}
		}

		{
			auto * vertexBufferObjectHandle = geometry.vertexBufferObject()->handle();
			constexpr VkDeviceSize offset = 0;

			vkCmdBindVertexBuffers(
				m_handle,
				0,
				1, &vertexBufferObjectHandle,
				&offset
			);
		}

		if ( geometry.useIndexBuffer() )
		{
			constexpr VkDeviceSize offset = 0;
			constexpr VkIndexType indexType = VK_INDEX_TYPE_UINT32;

			vkCmdBindIndexBuffer(
				m_handle,
				geometry.indexBufferObject()->handle(),
				offset,
				indexType
			);
		}
	}

	void
	CommandBuffer::bind (const Graphics::Geometry::Interface & geometry, const VertexBufferObject & modelVBO, uint32_t subGeometryIndex, VkDeviceSize modelVBOOffset) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !this->isCreated() )
			{
				TraceError{ClassId} <<
					"The command buffer is not created !" "\n"
					"Unable to bind geometry '" << geometry.name() << "' with model VBO " << modelVBO.handle();

				return;
			}
		}

		/* Binding vertex buffer object if exists. */
		{
			const auto range = geometry.subGeometryRange(subGeometryIndex);
			constexpr uint32_t firstBinding = 0;

			const std::array< VkBuffer, 2 > vertexBuffers{
				geometry.vertexBufferObject()->handle(),
				modelVBO.handle()
			};

			const std::array< VkDeviceSize, 2 > offsets{
				range[0],
				modelVBOOffset
			};

			vkCmdBindVertexBuffers(
				m_handle,
				firstBinding,
				static_cast< uint32_t >(vertexBuffers.size()), vertexBuffers.data(), offsets.data()
			);
		}

		/* Binding index buffer object if exists. */
		if ( geometry.useIndexBuffer() )
		{
			constexpr VkDeviceSize offset = 0;
			constexpr VkIndexType indexType = VK_INDEX_TYPE_UINT32;

			vkCmdBindIndexBuffer(
				m_handle,
				geometry.indexBufferObject()->handle(),
				offset,
				indexType
			);
		}
	}

	void
	CommandBuffer::draw (const Graphics::Geometry::Interface & geometry, uint32_t subGeometryIndex, uint32_t instanceCount) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !this->isCreated() )
			{
				TraceError{ClassId} << "The command buffer is not created for the geometry '" << geometry.name() << "' !";

				return;
			}

			if ( instanceCount == 0 )
			{
				TraceError{ClassId} << "No instance count for the geometry '" << geometry.name() << "' !";

				return;
			}

			if ( !geometry.isCreated() )
			{
				TraceError{ClassId} << "The geometry '" << geometry.name() << "' is not created !";

				return;
			}
		}

		const auto range = geometry.subGeometryRange(subGeometryIndex);

		if ( geometry.useIndexBuffer() )
		{
			constexpr int32_t vertexOffset{0};
			constexpr uint32_t firstInstance{0};

			vkCmdDrawIndexed(
				m_handle,
				range[1],
				instanceCount,
				range[0],
				vertexOffset,
				firstInstance
			);
		}
		else
		{
			constexpr uint32_t firstVertex{0};
			constexpr uint32_t firstInstance{0};

			vkCmdDraw(
				m_handle,
				range[1],
				instanceCount,
				firstVertex,
				firstInstance
			);
		}
	}
}

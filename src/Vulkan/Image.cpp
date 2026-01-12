/*
 * src/Vulkan/Image.cpp
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

#include "Image.hpp"

/* STL inclusions. */
#include <numeric>
#include <ranges>

/* Local inclusions. */
#include "Graphics/CubemapResource.hpp"
#include "Graphics/ImageResource.hpp"
#include "Graphics/MovieResource.hpp"
#include "Device.hpp"
#include "TransferManager.hpp"
#include "MemoryRegion.hpp"
#include "Utility.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace Libs;

	std::shared_ptr< Image >
	Image::createFromSwapChain (const std::shared_ptr< Device > & device, VkImage handle, const VkSwapchainCreateInfoKHR & createInfo) noexcept
	{
		auto swapChainImage = std::make_shared< Vulkan::Image >(
			device,
			VK_IMAGE_TYPE_2D,
			createInfo.imageFormat,
			VkExtent3D{createInfo.imageExtent.width, createInfo.imageExtent.height, 1},
			createInfo.imageUsage,
			0,
			1,
			createInfo.imageArrayLayers
		);
		swapChainImage->setIdentifier(ClassId, "OSBuffer", "Image");

		/* NOTE: Set internal values manually and declare the image as created. */
		swapChainImage->m_handle = handle;
		swapChainImage->m_isSwapChainImage = true;
		swapChainImage->setCreated();

		return swapChainImage;
	}

	bool
	Image::createOnHardware () noexcept
	{
		// NOTE: Special case for swap chain images.
		if ( m_isSwapChainImage )
		{
			Tracer::error(ClassId, "This is an image provided by the swap chain ! No need to create it.");

			this->setCreated();

			return true;
		}

		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to create this image !");

			return false;
		}

		const auto result =
			this->device()->useMemoryAllocator() ?
			this->createWithVMA() :
			this->createManually();

		if ( !result )
		{
			this->destroyFromHardware();

			return false;
		}

		this->setCreated();

		return true;
	}

	bool
	Image::destroyFromHardware () noexcept
	{
		/* NOTE: The OS destroys the swap chain image. */
		if ( m_isSwapChainImage )
		{
			m_handle = VK_NULL_HANDLE;

			this->setDestroyed();

			return true;
		}

		const auto result =
			this->device()->useMemoryAllocator() ?
			this->destroyWithVMA() :
			this->destroyManually();

		if ( !result )
		{
			return false;
		}

		this->setDestroyed();

		return true;
	}

	bool
	Image::createManually () noexcept
	{
		// 1. Create the hardware image.
		if ( const auto result = vkCreateImage(this->device()->handle(), &m_createInfo, VK_NULL_HANDLE, &m_handle); result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to create an image : " << vkResultToCString(result) << " !";

			return false;
		}

		// 2. Allocate memory for the new image.
		VkImageMemoryRequirementsInfo2 info{};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2;
		info.pNext = VK_NULL_HANDLE;
		info.image = m_handle;

		VkMemoryRequirements2 memoryRequirement{};
		memoryRequirement.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
		memoryRequirement.pNext = VK_NULL_HANDLE;

		vkGetImageMemoryRequirements2(
			this->device()->handle(),
			&info,
			&memoryRequirement
		);

		const auto memoryProperties = m_hostVisible ?
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT :
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		m_deviceMemory = std::make_unique< DeviceMemory >(this->device(), memoryRequirement, memoryProperties);
		m_deviceMemory->setIdentifier(ClassId, this->identifier(), "DeviceMemory");

		if ( !m_deviceMemory->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create a device memory for the image " << m_handle << " !";

			return false;
		}

		// 3. Bind the image to the device memory.
		if ( const auto result = vkBindImageMemory(this->device()->handle(), m_handle, m_deviceMemory->handle(), 0); result != VK_SUCCESS )
		{
			TraceError{ClassId} <<
				"Unable to bind the image " << m_handle << " to the device memory " << m_deviceMemory->handle() <<
				" : " << vkResultToCString(result) << " !";

			return false;
		}

		return true;
	}

	bool
	Image::destroyManually () noexcept
	{
		if ( !this->hasDevice() )
		{
			TraceError{ClassId} << "No device to destroy the image " << m_handle << " (" << this->identifier() << ") !";

			return false;
		}

		if ( m_deviceMemory != nullptr )
		{
			m_deviceMemory.reset();
		}

		if ( m_handle != VK_NULL_HANDLE )
		{
			vkDestroyImage(this->device()->handle(), m_handle, VK_NULL_HANDLE);

			m_handle = VK_NULL_HANDLE;
		}

		return true;
	}

	bool
	Image::createWithVMA () noexcept
	{
		VmaAllocationCreateInfo allocInfo{};
		if ( m_hostVisible )
		{
			allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		}
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		//allocInfo.requiredFlags = 0;
		//allocInfo.preferredFlags = 0;
		//allocInfo.memoryTypeBits = 0;
		//allocInfo.pool = VK_NULL_HANDLE; /* Default pool. */
		//allocInfo.pUserData = nullptr;
		//allocInfo.priority = 0.5F;

		/* Bind the buffer to the device memory */
		if ( const auto result = vmaCreateImage(this->device()->memoryAllocatorHandle(), &m_createInfo, &allocInfo, &m_handle, &m_memoryAllocation, nullptr); result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to create an image with VMA : " << vkResultToCString(result) << " !";

			return false;
		}

		return true;
	}

	bool
	Image::destroyWithVMA () noexcept
	{
		if ( m_handle != VK_NULL_HANDLE )
		{
			vmaDestroyImage(this->device()->memoryAllocatorHandle(), m_handle, m_memoryAllocation);

			m_handle = VK_NULL_HANDLE;
		}

		return true;
	}

	bool
	Image::create (TransferManager & transferManager, const PixelFactory::Pixmap< uint8_t > & pixmap) noexcept
	{
		if ( !pixmap.isValid() )
		{
			Tracer::error(ClassId, "The pixmap data ara invalid! Skipping transfer ...");

			return false;
		}

		if ( !this->createOnHardware() )
		{
			return false;
		}

		return transferManager.uploadImage(*this, pixmap.bytes(), [&pixmap] (const Buffer & stagingBuffer) {
			return stagingBuffer.writeData({pixmap.data().data(), pixmap.bytes()});
		});
	}

	bool
	Image::create (TransferManager & transferManager, const std::shared_ptr< Graphics::ImageResource > & imageResource) noexcept
	{
		if ( imageResource == nullptr || !imageResource->isLoaded() )
		{
			Tracer::error(ClassId, "The image resource is null or not loaded! Skipping transfer ...");

			return false;
		}

		return this->create(transferManager, imageResource->data());
	}

	bool
	Image::create (TransferManager & transferManager, const std::shared_ptr< Graphics::CubemapResource > & cubemapResource) noexcept
	{
		if ( cubemapResource == nullptr || !cubemapResource->isLoaded() )
		{
			Tracer::error(ClassId, "The image resource is null or not loaded! Skipping transfer ...");

			return false;
		}

		if ( !this->createOnHardware() )
		{
			return false;
		}

		const auto & pixmaps = cubemapResource->faces();

		/* Get the total bytes requested for the 6 faces. */
		const size_t totalBytes = std::accumulate(pixmaps.cbegin(), pixmaps.cend(), 0, [] (auto sum, const auto & pixmap) {
			return sum + pixmap.bytes();
		});

		/* NOTE: We will write all 6 pixmaps next to each others in the staging buffer. */
		return transferManager.uploadImage(*this, totalBytes, [&pixmaps] (const Buffer & stagingBuffer) {
			size_t offset = 0;

			for ( const auto & pixmap : pixmaps )
			{
				if ( !stagingBuffer.writeData({pixmap.data().data(), pixmap.bytes(), offset}) )
				{
					TraceError{ClassId} << "Unable to write " << pixmap.bytes() << " bytes of data in the staging buffer !";

					return false;
				}

				offset += pixmap.bytes();
			}

			return true;
		});
	}

	bool
	Image::create (TransferManager & transferManager, const std::shared_ptr< Graphics::MovieResource > & movieResource) noexcept
	{
		if ( !this->createOnHardware() )
		{
			return false;
		}

		const auto & frames = movieResource->frames();

		const size_t totalBytes = std::accumulate(frames.cbegin(), frames.cend(), 0, [] (auto sum, const auto & frame) {
			return sum + frame.first.bytes();
		});

		return transferManager.uploadImage(*this, totalBytes, [&frames] (const Buffer & stagingBuffer) {
			size_t offset = 0;

			for ( const auto & pixmap : std::ranges::views::keys(frames) )
			{
				if ( !stagingBuffer.writeData({pixmap.data().data(), pixmap.bytes(), offset}) )
				{
					TraceError{ClassId} << "Unable to write " << pixmap.bytes() << " bytes of data in the staging buffer !";

					return false;
				}

				offset += pixmap.bytes();
			}

			return true;
		});
	}

	bool
	Image::writeData (TransferManager & transferManager, const MemoryRegion & memoryRegion) noexcept
	{
		if ( !this->isCreated() )
		{
			Tracer::error(ClassId, "The image is not created ! Use one of the Image::create() methods first.");

			return false;
		}

		return transferManager.uploadImage(*this, memoryRegion.bytes(), [&memoryRegion] (const Buffer & stagingBuffer) {
			return stagingBuffer.writeData(memoryRegion);
		});
	}

	void *
	Image::mapMemory () const noexcept
	{
		if ( !this->isHostVisible() )
		{
			Tracer::error(ClassId, "This image is not host visible! You can't map it.");

			return nullptr;
		}

		if ( m_memoryAllocation != VK_NULL_HANDLE )
		{
			void * pointer = nullptr;

			if ( const auto result = vmaMapMemory(this->device()->memoryAllocatorHandle(), m_memoryAllocation, &pointer); result != VK_SUCCESS )
			{
				TraceError{ClassId} << "Unable to map (VMA) the image memory.";

				return nullptr;
			}

			return pointer;
		}

		return m_deviceMemory->mapMemory(0, VK_WHOLE_SIZE);
	}

	void
	Image::unmapMemory () const noexcept
	{
		if ( !this->isHostVisible() )
		{
			return;
		}

		if ( m_memoryAllocation != VK_NULL_HANDLE )
		{
			const auto allocator = this->device()->memoryAllocatorHandle();

			vmaFlushAllocation(allocator, m_memoryAllocation, 0, VK_WHOLE_SIZE);

			vmaUnmapMemory(allocator, m_memoryAllocation);
		}
		else
		{
			m_deviceMemory->unmapMemory();
		}
	}

	VkDeviceSize
	Image::rowPitch () const noexcept
	{
		if ( m_createInfo.tiling != VK_IMAGE_TILING_LINEAR )
		{
			return 0;
		}

		VkImageSubresource subresource{};
		subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresource.mipLevel = 0;
		subresource.arrayLayer = 0;

		VkSubresourceLayout layout{};
		vkGetImageSubresourceLayout(this->device()->handle(), m_handle, &subresource, &layout);

		return layout.rowPitch;
	}
}

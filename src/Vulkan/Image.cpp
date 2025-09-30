/*
 * src/Vulkan/Image.cpp
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

#include "Image.hpp"

/* STL inclusions. */
#include <cstdint>
#include <mutex>
#include <numeric>
#include <ranges>

/* Local inclusions. */
#include "Graphics/CubemapResource.hpp"
#include "Graphics/ImageResource.hpp"
#include "Graphics/MovieResource.hpp"
#include "Device.hpp"
#include "DeviceMemory.hpp"
#include "MemoryRegion.hpp"
#include "TransferManager.hpp"
#include "Utility.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace EmEn::Libs;

	std::shared_ptr< Image >
	Image::createFromSwapChain (const std::shared_ptr< Device > & device, VkImage handle, const VkSwapchainCreateInfoKHR & createInfo) noexcept
	{
		auto swapChainImage = std::make_shared< Image >(
			device,
			VK_IMAGE_TYPE_2D, // Image type
			createInfo.imageFormat, // Image format (bits description)
			VkExtent3D{createInfo.imageExtent.width, createInfo.imageExtent.height, 1}, // Image extent
			createInfo.imageUsage, // Image usage (color, depth, ...)
			VK_IMAGE_LAYOUT_UNDEFINED, // Layout
			0, // flags
			1, // Image mip levels
			createInfo.imageArrayLayers, // Image array layers
			VK_SAMPLE_COUNT_1_BIT, // Image multi sampling
			VK_IMAGE_TILING_OPTIMAL // Image tiling
		);
		swapChainImage->setIdentifier(ClassId, "OSBuffer", "Image");

		/* NOTE: Set internal values manually and declare the image as created. */
		swapChainImage->m_handle = handle;
		swapChainImage->m_flags[IsSwapChainImage] = true;
		swapChainImage->setCreated();

		return swapChainImage;
	}

	bool
	Image::createOnHardware () noexcept
	{
		/* NOTE: Special case for swap chain images. */
		if ( m_flags[IsSwapChainImage] )
		{
			Tracer::error(ClassId, "This is an image provided by the swap chain ! No need to create it.");

			return true;
		}

		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to create this image !");

			return false;
		}

		/* 1. Create the hardware image. */
		auto result = vkCreateImage(
			this->device()->handle(),
			&m_createInfo,
			VK_NULL_HANDLE,
			&m_handle
		);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to create an image : " << vkResultToCString(result) << " !";

			return false;
		}

		/* Allocate memory for the new image. */
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

		m_deviceMemory = std::make_unique< DeviceMemory >(this->device(), memoryRequirement, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		m_deviceMemory->setIdentifier(ClassId, this->identifier(), "DeviceMemory");

		if ( !m_deviceMemory->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create a device memory for the image " << m_handle << " !";

			this->destroyFromHardware();

			return false;
		}

		/* 3. Bind the image to the device memory. */
		result = vkBindImageMemory(
			this->device()->handle(),
			m_handle,
			m_deviceMemory->handle(),
			0// offset
		);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} <<
				"Unable to bind the image " << m_handle << " to the device memory " << m_deviceMemory->handle() <<
				" : " << vkResultToCString(result) << " !";

			this->destroyFromHardware();

			return false;
		}

		this->setCreated();

		return true;
	}

	bool
	Image::create (TransferManager & transferManager, const PixelFactory::Pixmap< uint8_t > & pixmap) noexcept
	{
		if ( !this->createOnHardware() )
		{
			return false;
		}

		return transferManager.transferImage(*this, pixmap.bytes(), [&pixmap] (const Buffer & stagingBuffer) {
			return stagingBuffer.writeData({pixmap.data().data(), pixmap.bytes()});
		});
	}

	bool
	Image::create (TransferManager & transferManager, const std::shared_ptr< Graphics::ImageResource > & imageResource) noexcept
	{
		return this->create(transferManager, imageResource->data());
	}

	bool
	Image::create (TransferManager & transferManager, const std::shared_ptr< Graphics::CubemapResource > & cubemapResource) noexcept
	{
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
		return transferManager.transferImage(*this, totalBytes, [&pixmaps] (const Buffer & stagingBuffer) {
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

		return transferManager.transferImage(*this, totalBytes, [&frames] (const Buffer & stagingBuffer) {
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

		return transferManager.transferImage(*this, memoryRegion.bytes(), [&memoryRegion] (const Buffer & stagingBuffer) {
			return stagingBuffer.writeData(memoryRegion);
		});
	}

	bool
	Image::destroyFromHardware () noexcept
	{
		/* NOTE: The OS destroys the swap chain image. */
		if ( m_flags[IsSwapChainImage] )
		{
			m_handle = VK_NULL_HANDLE;

			this->setDestroyed();

			return true;
		}

		if ( !this->hasDevice() )
		{
			TraceError{ClassId} << "No device to destroy the image " << m_handle << " (" << this->identifier() << ") !";

			return false;
		}

		if ( m_deviceMemory != nullptr )
		{
			m_deviceMemory->destroyFromHardware();
			m_deviceMemory.reset();
		}

		if ( m_handle != VK_NULL_HANDLE )
		{
			vkDestroyImage(this->device()->handle(), m_handle, VK_NULL_HANDLE);

			m_handle = VK_NULL_HANDLE;
		}

		this->setDestroyed();

		return true;
	}
}

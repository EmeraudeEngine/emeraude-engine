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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "Image.hpp"

/* STL inclusions. */
#include <numeric>
#include <ranges>

#if IS_WINDOWS
/* NOTE: Win32 external-memory import (VkImportMemoryWin32HandleInfoKHR lives in vulkan_win32.h,
 * which requires the Windows API types). Project-wide WIN32_LEAN_AND_MEAN/NOMINMAX apply. */
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#endif

#if IS_MACOS
/* NOTE: IOSurface import through VK_EXT_metal_objects (VkImportMetalIOSurfaceInfoEXT lives in
 * vulkan_metal.h). The header is self-contained in C++: it forward-declares IOSurfaceRef itself,
 * so no IOSurface framework header is needed here. */
#include <vulkan/vulkan_metal.h>
#endif

/* Local inclusions. */
#include "Device.hpp"
#include "Graphics/CubemapMovieResource.hpp"
#include "Graphics/CubemapResource.hpp"
#include "Graphics/ImageResource.hpp"
#include "Graphics/MovieResource.hpp"
#include "MemoryRegion.hpp"
#include "Tracer.hpp"
#include "TransferManager.hpp"
#include "Utility.hpp"

namespace EmEn::Vulkan
{
	using namespace Base;

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

#if IS_WINDOWS
	std::shared_ptr< Image >
	Image::importFromWin32Handle (const std::shared_ptr< Device > & device, const ExternalImageDescriptor & descriptor) noexcept
	{
		if ( descriptor.handleType != ExternalImageDescriptor::HandleType::Win32D3D11Texture || !descriptor.isValid() )
		{
			Tracer::error(ClassId, "Invalid external image descriptor for a Win32 D3D11 texture import !");

			return nullptr;
		}

		if ( !device->externalMemoryWin32Enabled() )
		{
			Tracer::error(ClassId, "VK_KHR_external_memory_win32 is not enabled on this device ! Unable to import the external image.");

			return nullptr;
		}

		/* 1. Create the image handle with the external-memory declaration chained.
		 * NOTE: The pNext chain points to the stack and is reset right after creation. */
		VkExternalMemoryImageCreateInfo externalMemoryCreateInfo{};
		externalMemoryCreateInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
		externalMemoryCreateInfo.pNext = nullptr;
		externalMemoryCreateInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT;

		auto importedImage = std::make_shared< Image >(
			device,
			VK_IMAGE_TYPE_2D,
			descriptor.format,
			VkExtent3D{descriptor.width, descriptor.height, 1},
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT
		);
		importedImage->setIdentifier(ClassId, "ExternalD3D11Texture", "Image");
		importedImage->m_isImportedImage = true;
		importedImage->m_createInfo.pNext = &externalMemoryCreateInfo;

		if ( const auto result = vkCreateImage(device->handle(), &importedImage->m_createInfo, nullptr, &importedImage->m_handle); result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to create the external image : " << vkResultToCString(result) << " !";

			return nullptr;
		}

		importedImage->m_createInfo.pNext = nullptr;

		/* 2. Query the memory requirements (external images typically require a dedicated allocation). */
		VkImageMemoryRequirementsInfo2 requirementsInfo{};
		requirementsInfo.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2;
		requirementsInfo.pNext = nullptr;
		requirementsInfo.image = importedImage->m_handle;

		VkMemoryDedicatedRequirements dedicatedRequirements{};
		dedicatedRequirements.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS;
		dedicatedRequirements.pNext = nullptr;

		VkMemoryRequirements2 memoryRequirements{};
		memoryRequirements.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
		memoryRequirements.pNext = &dedicatedRequirements;

		vkGetImageMemoryRequirements2(device->handle(), &requirementsInfo, &memoryRequirements);

		/* NOTE: The requirement struct is copied into DeviceMemory — detach the stack pNext chain first. */
		memoryRequirements.pNext = nullptr;

		/* 3. Import the D3D11 texture memory as a dedicated allocation (bypasses VMA — it cannot import external memory). */
		VkImportMemoryWin32HandleInfoKHR importInfo{};
		importInfo.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR;
		importInfo.pNext = nullptr;
		importInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT;
		importInfo.handle = static_cast< HANDLE >(descriptor.win32Handle);
		importInfo.name = nullptr;

		VkMemoryDedicatedAllocateInfo dedicatedAllocateInfo{};
		dedicatedAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
		dedicatedAllocateInfo.pNext = &importInfo;
		dedicatedAllocateInfo.image = importedImage->m_handle;
		dedicatedAllocateInfo.buffer = VK_NULL_HANDLE;

		/* NOTE: The chain lives on this stack frame — DeviceMemory only reads it inside createOnHardware() below. */
		importedImage->m_deviceMemory = std::make_unique< DeviceMemory >(device, memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &dedicatedAllocateInfo);
		importedImage->m_deviceMemory->setIdentifier(ClassId, "ExternalD3D11Texture", "DeviceMemory");

		if ( !importedImage->m_deviceMemory->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to import the external D3D11 texture memory (handle @" << descriptor.win32Handle << ") !";

			/* NOTE: The image handle is destroyed by destroyFromHardware() through the manual path. */
			return nullptr;
		}

		/* 4. Bind the image to the imported memory. */
		if ( const auto result = vkBindImageMemory(device->handle(), importedImage->m_handle, importedImage->m_deviceMemory->handle(), 0); result != VK_SUCCESS )
		{
			TraceError{ClassId} <<
				"Unable to bind the external image " << importedImage->m_handle << " to the imported device memory " << importedImage->m_deviceMemory->handle() <<
				" : " << vkResultToCString(result) << " !";

			return nullptr;
		}

		importedImage->setVulkanObjectName(device->handle(), VK_OBJECT_TYPE_IMAGE, reinterpret_cast< uint64_t >(importedImage->m_handle));

		/* NOTE: The content already exists on the GPU, but from Vulkan's point of view the layout is
		 * undefined until the acquire barrier (VK_QUEUE_FAMILY_EXTERNAL → graphics queue) is recorded. */
		importedImage->m_currentImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		importedImage->setCreated();

		return importedImage;
	}
#endif

#if IS_MACOS
	std::shared_ptr< Image >
	Image::importFromIOSurface (const std::shared_ptr< Device > & device, const ExternalImageDescriptor & descriptor) noexcept
	{
		if ( descriptor.handleType != ExternalImageDescriptor::HandleType::IOSurface || !descriptor.isValid() )
		{
			Tracer::error(ClassId, "Invalid external image descriptor for an IOSurface import !");

			return nullptr;
		}

		if ( !device->metalObjectsEnabled() )
		{
			Tracer::error(ClassId, "VK_EXT_metal_objects is not enabled on this device ! Unable to import the external image.");

			return nullptr;
		}

		/* 1. Create the image handle with the IOSurface import chained. Unlike the Win32 path this
		 * is NOT Vulkan external memory: MoltenVK builds the backing MTLTexture directly from the
		 * IOSurface at image creation (VK_EXT_metal_objects).
		 * NOTE: The pNext chain points to the stack and is reset right after creation. */
		VkImportMetalIOSurfaceInfoEXT importIOSurfaceInfo{};
		importIOSurfaceInfo.sType = VK_STRUCTURE_TYPE_IMPORT_METAL_IO_SURFACE_INFO_EXT;
		importIOSurfaceInfo.pNext = nullptr;
		importIOSurfaceInfo.ioSurface = static_cast< IOSurfaceRef >(descriptor.ioSurface);

		auto importedImage = std::make_shared< Image >(
			device,
			VK_IMAGE_TYPE_2D,
			descriptor.format,
			VkExtent3D{descriptor.width, descriptor.height, 1},
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT
		);
		importedImage->setIdentifier(ClassId, "ExternalIOSurface", "Image");
		importedImage->m_isImportedImage = true;
		importedImage->m_createInfo.pNext = &importIOSurfaceInfo;

		if ( const auto result = vkCreateImage(device->handle(), &importedImage->m_createInfo, nullptr, &importedImage->m_handle); result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to create the external image : " << vkResultToCString(result) << " !";

			return nullptr;
		}

		importedImage->m_createInfo.pNext = nullptr;

		/* 2. Query the memory requirements (dedicated allocation, mirrors the Win32 path). */
		VkImageMemoryRequirementsInfo2 requirementsInfo{};
		requirementsInfo.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2;
		requirementsInfo.pNext = nullptr;
		requirementsInfo.image = importedImage->m_handle;

		VkMemoryDedicatedRequirements dedicatedRequirements{};
		dedicatedRequirements.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS;
		dedicatedRequirements.pNext = nullptr;

		VkMemoryRequirements2 memoryRequirements{};
		memoryRequirements.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
		memoryRequirements.pNext = &dedicatedRequirements;

		vkGetImageMemoryRequirements2(device->handle(), &requirementsInfo, &memoryRequirements);

		/* NOTE: The requirement struct is copied into DeviceMemory — detach the stack pNext chain first. */
		memoryRequirements.pNext = nullptr;

		/* 3. Allocate a dedicated device memory and bind it. There is no memory-import struct in
		 * VK_EXT_metal_objects (the import happened at image creation): the allocation only
		 * satisfies the Vulkan binding contract — MoltenVK keeps the IOSurface as the actual
		 * texture storage. Bypass VMA like the Win32 path (dedicated allocation). */
		VkMemoryDedicatedAllocateInfo dedicatedAllocateInfo{};
		dedicatedAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
		dedicatedAllocateInfo.pNext = nullptr;
		dedicatedAllocateInfo.image = importedImage->m_handle;
		dedicatedAllocateInfo.buffer = VK_NULL_HANDLE;

		/* NOTE: The chain lives on this stack frame — DeviceMemory only reads it inside createOnHardware() below. */
		importedImage->m_deviceMemory = std::make_unique< DeviceMemory >(device, memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &dedicatedAllocateInfo);
		importedImage->m_deviceMemory->setIdentifier(ClassId, "ExternalIOSurface", "DeviceMemory");

		if ( !importedImage->m_deviceMemory->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to allocate the dedicated memory for the imported IOSurface (@" << descriptor.ioSurface << ") !";

			/* NOTE: The image handle is destroyed by destroyFromHardware() through the manual path. */
			return nullptr;
		}

		/* 4. Bind the image to the dedicated memory. */
		if ( const auto result = vkBindImageMemory(device->handle(), importedImage->m_handle, importedImage->m_deviceMemory->handle(), 0); result != VK_SUCCESS )
		{
			TraceError{ClassId} <<
				"Unable to bind the external image " << importedImage->m_handle << " to the dedicated device memory " << importedImage->m_deviceMemory->handle() <<
				" : " << vkResultToCString(result) << " !";

			return nullptr;
		}

		importedImage->setVulkanObjectName(device->handle(), VK_OBJECT_TYPE_IMAGE, reinterpret_cast< uint64_t >(importedImage->m_handle));

		/* NOTE: The content already exists on the GPU (the IOSurface is the storage). Vulkan-wise the
		 * layout is undefined until the first transition; MoltenVK layout transitions are Metal no-ops,
		 * so the UNDEFINED→TRANSFER_SRC transition in the consumer preserves the pixels. */
		importedImage->m_currentImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		importedImage->setCreated();

		return importedImage;
	}
#endif

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

		// NOTE: Special case for imported (external-memory) images: they are fully built by their import factory.
		if ( m_isImportedImage )
		{
			Tracer::error(ClassId, "This image was imported from an external memory handle ! It is created by its import factory.");

			return this->isCreated();
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

		this->setVulkanObjectName(this->device()->handle(), VK_OBJECT_TYPE_IMAGE, reinterpret_cast< uint64_t >(m_handle));

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

		/* NOTE: Imported images (external memory) never belong to VMA — always destroy them manually. */
		const auto result =
			this->device()->useMemoryAllocator() && !m_isImportedImage ?
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

		if ( m_hostVisible )
		{
			/* NOTE: Proof of where VMA actually placed a host-visible (mappable) image: DEVICE_LOCAL
			 * means VRAM (Resizable BAR), otherwise system RAM sampled across PCIe. */
			VkMemoryPropertyFlags chosenFlags = 0;
			vmaGetAllocationMemoryProperties(this->device()->memoryAllocatorHandle(), m_memoryAllocation, &chosenFlags);
			TraceInfo{ClassId} << "Host-visible image '" << this->identifier() << "' placed by VMA in " << ((chosenFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ? "DEVICE_LOCAL (VRAM)" : "host (system RAM)") << " memory.";
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

		/* HDR cubemap: the faces are raw RGBA16F texel vectors. */
		if ( cubemapResource->isHDR() )
		{
			const size_t faceBytes = static_cast< size_t >(cubemapResource->cubeSize()) * cubemapResource->cubeSize() * 4 * sizeof(uint16_t);
			const size_t totalBytes = faceBytes * Graphics::CubemapFaceCount;

			return transferManager.uploadImage(*this, totalBytes, [&cubemapResource, faceBytes] (const Buffer & stagingBuffer) {
				size_t offset = 0;

				for ( size_t faceIndex = 0; faceIndex < Graphics::CubemapFaceCount; ++faceIndex )
				{
					const auto & face = cubemapResource->hdrFaceData(faceIndex);

					if ( !stagingBuffer.writeData({face.data(), faceBytes, offset}) )
					{
						TraceError{ClassId} << "Unable to write " << faceBytes << " bytes of HDR data in the staging buffer !";

						return false;
					}

					offset += faceBytes;
				}

				return true;
			});
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
	Image::create (TransferManager & transferManager, const std::shared_ptr< Graphics::CubemapMovieResource > & cubemapMovieResource) noexcept
	{
		if ( cubemapMovieResource == nullptr || !cubemapMovieResource->isLoaded() )
		{
			Tracer::error(ClassId, "The cubemap movie resource is null or not loaded! Skipping transfer ...");

			return false;
		}

		if ( !this->createOnHardware() )
		{
			return false;
		}

		const auto & frames = cubemapMovieResource->frames();

		/* Get the total bytes requested for all frames x all faces. */
		size_t totalBytes = 0;

		for ( const auto & [faces, duration] : frames )
		{
			for ( const auto & pixmap : faces )
			{
				totalBytes += pixmap.bytes();
			}
		}

		/* NOTE: We will write all frames, each with 6 face pixmaps, sequentially in the staging buffer.
		 * Layout: Frame0-Face0, Frame0-Face1, ..., Frame0-Face5, Frame1-Face0, ..., FrameN-Face5 */
		return transferManager.uploadImage(*this, totalBytes, [&frames] (const Buffer & stagingBuffer) {
			size_t offset = 0;

			for ( const auto & [faces, duration] : frames )
			{
				for ( const auto & pixmap : faces )
				{
					if ( !stagingBuffer.writeData({pixmap.data().data(), pixmap.bytes(), offset}) )
					{
						TraceError{ClassId} << "Unable to write " << pixmap.bytes() << " bytes of data in the staging buffer !";

						return false;
					}

					offset += pixmap.bytes();
				}
			}

			return true;
		});
	}

	bool
	Image::createFromCompressed (TransferManager & transferManager, std::span< const CompressedMip > mips) noexcept
	{
		if ( mips.empty() )
		{
			Tracer::error(ClassId, "No compressed mip levels provided !");

			return false;
		}

		if ( !this->createOnHardware() )
		{
			return false;
		}

		/* Calculate total bytes and build VkBufferImageCopy regions. */
		size_t totalBytes = 0;
		std::vector< VkBufferImageCopy > regions;
		regions.reserve(mips.size());

		for ( uint32_t level = 0; level < static_cast< uint32_t >(mips.size()); ++level )
		{
			VkBufferImageCopy region{};
			region.bufferOffset = totalBytes;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = level;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = 1;
			region.imageOffset = {0, 0, 0};
			region.imageExtent = {mips[level].width, mips[level].height, 1};

			regions.emplace_back(region);

			totalBytes += mips[level].size;
		}

		/* Write all compressed mip data sequentially to the staging buffer. */
		return transferManager.uploadCompressedImage(*this, totalBytes, [&mips] (const Buffer & stagingBuffer) {
			size_t offset = 0;

			for ( const auto & mip : mips )
			{
				if ( !stagingBuffer.writeData({mip.data, mip.size, offset}) )
				{
					TraceError{ClassId} << "Unable to write " << mip.size << " bytes of compressed mip data !";

					return false;
				}

				offset += mip.size;
			}

			return true;
		}, regions);
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

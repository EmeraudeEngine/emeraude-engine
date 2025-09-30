/*
 * src/Vulkan/Image.hpp
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
#include <cstddef>
#include <cmath>
#include <array>
#include <memory>

/* Local inclusions for inheritances. */
#include "AbstractDeviceDependentObject.hpp"

/* Local inclusions for usages. */
#include "Libs/PixelFactory/Pixmap.hpp"
#include "DeviceMemory.hpp"

#if IS_WINDOWS
#undef min
#undef max
#endif

namespace EmEn
{
	namespace Graphics
	{
		class ImageResource;
		class CubemapResource;
		class MovieResource;
	}

	namespace Vulkan
	{
		class MemoryRegion;
	}
}

namespace EmEn::Vulkan
{
	class TransferManager;

	/**
	 * @brief The vulkan image-specific buffer class.
	 * @extends std::enable_shared_from_this Adds the ability to self-replicate the smart pointer.
	 * @extends EmEn::Vulkan::AbstractDeviceDependentObject This object needs a device.
	 */
	class Image final : public AbstractDeviceDependentObject
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanImage"};

			/**
			 * @brief Constructs an image.
			 * @param device A reference to a device smart pointer.
			 * @param imageType The type of image.
			 * @param format The format of the image.
			 * @param extent A reference to an VkExtent3D for the image dimensions.
			 * @param usageFlags The usage purpose of the image.
			 * @param imageLayout The layout of the image.
			 * @param createFlags The creation info flags. Default none.
			 * @param mipLevels The number of mip-levels. Default 1.
			 * @param arrayLayers The number of array levels. Default 1.
			 * @param samples The number of samples (multisampling). Default VK_SAMPLE_COUNT_1_BIT.
			 * @param imageTiling The image tiling (memory layout). Default VK_IMAGE_TILING_OPTIMAL.
			 */
			Image (const std::shared_ptr< Device > & device, VkImageType imageType, VkFormat format, const VkExtent3D & extent, VkImageUsageFlags usageFlags, VkImageLayout imageLayout, VkImageCreateFlags createFlags = 0, uint32_t mipLevels = 1, uint32_t arrayLayers = 1, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT, VkImageTiling imageTiling = VK_IMAGE_TILING_OPTIMAL) noexcept
				: AbstractDeviceDependentObject{device}
			{
				m_createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
				m_createInfo.pNext = nullptr;
				m_createInfo.flags = createFlags;
				m_createInfo.imageType = imageType;
				m_createInfo.format = format;
				m_createInfo.extent = extent;
				m_createInfo.mipLevels = mipLevels;
				m_createInfo.arrayLayers = arrayLayers;
				m_createInfo.samples = samples;
				m_createInfo.tiling = imageTiling;
				m_createInfo.usage = usageFlags;
				/* TODO: If one day we had to share a buffer between a dedicated compute and graphics family, we need to describe it here. */
				m_createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				m_createInfo.queueFamilyIndexCount = 0;
				m_createInfo.pQueueFamilyIndices = nullptr;
				m_createInfo.initialLayout = imageLayout;
			}

			/**
			 * @brief Constructs an image with a createInfo.
			 * @param device A reference to a smart pointer of the device.
			 * @param createInfo A reference to the createInfo.
			 */
			Image (const std::shared_ptr< Device > & device, const VkImageCreateInfo & createInfo) noexcept
				: AbstractDeviceDependentObject{device},
				m_createInfo{createInfo}
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Image (const Image & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Image (Image && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return Image &
			 */
			Image & operator= (const Image & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return Image &
			 */
			Image & operator= (Image && copy) noexcept = delete;

			/**
			 * @brief Destructs the image.
			 */
			~Image () override
			{
				this->destroyFromHardware();
			}

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::createOnHardware() */
			bool createOnHardware () noexcept override;

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::destroyFromHardware() */
			bool destroyFromHardware () noexcept override;

			/**
			 * @brief Returns the image vulkan handle.
			 * @return VkImage
			 */
			[[nodiscard]]
			VkImage
			handle () const noexcept
			{
				return m_handle;
			}

			/**
			 * @brief Returns the image createInfo.
			 * @return const VkImageCreateInfo &
			 */
			[[nodiscard]]
			const VkImageCreateInfo &
			createInfo () const noexcept
			{
				return m_createInfo;
			}

			/**
			 * @brief Sets the current image layout.
			 * @warning This should be call after image manipulation on GPU.
			 * @param imageLayout The Vulkan image layout.
			 * @return void
			 */
			void
			setCurrentImageLayout (VkImageLayout imageLayout) noexcept
			{
				m_currentImageLayout = imageLayout;
			}

			/**
			 * @brief Returns the current image layout.
			 * @return VkImageLayout
			 */
			[[nodiscard]]
			VkImageLayout
			currentImageLayout () const noexcept
			{
				return m_currentImageLayout;
			}

			/**
			 * @brief Returns the image width.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			width () const noexcept
			{
				return m_createInfo.extent.width;
			}

			/**
			 * @brief Returns the image height.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			height () const noexcept
			{
				return m_createInfo.extent.height;
			}

			/**
			 * @brief Returns the image depth.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			depth () const noexcept
			{
				return m_createInfo.extent.depth;
			}

			/**
			 * @brief Returns the color count.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			colorCount () const noexcept
			{
				switch ( m_createInfo.format )
				{
					/* NOTE: RGBA */
					case VK_FORMAT_R8G8B8A8_UNORM :
					case VK_FORMAT_R8G8B8A8_SNORM :
					case VK_FORMAT_R64G64B64A64_SFLOAT :
					case VK_FORMAT_R32G32B32A32_SFLOAT :
						return 4;

					/* NOTE: RGB */
					case VK_FORMAT_R8G8B8_UNORM :
					case VK_FORMAT_R8G8B8_SNORM :
					case VK_FORMAT_R64G64B64_SFLOAT :
					case VK_FORMAT_R32G32B32_SFLOAT :
						return 3;

					/* NOTE: GRAY+ALPHA */
					case VK_FORMAT_R8G8_UNORM :
					case VK_FORMAT_R8G8_SNORM :
					case VK_FORMAT_R64G64_SFLOAT :
					case VK_FORMAT_R32G32_SFLOAT :
						return 2;

					/* NOTE: GRAY */
					case VK_FORMAT_R8_UNORM :
					case VK_FORMAT_R8_SNORM :
					case VK_FORMAT_R64_SFLOAT :
					case VK_FORMAT_R32_SFLOAT :
						return 1;

					/* NOTE: Unhandled */
					case VK_FORMAT_UNDEFINED :
					default :
						return 0;
				}
			}

			/**
			 * @brief Returns the pixel bytes.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			pixelBytes () const noexcept
			{
				switch ( m_createInfo.format )
				{
					/* NOTE: RGBA */
					case VK_FORMAT_R8G8B8A8_UNORM :
						return 4 * sizeof(uint8_t);
					case VK_FORMAT_R8G8B8A8_SNORM :
						return 4 * sizeof(int8_t);
					case VK_FORMAT_R64G64B64A64_SFLOAT :
						return 4 * sizeof(double);
					case VK_FORMAT_R32G32B32A32_SFLOAT :
						return 4 * sizeof(float);

					/* NOTE: RGB */
					case VK_FORMAT_R8G8B8_UNORM :
						return 3 * sizeof(uint8_t);
					case VK_FORMAT_R8G8B8_SNORM :
						return 3 * sizeof(int8_t);
					case VK_FORMAT_R64G64B64_SFLOAT :
						return 3 * sizeof(double);
					case VK_FORMAT_R32G32B32_SFLOAT :
						return 3 * sizeof(float);

					/* NOTE: GRAY+ALPHA */
					case VK_FORMAT_R8G8_UNORM :
						return 2 * sizeof(uint8_t);
					case VK_FORMAT_R8G8_SNORM :
						return 2 * sizeof(int8_t);
					case VK_FORMAT_R64G64_SFLOAT :
						return 2 * sizeof(double);
					case VK_FORMAT_R32G32_SFLOAT :
						return 2 * sizeof(float);

					/* NOTE: GRAY */
					case VK_FORMAT_R8_UNORM :
						return sizeof(uint8_t);
					case VK_FORMAT_R8_SNORM :
						return sizeof(int8_t);
					case VK_FORMAT_R64_SFLOAT :
						return sizeof(double);
					case VK_FORMAT_R32_SFLOAT :
						return sizeof(float);

					/* NOTE: Unhandled */
					case VK_FORMAT_UNDEFINED :
					default :
						return 0;
				}
			}

			/**
			 * @brief Returns the image size in bytes.
			 * @warning This information comes from the creation info with no mipmap, not the device memory.
			 * @return VkDeviceSize
			 */
			[[nodiscard]]
			VkDeviceSize
			bytes () const noexcept
			{
				return
					static_cast< VkDeviceSize >(this->width()) *
					static_cast< VkDeviceSize >(this->height()) *
					static_cast< VkDeviceSize >(this->depth()) *
					static_cast< VkDeviceSize >(this->pixelBytes());
			}

			/**
			 * @brief Creates, allocates and returns a usable image.
			 * @param transferManager A reference to a transfer manager.
			 * @param pixmap A reference to a pixmap.
			 * @return bool
			 */
			[[nodiscard]]
			bool create (TransferManager & transferManager, const Libs::PixelFactory::Pixmap< uint8_t > & pixmap) noexcept;

			/**
			 * @brief Creates, allocates and returns a usable image.
			 * @param transferManager A reference to a transfer manager.
			 * @param imageResource A reference to an image resource smart pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool create (TransferManager & transferManager, const std::shared_ptr< Graphics::ImageResource > & imageResource) noexcept;

			/**
			 * @brief Creates, allocates and returns a usable image.
			 * @param transferManager A reference to a transfer manager.
			 * @param cubemapResource A reference to a cubemap resource smart pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool create (TransferManager & transferManager, const std::shared_ptr< Graphics::CubemapResource > & cubemapResource) noexcept;

			/**
			 * @brief Creates, allocates and returns a usable image.
			 * @param transferManager A reference to a transfer manager.
			 * @param movieResource A reference to a movie resource smart pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool create (TransferManager & transferManager, const std::shared_ptr< Graphics::MovieResource > & movieResource) noexcept;

			/**
			 * @brief Writes raw data to this image using a staging buffer.
			 * @param transferManager A reference to a transfer manager.
			 * @param memoryRegion A reference to a memory region structure.
			 * @return bool
			 */
			bool writeData (TransferManager & transferManager, const MemoryRegion & memoryRegion) noexcept;

			/**
			 * @brief Creates an image object from a vulkan handle form the swap chain.
			 * @param device A reference to a device smart pointer.
			 * @param handle The handle provided by the swap chain.
			 * @param createInfo A reference to the creation info used to create the swap chain.
			 * @return std::shared_ptr< Image >
			 */
			[[nodiscard]]
			static std::shared_ptr< Image > createFromSwapChain (const std::shared_ptr< Device > & device, VkImage handle, const VkSwapchainCreateInfoKHR & createInfo) noexcept;

			/**
			 * @brief Returns the vulkan image format suitable for this pixmap.
			 * @warngin This method does not handle a packed pixel format.
			 * @tparam precision_t The pixel data precision.
			 * @param colorCount The color count.
			 * @return VkFormat
			 */
			template< typename precision_t >
			[[nodiscard]]
			static
			VkFormat
			getFormat (size_t colorCount) noexcept requires (std::is_arithmetic_v< precision_t >)
			{
				/* Reminder :
				 * [Numeric format][SPIR-V Sampled Type][Description]
				 * UNORM, OpTypeFloat, The components are unsigned normalized values in the range [0,1]
				 * SNORM, OpTypeFloat, The components are signed normalized values in the range [-1,1]
				 * USCALED, OpTypeFloat, The components are unsigned integer values that get converted to floating-point in the range [0,2n-1]
				 * SSCALED, OpTypeFloat, The components are signed integer values that get converted to floating-point in the range [-2n-1,2n-1-1]
				 * UINT, OpTypeInt, The components are unsigned integer values in the range [0,2n-1]
				 * SINT, OpTypeInt, The components are signed integer values in the range [-2n-1,2n-1-1]
				 * UFLOAT, OpTypeFloat, The components are unsigned floating-point numbers (used by packed, shared exponent, and some compressed formats)
				 * SFLOAT, OpTypeFloat, The components are signed floating-point numbers
				 * SRGB, OpTypeFloat, The R, G, and B components are unsigned normalized values that represent values using sRGB nonlinear encoding, while the A component (if one exists) is a regular unsigned normalized value
				 *	n is the number of bits in the component.
				 */

				/* NOTE: Floating point value. */
				if constexpr ( std::is_floating_point_v< precision_t > )
				{
					/* NOTE: float (4 bytes) */
					if constexpr ( sizeof(precision_t) == 4 )
					{
						switch ( colorCount )
						{
							case 4 :
								return VK_FORMAT_R32G32B32A32_SFLOAT;

							case 3 :
								return VK_FORMAT_R32G32B32_SFLOAT;

							case 2 :
								return VK_FORMAT_R32G32_SFLOAT;

							case 1 :
								return VK_FORMAT_R32_SFLOAT;

							default :
								return VK_FORMAT_UNDEFINED;
						}
					}
					/* NOTE: double (8 bytes) */
					else if constexpr ( sizeof(precision_t) == 8 )
					{
						switch ( colorCount )
						{
							case 4 :
								return VK_FORMAT_R64G64B64A64_SFLOAT;

							case 3 :
								return VK_FORMAT_R64G64B64_SFLOAT;

							case 2 :
								return VK_FORMAT_R64G64_SFLOAT;

							case 1 :
								return VK_FORMAT_R64_SFLOAT;

							default :
								return VK_FORMAT_UNDEFINED;
						}
					}
				}

				/* NOTE: Integer value. */
				if constexpr ( std::is_integral_v< precision_t > )
				{
					/* FIXME: Check for non 8bits format ! */

					/* NOTE: uint8_t (1 byte). */
					if constexpr ( std::is_unsigned_v< precision_t > )
					{
						switch ( colorCount )
						{
							case 4 :
								return VK_FORMAT_R8G8B8A8_UNORM;

							case 3 :
								return VK_FORMAT_R8G8B8_UNORM;

							case 2 :
								return VK_FORMAT_R8G8_UNORM;

							case 1 :
								return VK_FORMAT_R8_UNORM;

							default :
								return VK_FORMAT_UNDEFINED;
						}
					}
					/* NOTE: int8_t (1 byte). */
					else
					{
						switch ( colorCount )
						{
							case 4 :
								return VK_FORMAT_R8G8B8A8_SNORM;

							case 3 :
								return VK_FORMAT_R8G8B8_SNORM;

							case 2 :
								return VK_FORMAT_R8G8_SNORM;

							case 1 :
								return VK_FORMAT_R8_SNORM;

							default :
								return VK_FORMAT_UNDEFINED;
						}
					}
				}

				return VK_FORMAT_UNDEFINED;
			}

			/**
			 * @brief Returns the number of mip levels possible for a given dimension.
			 * @param width The width of the picture.
			 * @param height The height of the picture.
			 * @return uint32_t
			 */
			[[nodiscard]]
			static
			uint32_t
			getMIPLevels (size_t width, size_t height) noexcept
			{
				return static_cast< uint32_t >(std::floor(std::log2(std::max(width, height)))) + 1;
			}

		private:

			/* Flag names. */
			static constexpr auto IsSwapChainImage{0UL};

			VkImage m_handle{VK_NULL_HANDLE};
			VkImageCreateInfo m_createInfo{};
			std::unique_ptr< DeviceMemory > m_deviceMemory;
			VkImageLayout m_currentImageLayout{VK_IMAGE_LAYOUT_UNDEFINED};
			std::array< bool, 8 > m_flags{
				false/*IsSwapChainImage*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
	};
}

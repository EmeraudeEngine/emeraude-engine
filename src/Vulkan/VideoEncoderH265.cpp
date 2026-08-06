/*
 * src/Vulkan/VideoEncoderH265.cpp
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

#include "VideoEncoderH265.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cstring>

/* Local inclusions. */
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/CommandPool.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/PhysicalDevice.hpp"
#include "Vulkan/Queue.hpp"

namespace EmEn::Vulkan
{
	/* The source and reconstructed pictures share the NV12 layout produced by the
	 * frame converter. 8-bit main profile today; the Settings/profile plumbing stays
	 * parametric for the Main 10 / P010 HDR10 extension. */
	constexpr auto PictureFormat{VK_FORMAT_G8_B8R8_2PLANE_420_UNORM};

	static uint32_t
	alignUp (uint32_t value, uint32_t alignment) noexcept
	{
		return (value + alignment - 1) / alignment * alignment;
	}

	VideoEncoderH265::VideoEncoderH265 (const std::shared_ptr< Device > & device) noexcept
		: m_device{device}
	{

	}

	VideoEncoderH265::~VideoEncoderH265 ()
	{
		this->destroy();
	}

	bool
	VideoEncoderH265::resolveEntryPoints () noexcept
	{
		const auto deviceHandle = m_device->handle();

		m_fpCreateVideoSession = reinterpret_cast< PFN_vkCreateVideoSessionKHR >(vkGetDeviceProcAddr(deviceHandle, "vkCreateVideoSessionKHR"));
		m_fpDestroyVideoSession = reinterpret_cast< PFN_vkDestroyVideoSessionKHR >(vkGetDeviceProcAddr(deviceHandle, "vkDestroyVideoSessionKHR"));
		m_fpGetSessionMemoryRequirements = reinterpret_cast< PFN_vkGetVideoSessionMemoryRequirementsKHR >(vkGetDeviceProcAddr(deviceHandle, "vkGetVideoSessionMemoryRequirementsKHR"));
		m_fpBindSessionMemory = reinterpret_cast< PFN_vkBindVideoSessionMemoryKHR >(vkGetDeviceProcAddr(deviceHandle, "vkBindVideoSessionMemoryKHR"));
		m_fpCreateSessionParameters = reinterpret_cast< PFN_vkCreateVideoSessionParametersKHR >(vkGetDeviceProcAddr(deviceHandle, "vkCreateVideoSessionParametersKHR"));
		m_fpDestroySessionParameters = reinterpret_cast< PFN_vkDestroyVideoSessionParametersKHR >(vkGetDeviceProcAddr(deviceHandle, "vkDestroyVideoSessionParametersKHR"));
		m_fpGetEncodedSessionParameters = reinterpret_cast< PFN_vkGetEncodedVideoSessionParametersKHR >(vkGetDeviceProcAddr(deviceHandle, "vkGetEncodedVideoSessionParametersKHR"));
		m_fpCmdBeginVideoCoding = reinterpret_cast< PFN_vkCmdBeginVideoCodingKHR >(vkGetDeviceProcAddr(deviceHandle, "vkCmdBeginVideoCodingKHR"));
		m_fpCmdEndVideoCoding = reinterpret_cast< PFN_vkCmdEndVideoCodingKHR >(vkGetDeviceProcAddr(deviceHandle, "vkCmdEndVideoCodingKHR"));
		m_fpCmdControlVideoCoding = reinterpret_cast< PFN_vkCmdControlVideoCodingKHR >(vkGetDeviceProcAddr(deviceHandle, "vkCmdControlVideoCodingKHR"));
		m_fpCmdEncodeVideo = reinterpret_cast< PFN_vkCmdEncodeVideoKHR >(vkGetDeviceProcAddr(deviceHandle, "vkCmdEncodeVideoKHR"));

		return m_fpCreateVideoSession != nullptr
			&& m_fpDestroyVideoSession != nullptr
			&& m_fpGetSessionMemoryRequirements != nullptr
			&& m_fpBindSessionMemory != nullptr
			&& m_fpCreateSessionParameters != nullptr
			&& m_fpDestroySessionParameters != nullptr
			&& m_fpGetEncodedSessionParameters != nullptr
			&& m_fpCmdBeginVideoCoding != nullptr
			&& m_fpCmdEndVideoCoding != nullptr
			&& m_fpCmdControlVideoCoding != nullptr
			&& m_fpCmdEncodeVideo != nullptr;
	}

	bool
	VideoEncoderH265::create (const Settings & settings) noexcept
	{
		if ( !m_device->videoEncodeH265Enabled() )
		{
			Tracer::error(ClassId, "The device has no H.265 video-encode support !");

			return false;
		}

		if ( settings.width == 0 || settings.height == 0 || settings.width % 2 != 0 || settings.height % 2 != 0 )
		{
			Tracer::error(ClassId, "Invalid frame dimensions (must be non-zero and even) !");

			return false;
		}

		m_settings = settings;

		if ( !this->resolveEntryPoints() )
		{
			Tracer::error(ClassId, "Unable to resolve the Vulkan Video entry points !");

			return false;
		}

		/* Video profile — the SAME chain is attached to the session, the pictures,
		 * the bitstream buffer and the feedback query pool. */
		m_h265Profile = {};
		m_h265Profile.sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_PROFILE_INFO_KHR;
		m_h265Profile.stdProfileIdc = STD_VIDEO_H265_PROFILE_IDC_MAIN;

		m_profile = {};
		m_profile.sType = VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR;
		m_profile.pNext = &m_h265Profile;
		m_profile.videoCodecOperation = VK_VIDEO_CODEC_OPERATION_ENCODE_H265_BIT_KHR;
		m_profile.chromaSubsampling = VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR;
		m_profile.lumaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR;
		m_profile.chromaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR;

		m_profileList = {};
		m_profileList.sType = VK_STRUCTURE_TYPE_VIDEO_PROFILE_LIST_INFO_KHR;
		m_profileList.profileCount = 1;
		m_profileList.pProfiles = &m_profile;

		if ( !this->createSession() || !this->createSessionParameters() || !this->createPictures() )
		{
			this->destroy();

			return false;
		}

		/* Command pools: plane copies on the graphics queue, encode on the video queue. */
		m_graphicsCommandPool = std::make_shared< CommandPool >(m_device, m_device->getGraphicsFamilyIndex(), true, true, false);
		m_videoCommandPool = std::make_shared< CommandPool >(m_device, m_device->getVideoEncodeFamilyIndex(), true, true, false);

		if ( !m_graphicsCommandPool->createOnHardware() || !m_videoCommandPool->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the command pools !");

			this->destroy();

			return false;
		}

		TraceSuccess{ClassId} << "H.265 hardware encoder created (" << settings.width << "x" << settings.height << " @ " << settings.frameRate << " FPS, VBR " << settings.averageBitrateKbps << "/" << settings.maximumBitrateKbps << " kbps, IDR every " << settings.idrPeriod << " frames).";

		return true;
	}

	bool
	VideoEncoderH265::createSession () noexcept
	{
		static const VkExtensionProperties stdHeaderVersion{
			VK_STD_VULKAN_VIDEO_CODEC_H265_ENCODE_EXTENSION_NAME,
			VK_STD_VULKAN_VIDEO_CODEC_H265_ENCODE_SPEC_VERSION
		};

		/* NOTE: The storage extent is aligned on the picture access granularity
		 * (32x32 on NVIDIA — under-aligned images hang the encode engine); the
		 * logical codedExtent used at encode time stays the exact frame size. */
		const VkExtent2D codedExtent{alignUp(m_settings.width, 64), alignUp(m_settings.height, 64)};

		VkVideoSessionCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_VIDEO_SESSION_CREATE_INFO_KHR;
		createInfo.queueFamilyIndex = m_device->getVideoEncodeFamilyIndex();
		createInfo.flags = 0;
		createInfo.pVideoProfile = &m_profile;
		createInfo.pictureFormat = PictureFormat;
		createInfo.maxCodedExtent = codedExtent;
		createInfo.referencePictureFormat = PictureFormat;
		createInfo.maxDpbSlots = DpbSlotCount;
		createInfo.maxActiveReferencePictures = 1;
		createInfo.pStdHeaderVersion = &stdHeaderVersion;

		if ( m_fpCreateVideoSession(m_device->handle(), &createInfo, nullptr, &m_session) != VK_SUCCESS )
		{
			Tracer::error(ClassId, "Unable to create the video session !");

			return false;
		}

		/* Bind the session device memory (one allocation per requirement). */
		uint32_t requirementCount = 0;

		if ( m_fpGetSessionMemoryRequirements(m_device->handle(), m_session, &requirementCount, nullptr) != VK_SUCCESS )
		{
			return false;
		}

		std::vector< VkVideoSessionMemoryRequirementsKHR > requirements(requirementCount);

		for ( auto & requirement : requirements )
		{
			requirement.sType = VK_STRUCTURE_TYPE_VIDEO_SESSION_MEMORY_REQUIREMENTS_KHR;
		}

		if ( m_fpGetSessionMemoryRequirements(m_device->handle(), m_session, &requirementCount, requirements.data()) != VK_SUCCESS )
		{
			return false;
		}

		VkPhysicalDeviceMemoryProperties memoryProperties{};
		vkGetPhysicalDeviceMemoryProperties(m_device->physicalDevice()->handle(), &memoryProperties);

		std::vector< VkBindVideoSessionMemoryInfoKHR > bindInfos;
		bindInfos.reserve(requirementCount);

		for ( const auto & requirement : requirements )
		{
			uint32_t memoryTypeIndex = memoryProperties.memoryTypeCount;

			for ( uint32_t typeIndex = 0; typeIndex < memoryProperties.memoryTypeCount; typeIndex++ )
			{
				if ( (requirement.memoryRequirements.memoryTypeBits & (1U << typeIndex)) != 0 )
				{
					memoryTypeIndex = typeIndex;

					/* Prefer a device-local type when available. */
					if ( (memoryProperties.memoryTypes[typeIndex].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0 )
					{
						break;
					}
				}
			}

			if ( memoryTypeIndex == memoryProperties.memoryTypeCount )
			{
				Tracer::error(ClassId, "No suitable memory type for the video session !");

				return false;
			}

			VkMemoryAllocateInfo allocateInfo{};
			allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocateInfo.allocationSize = requirement.memoryRequirements.size;
			allocateInfo.memoryTypeIndex = memoryTypeIndex;

			VkDeviceMemory memory{VK_NULL_HANDLE};

			if ( vkAllocateMemory(m_device->handle(), &allocateInfo, nullptr, &memory) != VK_SUCCESS )
			{
				Tracer::error(ClassId, "Unable to allocate the video session memory !");

				return false;
			}

			m_sessionMemory.emplace_back(memory);

			VkBindVideoSessionMemoryInfoKHR bindInfo{};
			bindInfo.sType = VK_STRUCTURE_TYPE_BIND_VIDEO_SESSION_MEMORY_INFO_KHR;
			bindInfo.memoryBindIndex = requirement.memoryBindIndex;
			bindInfo.memory = memory;
			bindInfo.memoryOffset = 0;
			bindInfo.memorySize = requirement.memoryRequirements.size;

			bindInfos.emplace_back(bindInfo);
		}

		if ( m_fpBindSessionMemory(m_device->handle(), m_session, static_cast< uint32_t >(bindInfos.size()), bindInfos.data()) != VK_SUCCESS )
		{
			Tracer::error(ClassId, "Unable to bind the video session memory !");

			return false;
		}

		return true;
	}

	bool
	VideoEncoderH265::createSessionParameters () noexcept
	{
		/* The NVENC hardware works with 16x16 minimum coding blocks (the reference
		 * encoder hardcodes minCB 16 / CTB 32): SPS picture dimensions are aligned
		 * on 16 and the conformance window crops back to the visible size. A minCB
		 * of 8 desynchronises the CTU raster whenever a dimension is not a multiple
		 * of 16 (2880x1620 was corrupt while 1280x720 was clean). */
		const uint32_t alignedWidth = alignUp(m_settings.width, 16);
		const uint32_t alignedHeight = alignUp(m_settings.height, 16);

		/* Profile / tier / level. */
		m_stdProfileTierLevel = {};
		m_stdProfileTierLevel.flags.general_progressive_source_flag = 1;
		m_stdProfileTierLevel.flags.general_frame_only_constraint_flag = 1;
		m_stdProfileTierLevel.general_profile_idc = STD_VIDEO_H265_PROFILE_IDC_MAIN;
		m_stdProfileTierLevel.general_level_idc = STD_VIDEO_H265_LEVEL_IDC_5_1;

		/* Decoded picture buffer management (single temporal sub-layer). */
		m_stdDecPicBufMgr = {};
		m_stdDecPicBufMgr.max_dec_pic_buffering_minus1[0] = DpbSlotCount - 1;

		/* VPS. */
		m_stdVPS = {};
		m_stdVPS.flags.vps_temporal_id_nesting_flag = 1;
		m_stdVPS.pProfileTierLevel = &m_stdProfileTierLevel;
		m_stdVPS.pDecPicBufMgr = &m_stdDecPicBufMgr;

		/* SPS — the coded surface is aligned on the minimum coding block (8), the
		 * conformance window crops back to the requested display size. */
		m_stdSPS = {};
		m_stdSPS.flags.sps_temporal_id_nesting_flag = 1;
		m_stdSPS.flags.sample_adaptive_offset_enabled_flag = 1;
		m_stdSPS.chroma_format_idc = STD_VIDEO_H265_CHROMA_FORMAT_IDC_420;
		m_stdSPS.pic_width_in_luma_samples = alignedWidth;
		m_stdSPS.pic_height_in_luma_samples = alignedHeight;
		m_stdSPS.log2_max_pic_order_cnt_lsb_minus4 = 4;
		m_stdSPS.log2_min_luma_coding_block_size_minus3 = 1; /* minCB 16 (NVENC). */
		m_stdSPS.log2_diff_max_min_luma_coding_block_size = 1; /* 16..32 CTB. */
		m_stdSPS.log2_diff_max_min_luma_transform_block_size = 3; /* 4..32 TB. */
		m_stdSPS.pProfileTierLevel = &m_stdProfileTierLevel;
		m_stdSPS.pDecPicBufMgr = &m_stdDecPicBufMgr;

		if ( alignedWidth != m_settings.width || alignedHeight != m_settings.height )
		{
			m_stdSPS.flags.conformance_window_flag = 1;
			/* Offsets are expressed in chroma units (4:2:0 → 2 luma samples). */
			m_stdSPS.conf_win_right_offset = (alignedWidth - m_settings.width) / 2;
			m_stdSPS.conf_win_bottom_offset = (alignedHeight - m_settings.height) / 2;
		}

		/* PPS — do NOT set cu_qp_delta_enabled_flag: the driver overrides the PPS
		 * as it needs, and forcing it desynchronises the slice entropy (re-measured). */
		m_stdPPS = {};

		VkVideoEncodeH265SessionParametersAddInfoKHR addInfo{};
		addInfo.sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_SESSION_PARAMETERS_ADD_INFO_KHR;
		addInfo.stdVPSCount = 1;
		addInfo.pStdVPSs = &m_stdVPS;
		addInfo.stdSPSCount = 1;
		addInfo.pStdSPSs = &m_stdSPS;
		addInfo.stdPPSCount = 1;
		addInfo.pStdPPSs = &m_stdPPS;

		VkVideoEncodeH265SessionParametersCreateInfoKHR h265CreateInfo{};
		h265CreateInfo.sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_SESSION_PARAMETERS_CREATE_INFO_KHR;
		h265CreateInfo.maxStdVPSCount = 1;
		h265CreateInfo.maxStdSPSCount = 1;
		h265CreateInfo.maxStdPPSCount = 1;
		h265CreateInfo.pParametersAddInfo = &addInfo;

		VkVideoSessionParametersCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_VIDEO_SESSION_PARAMETERS_CREATE_INFO_KHR;
		createInfo.pNext = &h265CreateInfo;
		createInfo.videoSession = m_session;

		if ( m_fpCreateSessionParameters(m_device->handle(), &createInfo, nullptr, &m_sessionParameters) != VK_SUCCESS )
		{
			Tracer::error(ClassId, "Unable to create the video session parameters !");

			return false;
		}

		/* Retrieve the driver-encoded VPS/SPS/PPS (Annex-B) — written once at the
		 * beginning of the elementary stream. */
		VkVideoEncodeH265SessionParametersGetInfoKHR h265GetInfo{};
		h265GetInfo.sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_SESSION_PARAMETERS_GET_INFO_KHR;
		h265GetInfo.writeStdVPS = VK_TRUE;
		h265GetInfo.writeStdSPS = VK_TRUE;
		h265GetInfo.writeStdPPS = VK_TRUE;

		VkVideoEncodeSessionParametersGetInfoKHR getInfo{};
		getInfo.sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_SESSION_PARAMETERS_GET_INFO_KHR;
		getInfo.pNext = &h265GetInfo;
		getInfo.videoSessionParameters = m_sessionParameters;

		size_t dataSize = 0;

		if ( m_fpGetEncodedSessionParameters(m_device->handle(), &getInfo, nullptr, &dataSize, nullptr) != VK_SUCCESS || dataSize == 0 )
		{
			Tracer::error(ClassId, "Unable to size the encoded session parameters !");

			return false;
		}

		m_encodedParameters.resize(dataSize);

		if ( m_fpGetEncodedSessionParameters(m_device->handle(), &getInfo, nullptr, &dataSize, m_encodedParameters.data()) != VK_SUCCESS )
		{
			Tracer::error(ClassId, "Unable to retrieve the encoded session parameters !");

			return false;
		}

		m_encodedParameters.resize(dataSize);

		TraceInfo{ClassId} << "Driver-encoded VPS/SPS/PPS retrieved (" << dataSize << " bytes).";

		return true;
	}

	bool
	VideoEncoderH265::createPictures () noexcept
	{
		/* Aligned on the picture access granularity, see createSession(). */
		const VkExtent3D codedExtent{alignUp(m_settings.width, 64), alignUp(m_settings.height, 64), 1};

		/* NV12 source picture — EXCLUSIVE to the video queue family: the plane copies
		 * run on the video queue itself (it exposes TRANSFER), the bounce buffer is
		 * the only object crossing queues. */
		{
			VkImageCreateInfo imageInfo{};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.pNext = &m_profileList;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.format = PictureFormat;
			imageInfo.extent = codedExtent;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.usage = VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			m_sourcePicture = std::make_unique< Image >(m_device, imageInfo);
			m_sourcePicture->setIdentifier(ClassId, "SourcePicture", "Image");

			if ( !m_sourcePicture->createOnHardware() )
			{
				Tracer::error(ClassId, "Unable to create the NV12 source picture !");

				return false;
			}
		}

		/* DPB: ONE layered image (the NVIDIA encoder's native DPB organisation),
		 * one array layer per slot. */
		{
			VkImageCreateInfo imageInfo{};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.pNext = &m_profileList;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.format = PictureFormat;
			imageInfo.extent = codedExtent;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = DpbSlotCount;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.usage = VK_IMAGE_USAGE_VIDEO_ENCODE_DPB_BIT_KHR;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			m_dpbPictures[0] = std::make_unique< Image >(m_device, imageInfo);
			m_dpbPictures[0]->setIdentifier(ClassId, "DPBPicture", "Image");

			if ( !m_dpbPictures[0]->createOnHardware() )
			{
				Tracer::error(ClassId, "Unable to create the DPB picture array !");

				return false;
			}
		}

		/* Full multi-planar views. */
		const auto makeView = [this] (const Image & image, VkImageView & view) {
			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = image.handle();
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = PictureFormat;
			viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

			return vkCreateImageView(m_device->handle(), &viewInfo, nullptr, &view) == VK_SUCCESS;
		};

		if ( !makeView(*m_sourcePicture, m_sourceView) )
		{
			Tracer::error(ClassId, "Unable to create the source picture view !");

			return false;
		}

		for ( uint32_t slot = 0; slot < DpbSlotCount; slot++ )
		{
			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_dpbPictures[0]->handle();
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = PictureFormat;
			viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, slot, 1};

			if ( vkCreateImageView(m_device->handle(), &viewInfo, nullptr, &m_dpbViews[slot]) != VK_SUCCESS )
			{
				Tracer::error(ClassId, "Unable to create a DPB layer view !");

				return false;
			}
		}

		/* Plane bounce buffer (device-local, luma plane followed by chroma plane),
		 * CONCURRENT graphics+video: written by the graphics queue (converter plane
		 * readbacks), read by the video queue (plane uploads). */
		{
			const VkDeviceSize lumaBytes = static_cast< VkDeviceSize >(m_settings.width) * m_settings.height;
			const std::array< uint32_t, 2 > families{m_device->getGraphicsFamilyIndex(), m_device->getVideoEncodeFamilyIndex()};
			const auto sameFamily = families[0] == families[1];

			VkBufferCreateInfo bufferInfo{};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = lumaBytes + lumaBytes / 2;
			bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			bufferInfo.sharingMode = sameFamily ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
			bufferInfo.queueFamilyIndexCount = sameFamily ? 0 : static_cast< uint32_t >(families.size());
			bufferInfo.pQueueFamilyIndices = sameFamily ? nullptr : families.data();

			m_planeBounceBuffer = std::make_unique< Buffer >(m_device, bufferInfo, false);
			m_planeBounceBuffer->setIdentifier(ClassId, "PlaneBounce", "Buffer");

			if ( !m_planeBounceBuffer->createOnHardware() )
			{
				Tracer::error(ClassId, "Unable to create the plane bounce buffer !");

				return false;
			}
		}

		/* Bitstream buffer (host-visible, generously sized for one packet).
		 * NOTE: dstBufferRange must be a multiple of minBitstreamBufferSizeAlignment
		 * (256 on NVIDIA) — a misaligned range CORRUPTS the produced bitstream
		 * (measured Aug 2026: 2880x1620 landed on x.5 alignment units and produced
		 * green/magenta smears + CABAC errors, while 1280x720 was aligned by luck). */
		{
			const VkDeviceSize rawSize = static_cast< VkDeviceSize >(m_settings.width) * m_settings.height * 3 / 2 + (1U << 20U);
			const VkDeviceSize bufferSize = (rawSize + 4095U) / 4096U * 4096U;

			VkBufferCreateInfo bufferInfo{};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.pNext = &m_profileList;
			bufferInfo.size = bufferSize;
			bufferInfo.usage = VK_BUFFER_USAGE_VIDEO_ENCODE_DST_BIT_KHR;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			m_bitstreamBuffer = std::make_unique< Buffer >(m_device, bufferInfo, true);
			m_bitstreamBuffer->setIdentifier(ClassId, "Bitstream", "Buffer");
			m_bitstreamBuffer->setHostReadable(true);

			if ( !m_bitstreamBuffer->createOnHardware() )
			{
				Tracer::error(ClassId, "Unable to create the bitstream buffer !");

				return false;
			}

			m_bitstreamMapped = m_bitstreamBuffer->mapMemoryAs< uint8_t >();

			if ( m_bitstreamMapped == nullptr )
			{
				Tracer::error(ClassId, "Unable to map the bitstream buffer !");

				return false;
			}
		}

		/* Encode feedback query pool (bitstream offset + bytes written). */
		{
			VkQueryPoolVideoEncodeFeedbackCreateInfoKHR feedbackInfo{};
			feedbackInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_VIDEO_ENCODE_FEEDBACK_CREATE_INFO_KHR;
			feedbackInfo.pNext = &m_profile;
			feedbackInfo.encodeFeedbackFlags =
				VK_VIDEO_ENCODE_FEEDBACK_BITSTREAM_BUFFER_OFFSET_BIT_KHR |
				VK_VIDEO_ENCODE_FEEDBACK_BITSTREAM_BYTES_WRITTEN_BIT_KHR;

			VkQueryPoolCreateInfo queryPoolInfo{};
			queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
			queryPoolInfo.pNext = &feedbackInfo;
			queryPoolInfo.queryType = VK_QUERY_TYPE_VIDEO_ENCODE_FEEDBACK_KHR;
			queryPoolInfo.queryCount = 1;

			if ( vkCreateQueryPool(m_device->handle(), &queryPoolInfo, nullptr, &m_feedbackQueryPool) != VK_SUCCESS )
			{
				Tracer::error(ClassId, "Unable to create the encode feedback query pool !");

				return false;
			}
		}

		return true;
	}

	bool
	VideoEncoderH265::encodeFrame (const Image & lumaPlane, const Image & chromaPlane, std::vector< uint8_t > & packet, bool & wasIDR) noexcept
	{
		const auto isIDR = m_gopFrameIndex == 0;
		wasIDR = isIDR;

		const uint32_t referenceSlot = (m_setupSlot + 1) % DpbSlotCount;

		/* NOTE: The coded extent is the exact visible size — the NVIDIA driver derives
		 * the aligned coded surface itself (an 8-aligned extent here made things worse). */
		const VkExtent2D codedExtent{m_settings.width, m_settings.height};

		/* === Step 1 (graphics queue): converter planes -> bounce buffer. === */
		{
			const auto commandBuffer = std::make_unique< CommandBuffer >(m_graphicsCommandPool, true);

			if ( !commandBuffer->isCreated() || !commandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
			{
				Tracer::error(ClassId, "Unable to begin the plane-copy command buffer !");

				return false;
			}

			{
				const VkDeviceSize lumaBytes = static_cast< VkDeviceSize >(m_settings.width) * m_settings.height;

				VkBufferImageCopy bufferRegion{};
				bufferRegion.bufferOffset = 0;
				bufferRegion.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
				bufferRegion.imageExtent = {m_settings.width, m_settings.height, 1};

				vkCmdCopyImageToBuffer(commandBuffer->handle(), lumaPlane.handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_planeBounceBuffer->handle(), 1, &bufferRegion);

				bufferRegion.bufferOffset = lumaBytes;
				bufferRegion.imageExtent = {m_settings.width / 2, m_settings.height / 2, 1};

				vkCmdCopyImageToBuffer(commandBuffer->handle(), chromaPlane.handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_planeBounceBuffer->handle(), 1, &bufferRegion);
			}

			if ( !commandBuffer->end() )
			{
				Tracer::error(ClassId, "Unable to end the plane-copy command buffer !");

				return false;
			}

			auto * queue = m_device->getGraphicsQueue(QueuePriority::High);

			if ( queue == nullptr || !queue->submit(*commandBuffer) || !queue->waitIdle() )
			{
				Tracer::error(ClassId, "The plane-copy submission failed !");

				return false;
			}
		}

		/* === Step 2 (video queue): encode. === */
		const auto commandBuffer = std::make_unique< CommandBuffer >(m_videoCommandPool, true);

		if ( !commandBuffer->isCreated() || !commandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
		{
			Tracer::error(ClassId, "Unable to begin the encode command buffer !");

			return false;
		}

		vkCmdResetQueryPool(commandBuffer->handle(), m_feedbackQueryPool, 0, 1);

		/* Upload the planes into the NV12 picture ON THE VIDEO QUEUE (the picture is
		 * exclusive to this family — the concurrent-image path corrupted on NVIDIA). */
		{
			VkImageMemoryBarrier toTransfer{};
			toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			toTransfer.srcAccessMask = 0;
			toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			toTransfer.image = m_sourcePicture->handle();
			toTransfer.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

			vkCmdPipelineBarrier(commandBuffer->handle(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &toTransfer);

			const VkDeviceSize lumaBytes = static_cast< VkDeviceSize >(m_settings.width) * m_settings.height;

			VkBufferImageCopy bufferRegion{};
			bufferRegion.bufferOffset = 0;
			bufferRegion.imageSubresource = {VK_IMAGE_ASPECT_PLANE_0_BIT, 0, 0, 1};
			bufferRegion.imageExtent = {m_settings.width, m_settings.height, 1};

			vkCmdCopyBufferToImage(commandBuffer->handle(), m_planeBounceBuffer->handle(), m_sourcePicture->handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferRegion);

			bufferRegion.bufferOffset = lumaBytes;
			bufferRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT;
			bufferRegion.imageExtent = {m_settings.width / 2, m_settings.height / 2, 1};

			vkCmdCopyBufferToImage(commandBuffer->handle(), m_planeBounceBuffer->handle(), m_sourcePicture->handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferRegion);
		}

		/* Layout transitions on the video queue (synchronization2: the video stages
		 * only exist there). */
		{
			std::vector< VkImageMemoryBarrier2 > barriers;
			barriers.reserve(1 + DpbSlotCount);

			{
				VkImageMemoryBarrier2 barrier{};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
				barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
				barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
				barrier.dstStageMask = VK_PIPELINE_STAGE_2_VIDEO_ENCODE_BIT_KHR;
				barrier.dstAccessMask = VK_ACCESS_2_VIDEO_ENCODE_READ_BIT_KHR;
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_VIDEO_ENCODE_SRC_KHR;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.image = m_sourcePicture->handle();
				barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

				barriers.emplace_back(barrier);
			}

			/* DPB pictures: initial transition on the first frame, then a per-frame
			 * availability barrier — the reconstruction written by encode N must be
			 * made visible to the reference reads of encode N+1 (submissions do not
			 * imply memory dependencies). */
			{
				VkImageMemoryBarrier2 barrier{};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
				barrier.srcStageMask = m_dpbInitialized ? VK_PIPELINE_STAGE_2_VIDEO_ENCODE_BIT_KHR : VK_PIPELINE_STAGE_2_NONE;
				barrier.srcAccessMask = m_dpbInitialized ? VK_ACCESS_2_VIDEO_ENCODE_WRITE_BIT_KHR : 0;
				barrier.dstStageMask = VK_PIPELINE_STAGE_2_VIDEO_ENCODE_BIT_KHR;
				barrier.dstAccessMask = VK_ACCESS_2_VIDEO_ENCODE_READ_BIT_KHR | VK_ACCESS_2_VIDEO_ENCODE_WRITE_BIT_KHR;
				barrier.oldLayout = m_dpbInitialized ? VK_IMAGE_LAYOUT_VIDEO_ENCODE_DPB_KHR : VK_IMAGE_LAYOUT_UNDEFINED;
				barrier.newLayout = VK_IMAGE_LAYOUT_VIDEO_ENCODE_DPB_KHR;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.image = m_dpbPictures[0]->handle();
				barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, DpbSlotCount};

				barriers.emplace_back(barrier);
			}

			m_dpbInitialized = true;

			VkDependencyInfo dependencyInfo{};
			dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			dependencyInfo.imageMemoryBarrierCount = static_cast< uint32_t >(barriers.size());
			dependencyInfo.pImageMemoryBarriers = barriers.data();

			vkCmdPipelineBarrier2(commandBuffer->handle(), &dependencyInfo);
		}

		/* Persistent rate-control state, declared at every BeginVideoCoding. */
		VkVideoEncodeH265RateControlInfoKHR h265RateControl{};
		h265RateControl.sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_RATE_CONTROL_INFO_KHR;
		h265RateControl.flags = VK_VIDEO_ENCODE_H265_RATE_CONTROL_REGULAR_GOP_BIT_KHR | VK_VIDEO_ENCODE_H265_RATE_CONTROL_REFERENCE_PATTERN_FLAT_BIT_KHR;
		h265RateControl.gopFrameCount = m_settings.idrPeriod;
		h265RateControl.idrPeriod = m_settings.idrPeriod;
		h265RateControl.consecutiveBFrameCount = 0;
		h265RateControl.subLayerCount = 1;

		/* NVIDIA expects the codec-specific layer structure chained to the layer
		 * (QP bounds left to the implementation). */
		VkVideoEncodeH265RateControlLayerInfoKHR h265RateControlLayer{};
		h265RateControlLayer.sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_RATE_CONTROL_LAYER_INFO_KHR;

		VkVideoEncodeRateControlLayerInfoKHR rateControlLayer{};
		rateControlLayer.sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_RATE_CONTROL_LAYER_INFO_KHR;
		rateControlLayer.pNext = &h265RateControlLayer;
		rateControlLayer.averageBitrate = static_cast< uint64_t >(m_settings.averageBitrateKbps) * 1000ULL;
		rateControlLayer.maxBitrate = static_cast< uint64_t >(m_settings.maximumBitrateKbps) * 1000ULL;
		rateControlLayer.frameRateNumerator = m_settings.frameRate;
		rateControlLayer.frameRateDenominator = 1;

		VkVideoEncodeRateControlInfoKHR rateControl{};
		rateControl.sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_RATE_CONTROL_INFO_KHR;
		rateControl.pNext = &h265RateControl;
		rateControl.rateControlMode = VK_VIDEO_ENCODE_RATE_CONTROL_MODE_VBR_BIT_KHR;
		rateControl.layerCount = 1;
		rateControl.pLayers = &rateControlLayer;
		rateControl.virtualBufferSizeInMs = 1000;
		rateControl.initialVirtualBufferSizeInMs = 500;

		/* DPB slot std infos (setup + reference). */
		StdVideoEncodeH265ReferenceInfo stdSetupReference{};
		stdSetupReference.pic_type = isIDR ? STD_VIDEO_H265_PICTURE_TYPE_IDR : STD_VIDEO_H265_PICTURE_TYPE_P;
		stdSetupReference.PicOrderCntVal = static_cast< int32_t >(m_gopFrameIndex);

		VkVideoEncodeH265DpbSlotInfoKHR setupDpbInfo{};
		setupDpbInfo.sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_DPB_SLOT_INFO_KHR;
		setupDpbInfo.pStdReferenceInfo = &stdSetupReference;

		StdVideoEncodeH265ReferenceInfo stdPreviousReference{};
		stdPreviousReference.pic_type = m_gopFrameIndex == 1 ? STD_VIDEO_H265_PICTURE_TYPE_IDR : STD_VIDEO_H265_PICTURE_TYPE_P;
		stdPreviousReference.PicOrderCntVal = static_cast< int32_t >(m_gopFrameIndex) - 1;

		VkVideoEncodeH265DpbSlotInfoKHR referenceDpbInfo{};
		referenceDpbInfo.sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_DPB_SLOT_INFO_KHR;
		referenceDpbInfo.pStdReferenceInfo = &stdPreviousReference;

		VkVideoPictureResourceInfoKHR setupResource{};
		setupResource.sType = VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_INFO_KHR;
		setupResource.codedExtent = codedExtent;
		setupResource.imageViewBinding = m_dpbViews[m_setupSlot];

		VkVideoPictureResourceInfoKHR referenceResource{};
		referenceResource.sType = VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_INFO_KHR;
		referenceResource.codedExtent = codedExtent;
		referenceResource.imageViewBinding = m_dpbViews[referenceSlot];

		/* Begin coding: declare every DPB slot used in this scope, each with its REAL
		 * index (declaring the setup slot as -1, decode-sample style, breaks the
		 * reference reads on the NVIDIA encoder). */
		std::array< VkVideoReferenceSlotInfoKHR, 2 > boundSlots{};
		boundSlots[0].sType = VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_INFO_KHR;
		boundSlots[0].pNext = &setupDpbInfo;
		boundSlots[0].slotIndex = -1; /* Being activated in this scope (reference encoder convention). */
		boundSlots[0].pPictureResource = &setupResource;
		boundSlots[1].sType = VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_INFO_KHR;
		boundSlots[1].pNext = &referenceDpbInfo;
		boundSlots[1].slotIndex = static_cast< int32_t >(referenceSlot);
		boundSlots[1].pPictureResource = &referenceResource;

		VkVideoBeginCodingInfoKHR beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_VIDEO_BEGIN_CODING_INFO_KHR;
		beginInfo.pNext = m_rateControlInitialized ? &rateControl : nullptr;
		beginInfo.videoSession = m_session;
		beginInfo.videoSessionParameters = m_sessionParameters;
		beginInfo.referenceSlotCount = isIDR ? 1 : 2;
		beginInfo.pReferenceSlots = boundSlots.data();

		m_fpCmdBeginVideoCoding(commandBuffer->handle(), &beginInfo);

		/* First frame: reset the session, then program the rate control (two control
		 * operations — the combined RESET|RATE_CONTROL form is also legal, kept split
		 * for clarity). */
		if ( !m_rateControlInitialized )
		{
			{
				VkVideoCodingControlInfoKHR controlInfo{};
				controlInfo.sType = VK_STRUCTURE_TYPE_VIDEO_CODING_CONTROL_INFO_KHR;
				controlInfo.flags = VK_VIDEO_CODING_CONTROL_RESET_BIT_KHR;

				m_fpCmdControlVideoCoding(commandBuffer->handle(), &controlInfo);
			}

			{
				VkVideoCodingControlInfoKHR controlInfo{};
				controlInfo.sType = VK_STRUCTURE_TYPE_VIDEO_CODING_CONTROL_INFO_KHR;
				controlInfo.pNext = &rateControl;
				controlInfo.flags = VK_VIDEO_CODING_CONTROL_ENCODE_RATE_CONTROL_BIT_KHR;

				m_fpCmdControlVideoCoding(commandBuffer->handle(), &controlInfo);
			}

			m_rateControlInitialized = true;
		}

		/* Picture info. */
		StdVideoEncodeH265SliceSegmentHeader stdSliceHeader{};
		stdSliceHeader.flags.first_slice_segment_in_pic_flag = 1;
		stdSliceHeader.slice_type = isIDR ? STD_VIDEO_H265_SLICE_TYPE_I : STD_VIDEO_H265_SLICE_TYPE_P;
		stdSliceHeader.MaxNumMergeCand = 5;

		VkVideoEncodeH265NaluSliceSegmentInfoKHR sliceSegment{};
		sliceSegment.sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_NALU_SLICE_SEGMENT_INFO_KHR;
		sliceSegment.constantQp = 0;
		sliceSegment.pStdSliceSegmentHeader = &stdSliceHeader;

		StdVideoEncodeH265ReferenceListsInfo stdReferenceLists{};
		std::memset(stdReferenceLists.RefPicList0, STD_VIDEO_H265_NO_REFERENCE_PICTURE, sizeof(stdReferenceLists.RefPicList0));
		std::memset(stdReferenceLists.RefPicList1, STD_VIDEO_H265_NO_REFERENCE_PICTURE, sizeof(stdReferenceLists.RefPicList1));

		StdVideoH265ShortTermRefPicSet stdShortTermRefPicSet{};

		StdVideoEncodeH265PictureInfo stdPictureInfo{};
		stdPictureInfo.flags.is_reference = 1;
		stdPictureInfo.flags.IrapPicFlag = isIDR ? 1 : 0;
		stdPictureInfo.pic_type = isIDR ? STD_VIDEO_H265_PICTURE_TYPE_IDR : STD_VIDEO_H265_PICTURE_TYPE_P;
		stdPictureInfo.PicOrderCntVal = static_cast< int32_t >(m_gopFrameIndex);
		stdPictureInfo.pShortTermRefPicSet = &stdShortTermRefPicSet;

		if ( !isIDR )
		{
			stdReferenceLists.RefPicList0[0] = static_cast< uint8_t >(referenceSlot);
			stdPictureInfo.pRefLists = &stdReferenceLists;

			/* Short-term RPS: one negative-delta reference, used by the current picture. */
			stdShortTermRefPicSet.num_negative_pics = 1;
			stdShortTermRefPicSet.used_by_curr_pic_s0_flag = 1;
		}

		VkVideoEncodeH265PictureInfoKHR h265PictureInfo{};
		h265PictureInfo.sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_PICTURE_INFO_KHR;
		h265PictureInfo.naluSliceSegmentEntryCount = 1;
		h265PictureInfo.pNaluSliceSegmentEntries = &sliceSegment;
		h265PictureInfo.pStdPictureInfo = &stdPictureInfo;

		/* The setup slot now carries its real index for the encode itself. */
		VkVideoReferenceSlotInfoKHR setupSlotInfo = boundSlots[0];
		setupSlotInfo.slotIndex = static_cast< int32_t >(m_setupSlot);

		VkVideoEncodeInfoKHR encodeInfo{};
		encodeInfo.sType = VK_STRUCTURE_TYPE_VIDEO_ENCODE_INFO_KHR;
		encodeInfo.pNext = &h265PictureInfo;
		encodeInfo.dstBuffer = m_bitstreamBuffer->handle();
		encodeInfo.dstBufferOffset = 0;
		encodeInfo.dstBufferRange = m_bitstreamBuffer->bytes();
		encodeInfo.srcPictureResource.sType = VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_INFO_KHR;
		encodeInfo.srcPictureResource.codedExtent = codedExtent;
		encodeInfo.srcPictureResource.imageViewBinding = m_sourceView;
		encodeInfo.pSetupReferenceSlot = &setupSlotInfo;
		encodeInfo.referenceSlotCount = isIDR ? 0 : 1;
		encodeInfo.pReferenceSlots = isIDR ? nullptr : &boundSlots[1];

		vkCmdBeginQuery(commandBuffer->handle(), m_feedbackQueryPool, 0, 0);
		m_fpCmdEncodeVideo(commandBuffer->handle(), &encodeInfo);
		vkCmdEndQuery(commandBuffer->handle(), m_feedbackQueryPool, 0);

		VkVideoEndCodingInfoKHR endInfo{};
		endInfo.sType = VK_STRUCTURE_TYPE_VIDEO_END_CODING_INFO_KHR;

		m_fpCmdEndVideoCoding(commandBuffer->handle(), &endInfo);

		if ( !commandBuffer->end() )
		{
			Tracer::error(ClassId, "Unable to end the encode command buffer !");

			return false;
		}

		auto * videoQueue = m_device->getVideoEncodeQueue(QueuePriority::High);

		if ( videoQueue == nullptr || !videoQueue->submit(*commandBuffer) || !videoQueue->waitIdle() )
		{
			Tracer::error(ClassId, "The encode submission failed !");

			return false;
		}

		/* Fetch the packet location through the feedback query. */
		std::array< uint32_t, 2 > feedback{0, 0}; /* offset, bytes written. */

		if ( vkGetQueryPoolResults(m_device->handle(), m_feedbackQueryPool, 0, 1, sizeof(feedback), feedback.data(), sizeof(feedback), VK_QUERY_RESULT_WAIT_BIT) != VK_SUCCESS )
		{
			Tracer::error(ClassId, "Unable to read the encode feedback query !");

			return false;
		}

		packet.assign(m_bitstreamMapped + feedback[0], m_bitstreamMapped + feedback[0] + feedback[1]);

		/* GOP bookkeeping: the reconstructed picture becomes the next reference. */
		m_frameIndex++;
		m_gopFrameIndex++;

		if ( m_gopFrameIndex >= m_settings.idrPeriod )
		{
			m_gopFrameIndex = 0;
		}

		m_setupSlot = referenceSlot;

		return true;
	}

	void
	VideoEncoderH265::destroy () noexcept
	{
		if ( m_device == nullptr || m_device->handle() == VK_NULL_HANDLE )
		{
			return;
		}

		if ( auto * videoQueue = m_device->getVideoEncodeQueue(QueuePriority::High); videoQueue != nullptr )
		{
			static_cast< void >(videoQueue->waitIdle());
		}

		if ( m_feedbackQueryPool != VK_NULL_HANDLE )
		{
			vkDestroyQueryPool(m_device->handle(), m_feedbackQueryPool, nullptr);
			m_feedbackQueryPool = VK_NULL_HANDLE;
		}

		if ( m_bitstreamMapped != nullptr )
		{
			m_bitstreamBuffer->unmapMemory();
			m_bitstreamMapped = nullptr;
		}

		m_bitstreamBuffer.reset();
		m_planeBounceBuffer.reset();

		if ( m_sourceView != VK_NULL_HANDLE )
		{
			vkDestroyImageView(m_device->handle(), m_sourceView, nullptr);
			m_sourceView = VK_NULL_HANDLE;
		}

		for ( auto & view : m_dpbViews )
		{
			if ( view != VK_NULL_HANDLE )
			{
				vkDestroyImageView(m_device->handle(), view, nullptr);
				view = VK_NULL_HANDLE;
			}
		}

		m_sourcePicture.reset();

		for ( auto & picture : m_dpbPictures )
		{
			picture.reset();
		}

		if ( m_sessionParameters != VK_NULL_HANDLE )
		{
			m_fpDestroySessionParameters(m_device->handle(), m_sessionParameters, nullptr);
			m_sessionParameters = VK_NULL_HANDLE;
		}

		if ( m_session != VK_NULL_HANDLE )
		{
			m_fpDestroyVideoSession(m_device->handle(), m_session, nullptr);
			m_session = VK_NULL_HANDLE;
		}

		for ( auto & memory : m_sessionMemory )
		{
			vkFreeMemory(m_device->handle(), memory, nullptr);
		}

		m_sessionMemory.clear();
		m_videoCommandPool.reset();
		m_graphicsCommandPool.reset();
		m_encodedParameters.clear();
		m_frameIndex = 0;
		m_gopFrameIndex = 0;
		m_setupSlot = 0;
		m_rateControlInitialized = false;
		m_dpbInitialized = false;
	}
}

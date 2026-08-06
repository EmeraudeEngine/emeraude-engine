/*
 * src/Vulkan/VideoEncoderH265.hpp
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

/* STL inclusions. */
#include <array>
#include <cstdint>
#include <memory>
#include <vector>

/* Local inclusions. */
#include "Vulkan/Buffer.hpp"
#include "Vulkan/Image.hpp"

namespace EmEn::Vulkan
{
	class Device;
	class CommandPool;

	/**
	 * @class VideoEncoderH265
	 * @brief Hardware H.265 encoder built on the Vulkan Video encode extensions (RushMaker hardware path).
	 *
	 * Consumes the NV12 planes produced by Graphics::VideoFrameConverter, copies them into
	 * the multi-planar VIDEO_ENCODE_SRC picture, and drives the dedicated VIDEO_ENCODE queue:
	 * session, session parameters (the driver returns the encoded VPS/SPS/PPS), a two-slot
	 * DPB with an IDR+P GOP, VBR rate control, and an encode-feedback query to size each
	 * produced packet. Output is an Annex-B elementary stream (headerBytes() first, then one
	 * packet per encodeFrame()).
	 *
	 * References: Khronos proposal VK_KHR_video_encode_h265 (docs.vulkan.org) and
	 * nvpro-samples/vk_video_samples (reference encoder implementation).
	 *
	 * @note The profile is parametric by design (HDR10 lookahead): Settings selects the
	 * bit depth — only 8-bit main is implemented today, Main 10 / P010 is the planned
	 * extension and no API here assumes 8-bit.
	 *
	 * @version 0.8.52
	 */
	class VideoEncoderH265 final
	{
		public:

			/** @brief Class identifier for logging. */
			static constexpr auto ClassId{"VideoEncoderH265"};

			/** @brief Encoder creation settings. */
			struct Settings
			{
				uint32_t width{0}; ///< Frame width in pixels (even).
				uint32_t height{0}; ///< Frame height in pixels (even).
				uint32_t frameRate{30}; ///< Constant frame rate (timebase for the rate control).
				uint32_t averageBitrateKbps{12000}; ///< VBR average bitrate.
				uint32_t maximumBitrateKbps{25000}; ///< VBR ceiling.
				uint32_t idrPeriod{60}; ///< Keyframe interval in frames (seeking granularity).
				uint32_t qualityLevel{0}; ///< Implementation quality level (clamped to the device maximum).
			};

			/**
			 * @brief Constructs the encoder.
			 * @param device A reference to the device smart pointer (must have videoEncodeH265Enabled()).
			 */
			explicit VideoEncoderH265 (const std::shared_ptr< Device > & device) noexcept;

			/** @brief Destructor, releases every Vulkan Video resource. */
			~VideoEncoderH265 ();

			VideoEncoderH265 (const VideoEncoderH265 & copy) noexcept = delete;
			VideoEncoderH265 (VideoEncoderH265 && copy) noexcept = delete;
			VideoEncoderH265 & operator= (const VideoEncoderH265 & copy) noexcept = delete;
			VideoEncoderH265 & operator= (VideoEncoderH265 && copy) noexcept = delete;

			/**
			 * @brief Creates the video session, parameters, DPB and bitstream resources.
			 * @param settings The encoder settings.
			 * @return bool True on success.
			 */
			[[nodiscard]]
			bool create (const Settings & settings) noexcept;

			/** @brief Releases every resource (waits for the video queue). */
			void destroy () noexcept;

			/**
			 * @brief Returns the encoded VPS/SPS/PPS as an Annex-B block, to be written once
			 * at the beginning of the elementary stream.
			 * @return const std::vector< uint8_t > &
			 */
			[[nodiscard]]
			const std::vector< uint8_t > &
			headerBytes () const noexcept
			{
				return m_encodedParameters;
			}

			/**
			 * @brief Encodes one frame from the converter planes (synchronous).
			 *
			 * Copies the R8/R8G8 planes into the NV12 source picture (graphics queue), then
			 * records and submits the encode on the video queue, waits, and reads the packet
			 * back through the encode-feedback query.
			 *
			 * @param lumaPlane The R8 luma image (TRANSFER_SRC layout, converter output).
			 * @param chromaPlane The R8G8 chroma image (TRANSFER_SRC layout, converter output).
			 * @param packet Writable Annex-B packet for this frame.
			 * @param wasIDR Writable flag: true when the frame was encoded as an IDR.
			 * @return bool True on success.
			 */
			[[nodiscard]]
			bool encodeFrame (const Image & lumaPlane, const Image & chromaPlane, std::vector< uint8_t > & packet, bool & wasIDR) noexcept;

		private:

			/**
			 * @brief Resolves the Vulkan Video entry points through vkGetDeviceProcAddr.
			 * @return bool True when every function was found.
			 */
			[[nodiscard]]
			bool resolveEntryPoints () noexcept;

			/**
			 * @brief Creates the video session and binds its device memory.
			 * @return bool True on success.
			 */
			[[nodiscard]]
			bool createSession () noexcept;

			/**
			 * @brief Builds the std VPS/SPS/PPS, creates the session parameters and
			 * retrieves the driver-encoded Annex-B header block.
			 * @return bool True on success.
			 */
			[[nodiscard]]
			bool createSessionParameters () noexcept;

			/**
			 * @brief Creates the NV12 source picture, the DPB images and the bitstream buffer.
			 * @return bool True on success.
			 */
			[[nodiscard]]
			bool createPictures () noexcept;

			/** @brief Number of DPB slots (1 active reference + 1 being set up). */
			static constexpr uint32_t DpbSlotCount{2};

			std::shared_ptr< Device > m_device;
			Settings m_settings{};

			/* Video profile (identical chain everywhere: images, buffer, query pool, session). */
			VkVideoEncodeH265ProfileInfoKHR m_h265Profile{};
			VkVideoProfileInfoKHR m_profile{};
			VkVideoProfileListInfoKHR m_profileList{};

			/* Entry points (device-level extension functions). */
			PFN_vkCreateVideoSessionKHR m_fpCreateVideoSession{nullptr};
			PFN_vkDestroyVideoSessionKHR m_fpDestroyVideoSession{nullptr};
			PFN_vkGetVideoSessionMemoryRequirementsKHR m_fpGetSessionMemoryRequirements{nullptr};
			PFN_vkBindVideoSessionMemoryKHR m_fpBindSessionMemory{nullptr};
			PFN_vkCreateVideoSessionParametersKHR m_fpCreateSessionParameters{nullptr};
			PFN_vkDestroyVideoSessionParametersKHR m_fpDestroySessionParameters{nullptr};
			PFN_vkGetEncodedVideoSessionParametersKHR m_fpGetEncodedSessionParameters{nullptr};
			PFN_vkCmdBeginVideoCodingKHR m_fpCmdBeginVideoCoding{nullptr};
			PFN_vkCmdEndVideoCodingKHR m_fpCmdEndVideoCoding{nullptr};
			PFN_vkCmdControlVideoCodingKHR m_fpCmdControlVideoCoding{nullptr};
			PFN_vkCmdEncodeVideoKHR m_fpCmdEncodeVideo{nullptr};

			/* Session objects. */
			VkVideoSessionKHR m_session{VK_NULL_HANDLE};
			VkVideoSessionParametersKHR m_sessionParameters{VK_NULL_HANDLE};
			std::vector< VkDeviceMemory > m_sessionMemory;
			std::vector< uint8_t > m_encodedParameters;

			/* Std parameter sets (must outlive the session parameters creation). */
			StdVideoH265ProfileTierLevel m_stdProfileTierLevel{};
			StdVideoH265DecPicBufMgr m_stdDecPicBufMgr{};
			StdVideoH265VideoParameterSet m_stdVPS{};
			StdVideoH265SequenceParameterSet m_stdSPS{};
			StdVideoH265PictureParameterSet m_stdPPS{};

			/* Pictures. */
			std::unique_ptr< Image > m_sourcePicture; ///< NV12 multi-planar VIDEO_ENCODE_SRC (concurrent graphics+video).
			std::array< std::unique_ptr< Image >, DpbSlotCount > m_dpbPictures{}; ///< Reconstructed reference pictures.
			VkImageView m_sourceView{VK_NULL_HANDLE};
			std::array< VkImageView, DpbSlotCount > m_dpbViews{VK_NULL_HANDLE, VK_NULL_HANDLE};

			/* Plane transfer bounce (image->buffer->multi-planar image: the direct
			 * image-to-plane copy path corrupts on NVIDIA, the buffer path is the one
			 * proven by vk_video_samples). */
			std::unique_ptr< Buffer > m_planeBounceBuffer;

			/* Bitstream readback. */
			std::unique_ptr< Buffer > m_bitstreamBuffer;
			const uint8_t * m_bitstreamMapped{nullptr};
			VkQueryPool m_feedbackQueryPool{VK_NULL_HANDLE};
			VkDeviceSize m_bitstreamAlignment{1};

			/* Command recording. */
			std::shared_ptr< CommandPool > m_graphicsCommandPool;
			std::shared_ptr< CommandPool > m_videoCommandPool;

			/* GOP state. */
			uint64_t m_frameIndex{0};
			uint32_t m_gopFrameIndex{0}; ///< Position inside the current GOP (0 = IDR).
			uint32_t m_setupSlot{0}; ///< DPB slot receiving the reconstructed picture.
			bool m_rateControlInitialized{false};
			bool m_dpbInitialized{false};
	};
}

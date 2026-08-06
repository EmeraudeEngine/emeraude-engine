/*
 * src/Graphics/VideoFrameConverter.hpp
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
#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

/* Local inclusions. */
#include "Vulkan/Image.hpp"

namespace EmEn
{
	namespace Vulkan
	{
		class Device;
		class ImageView;
		class DescriptorSetLayout;
		class DescriptorPool;
		class DescriptorSet;
		class PipelineLayout;
		class ComputePipeline;
	}

	namespace Saphir
	{
		class ShaderManager;
	}
}

namespace EmEn::Graphics
{
	/**
	 * @class VideoFrameConverter
	 * @brief GPU BGRA→I420 converter for the hardware video-encode path (RushMaker).
	 *
	 * Produces a full-resolution R8 luma plane and a half-resolution R8G8 interleaved
	 * chroma plane (the NV12 layout) with a compute shader. The shader uses the SHARED
	 * BT.709 integer coefficients (VideoColorConversion.hpp) with the exact integer
	 * math of the CPU converters, so the GPU output is byte-for-byte identical to the
	 * software path — validated by selfTest().
	 *
	 * The M3 encode session copies these planes into the multi-planar
	 * VIDEO_ENCODE_SRC image (aspects PLANE_0 / PLANE_1).
	 *
	 * @version 0.8.52
	 */
	class VideoFrameConverter final
	{
		public:

			/** @brief Class identifier for logging. */
			static constexpr auto ClassId{"VideoFrameConverter"};

			/**
			 * @brief Constructs the converter.
			 * @param device A reference to the device smart pointer.
			 * @param shaderManager A reference to the shader manager (compute shader compilation).
			 */
			VideoFrameConverter (const std::shared_ptr< Vulkan::Device > & device, Saphir::ShaderManager & shaderManager) noexcept;

			/** @brief Destructor, releases the GPU resources. */
			~VideoFrameConverter ();

			VideoFrameConverter (const VideoFrameConverter & copy) noexcept = delete;
			VideoFrameConverter (VideoFrameConverter && copy) noexcept = delete;
			VideoFrameConverter & operator= (const VideoFrameConverter & copy) noexcept = delete;
			VideoFrameConverter & operator= (VideoFrameConverter && copy) noexcept = delete;

			/**
			 * @brief Creates the GPU resources for a given frame size.
			 * @param width The frame width in pixels (must be even).
			 * @param height The frame height in pixels (must be even).
			 * @return bool True on success.
			 */
			[[nodiscard]]
			bool create (uint32_t width, uint32_t height) noexcept;

			/** @brief Releases every GPU resource. */
			void destroy () noexcept;

			/**
			 * @brief Converts a real BGRA source image into the NV12 planes (synchronous).
			 * @note The source must be in SHADER_READ_ONLY_OPTIMAL layout and match the
			 * converter dimensions. The planes are left in TRANSFER_SRC_OPTIMAL for the
			 * encoder plane copies.
			 * @param sourceView The view of the BGRA source image.
			 * @return bool True on success.
			 */
			[[nodiscard]]
			bool convertFrom (const std::shared_ptr< Vulkan::ImageView > & sourceView) noexcept;

			/**
			 * @brief TEMPORARY DEBUG: dumps both planes as binary PGM files.
			 * @param basePath Base path (suffixes _luma.pgm / _chroma.pgm are appended).
			 * @return bool
			 */
			[[nodiscard]]
			bool debugDumpPlanes (const std::filesystem::path & basePath) noexcept;

			/**
			 * @brief Runs the conversion on the PROCEDURAL test pattern and compares the
			 * GPU planes byte-for-byte against the CPU reference conversion.
			 *
			 * The test source is a deterministic integer hash of the pixel coordinates,
			 * generated identically in GLSL and in C++ — no upload involved, the whole
			 * integer conversion math and plane layout are exercised.
			 *
			 * @param mismatchedBytes Writable count of mismatching bytes (0 on success).
			 * @return bool True when the planes match exactly.
			 */
			[[nodiscard]]
			bool selfTest (uint64_t & mismatchedBytes) noexcept;

			/** @brief Returns the luma plane image (R8, full resolution). */
			[[nodiscard]]
			const std::shared_ptr< Vulkan::Image > &
			lumaImage () const noexcept
			{
				return m_lumaImage;
			}

			/** @brief Returns the chroma plane image (R8G8 interleaved U/V, half resolution). */
			[[nodiscard]]
			const std::shared_ptr< Vulkan::Image > &
			chromaImage () const noexcept
			{
				return m_chromaImage;
			}

		private:

			/**
			 * @brief Records the barriers + dispatch into a one-shot command buffer and submits it.
			 * @param production True to run the real-source pipeline, false for the test pattern.
			 * @return bool True on success.
			 */
			[[nodiscard]]
			bool dispatchConversion (bool production) noexcept;

			/**
			 * @brief Reads both planes back to host memory.
			 * @param luma Writable luma bytes (width × height).
			 * @param chroma Writable chroma bytes (width × height / 2, interleaved U/V).
			 * @return bool True on success.
			 */
			[[nodiscard]]
			bool readbackPlanes (std::vector< uint8_t > & luma, std::vector< uint8_t > & chroma) noexcept;

			std::shared_ptr< Vulkan::Device > m_device;
			Saphir::ShaderManager * m_shaderManager{nullptr};
			std::shared_ptr< Vulkan::Image > m_lumaImage;
			std::shared_ptr< Vulkan::ImageView > m_lumaView;
			std::shared_ptr< Vulkan::Image > m_chromaImage;
			std::shared_ptr< Vulkan::ImageView > m_chromaView;
			std::shared_ptr< Vulkan::DescriptorSetLayout > m_descriptorSetLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_pipelineLayout;
			std::unique_ptr< Vulkan::ComputePipeline > m_computePipeline;
			std::shared_ptr< Vulkan::DescriptorPool > m_descriptorPool;
			std::unique_ptr< Vulkan::DescriptorSet > m_descriptorSet;
			/* Production pipeline (real BGRA source through a sampler). */
			std::shared_ptr< Vulkan::DescriptorSetLayout > m_productionSetLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_productionPipelineLayout;
			std::unique_ptr< Vulkan::ComputePipeline > m_productionPipeline;
			std::unique_ptr< Vulkan::DescriptorSet > m_productionSet;
			VkSampler m_sampler{VK_NULL_HANDLE};
			uint32_t m_width{0};
			uint32_t m_height{0};
	};
}

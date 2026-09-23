/*
 * src/Graphics/FrameCapture.cpp
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

#include "FrameCapture.hpp"

/* STL inclusions. */
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <span>
#include <sstream>

/* Local inclusions. */
#include "PixelFactory/FileIO.hpp"
#include "PixelFactory/Processor.hpp"
#include "ThreadPool.hpp"
#include "Tracer.hpp"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/Sync/BufferMemoryBarrier.hpp"
#include "Vulkan/Sync/ImageMemoryBarrier.hpp"

namespace EmEn::Graphics
{
	using namespace Base;
	using namespace Base::PixelFactory;

	namespace
	{
		/**
		 * @brief Writes a float array as a JSON array.
		 * @param output The stream.
		 * @param values The values.
		 * @return void
		 */
		template< size_t count_t >
		void
		writeArray (std::ostream & output, const std::array< float, count_t > & values)
		{
			output << '[';

			for ( size_t index = 0; index < count_t; ++index )
			{
				output << (index > 0 ? ", " : "") << values[index];
			}

			output << ']';
		}
	}

	FrameCapture::FrameCapture () noexcept = default;

	FrameCapture::~FrameCapture () = default;

	bool
	FrameCapture::arm (const std::shared_ptr< Vulkan::Device > & device, const std::shared_ptr< ThreadPool > & threadPool, uint32_t width, uint32_t height, uint32_t frameCount, const std::filesystem::path & directory, bool temporal, std::string & error) noexcept
	{
		if ( device == nullptr || threadPool == nullptr )
		{
			error = "No device or thread pool to capture with.";

			return false;
		}

		if ( width == 0 || height == 0 || frameCount == 0 )
		{
			error = "Nothing to capture (empty swap-chain or zero frames).";

			return false;
		}

		const auto frameBytes = static_cast< uint64_t >(width) * height * 4;

		if ( frameBytes * frameCount > MaxCaptureBytes )
		{
			std::stringstream message;
			message << frameCount << " frames of " << width << "x" << height << " need " << (frameBytes * frameCount) / (1024 * 1024) << " MiB of staging memory, above the " << MaxCaptureBytes / (1024 * 1024) << " MiB a capture may hold (at most " << MaxCaptureBytes / frameBytes << " frames at this size).";
			error = message.str();

			return false;
		}

		{
			const std::scoped_lock lock{m_access};

			if ( m_state != State::Idle )
			{
				error = "A capture is already in progress.";

				return false;
			}

			/* Reserved: a concurrent arm() is refused while the buffers are allocated outside the lock. */
			m_state = State::Writing;
		}

		/* The staging buffers, allocated here so the captured frames pay nothing but their copy. */
		std::vector< CapturedFrame > frames(frameCount);

		for ( auto & frame : frames )
		{
			frame.stagingBuffer = std::make_unique< Vulkan::Buffer >(device, static_cast< VkBufferCreateFlags >(0), frameBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true);
			frame.stagingBuffer->setHostReadable(true);

			if ( !frame.stagingBuffer->createOnHardware() )
			{
				error = "Unable to allocate the capture staging buffers.";

				const std::scoped_lock lock{m_access};

				m_state = State::Idle;

				return false;
			}
		}

		const std::scoped_lock lock{m_access};

		m_frames = std::move(frames);
		m_threadPool = threadPool;
		m_directory = directory;
		m_stem = std::chrono::duration_cast< std::chrono::seconds >(std::chrono::system_clock::now().time_since_epoch()).count();
		m_width = width;
		m_height = height;
		m_nextFrame = 0;
		m_temporal = temporal;
		m_result = {};
		m_resultReady = false;
		m_state = State::Armed;
		m_wantsFrames.store(true, std::memory_order_release);

		return true;
	}

	FrameCapture::Result
	FrameCapture::waitForCompletion (std::chrono::milliseconds timeout) noexcept
	{
		std::unique_lock lock{m_access};

		if ( !m_completion.wait_for(lock, timeout, [this] {
			return m_resultReady;
		}) )
		{
			return {.files = {}, .error = "The capture did not complete in time (is the window rendering?).", .success = false};
		}

		m_resultReady = false;

		return std::move(m_result);
	}

	bool
	FrameCapture::busy () const noexcept
	{
		const std::scoped_lock lock{m_access};

		return m_state != State::Idle;
	}

	bool
	FrameCapture::recordCopy (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::Image & image, VkImageLayout finalLayout, uint32_t frameSlot, const FrameMetadata & metadata) noexcept
	{
		const std::scoped_lock lock{m_access};

		if ( m_state != State::Armed || m_nextFrame >= m_frames.size() || !m_result.error.empty() )
		{
			return false;
		}

		const auto & createInfo = image.createInfo();

		if ( createInfo.extent.width != m_width || createInfo.extent.height != m_height )
		{
			m_result.error = "The swap-chain was resized during the capture.";
			m_wantsFrames.store(false, std::memory_order_release);

			return false;
		}

		switch ( createInfo.format )
		{
			case VK_FORMAT_B8G8R8A8_UNORM :
			case VK_FORMAT_B8G8R8A8_SRGB :
				m_swapRedBlue = true;
				break;

			case VK_FORMAT_R8G8B8A8_UNORM :
			case VK_FORMAT_R8G8B8A8_SRGB :
				m_swapRedBlue = false;
				break;

			default :
			{
				std::stringstream message;
				message << "The swap-chain format " << createInfo.format << " is not an 8-bit RGBA/BGRA format: it cannot be written as a PNG.";
				m_result.error = message.str();
				m_wantsFrames.store(false, std::memory_order_release);

				return false;
			}
		}

		auto & frame = m_frames[m_nextFrame];

		/* The last pass left the image in its final layout (the render pass finalLayout): its colour writes are
		 * made available to the transfer, then the image goes back to that layout for the present, which waits on
		 * the semaphore this very submit signals. */
		commandBuffer.pipelineBarrier(
			Vulkan::Sync::ImageMemoryBarrier{image, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, finalLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT},
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT
		);

		commandBuffer.copyImageToBuffer(image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *frame.stagingBuffer);

		commandBuffer.pipelineBarrier(
			Vulkan::Sync::ImageMemoryBarrier{image, VK_ACCESS_TRANSFER_READ_BIT, 0, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, finalLayout, VK_IMAGE_ASPECT_COLOR_BIT},
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
		);

		/* The copy is made visible to the host reads that follow the fence. */
		commandBuffer.pipelineBarrier(
			Vulkan::Sync::BufferMemoryBarrier{*frame.stagingBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT},
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_HOST_BIT
		);

		frame.metadata = metadata;
		frame.recordTime = std::chrono::steady_clock::now();
		frame.frameSlot = frameSlot;
		frame.recorded = true;

		return true;
	}

	void
	FrameCapture::confirmSubmit (bool submitted) noexcept
	{
		const std::scoped_lock lock{m_access};

		if ( m_state != State::Armed || m_nextFrame >= m_frames.size() || !m_frames[m_nextFrame].recorded )
		{
			return;
		}

		auto & frame = m_frames[m_nextFrame];

		if ( submitted )
		{
			frame.submitted = true;
		}
		else
		{
			/* Never executed: its buffer is free, and a temporal capture cannot have a hole. */
			frame.complete = true;

			m_result.error = "A frame of the capture was abandoned before its submit.";
		}

		m_nextFrame++;

		if ( m_nextFrame == m_frames.size() || !m_result.error.empty() )
		{
			m_wantsFrames.store(false, std::memory_order_release);
		}
	}

	void
	FrameCapture::onFrameSlotRetired (uint32_t frameSlot) noexcept
	{
		bool write = false;

		{
			const std::scoped_lock lock{m_access};

			if ( m_state != State::Armed )
			{
				return;
			}

			for ( auto & frame : m_frames )
			{
				if ( frame.submitted && !frame.complete && frame.frameSlot == frameSlot )
				{
					frame.complete = true;
				}
			}

			/* Done when no recorded copy is still on the GPU, and either every frame was captured or the capture failed. */
			const bool failed = !m_result.error.empty();
			const bool finished = failed || m_nextFrame == m_frames.size();
			const bool drained = std::ranges::all_of(m_frames, [] (const CapturedFrame & frame) {
				return !frame.submitted || frame.complete;
			});

			if ( !finished || !drained )
			{
				return;
			}

			if ( failed )
			{
				auto error = std::move(m_result.error);

				m_frames.clear();

				this->publish({.files = {}, .error = std::move(error), .success = false});

				return;
			}

			m_state = State::Writing;
			write = true;
		}

		/* Outside the lock: the pool may run the task at once. */
		if ( write && !m_threadPool->enqueue([this] {
			this->writeFiles();
		}) )
		{
			this->writeFiles();
		}
	}

	void
	FrameCapture::abandon (const std::string & reason) noexcept
	{
		const std::scoped_lock lock{m_access};

		if ( m_state != State::Armed )
		{
			return;
		}

		m_frames.clear();

		this->publish({.files = {}, .error = reason, .success = false});
	}

	void
	FrameCapture::writeFiles () noexcept
	{
		/* The frames are this capture's alone while it is Writing: no lock for the read-back and the encoding. */
		const auto frameCount = m_frames.size();
		const auto frameBytes = static_cast< size_t >(m_width) * m_height * 4;

		std::vector< std::filesystem::path > files(frameCount);
		std::vector< uint8_t > written(frameCount, 0);

		const auto encode = [&] (size_t index) {
			auto & frame = m_frames[index];

			std::stringstream filename;
			filename << m_stem;

			if ( m_temporal )
			{
				filename << '-' << index;
			}

			filename << ".png";

			files[index] = m_directory / filename.str();

			const auto * pointer = frame.stagingBuffer->mapMemoryAs< uint8_t >();

			if ( pointer == nullptr )
			{
				return;
			}

			Pixmap< uint8_t > pixmap;
			const bool copied = pixmap.initialize(m_width, m_height, ChannelMode::RGBA, std::span< const uint8_t >{pointer, frameBytes});

			frame.stagingBuffer->unmapMemory();

			if ( !copied )
			{
				return;
			}

			if ( m_swapRedBlue )
			{
				pixmap = Processor< uint8_t >::swapChannels(pixmap, false);
			}

			pixmap = Processor< uint8_t >::toRGB(pixmap);

			written[index] = FileIO::write(pixmap, files[index]) ? 1 : 0;
		};

		m_threadPool->parallelFor(size_t{0}, frameCount, encode);

		Result result;

		for ( size_t index = 0; index < frameCount; ++index )
		{
			if ( written[index] == 0 )
			{
				result.error = "Unable to write " + files[index].string() + " (does it already exist?).";

				break;
			}
		}

		/* The metadata of a temporal capture: everything that moves a pixel from one frame to the next. */
		if ( result.error.empty() && m_temporal )
		{
			const auto jsonPath = m_directory / (std::to_string(m_stem) + ".json");
			std::ofstream json{jsonPath};

			if ( !json.is_open() )
			{
				result.error = "Unable to write " + jsonPath.string() + ".";
			}
			else
			{
				json << std::setprecision(9);
				json << "{\n";
				json << "\t\"stem\": " << m_stem << ",\n";
				json << "\t\"frameCount\": " << frameCount << ",\n";
				json << "\t\"width\": " << m_width << ",\n";
				json << "\t\"height\": " << m_height << ",\n";
				json << "\t\"frames\": [\n";

				const auto origin = m_frames.front().recordTime;
				auto previous = origin;

				for ( size_t index = 0; index < frameCount; ++index )
				{
					const auto & frame = m_frames[index];
					const auto & meta = frame.metadata;
					const auto timeMS = std::chrono::duration< double, std::milli >(frame.recordTime - origin).count();
					const auto deltaMS = std::chrono::duration< double, std::milli >(frame.recordTime - previous).count();

					previous = frame.recordTime;

					json << "\t\t{\n";
					json << "\t\t\t\"index\": " << index << ",\n";
					json << "\t\t\t\"file\": \"" << files[index].filename().string() << "\",\n";
					json << "\t\t\t\"rendererFrame\": " << meta.rendererFrame << ",\n";
					json << "\t\t\t\"recordTimeMS\": " << timeMS << ",\n";
					json << "\t\t\t\"deltaMS\": " << deltaMS << ",\n";
					json << "\t\t\t\"jitterNDC\": ";
					writeArray(json, meta.jitterNDC);
					json << ",\n";
					json << "\t\t\t\"jitterPixels\": [" << meta.jitterNDC[0] * 0.5F * static_cast< float >(m_width) << ", " << meta.jitterNDC[1] * 0.5F * static_cast< float >(m_height) << "],\n";
					json << "\t\t\t\"cameraPosition\": ";
					writeArray(json, meta.cameraPosition);
					json << ",\n";
					json << "\t\t\t\"viewMatrix\": ";
					writeArray(json, meta.viewMatrix);
					json << ",\n";

					if ( meta.hasCamera )
					{
						json << "\t\t\t\"exposure\": {\"auto\": " << (meta.autoExposure ? "true" : "false") << ", \"aperture\": " << meta.aperture << ", \"shutterSpeed\": " << meta.shutterSpeed << ", \"sensitivity\": " << meta.sensitivity << ", \"compensation\": " << meta.exposureCompensation << "}\n";
					}
					else
					{
						json << "\t\t\t\"exposure\": null\n";
					}

					json << "\t\t}" << (index + 1 < frameCount ? "," : "") << "\n";
				}

				json << "\t]\n";
				json << "}\n";

				files.push_back(jsonPath);
			}
		}

		if ( result.error.empty() )
		{
			result.files = std::move(files);
			result.success = true;
		}

		const std::scoped_lock lock{m_access};

		m_frames.clear();

		this->publish(std::move(result));
	}

	void
	FrameCapture::publish (Result && result) noexcept
	{
		m_result = std::move(result);
		m_resultReady = true;
		m_state = State::Idle;
		m_wantsFrames.store(false, std::memory_order_release);

		m_completion.notify_all();
	}
}

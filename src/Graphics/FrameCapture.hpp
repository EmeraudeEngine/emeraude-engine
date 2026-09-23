/*
 * src/Graphics/FrameCapture.hpp
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
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

/* Forward declarations. */
namespace EmEn
{
	namespace Base
	{
		class ThreadPool;
	}

	namespace Vulkan
	{
		class Buffer;
		class CommandBuffer;
		class Device;
		class Image;
	}
}

namespace EmEn::Graphics
{
	/**
	 * @brief Captures the PRESENTED frames, copied inside their own frame: after the last pass that writes the
	 * swap-chain image (the UI overlay included) and before the submit, while the image is still ACQUIRED.
	 * @note Owner request 2026-09-23. It replaces the former SwapChain::capture(), which downloaded an image the
	 * presentation engine owned (UNASSIGNED-non-acquired-swapchain-image-used on MoltenVK and the Windows
	 * layers). Two uses of the same mechanism:
	 *  - a SCREENSHOT: one frame, written as `<unix seconds>.png`;
	 *  - a TEMPORAL CAPTURE (a development tool against shimmer and temporal artefacts): N CONSECUTIVE presented
	 *    frames, written as `<unix seconds>-<n>.png` (n from 0), plus `<unix seconds>.json` with the per-frame
	 *    metadata (frame serial, CPU time, TAA jitter, camera, exposure).
	 * The copies ride the frames' own submits and fences. During a capture the render thread only RECORDS one
	 * image-to-buffer copy per frame: nothing is read on the CPU before the last frame's fence has passed (owner
	 * decision 2026-09-23), so the captured frames are the frames of a normal run. The read-back, the PNG
	 * encoding and the files happen afterwards on the thread pool.
	 * @warning With a MAILBOX present mode a presented image may be replaced before it is displayed: the capture
	 * holds every image SUBMITTED for presentation, which is what the renderer produced frame after frame.
	 */
	class FrameCapture final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"FrameCapture"};

			/** @brief The staging memory one capture may hold (every frame of a temporal capture is kept until its end). */
			static constexpr uint64_t MaxCaptureBytes{1024ULL * 1024ULL * 1024ULL};

			/** @brief The default frame count of a temporal capture. */
			static constexpr uint32_t DefaultTemporalFrameCount{5};

			/**
			 * @brief What the renderer knows of a frame when it records its copy.
			 */
			struct FrameMetadata
			{
				std::array< float, 16 > viewMatrix{};
				std::array< float, 3 > cameraPosition{};
				std::array< float, 2 > jitterNDC{};
				float aperture{0.0F};
				float shutterSpeed{0.0F};
				float sensitivity{0.0F};
				float exposureCompensation{0.0F};
				uint64_t rendererFrame{0};
				bool autoExposure{false};
				bool hasCamera{false};
			};

			/**
			 * @brief The outcome of a capture, handed to the waiter.
			 */
			struct Result
			{
				std::vector< std::filesystem::path > files;
				std::string error;
				bool success{false};
			};

			/**
			 * @brief Constructs the frame capture service.
			 * @note Out of line with the destructor: the members hold a unique_ptr to the forward-declared Buffer, and
			 * an inline constructor makes MSVC instantiate its deleter (C2027).
			 */
			FrameCapture () noexcept;

			/**
			 * @brief Destructs the frame capture service.
			 */
			~FrameCapture ();

			FrameCapture (const FrameCapture & copy) noexcept = delete;
			FrameCapture (FrameCapture && copy) noexcept = delete;
			FrameCapture & operator= (const FrameCapture & copy) noexcept = delete;
			FrameCapture & operator= (FrameCapture && copy) noexcept = delete;

			/**
			 * @brief Arms a capture of the next @a frameCount presented frames. Thread-safe.
			 * @note Allocates the staging buffers now, on the caller's thread, so the frames being captured pay nothing
			 * but their copy.
			 * @param device The device.
			 * @param threadPool The pool the read-back and the PNG encoding run on.
			 * @param width The swap-chain width.
			 * @param height The swap-chain height.
			 * @param frameCount The number of consecutive frames, 1 for a screenshot.
			 * @param directory The directory the files are written in.
			 * @param temporal True for a temporal capture (numbered files and a metadata JSON).
			 * @param error A reference to a string receiving the reason of a refusal.
			 * @return bool
			 */
			[[nodiscard]]
			bool arm (const std::shared_ptr< Vulkan::Device > & device, const std::shared_ptr< Base::ThreadPool > & threadPool, uint32_t width, uint32_t height, uint32_t frameCount, const std::filesystem::path & directory, bool temporal, std::string & error) noexcept;

			/**
			 * @brief Waits for the armed capture to finish, files written. Thread-safe; never call it on the render thread.
			 * @param timeout The longest wait.
			 * @return Result
			 */
			[[nodiscard]]
			Result waitForCompletion (std::chrono::milliseconds timeout) noexcept;

			/**
			 * @brief RENDER THREAD. Returns whether a capture wants the frame being recorded: one atomic load, no lock.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			wantsFrame () const noexcept
			{
				return m_wantsFrames.load(std::memory_order_acquire);
			}

			/**
			 * @brief Returns whether a capture is armed or still being written.
			 * @return bool
			 */
			[[nodiscard]]
			bool busy () const noexcept;

			/**
			 * @brief RENDER THREAD. Records the copy of the acquired swap-chain image into the capture's next staging
			 * buffer, when a capture wants this frame.
			 * @param commandBuffer The frame's command buffer, outside any render pass.
			 * @param image The acquired swap-chain color image.
			 * @param finalLayout The layout the last pass left the image in (PRESENT_SRC, or COLOR_ATTACHMENT headless).
			 * @param frameSlot The frame-in-flight slot whose fence guards this submit.
			 * @param metadata What the renderer knows of the frame.
			 * @return bool True when a copy was recorded (the caller then confirms or cancels it after its submit).
			 */
			bool recordCopy (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::Image & image, VkImageLayout finalLayout, uint32_t frameSlot, const FrameMetadata & metadata) noexcept;

			/**
			 * @brief RENDER THREAD. Tells the capture whether the frame whose copy was recorded reached the queue.
			 * @param submitted False when the frame was abandoned: the capture then fails, a temporal capture cannot have
			 * a hole.
			 * @return void
			 */
			void confirmSubmit (bool submitted) noexcept;

			/**
			 * @brief RENDER THREAD. Called right after a frame slot's fence wait: every copy submitted with that slot is
			 * complete. Once the last frame's copy is, the read-back is handed to the thread pool.
			 * @param frameSlot The frame-in-flight slot whose fence just passed.
			 * @return void
			 */
			void onFrameSlotRetired (uint32_t frameSlot) noexcept;

			/**
			 * @brief Abandons an armed or running capture (the swap-chain is being recreated, or the renderer stops).
			 * @warning The GPU must be idle, or the frames of the capture retired.
			 * @param reason Why.
			 * @return void
			 */
			void abandon (const std::string & reason) noexcept;

		private:

			/**
			 * @brief One captured frame.
			 */
			struct CapturedFrame
			{
				std::unique_ptr< Vulkan::Buffer > stagingBuffer;
				FrameMetadata metadata{};
				std::chrono::steady_clock::time_point recordTime;
				uint32_t frameSlot{0};
				bool recorded{false};
				bool submitted{false};
				bool complete{false};
			};

			/**
			 * @brief The state of the capture in progress.
			 */
			enum class State : uint8_t
			{
				Idle,
				Armed,
				Writing
			};

			/**
			 * @brief Reads every frame back, writes the PNG files and the JSON, then publishes the result. Runs on the
			 * thread pool.
			 * @return void
			 */
			void writeFiles () noexcept;

			/**
			 * @brief Publishes the result and wakes the waiter. The lock must be held.
			 * @param result The result.
			 * @return void
			 */
			void publish (Result && result) noexcept;

			std::vector< CapturedFrame > m_frames;
			std::shared_ptr< Base::ThreadPool > m_threadPool;
			std::filesystem::path m_directory;
			Result m_result;
			mutable std::mutex m_access;
			std::condition_variable m_completion;
			int64_t m_stem{0};
			uint32_t m_width{0};
			uint32_t m_height{0};
			uint32_t m_nextFrame{0};
			State m_state{State::Idle};
			std::atomic_bool m_wantsFrames{false};
			bool m_swapRedBlue{false};
			bool m_temporal{false};
			bool m_resultReady{false};
	};
}

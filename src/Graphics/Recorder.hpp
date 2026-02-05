/*
 * src/Graphics/Recorder.hpp
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

#pragma once

#include "emeraude_config.hpp"

/* STL inclusions. */
#include <cstdint>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <mutex>
#include <thread>
#include <vector>

#ifdef EMERAUDE_VIDEO_RECORDING_ENABLED
/* Third-party inclusions. */
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4505) /* unreferenced function with internal linkage has been removed */
#endif
#include "vpx/vpx_encoder.h"
#include "vpx/vp8cx.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif

/* Local inclusions. */
#include "ServiceInterface.hpp"

#ifdef EMERAUDE_VIDEO_RECORDING_ENABLED
#include "Vulkan/Buffer.hpp"
#include "Vulkan/CommandPool.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/Sync/Fence.hpp"
#include "Vulkan/Sync/Semaphore.hpp"
#endif

/* Forward declarations. */
namespace EmEn
{
	namespace Graphics
	{
		class Renderer;
	}

	class PrimaryServices;
}

namespace EmEn::Graphics
{
	/**
	 * @class Recorder
	 * @brief Video recording service that captures the framebuffer and encodes it as VP9/IVF.
	 *
	 * Provides real-time video recording of the Vulkan swap-chain framebuffer using asynchronous
	 * GPU readback, unbounded frame queue, and hardware-accelerated VP9 encoding.
	 * The recording pipeline operates in three stages:
	 * 1. GPU async readback (double-buffered) captures BGRA frames from the swap-chain
	 * 2. Unbounded frame queue accumulates frames for the encoding thread
	 * 3. Dedicated encoding thread drains the queue, converts BGRA to I420, encodes VP9, and writes IVF container
	 *
	 * PTS timing uses wall-clock milliseconds with smoothing to handle variable game framerate
	 * while maintaining correct playback speed. Recording dimensions are locked at start time.
	 *
	 * @note When compiled without EMERAUDE_VIDEO_RECORDING_ENABLED, all methods are no-op stubs.
	 * @see EmEn::ServiceInterface
	 * @version 0.8.51
	 */
	class Recorder final : public ServiceInterface
	{
		public:

			/** @brief Service identifier for logging and registration. */
			static constexpr auto ClassId{"GraphicsRecorderService"};

			/**
			 * @brief Constructs the video recorder service.
			 *
			 * @param primaryServices Reference to primary services for settings and filesystem access.
			 * @param renderer Reference to the graphics renderer for swap-chain access.
			 */
			Recorder (PrimaryServices & primaryServices, Renderer & renderer) noexcept;

			/**
			 * @brief Starts video recording to an IVF file.
			 *
			 * Initializes the VP9 encoder, creates async GPU readback resources, starts the
			 * encoding thread, and begins capturing frames. Recording dimensions are
			 * locked to the current framebuffer size (rounded down to even values for I420).
			 *
			 * @param outputPath The filesystem path for the output IVF file.
			 * @return True if recording started successfully, false otherwise.
			 * @pre Framebuffer dimensions must be non-zero.
			 * @post Encoding thread is running and GPU resources are allocated.
			 * @note If already recording, this method returns false and logs a warning.
			 */
			bool startRecording (const std::filesystem::path & outputPath) noexcept;

			/**
			 * @brief Stops video recording and finalizes the output file.
			 *
			 * Waits for pending GPU readbacks, signals the encoding thread to stop, flushes
			 * the VP9 encoder, patches the IVF frame count, and releases all resources.
			 *
			 * @return True if recording was active and stopped successfully, false if not recording.
			 * @post All GPU resources released, encoding thread joined, output file closed.
			 */
			bool stopRecording () noexcept;

			/**
			 * @brief Checks if recording is currently active.
			 *
			 * @return True if recording, false otherwise.
			 */
			[[nodiscard]]
			bool isRecording () const noexcept;

			/**
			 * @brief Checks if enough time has elapsed to capture the next frame.
			 *
			 * Uses frame duration based on target FPS to determine if a new frame should
			 * be captured. This pacing mechanism ensures the recording matches the target
			 * framerate regardless of game render rate.
			 *
			 * @return True if a frame should be captured now, false otherwise.
			 * @note Returns false immediately if not recording.
			 */
			[[nodiscard]]
			bool shouldCaptureFrame () const noexcept;

			/**
			 * @brief Captures the current swap-chain framebuffer and submits GPU copy operation.
			 *
			 * Harvests any completed async readbacks, finds a free async slot (or drops the frame
			 * if both slots are busy), tags the frame with wall-clock PTS, and submits a GPU
			 * image-to-buffer copy command. The copy executes asynchronously on the GPU.
			 *
			 * @pre shouldCaptureFrame() returned true.
			 * @note If no async slots are available, the frame is silently dropped.
			 */
			void captureAndSubmitFrame () noexcept;

		private:

			/**
			 * @brief Initializes video recording configuration from settings.
			 *
			 * Reads target FPS, bitrate, and debug stats flag from settings. Computes
			 * frame duration for pacing.
			 *
			 * @return True on successful initialization.
			 */
			bool onInitialize () noexcept override;

			/**
			 * @brief Terminates the video recorder service.
			 *
			 * Ensures recording is stopped before termination.
			 *
			 * @return True on successful termination.
			 */
			bool onTerminate () noexcept override;

#ifdef EMERAUDE_VIDEO_RECORDING_ENABLED
			/**
			 * @brief Encoding thread entry point.
			 *
			 * Continuously waits for frames in the queue, converts BGRA to I420,
			 * encodes VP9 with smoothed PTS timing, writes IVF frame packets, and optionally
			 * logs statistics. Runs until m_threadRunning becomes false and the queue is drained.
			 *
			 * @note This function executes on the encoding thread, separate from the main thread.
			 */
			void encodingThreadFunc () noexcept;

			/**
			 * @brief Writes IVF file header to output file.
			 *
			 * Writes the 32-byte IVF header containing DKIF signature, VP90 codec FourCC,
			 * dimensions, time base, and frame count placeholder (patched on stop).
			 *
			 * @return True if header written successfully, false otherwise.
			 */
			bool writeIVFFileHeader () const noexcept;

			/**
			 * @brief Writes IVF frame header before each encoded frame.
			 *
			 * Writes the 12-byte frame header containing frame size and presentation timestamp.
			 *
			 * @param frameSize Size of the encoded frame in bytes.
			 * @param pts Presentation timestamp in milliseconds.
			 * @return True if header written successfully, false otherwise.
			 */
			bool writeIVFFrameHeader (uint32_t frameSize, uint64_t pts) const noexcept;

			/**
			 * @brief Patches the frame count field in the IVF header after recording stops.
			 *
			 * Seeks to byte offset 24 in the output file and writes the final frame count.
			 *
			 * @return True if frame count patched successfully, false otherwise.
			 */
			bool patchIVFFrameCount () const noexcept;

			/**
			 * @brief Creates async GPU readback resources (command pool, buffers, fences).
			 *
			 * Allocates a transient command pool and initializes AsyncBufferCount readback slots,
			 * each with a command buffer, fence (signaled), and host-visible staging buffer sized
			 * for a full BGRA framebuffer.
			 *
			 * @return True if all resources created successfully, false otherwise.
			 * @post m_asyncCommandPool and m_asyncSlots are fully initialized.
			 */
			bool createAsyncResources () noexcept;

			/**
			 * @brief Destroys async GPU readback resources and waits for pending operations.
			 *
			 * Unconditionally waits on all fences (safe for signaled fences), then releases
			 * command buffers, fences, staging buffers, and the command pool.
			 *
			 * @post All async resources are released.
			 */
			void destroyAsyncResources () noexcept;

			/**
			 * @brief Submits a GPU copy command to transfer swap-chain image to staging buffer.
			 *
			 * Records a command buffer that transitions the swap-chain image to TRANSFER_SRC_OPTIMAL,
			 * copies it to the staging buffer, then transitions back to PRESENT_SRC_KHR. Submits
			 * the command buffer with a fence and marks the slot as pending.
			 *
			 * @param slotIndex Index of the async readback slot to use (0 or 1).
			 * @return True if GPU copy submitted successfully, false otherwise.
			 * @pre Slot is not pending.
			 * @post Slot fence is reset and marked pending.
			 */
			bool submitGPUCopy (size_t slotIndex) noexcept;

			/**
			 * @brief Submits a two-step GPU copy using the dedicated transfer queue.
			 *
			 * Step 1 (graphics queue): copies swapchain image to a device-local intermediate
			 * buffer with layout transitions, signals a semaphore.
			 * Step 2 (transfer queue): DMA copies device-local buffer to host-visible staging
			 * buffer on dedicated transfer hardware, signals fence.
			 *
			 * @param slotIndex Index of the async readback slot to use (0 or 1).
			 * @return True if both submissions succeeded, false otherwise.
			 * @pre m_useTransferQueue is true and slot is not pending.
			 */
			bool submitTransferQueueCopy (size_t slotIndex) noexcept;

			/**
			 * @brief Harvests a completed async readback and pushes data to the frame queue.
			 *
			 * Checks the fence status non-blockingly. If ready, maps the staging buffer, copies
			 * the BGRA pixel data into a new frame queue entry, and signals the encoding thread
			 * via condition variable.
			 *
			 * @param slotIndex Index of the async readback slot to harvest (0 or 1).
			 * @return True if readback harvested successfully, false if not pending or not ready.
			 * @post If successful, frame is in the queue and slot is no longer pending.
			 */
			bool harvestReadback (size_t slotIndex) noexcept;

			/** @brief Function signature for BGRA-to-I420 conversion implementations. */
			using BGRAToI420Func = void (*)(const uint8_t * bgra, uint32_t w, uint32_t h, uint8_t * y, uint8_t * u, uint8_t * v);

			/** @brief Number of async GPU readback slots for double-buffering. */
			static constexpr size_t AsyncBufferCount{4};

			/**
			 * @struct AsyncReadbackSlot
			 * @brief Represents one async GPU readback slot with command buffer, fence, and staging buffer.
			 */
			struct AsyncReadbackSlot
			{
				std::unique_ptr< Vulkan::CommandBuffer > commandBuffer{}; ///< Command buffer for image-to-buffer copy.
				std::unique_ptr< Vulkan::Sync::Fence > fence{}; ///< Fence signaled when GPU copy completes.
				std::unique_ptr< Vulkan::Buffer > stagingBuffer{}; ///< Host-visible staging buffer for readback.
				uint8_t * mappedPtr{nullptr}; ///< Persistently mapped pointer to staging buffer (valid from create to destroy).
				/* Transfer queue path (only used when m_useTransferQueue is true). */
				std::unique_ptr< Vulkan::CommandBuffer > transferCommandBuffer{}; ///< Transfer queue: buffer-to-buffer DMA copy.
				std::unique_ptr< Vulkan::Sync::Semaphore > transferSemaphore{}; ///< Signaled after graphics copy, waited by transfer DMA.
				std::unique_ptr< Vulkan::Buffer > deviceLocalBuffer{}; ///< Device-local intermediate buffer for two-step readback.
				std::chrono::steady_clock::time_point captureTime{}; ///< Wall-clock time when GPU copy was submitted.
				bool pending{false}; ///< True if GPU copy is in flight.
			};

			/**
			 * @struct FrameSlot
			 * @brief Holds a captured frame for the encoding queue.
			 */
			struct FrameSlot
			{
				std::vector< uint8_t > data{}; ///< BGRA pixel data (width * height * 4 bytes).
				vpx_codec_pts_t pts{0}; ///< Wall-clock presentation timestamp in timebase units.
			};

			/* Service dependencies. */
			PrimaryServices & m_primaryServices; ///< Primary services for settings and filesystem.
			Renderer & m_renderer; ///< Graphics renderer for swap-chain access.

			/* Async GPU readback resources. */
			std::shared_ptr< Vulkan::CommandPool > m_asyncCommandPool{}; ///< Transient command pool for async readback.
			std::shared_ptr< Vulkan::CommandPool > m_transferCommandPool{}; ///< Command pool for transfer queue operations.
			std::array< AsyncReadbackSlot, AsyncBufferCount > m_asyncSlots{}; ///< Double-buffered async readback slots.
			size_t m_currentAsyncSlot{0}; ///< Currently selected async slot index (round-robin).
			uint32_t m_graphicsFamilyIndex{0}; ///< Graphics queue family index for ownership transfers.
			uint32_t m_transferFamilyIndex{0}; ///< Transfer queue family index for ownership transfers.

			/* VP9 codec state. */
			vpx_codec_ctx_t m_codec{}; ///< VP9 encoder context.
			vpx_image_t m_vpxImage{}; ///< VPX image in I420 format for encoding input.

			/* IVF output. */
			std::FILE * m_outputFile{nullptr}; ///< Output IVF file handle.
			std::filesystem::path m_outputPath{}; ///< Path to output IVF file.

			/* Unbounded frame queue for producer-consumer communication. */
			std::deque< FrameSlot > m_frameQueue{}; ///< Unbounded queue of captured frames.
			std::deque< FrameSlot > m_freeFrames{}; ///< Recycling pool of pre-allocated frame buffers.
			std::mutex m_queueMutex{}; ///< Mutex protecting frame queue and free pool access.
			std::condition_variable m_queueCV{}; ///< Condition variable to signal encoding thread.

			/* Encoding thread state. */
			std::thread m_encodingThread{}; ///< Dedicated encoding thread.
			std::atomic< bool > m_isRecording{false}; ///< True when recording is active.
			std::atomic< bool > m_threadRunning{false}; ///< True when encoding thread should continue running.

			/* Timing and frame pacing. */
			uint32_t m_targetFPS{30}; ///< Target recording framerate (default 30 FPS).
			uint64_t m_frameCount{0}; ///< Total number of frames encoded.
			std::atomic< uint64_t > m_captureCount{0}; ///< Total number of frames captured (incremented from main thread).
			std::chrono::steady_clock::time_point m_lastCaptureTime{}; ///< Last frame capture timestamp for pacing.
			std::chrono::steady_clock::time_point m_recordStartTime{}; ///< Wall-clock time when recording started (PTS origin).
			vpx_codec_pts_t m_lastEncodedPts{0}; ///< PTS of the last encoded frame (for duration computation).
			std::chrono::nanoseconds m_frameDuration{0}; ///< Duration between frames based on target FPS.

			/* Recording parameters (locked at start). */
			uint32_t m_recordWidth{0}; ///< Recording width in pixels (even, locked at start).
			uint32_t m_recordHeight{0}; ///< Recording height in pixels (even, locked at start).
			uint32_t m_bitrate{2000}; ///< VP9 target bitrate in kilobits per second.

			/* State flags. */
			bool m_codecInitialized{false}; ///< True when VP9 codec is initialized.
			bool m_useTransferQueue{false}; ///< True when dedicated transfer queue is available and in use.
			bool m_useCBR{true}; ///< True to use CBR (constant bitrate) rate control instead of VBR.
			bool m_showStats{false}; ///< True to log periodic encoding statistics.

			/* SIMD dispatch. */
			BGRAToI420Func m_bgraToI420{nullptr}; ///< Selected BGRA-to-I420 conversion function (scalar, SSSE3, or AVX2).
#endif
	};
}

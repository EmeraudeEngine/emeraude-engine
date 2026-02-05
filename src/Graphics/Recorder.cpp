/*
 * src/Graphics/Recorder.cpp
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

#include "Recorder.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cstring>
#include <sstream>

/* Local inclusions. */
#include "Renderer.hpp"
#include "RenderTarget/Abstract.hpp"
#include "Libs/IO/IO.hpp"
#include "PrimaryServices.hpp"
#include "SettingKeys.hpp"
#include "Tracer.hpp"

#ifdef EMERAUDE_VIDEO_RECORDING_ENABLED
#include "Vulkan/Device.hpp"
#include "Vulkan/Queue.hpp"
#include "Vulkan/Sync/ImageMemoryBarrier.hpp"

/* CPU feature detection and SIMD intrinsics for BGRA-to-I420 dispatch. */
#if IS_X86_ARCH
#include "cpu_features/cpuinfo_x86.h"
#include <immintrin.h>
#endif
#endif

namespace EmEn::Graphics
{
	using namespace Libs;

#ifdef EMERAUDE_VIDEO_RECORDING_ENABLED
	/* Helper to write a little-endian value to a byte buffer. */
	static void
	writeLE16 (uint8_t * dst, uint16_t value) noexcept
	{
		dst[0] = static_cast< uint8_t >(value & 0xFF);
		dst[1] = static_cast< uint8_t >((value >> 8) & 0xFF);
	}

	static void
	writeLE32 (uint8_t * dst, uint32_t value) noexcept
	{
		dst[0] = static_cast< uint8_t >(value & 0xFF);
		dst[1] = static_cast< uint8_t >((value >> 8) & 0xFF);
		dst[2] = static_cast< uint8_t >((value >> 16) & 0xFF);
		dst[3] = static_cast< uint8_t >((value >> 24) & 0xFF);
	}

	static void
	writeLE64 (uint8_t * dst, uint64_t value) noexcept
	{
		for ( int i = 0; i < 8; i++ )
		{
			dst[i] = static_cast< uint8_t >((value >> (i * 8)) & 0xFF);
		}
	}

	/* Forward declarations for BGRA-to-I420 conversion variants. */
	static void BGRAToI420_Scalar (const uint8_t * bgra, uint32_t w, uint32_t h, uint8_t * y, uint8_t * u, uint8_t * v) noexcept;
#if IS_X86_ARCH
#ifndef _MSC_VER
	__attribute__((target("sse4.1"))) static void BGRAToI420_SSE41 (const uint8_t * bgra, uint32_t w, uint32_t h, uint8_t * y, uint8_t * u, uint8_t * v) noexcept;
	__attribute__((target("avx2"))) static void BGRAToI420_AVX2 (const uint8_t * bgra, uint32_t w, uint32_t h, uint8_t * y, uint8_t * u, uint8_t * v) noexcept;
#else
	static void BGRAToI420_SSE41 (const uint8_t * bgra, uint32_t w, uint32_t h, uint8_t * y, uint8_t * u, uint8_t * v) noexcept;
	static void BGRAToI420_AVX2 (const uint8_t * bgra, uint32_t w, uint32_t h, uint8_t * y, uint8_t * u, uint8_t * v) noexcept;
#endif
#endif

	Recorder::Recorder (PrimaryServices & primaryServices, Renderer & renderer) noexcept
		: ServiceInterface{ClassId},
		m_primaryServices{primaryServices},
		m_renderer{renderer}
	{

	}

	bool
	Recorder::onInitialize () noexcept
	{
		auto & settings = m_primaryServices.settings();

		if ( !settings.getOrSetDefault< bool >(VideoRecorderEnableKey, DefaultVideoRecorderEnable) )
		{
			return false;
		}

		m_targetFPS = settings.getOrSetDefault< unsigned int >(VideoRecorderFPSKey, DefaultVideoRecorderFPS);
		m_bitrate = settings.getOrSetDefault< unsigned int >(VideoRecorderQualityKey, DefaultVideoRecorderQuality);
		m_showStats = settings.getOrSetDefault< bool >(VideoRecorderShowStatsKey, DefaultVideoRecorderShowStats);
		m_useCBR = settings.getOrSetDefault< bool >(VideoRecorderUseCBRKey, DefaultVideoRecorderUseCBR);

		if ( m_targetFPS == 0 )
		{
			m_targetFPS = DefaultVideoRecorderFPS;
		}

		m_frameDuration = std::chrono::nanoseconds{1000000000ULL / m_targetFPS};

		/* Select the best BGRA-to-I420 conversion path based on CPU features. */
#if IS_X86_ARCH
		{
			const auto features = cpu_features::GetX86Info().features;

			if ( features.avx2 )
			{
				m_bgraToI420 = BGRAToI420_AVX2;

				TraceInfo{ClassId} << "BGRAToI420: using AVX2 path.";
			}
			else if ( features.sse4_1 )
			{
				m_bgraToI420 = BGRAToI420_SSE41;

				TraceInfo{ClassId} << "BGRAToI420: using SSE4.1 path.";
			}
			else
			{
				m_bgraToI420 = BGRAToI420_Scalar;

				TraceInfo{ClassId} << "BGRAToI420: using scalar path.";
			}
		}
#else
		m_bgraToI420 = BGRAToI420_Scalar;

		TraceInfo{ClassId} << "BGRAToI420: using scalar path.";
#endif

		TraceInfo{ClassId} << "Video recording configured : " << m_targetFPS << " FPS, " << m_bitrate << " kbps bitrate.";

		return true;
	}

	bool
	Recorder::onTerminate () noexcept
	{
		if ( m_isRecording )
		{
			this->stopRecording();
		}

		return true;
	}

	bool
	Recorder::isRecording () const noexcept
	{
		return m_isRecording;
	}

	bool
	Recorder::shouldCaptureFrame () const noexcept
	{
		if ( !m_isRecording )
		{
			return false;
		}

		const auto now = std::chrono::steady_clock::now();

		return (now - m_lastCaptureTime) >= m_frameDuration;
	}

	bool
	Recorder::startRecording (const std::filesystem::path & outputPath) noexcept
	{
		if ( !this->usable() )
		{
			Tracer::warning(ClassId, "The video recorder was not initialized.");

			return false;
		}

		if ( m_isRecording )
		{
			TraceWarning{ClassId} << "Already recording !";

			return false;
		}

		/* Get framebuffer dimensions. */
		const auto renderTarget = m_renderer.mainRenderTarget();

		if ( renderTarget == nullptr )
		{
			TraceError{ClassId} << "No main render target available !";

			return false;
		}

		const auto & ext = renderTarget->extent();
		m_recordWidth = ext.width;
		m_recordHeight = ext.height;

		if ( m_recordWidth == 0 || m_recordHeight == 0 )
		{
			TraceError{ClassId} << "Invalid framebuffer dimensions !";

			return false;
		}

		/* Ensure width and height are even (required by VP9/I420). */
		m_recordWidth &= ~1U;
		m_recordHeight &= ~1U;

		m_outputPath = outputPath;

		/* Open output file. */
#ifdef _WIN32
		m_outputFile = _wfopen(m_outputPath.c_str(), L"wb");
#else
		m_outputFile = std::fopen(m_outputPath.c_str(), "wb");
#endif

		if ( m_outputFile == nullptr )
		{
			TraceError{ClassId} << "Unable to open output file " << m_outputPath << " !";

			return false;
		}

		/* Initialize VP9 encoder. */
		vpx_codec_enc_cfg_t cfg{};

		if ( vpx_codec_enc_config_default(vpx_codec_vp9_cx(), &cfg, 0) != VPX_CODEC_OK )
		{
			TraceError{ClassId} << "Unable to get default VP9 encoder configuration !";

			std::fclose(m_outputFile);
			m_outputFile = nullptr;

			return false;
		}

		cfg.g_w = m_recordWidth;
		cfg.g_h = m_recordHeight;
		cfg.g_timebase.num = 1;
		cfg.g_timebase.den = m_targetFPS;
		cfg.rc_target_bitrate = m_bitrate;

		if ( m_useCBR )
		{
			cfg.rc_end_usage = VPX_CBR; /* Strict constant bitrate for real-time encoding. */
			cfg.rc_buf_sz = 1000;         /* Rate control buffer size (ms). */
			cfg.rc_buf_initial_sz = 500;  /* Initial buffer fill before encoding starts (ms). */
			cfg.rc_buf_optimal_sz = 600;  /* Optimal buffer level (ms). */
			cfg.rc_undershoot_pct = 95;   /* Allow slight undershoot to prevent quality drops. */
			cfg.rc_overshoot_pct = 5;     /* Minimize overshoot for predictable file sizes. */
		}
		cfg.g_threads = std::max(2U, std::thread::hardware_concurrency() / 2);
		cfg.g_profile = 0; /* 8-bit 4:2:0 (I420). */
		cfg.g_lag_in_frames = 0; /* Real-time encoding. */
		cfg.g_error_resilient = VPX_ERROR_RESILIENT_DEFAULT;

		if ( vpx_codec_enc_init(&m_codec, vpx_codec_vp9_cx(), &cfg, 0) != VPX_CODEC_OK )
		{
			TraceError{ClassId} << "VP9 encoder initialization failed !";

			std::fclose(m_outputFile);
			m_outputFile = nullptr;

			return false;
		}

		m_codecInitialized = true;

		/* VP9 encoder controls for real-time capture. */
		vpx_codec_control(&m_codec, VP8E_SET_CPUUSED, 7);                 /* VP9: 0-9, 7 = good real-time/quality compromise. */
		vpx_codec_control(&m_codec, VP9E_SET_ROW_MT, 1);                  /* Multi-threading per superblock rows. */
		vpx_codec_control(&m_codec, VP9E_SET_TILE_COLUMNS, 2);            /* 4 tile columns (2^2) for parallelism. */
		vpx_codec_control(&m_codec, VP9E_SET_FRAME_PARALLEL_DECODING, 1); /* Parallel decoding on playback. */
		vpx_codec_control(&m_codec, VP9E_SET_AQ_MODE, 3);                 /* Cyclic refresh, ideal for real-time capture. */

		/* Allocate VPX image. */
		if ( vpx_img_alloc(&m_vpxImage, VPX_IMG_FMT_I420, m_recordWidth, m_recordHeight, 1) == nullptr )
		{
			TraceError{ClassId} << "VPX image allocation failed !";

			vpx_codec_destroy(&m_codec);
			m_codecInitialized = false;
			std::fclose(m_outputFile);
			m_outputFile = nullptr;

			return false;
		}

		/* Write IVF file header. */
		if ( !this->writeIVFFileHeader() )
		{
			TraceError{ClassId} << "Failed to write IVF file header !";

			vpx_img_free(&m_vpxImage);
			vpx_codec_destroy(&m_codec);
			m_codecInitialized = false;
			std::fclose(m_outputFile);
			m_outputFile = nullptr;

			return false;
		}

		m_frameQueue.clear();
		m_frameCount = 0;
		m_captureCount = 0;
		m_lastEncodedPts = 0;
		m_recordStartTime = std::chrono::steady_clock::now();
		m_lastCaptureTime = m_recordStartTime;

		/* Create async GPU readback resources. */
		if ( !this->createAsyncResources() )
		{
			TraceError{ClassId} << "Failed to create async readback resources !";

			vpx_img_free(&m_vpxImage);
			vpx_codec_destroy(&m_codec);
			m_codecInitialized = false;
			std::fclose(m_outputFile);
			m_outputFile = nullptr;

			return false;
		}

		/* Start encoding thread. */
		m_isRecording = true;
		m_threadRunning = true;

		m_encodingThread = std::thread{[this] {
			this->encodingThreadFunc();
		}};

		TraceSuccess{ClassId} << "Recording started : " << m_recordWidth << "x" << m_recordHeight << " @ " << m_targetFPS << " FPS -> " << m_outputPath;

		return true;
	}

	bool
	Recorder::stopRecording () noexcept
	{
		if ( !m_isRecording )
		{
			return false;
		}

		/* Wait for and harvest any pending async readbacks. */
		for ( size_t index = 0; index < AsyncBufferCount; index++ )
		{
			if ( m_asyncSlots[index].pending )
			{
				static_cast< void >(m_asyncSlots[index].fence->wait());

				this->harvestReadback(index);
			}
		}

		/* Signal encoding thread to stop (it will drain remaining frames first). */
		m_isRecording = false;
		m_threadRunning = false;
		m_queueCV.notify_all();

		/* Wait for encoding thread to finish. */
		if ( m_encodingThread.joinable() )
		{
			m_encodingThread.join();
		}

		/* Flush encoder : send NULL frame to get remaining packets. */
		if ( m_codecInitialized )
		{
			const auto flushPts = m_lastEncodedPts + 1;
			vpx_codec_encode(&m_codec, nullptr, flushPts, 1, 0, VPX_DL_REALTIME);

			const vpx_codec_cx_pkt_t * pkt = nullptr;
			vpx_codec_iter_t iter = nullptr;

			while ( (pkt = vpx_codec_get_cx_data(&m_codec, &iter)) != nullptr )
			{
				if ( pkt->kind == VPX_CODEC_CX_FRAME_PKT )
				{
					this->writeIVFFrameHeader(static_cast< uint32_t >(pkt->data.frame.sz), pkt->data.frame.pts);
					std::fwrite(pkt->data.frame.buf, 1, pkt->data.frame.sz, m_outputFile);
				}
			}
		}

		/* Patch frame count in IVF header. */
		this->patchIVFFrameCount();

		/* Cleanup. */
		if ( m_outputFile != nullptr )
		{
			std::fclose(m_outputFile);
			m_outputFile = nullptr;
		}

		if ( m_codecInitialized )
		{
			vpx_codec_destroy(&m_codec);
			vpx_img_free(&m_vpxImage);
			m_codecInitialized = false;
		}

		/* Destroy async GPU readback resources. */
		this->destroyAsyncResources();

		/* Clear frame queue and recycling pool. */
		m_frameQueue.clear();
		m_freeFrames.clear();
		m_frameQueue.shrink_to_fit();

		TraceSuccess{ClassId} << "Recording stopped : " << m_frameCount << " frames written to " << m_outputPath;

		return true;
	}

	void
	Recorder::captureAndSubmitFrame () noexcept
	{
		/* 1. Harvest any completed readbacks from both slots. */
		for ( size_t index = 0; index < AsyncBufferCount; index++ )
		{
			this->harvestReadback(index);
		}

		/* 2. Find a free slot. */
		size_t freeSlot = AsyncBufferCount; /* sentinel = none found */

		for ( size_t index = 0; index < AsyncBufferCount; index++ )
		{
			if ( !m_asyncSlots[index].pending )
			{
				freeSlot = index;

				break;
			}
		}

		if ( freeSlot == AsyncBufferCount )
		{
			return; /* Both slots busy, retry next game frame. */
		}

		/* 3. Submit GPU copy and update capture timing only on success. */
		if ( this->submitGPUCopy(freeSlot) )
		{
			const auto now = std::chrono::steady_clock::now();
			m_asyncSlots[freeSlot].captureTime = now;
			m_lastCaptureTime = now;
			m_captureCount++;
		}
	}

	void
	Recorder::encodingThreadFunc () noexcept
	{
		FrameSlot localFrame;

		/* Stats tracking. */
		auto statsLastTime = std::chrono::steady_clock::now();
		uint64_t statsFrameCount = 0;
		uint64_t statsEncodedBytes = 0;
		uint64_t lastCaptureCount = m_captureCount.load();

		while ( true )
		{
			/* Wait for a frame to be available or thread stop signal. */
			{
				std::unique_lock lock{m_queueMutex};

				m_queueCV.wait(lock, [this] {
					return !m_frameQueue.empty() || !m_threadRunning;
				});

				if ( m_frameQueue.empty() )
				{
					break; /* Thread stop requested and queue is drained. */
				}

				/* Move frame data out of the queue (zero-copy). */
				localFrame = std::move(m_frameQueue.front());
				m_frameQueue.pop_front();
			}

			/* Convert BGRA to I420 using the selected SIMD path. */
			m_bgraToI420(
				localFrame.data.data(),
				m_recordWidth, m_recordHeight,
				m_vpxImage.planes[0],
				m_vpxImage.planes[1],
				m_vpxImage.planes[2]
			);

			/* Wall-clock PTS: derived from capture timestamp so that the video
			 * timeline matches real elapsed time even when frames are dropped.
			 * Duration is kept at 1 tick for stable CBR rate control; the PTS
			 * alone provides correct playback timing. */
			const auto pts = localFrame.pts;

			const auto encodeResult = vpx_codec_encode(
				&m_codec,
				&m_vpxImage,
				pts,
				1, /* Duration = 1 tick for stable rate control. */
				0,
				VPX_DL_REALTIME
			);

			if ( encodeResult != VPX_CODEC_OK )
			{
				TraceError{ClassId} << "VP9 encoding failed for frame " << m_frameCount << " : " << vpx_codec_err_to_string(encodeResult);

				continue;
			}

			/* Write encoded packets to IVF file. */
			const vpx_codec_cx_pkt_t * pkt = nullptr;
			vpx_codec_iter_t iter = nullptr;

			while ( (pkt = vpx_codec_get_cx_data(&m_codec, &iter)) != nullptr )
			{
				if ( pkt->kind == VPX_CODEC_CX_FRAME_PKT )
				{
					this->writeIVFFrameHeader(static_cast< uint32_t >(pkt->data.frame.sz), pkt->data.frame.pts);
					std::fwrite(pkt->data.frame.buf, 1, pkt->data.frame.sz, m_outputFile);
					statsEncodedBytes += pkt->data.frame.sz;
				}
			}

			m_frameCount++;
			m_lastEncodedPts = pts;

			/* Recycle the frame buffer back to the free pool. */
			{
				const std::lock_guard lock{m_queueMutex};
				m_freeFrames.push_back(std::move(localFrame));
			}

			/* Periodic stats output. */
			if ( m_showStats )
			{
				statsFrameCount++;

				const auto now = std::chrono::steady_clock::now();
				const auto elapsed = std::chrono::duration_cast< std::chrono::milliseconds >(now - statsLastTime).count();

				if ( elapsed >= 500 )
				{
					const double encodeFPS = static_cast< double >(statsFrameCount) * 1000.0 / static_cast< double >(elapsed);
					const double bitrateKbps = static_cast< double >(statsEncodedBytes) * 8.0 / static_cast< double >(elapsed);

					const uint64_t currentCaptures = m_captureCount.load();
					const double captureFPS = static_cast< double >(currentCaptures - lastCaptureCount) * 1000.0 / static_cast< double >(elapsed);

					size_t queueDepth = 0;

					{
						const std::lock_guard lock{m_queueMutex};
						queueDepth = m_frameQueue.size();
					}

					TraceInfo{ClassId}
						<< "[Stats] Frames: " << m_frameCount
						<< " | Capture: " << captureFPS << "/" << m_targetFPS << " FPS"
						<< " | Encode: " << encodeFPS << " FPS"
						<< " | Queue: " << queueDepth
						<< " | Bitrate: " << bitrateKbps << " kbps";

					statsLastTime = now;
					statsFrameCount = 0;
					statsEncodedBytes = 0;
					lastCaptureCount = currentCaptures;
				}
			}
		}
	}

	bool
	Recorder::writeIVFFileHeader () const noexcept
	{
		/* IVF file header: 32 bytes.
		 * Bytes 0-3   : "DKIF" signature
		 * Bytes 4-5   : version (0)
		 * Bytes 6-7   : header length (32)
		 * Bytes 8-11  : codec FourCC ("VP90")
		 * Bytes 12-13 : width
		 * Bytes 14-15 : height
		 * Bytes 16-19 : time base denominator — ffmpeg reads this field as den
		 * Bytes 20-23 : time base numerator — ffmpeg reads this field as num
		 * Bytes 24-27 : frame count (patched later)
		 * Bytes 28-31 : unused (0)
		 */
		uint8_t header[32] = {};

		header[0] = 'D';
		header[1] = 'K';
		header[2] = 'I';
		header[3] = 'F';

		writeLE16(header + 4, 0);	  /* Version. */
		writeLE16(header + 6, 32);	 /* Header length. */

		header[8]  = 'V';
		header[9]  = 'P';
		header[10] = '9';
		header[11] = '0';

		writeLE16(header + 12, static_cast< uint16_t >(m_recordWidth));
		writeLE16(header + 14, static_cast< uint16_t >(m_recordHeight));
		writeLE32(header + 16, static_cast< uint32_t >(m_targetFPS)); /* Time base denominator (ffmpeg reads as den). */
		writeLE32(header + 20, 1);                                    /* Time base numerator (ffmpeg reads as num). */
		writeLE32(header + 24, 0);			 /* Frame count (patched on stop). */
		writeLE32(header + 28, 0);			 /* Unused. */

		return std::fwrite(header, 1, sizeof(header), m_outputFile) == sizeof(header);
	}

	bool
	Recorder::writeIVFFrameHeader (uint32_t frameSize, uint64_t pts) const noexcept
	{
		/* IVF frame header: 12 bytes.
		 * Bytes 0-3 : frame size
		 * Bytes 4-11: presentation timestamp
		 */
		uint8_t header[12];

		writeLE32(header, frameSize);
		writeLE64(header + 4, pts);

		return std::fwrite(header, 1, sizeof(header), m_outputFile) == sizeof(header);
	}

	bool
	Recorder::patchIVFFrameCount () const noexcept
	{
		if ( m_outputFile == nullptr )
		{
			return false;
		}

		/* Seek to byte 24 in the IVF header and write the frame count. */
		if ( std::fseek(m_outputFile, 24, SEEK_SET) != 0 )
		{
			return false;
		}

		uint8_t buf[4];
		writeLE32(buf, static_cast< uint32_t >(m_frameCount));

		if ( std::fwrite(buf, 1, 4, m_outputFile) != 4 )
		{
			return false;
		}

		/* Seek back to end. */
		std::fseek(m_outputFile, 0, SEEK_END);

		return true;
	}

	bool
	Recorder::createAsyncResources () noexcept
	{
		const auto device = m_renderer.device();

		if ( device == nullptr )
		{
			return false;
		}

		/* Detect dedicated transfer queue availability. */
		m_graphicsFamilyIndex = device->getGraphicsFamilyIndex();

		if ( device->hasTransferQueues() && device->getTransferFamilyIndex() != m_graphicsFamilyIndex )
		{
			m_useTransferQueue = true;
			m_transferFamilyIndex = device->getTransferFamilyIndex();

			TraceInfo{ClassId} << "Dedicated transfer queue detected (family " << m_transferFamilyIndex << "), enabling DMA readback path.";
		}
		else
		{
			m_useTransferQueue = false;
		}

		/* Create a command pool with transient + reset-per-buffer flags. */
		m_asyncCommandPool = std::make_shared< Vulkan::CommandPool >(device, m_graphicsFamilyIndex, true, true, false);
		m_asyncCommandPool->setIdentifier(ClassId, "AsyncReadback", "CommandPool");

		if ( !m_asyncCommandPool->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create async readback command pool !";

			m_asyncCommandPool.reset();

			return false;
		}

		/* Create transfer command pool if using dedicated transfer queue. */
		if ( m_useTransferQueue )
		{
			m_transferCommandPool = std::make_shared< Vulkan::CommandPool >(device, m_transferFamilyIndex, true, true, false);
			m_transferCommandPool->setIdentifier(ClassId, "TransferReadback", "CommandPool");

			if ( !m_transferCommandPool->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create transfer readback command pool !";

				m_transferCommandPool.reset();

				return false;
			}
		}

		const auto frameBytes = static_cast< VkDeviceSize >(m_recordWidth) * m_recordHeight * 4;

		for ( size_t index = 0; index < AsyncBufferCount; index++ )
		{
			auto & slot = m_asyncSlots[index];

			/* Allocate a primary command buffer from the pool. */
			slot.commandBuffer = std::make_unique< Vulkan::CommandBuffer >(m_asyncCommandPool, true);

			if ( !slot.commandBuffer->isCreated() )
			{
				TraceError{ClassId} << "Unable to allocate async readback command buffer #" << index << " !";

				return false;
			}

			/* Create a signaled fence (safe for unconditional wait on destroy). */
			slot.fence = std::make_unique< Vulkan::Sync::Fence >(device, VK_FENCE_CREATE_SIGNALED_BIT);

			if ( !slot.fence->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create async readback fence #" << index << " !";

				return false;
			}

			/* Create a host-visible staging buffer with cached memory for fast CPU reads. */
			slot.stagingBuffer = std::make_unique< Vulkan::Buffer >(device, static_cast< VkBufferCreateFlags >(0), frameBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true);
			slot.stagingBuffer->setHostReadable(true);

			if ( !slot.stagingBuffer->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create async readback staging buffer #" << index << " !";

				return false;
			}

			/* Persistently map the staging buffer to avoid per-frame map/unmap driver calls. */
			slot.mappedPtr = slot.stagingBuffer->mapMemoryAs< uint8_t >();

			if ( slot.mappedPtr == nullptr )
			{
				TraceError{ClassId} << "Unable to persistently map staging buffer #" << index << " !";

				return false;
			}

			slot.pending = false;

			/* Create transfer queue resources per slot. */
			if ( m_useTransferQueue )
			{
				/* Device-local intermediate buffer for two-step readback. */
				slot.deviceLocalBuffer = std::make_unique< Vulkan::Buffer >(device, static_cast< VkBufferCreateFlags >(0), frameBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, false);

				if ( !slot.deviceLocalBuffer->createOnHardware() )
				{
					TraceError{ClassId} << "Unable to create device-local intermediate buffer #" << index << " !";

					return false;
				}

				/* Transfer command buffer for DMA copy (device-local → host-visible). */
				slot.transferCommandBuffer = std::make_unique< Vulkan::CommandBuffer >(m_transferCommandPool, true);

				if ( !slot.transferCommandBuffer->isCreated() )
				{
					TraceError{ClassId} << "Unable to allocate transfer command buffer #" << index << " !";

					return false;
				}

				/* Semaphore: graphics signals after image copy, transfer waits before DMA. */
				slot.transferSemaphore = std::make_unique< Vulkan::Sync::Semaphore >(device);

				if ( !slot.transferSemaphore->createOnHardware() )
				{
					TraceError{ClassId} << "Unable to create transfer semaphore #" << index << " !";

					return false;
				}
			}
		}

		m_currentAsyncSlot = 0;

		return true;
	}

	void
	Recorder::destroyAsyncResources () noexcept
	{
		/* Unconditionally wait on every fence before destroying.
		 * - Never submitted: still SIGNALED from creation → returns immediately.
		 * - Submitted + completed: SIGNALED by GPU → returns immediately.
		 * - Submitted + in-flight: blocks until GPU finishes. */
		for ( auto & slot : m_asyncSlots )
		{
			if ( slot.fence != nullptr )
			{
				static_cast< void >(slot.fence->wait());
			}

			slot.pending = false;
		}

		/* Destroy in reverse order: command buffers, fences, staging buffers, then pool. */
		for ( auto & slot : m_asyncSlots )
		{
			/* Transfer queue resources. */
			slot.transferCommandBuffer.reset();
			slot.transferSemaphore.reset();
			slot.deviceLocalBuffer.reset();

			/* Unmap persistently mapped staging buffer before destroying it. */
			if ( slot.mappedPtr != nullptr )
			{
				slot.stagingBuffer->unmapMemory();
				slot.mappedPtr = nullptr;
			}

			/* Standard resources. */
			slot.commandBuffer.reset();
			slot.fence.reset();
			slot.stagingBuffer.reset();
		}

		m_transferCommandPool.reset();
		m_asyncCommandPool.reset();
		m_currentAsyncSlot = 0;
		m_useTransferQueue = false;
	}

	bool
	Recorder::submitGPUCopy (size_t slotIndex) noexcept
	{
		/* Use dedicated transfer queue path when available. */
		if ( m_useTransferQueue )
		{
			return this->submitTransferQueueCopy(slotIndex);
		}

		/* Get the source swap-chain image. */
		const auto sourceImage = m_renderer.currentSwapChainColorImage();

		if ( sourceImage == nullptr )
		{
			return false;
		}

		auto & slot = m_asyncSlots[slotIndex];

		/* Reset and record the command buffer. */
		if ( !slot.commandBuffer->reset() )
		{
			return false;
		}

		if ( !slot.commandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
		{
			return false;
		}

		/* Barrier: PRESENT_SRC_KHR -> TRANSFER_SRC_OPTIMAL */
		{
			const Vulkan::Sync::ImageMemoryBarrier barrier{
				*sourceImage,
				VK_ACCESS_MEMORY_READ_BIT,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			};

			slot.commandBuffer->pipelineBarrier(barrier, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		}

		/* Copy image to staging buffer. */
		{
			VkBufferImageCopy region{};
			region.bufferOffset = 0;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;
			region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
			region.imageOffset = {0, 0, 0};
			region.imageExtent = {m_recordWidth, m_recordHeight, 1};

			vkCmdCopyImageToBuffer(
				slot.commandBuffer->handle(),
				sourceImage->handle(),
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				slot.stagingBuffer->handle(),
				1,
				&region
			);
		}

		/* Barrier: TRANSFER_SRC_OPTIMAL -> PRESENT_SRC_KHR */
		{
			const Vulkan::Sync::ImageMemoryBarrier barrier{
				*sourceImage,
				VK_ACCESS_TRANSFER_READ_BIT,
				static_cast< VkAccessFlags >(0),
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				VK_IMAGE_ASPECT_COLOR_BIT
			};

			slot.commandBuffer->pipelineBarrier(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
		}

		if ( !slot.commandBuffer->end() )
		{
			return false;
		}

		/* Reset fence and submit. */
		if ( !slot.fence->reset() )
		{
			return false;
		}

		const auto * queue = m_renderer.device()->getGraphicsQueue(Vulkan::QueuePriority::High);

		if ( queue == nullptr )
		{
			return false;
		}

		if ( !queue->submit(*slot.commandBuffer, Vulkan::SynchInfo{}.withFence(slot.fence->handle())) )
		{
			return false;
		}

		slot.pending = true;

		return true;
	}

	bool
	Recorder::submitTransferQueueCopy (size_t slotIndex) noexcept
	{
		const auto sourceImage = m_renderer.currentSwapChainColorImage();

		if ( sourceImage == nullptr )
		{
			return false;
		}

		auto & slot = m_asyncSlots[slotIndex];
		const auto device = m_renderer.device();

		/* === Step 1: Graphics queue — copy swapchain image to device-local buffer === */
		if ( !slot.commandBuffer->reset() )
		{
			return false;
		}

		if ( !slot.commandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
		{
			return false;
		}

		/* Barrier: PRESENT_SRC_KHR → TRANSFER_SRC_OPTIMAL */
		{
			const Vulkan::Sync::ImageMemoryBarrier barrier{
				*sourceImage,
				VK_ACCESS_MEMORY_READ_BIT,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			};

			slot.commandBuffer->pipelineBarrier(barrier, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		}

		/* Copy image to device-local intermediate buffer (fast GPU → GPU). */
		{
			VkBufferImageCopy region{};
			region.bufferOffset = 0;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;
			region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
			region.imageOffset = {0, 0, 0};
			region.imageExtent = {m_recordWidth, m_recordHeight, 1};

			vkCmdCopyImageToBuffer(
				slot.commandBuffer->handle(),
				sourceImage->handle(),
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				slot.deviceLocalBuffer->handle(),
				1,
				&region
			);
		}

		/* Barrier: TRANSFER_SRC_OPTIMAL → PRESENT_SRC_KHR */
		{
			const Vulkan::Sync::ImageMemoryBarrier barrier{
				*sourceImage,
				VK_ACCESS_TRANSFER_READ_BIT,
				static_cast< VkAccessFlags >(0),
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				VK_IMAGE_ASPECT_COLOR_BIT
			};

			slot.commandBuffer->pipelineBarrier(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
		}

		if ( !slot.commandBuffer->end() )
		{
			return false;
		}

		const auto * graphicsQueue = device->getGraphicsQueue(Vulkan::QueuePriority::High);

		if ( graphicsQueue == nullptr )
		{
			return false;
		}

		const VkSemaphore semaphoreHandle = slot.transferSemaphore->handle();

		if ( !graphicsQueue->submit(*slot.commandBuffer, Vulkan::SynchInfo{}.signals({&semaphoreHandle, 1})) )
		{
			return false;
		}

		/* === Step 2: Transfer queue — DMA copy device-local → host-visible === */
		if ( !slot.transferCommandBuffer->reset() )
		{
			return false;
		}

		if ( !slot.transferCommandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
		{
			return false;
		}

		const auto frameBytes = static_cast< VkDeviceSize >(m_recordWidth) * m_recordHeight * 4;
		slot.transferCommandBuffer->copy(*slot.deviceLocalBuffer, *slot.stagingBuffer, 0, 0, frameBytes);

		if ( !slot.transferCommandBuffer->end() )
		{
			return false;
		}

		/* Reset fence and submit: wait on graphics semaphore, signal fence for harvest. */
		if ( !slot.fence->reset() )
		{
			return false;
		}

		const auto * transferQueue = device->getGraphicsTransferQueue(Vulkan::QueuePriority::High);

		if ( transferQueue == nullptr )
		{
			return false;
		}

		const VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

		if ( !transferQueue->submit(*slot.transferCommandBuffer, Vulkan::SynchInfo{}.waits({&semaphoreHandle, 1}, {&waitStage, 1}).withFence(slot.fence->handle())) )
		{
			return false;
		}

		slot.pending = true;

		return true;
	}

	bool
	Recorder::harvestReadback (size_t slotIndex) noexcept
	{
		auto & slot = m_asyncSlots[slotIndex];

		if ( !slot.pending )
		{
			return false;
		}

		/* Non-blocking fence check. */
		if ( slot.fence->getStatus() != Vulkan::Sync::FenceStatus::Ready )
		{
			return false;
		}

		/* Acquire a frame buffer from the recycling pool (avoids allocation + zero-fill). */
		const auto frameBytes = static_cast< size_t >(m_recordWidth) * m_recordHeight * 4;
		FrameSlot frame;

		{
			const std::lock_guard lock{m_queueMutex};

			if ( !m_freeFrames.empty() )
			{
				frame = std::move(m_freeFrames.back());
				m_freeFrames.pop_back();
			}
		}

		if ( frame.data.size() != frameBytes )
		{
			frame.data.resize(frameBytes);
		}

		/* Copy from persistently mapped staging buffer. */
		std::memcpy(frame.data.data(), slot.mappedPtr, frameBytes);

		/* Compute wall-clock PTS from capture timestamp. */
		const auto elapsed = std::chrono::duration< double >(slot.captureTime - m_recordStartTime);
		frame.pts = static_cast< vpx_codec_pts_t >(elapsed.count() * m_targetFPS);

		{
			const std::lock_guard lock{m_queueMutex};
			m_frameQueue.push_back(std::move(frame));
		}

		m_queueCV.notify_one();

		slot.pending = false;

		return true;
	}

	/* ======================================================================
	 * BGRA-to-I420 conversion implementations (Scalar, SSSE3, AVX2).
	 *
	 * BT.601 coefficients for BGRA (B8G8R8A8_UNORM) input:
	 *   Y  = ((  66*R + 129*G +  25*B + 128) >> 8) + 16
	 *   U  = (( -38*R -  74*G + 112*B + 128) >> 8) + 128
	 *   V  = (( 112*R -  94*G -  18*B + 128) >> 8) + 128
	 *
	 * All implementations produce identical output.
	 * ====================================================================== */

	static void
	BGRAToI420_Scalar (const uint8_t * bgra, uint32_t w, uint32_t h,
		uint8_t * y, uint8_t * u, uint8_t * v) noexcept
	{
		const uint32_t uvW = w >> 1;
		const uint32_t rowStride = w * 4;

		for ( uint32_t row = 0; row < h; row += 2 )
		{
			const uint8_t * row0 = bgra + row * rowStride;
			const uint8_t * row1 = row0 + rowStride;
			uint8_t * yRow0 = y + row * w;
			uint8_t * yRow1 = yRow0 + w;
			uint8_t * uRow = u + (row >> 1) * uvW;
			uint8_t * vRow = v + (row >> 1) * uvW;

			for ( uint32_t col = 0; col < w; col += 2 )
			{
				const auto * p00 = row0 + col * 4;
				const auto * p01 = p00 + 4;
				const auto * p10 = row1 + col * 4;
				const auto * p11 = p10 + 4;

				const int B00 = p00[0], G00 = p00[1], R00 = p00[2];
				const int B01 = p01[0], G01 = p01[1], R01 = p01[2];
				const int B10 = p10[0], G10 = p10[1], R10 = p10[2];
				const int B11 = p11[0], G11 = p11[1], R11 = p11[2];

				yRow0[col]     = static_cast< uint8_t >(std::clamp(((66 * R00 + 129 * G00 + 25 * B00 + 128) >> 8) + 16, 0, 255));
				yRow0[col + 1] = static_cast< uint8_t >(std::clamp(((66 * R01 + 129 * G01 + 25 * B01 + 128) >> 8) + 16, 0, 255));
				yRow1[col]     = static_cast< uint8_t >(std::clamp(((66 * R10 + 129 * G10 + 25 * B10 + 128) >> 8) + 16, 0, 255));
				yRow1[col + 1] = static_cast< uint8_t >(std::clamp(((66 * R11 + 129 * G11 + 25 * B11 + 128) >> 8) + 16, 0, 255));

				const auto avgR = (R00 + R01 + R10 + R11) >> 2;
				const auto avgG = (G00 + G01 + G10 + G11) >> 2;
				const auto avgB = (B00 + B01 + B10 + B11) >> 2;

				const auto uvIdx = col >> 1;

				uRow[uvIdx] = static_cast< uint8_t >(std::clamp(((-38 * avgR - 74 * avgG + 112 * avgB + 128) >> 8) + 128, 0, 255));
				vRow[uvIdx] = static_cast< uint8_t >(std::clamp(((112 * avgR - 94 * avgG - 18 * avgB + 128) >> 8) + 128, 0, 255));
			}
		}
	}

#if IS_X86_ARCH
	/* ------------------------------------------------------------------
	 * SSE4.1 implementation — processes 8 pixels per Y iteration,
	 * 4 chroma blocks (8 pixels from 2 rows) per UV iteration.
	 * Uses SSSE3 for Y (maddubs, hadd) and SSE4.1 for UV (mullo_epi32).
	 * ------------------------------------------------------------------ */
#ifndef _MSC_VER
	__attribute__((target("sse4.1")))
#endif
	static void
	BGRAToI420_SSE41 (const uint8_t * bgra, uint32_t w, uint32_t h,
		uint8_t * y, uint8_t * u, uint8_t * v) noexcept
	{
		const uint32_t uvW = w >> 1;
		const uint32_t rowStride = w * 4;

		/* BT.601 Y coefficients for _mm_madd_epi16 (signed 16-bit pairs).
		 * Input byte order per pixel is B,G,R,A → coefficient pairs are [25,129] and [66,0].
		 * Note: _mm_maddubs_epi16 cannot be used here because the G coefficient (129)
		 * exceeds signed char range (-128..127) and would be interpreted as -127. */
		const __m128i yCoef = _mm_setr_epi16(25, 129, 66, 0, 25, 129, 66, 0);

		/* Rounding + offset: 128 (rounding) + 16*256 (Y offset pre-shift) = 4224, as 32-bit. */
		const __m128i yBias = _mm_set1_epi32(4224);

		/* UV coefficients: applied to 16-bit channel sums.
		 * U = (-38*R - 74*G + 112*B + 128) >> 8 + 128
		 * V = (112*R - 94*G -  18*B + 128) >> 8 + 128 */
		const __m128i uCoefR = _mm_set1_epi32(-38);
		const __m128i uCoefG = _mm_set1_epi32(-74);
		const __m128i uCoefB = _mm_set1_epi32(112);
		const __m128i vCoefR = _mm_set1_epi32(112);
		const __m128i vCoefG = _mm_set1_epi32(-94);
		const __m128i vCoefB = _mm_set1_epi32(-18);
		const __m128i uvOffset = _mm_set1_epi32(128);
		const __m128i zero = _mm_setzero_si128();

		/* Pass 1: Y plane (full resolution). */
		for ( uint32_t row = 0; row < h; row++ )
		{
			const uint8_t * srcRow = bgra + row * rowStride;
			uint8_t * yRow = y + row * w;
			uint32_t col = 0;

			for ( ; col + 8 <= w; col += 8 )
			{
				/* Load 8 BGRA pixels = 32 bytes = 2 x 128-bit loads. */
				const __m128i px0 = _mm_loadu_si128(reinterpret_cast< const __m128i * >(srcRow + col * 4));      /* pixels 0-3 */
				const __m128i px1 = _mm_loadu_si128(reinterpret_cast< const __m128i * >(srcRow + col * 4 + 16)); /* pixels 4-7 */

				/* Unpack BGRA bytes to 16-bit for _mm_madd_epi16. */
				const __m128i lo0 = _mm_unpacklo_epi8(px0, zero); /* [B0,G0,R0,A0, B1,G1,R1,A1] as 16-bit */
				const __m128i hi0 = _mm_unpackhi_epi8(px0, zero); /* [B2,G2,R2,A2, B3,G3,R3,A3] as 16-bit */
				const __m128i lo1 = _mm_unpacklo_epi8(px1, zero); /* [B4,G4,R4,A4, B5,G5,R5,A5] as 16-bit */
				const __m128i hi1 = _mm_unpackhi_epi8(px1, zero); /* [B6,G6,R6,A6, B7,G7,R7,A7] as 16-bit */

				/* _mm_madd_epi16: signed 16-bit multiply-add pairs → 32-bit.
				 * With coef [25,129,66,0]: gives [25*B+129*G, 66*R+0] per pixel. */
				const __m128i mad0 = _mm_madd_epi16(lo0, yCoef);
				const __m128i mad1 = _mm_madd_epi16(hi0, yCoef);
				const __m128i mad2 = _mm_madd_epi16(lo1, yCoef);
				const __m128i mad3 = _mm_madd_epi16(hi1, yCoef);

				/* _mm_hadd_epi32 (SSSE3): horizontal add adjacent 32-bit pairs.
				 * Combines (25*B+129*G) + (66*R) per pixel → 4 Y sums per hadd. */
				const __m128i hadd0 = _mm_hadd_epi32(mad0, mad1); /* [Y0, Y1, Y2, Y3] */
				const __m128i hadd1 = _mm_hadd_epi32(mad2, mad3); /* [Y4, Y5, Y6, Y7] */

				/* Add bias (rounding + Y offset) and arithmetic right shift by 8. */
				const __m128i shifted0 = _mm_srai_epi32(_mm_add_epi32(hadd0, yBias), 8);
				const __m128i shifted1 = _mm_srai_epi32(_mm_add_epi32(hadd1, yBias), 8);

				/* Pack 32→16 (signed sat) → 8 (unsigned sat). */
				const __m128i packed16 = _mm_packs_epi32(shifted0, shifted1);
				const __m128i packed8 = _mm_packus_epi16(packed16, zero);

				/* Store 8 Y bytes. */
				_mm_storel_epi64(reinterpret_cast< __m128i * >(yRow + col), packed8);
			}

			/* Scalar tail. */
			for ( ; col < w; col++ )
			{
				const auto * px = srcRow + col * 4;

				yRow[col] = static_cast< uint8_t >(std::clamp(((66 * px[2] + 129 * px[1] + 25 * px[0] + 128) >> 8) + 16, 0, 255));
			}
		}

		/* Pass 2: UV planes (2x2 subsampled). */
		for ( uint32_t row = 0; row < h; row += 2 )
		{
			const uint8_t * row0 = bgra + row * rowStride;
			const uint8_t * row1 = row0 + rowStride;
			uint8_t * uRow = u + (row >> 1) * uvW;
			uint8_t * vRow = v + (row >> 1) * uvW;
			uint32_t col = 0;

			/* Process 8 pixels (= 4 chroma blocks) per iteration. */
			for ( ; col + 8 <= w; col += 8 )
			{
				/* Load 8 BGRA pixels from each row. */
				const __m128i r0a = _mm_loadu_si128(reinterpret_cast< const __m128i * >(row0 + col * 4));
				const __m128i r0b = _mm_loadu_si128(reinterpret_cast< const __m128i * >(row0 + col * 4 + 16));
				const __m128i r1a = _mm_loadu_si128(reinterpret_cast< const __m128i * >(row1 + col * 4));
				const __m128i r1b = _mm_loadu_si128(reinterpret_cast< const __m128i * >(row1 + col * 4 + 16));

				/* Unpack bytes to 16-bit for arithmetic.
				 * Each 128-bit register holds 4 BGRA pixels = 16 bytes.
				 * Unpack low/high gives us 16-bit channels. */
				const __m128i r0a_lo = _mm_unpacklo_epi8(r0a, zero); /* pixels 0-1 from row0 */
				const __m128i r0a_hi = _mm_unpackhi_epi8(r0a, zero); /* pixels 2-3 from row0 */
				const __m128i r0b_lo = _mm_unpacklo_epi8(r0b, zero); /* pixels 4-5 from row0 */
				const __m128i r0b_hi = _mm_unpackhi_epi8(r0b, zero); /* pixels 6-7 from row0 */
				const __m128i r1a_lo = _mm_unpacklo_epi8(r1a, zero);
				const __m128i r1a_hi = _mm_unpackhi_epi8(r1a, zero);
				const __m128i r1b_lo = _mm_unpacklo_epi8(r1b, zero);
				const __m128i r1b_hi = _mm_unpackhi_epi8(r1b, zero);

				/* Vertical sum: row0 + row1 for each pair of pixel positions.
				 * r0a_lo has pixels 0,1 from row0; r1a_lo has pixels 0,1 from row1. */
				const __m128i va_lo = _mm_add_epi16(r0a_lo, r1a_lo); /* vert sum pixels 0-1 */
				const __m128i va_hi = _mm_add_epi16(r0a_hi, r1a_hi); /* vert sum pixels 2-3 */
				const __m128i vb_lo = _mm_add_epi16(r0b_lo, r1b_lo); /* vert sum pixels 4-5 */
				const __m128i vb_hi = _mm_add_epi16(r0b_hi, r1b_hi); /* vert sum pixels 6-7 */

				/* Horizontal sum of adjacent pixel pairs.
				 * va_lo holds [B0,G0,R0,A0, B1,G1,R1,A1] as 16-bit.
				 * _mm_hadd_epi16(va_lo, va_hi) doesn't help directly because channels are interleaved.
				 *
				 * Instead, extract B,G,R channels for each 2x2 block manually.
				 * Each block pair in va_lo: pixel0 at [0,1,2,3] and pixel1 at [4,5,6,7].
				 * Sum = pixel0_channel + pixel1_channel gives 2x2 block sum. */

				/* For block 0 (cols 0-1): va_lo has vert sums for pixels 0,1.
				 * For block 1 (cols 2-3): va_hi has vert sums for pixels 2,3.
				 * For block 2 (cols 4-5): vb_lo has vert sums for pixels 4,5.
				 * For block 3 (cols 6-7): vb_hi has vert sums for pixels 6,7.
				 *
				 * Each register: [B_even, G_even, R_even, A_even, B_odd, G_odd, R_odd, A_odd]
				 * We need B_even+B_odd, G_even+G_odd, R_even+R_odd per block. */

				/* Extract 32-bit B,G,R channel sums for each chroma block.
				 * Each vsum register = [B0, G0, R0, A0, B1, G1, R1, A1] as 16-bit.
				 * Unpack to 32-bit, add even+odd pixels, then extract scalar channels. */
				/* Note: use shuffle+cvtsi128 (SSE2) instead of _mm_extract_epi32 (SSE4.1)
				 * because GCC does not propagate target attributes to lambdas. */
				auto extractBlockBGR = [&zero](const __m128i & vsum, int32_t & outB, int32_t & outG, int32_t & outR)
				{
					const __m128i even32 = _mm_unpacklo_epi16(vsum, zero);
					const __m128i odd32 = _mm_unpackhi_epi16(vsum, zero);
					const __m128i sum32 = _mm_add_epi32(even32, odd32);

					outB = _mm_cvtsi128_si32(sum32);
					outG = _mm_cvtsi128_si32(_mm_shuffle_epi32(sum32, _MM_SHUFFLE(1, 1, 1, 1)));
					outR = _mm_cvtsi128_si32(_mm_shuffle_epi32(sum32, _MM_SHUFFLE(2, 2, 2, 2)));
				};

				int32_t b0, g0, r0x, b1, g1, r1x, b2, g2, r2x, b3, g3, r3x;
				extractBlockBGR(va_lo, b0, g0, r0x);
				extractBlockBGR(va_hi, b1, g1, r1x);
				extractBlockBGR(vb_lo, b2, g2, r2x);
				extractBlockBGR(vb_hi, b3, g3, r3x);

				/* Pack the 4 blocks' B,G,R sums into single vectors. */
				const __m128i bSums = _mm_setr_epi32(b0, b1, b2, b3);
				const __m128i gSums = _mm_setr_epi32(g0, g1, g2, g3);
				const __m128i rSums = _mm_setr_epi32(r0x, r1x, r2x, r3x);

				/* Compute U and V for 4 blocks.
				 * U = (-38*R_sum - 74*G_sum + 112*B_sum + 128*4) >> 10 + 128
				 *   (divide by 4 is folded into >>10 instead of >>8)
				 * V = (112*R_sum - 94*G_sum - 18*B_sum + 128*4) >> 10 + 128 */
				const __m128i uvRound4 = _mm_set1_epi32(512); /* 128*4 = 512 */

				const __m128i uVal32 = _mm_add_epi32(
					_mm_add_epi32(
						_mm_add_epi32(_mm_mullo_epi32(uCoefR, rSums), _mm_mullo_epi32(uCoefG, gSums)),
						_mm_mullo_epi32(uCoefB, bSums)
					),
					uvRound4
				);
				const __m128i vVal32 = _mm_add_epi32(
					_mm_add_epi32(
						_mm_add_epi32(_mm_mullo_epi32(vCoefR, rSums), _mm_mullo_epi32(vCoefG, gSums)),
						_mm_mullo_epi32(vCoefB, bSums)
					),
					uvRound4
				);

				/* Arithmetic right shift by 10 (combines /4 average + >>8 BT.601 shift). */
				const __m128i uShifted = _mm_add_epi32(_mm_srai_epi32(uVal32, 10), uvOffset);
				const __m128i vShifted = _mm_add_epi32(_mm_srai_epi32(vVal32, 10), uvOffset);

				/* Clamp to [0, 255] via pack chain: 32→16 (signed sat) → 8 (unsigned sat). */
				const __m128i uPacked16 = _mm_packs_epi32(uShifted, zero);
				const __m128i vPacked16 = _mm_packs_epi32(vShifted, zero);
				const __m128i uPacked8 = _mm_packus_epi16(uPacked16, zero);
				const __m128i vPacked8 = _mm_packus_epi16(vPacked16, zero);

				/* Store 4 U and 4 V bytes. */
				const auto uvIdx = col >> 1;
				*reinterpret_cast< uint32_t * >(uRow + uvIdx) = static_cast< uint32_t >(_mm_cvtsi128_si32(uPacked8));
				*reinterpret_cast< uint32_t * >(vRow + uvIdx) = static_cast< uint32_t >(_mm_cvtsi128_si32(vPacked8));
			}

			/* Scalar tail for UV. */
			for ( ; col < w; col += 2 )
			{
				const auto * p00 = row0 + col * 4;
				const auto * p01 = p00 + 4;
				const auto * p10 = row1 + col * 4;
				const auto * p11 = p10 + 4;

				const auto avgR = (p00[2] + p01[2] + p10[2] + p11[2]) >> 2;
				const auto avgG = (p00[1] + p01[1] + p10[1] + p11[1]) >> 2;
				const auto avgB = (p00[0] + p01[0] + p10[0] + p11[0]) >> 2;

				const auto uvIdx = col >> 1;

				uRow[uvIdx] = static_cast< uint8_t >(std::clamp(((-38 * avgR - 74 * avgG + 112 * avgB + 128) >> 8) + 128, 0, 255));
				vRow[uvIdx] = static_cast< uint8_t >(std::clamp(((112 * avgR - 94 * avgG - 18 * avgB + 128) >> 8) + 128, 0, 255));
			}
		}
	}

	/* ------------------------------------------------------------------
	 * AVX2 implementation — processes 16 pixels per Y iteration,
	 * 8 chroma blocks (16 pixels from 2 rows) per UV iteration.
	 * ------------------------------------------------------------------ */
#ifndef _MSC_VER
	__attribute__((target("avx2")))
#endif
	static void
	BGRAToI420_AVX2 (const uint8_t * bgra, uint32_t w, uint32_t h,
		uint8_t * y, uint8_t * u, uint8_t * v) noexcept
	{
		const uint32_t uvW = w >> 1;
		const uint32_t rowStride = w * 4;

		/* BT.601 Y coefficients for _mm256_madd_epi16 (signed 16-bit pairs).
		 * Cannot use _mm256_maddubs_epi16 because G coefficient (129) exceeds signed char range. */
		const __m256i yCoef256 = _mm256_setr_epi16(
			25, 129, 66, 0,  25, 129, 66, 0,
			25, 129, 66, 0,  25, 129, 66, 0
		);

		const __m256i yBias256 = _mm256_set1_epi32(4224);
		const __m256i zero256 = _mm256_setzero_si256();
		const __m128i zero = _mm_setzero_si128();

		/* UV coefficients (32-bit). */
		const __m256i uCoefR256 = _mm256_set1_epi32(-38);
		const __m256i uCoefG256 = _mm256_set1_epi32(-74);
		const __m256i uCoefB256 = _mm256_set1_epi32(112);
		const __m256i vCoefR256 = _mm256_set1_epi32(112);
		const __m256i vCoefG256 = _mm256_set1_epi32(-94);
		const __m256i vCoefB256 = _mm256_set1_epi32(-18);
		const __m256i uvRound8 = _mm256_set1_epi32(512);
		const __m256i uvOffset256 = _mm256_set1_epi32(128);

		/* Pass 1: Y plane. */
		for ( uint32_t row = 0; row < h; row++ )
		{
			const uint8_t * srcRow = bgra + row * rowStride;
			uint8_t * yRow = y + row * w;
			uint32_t col = 0;

			for ( ; col + 16 <= w; col += 16 )
			{
				/* Load 16 BGRA pixels = 64 bytes = 2 x 256-bit loads. */
				const __m256i px0 = _mm256_loadu_si256(reinterpret_cast< const __m256i * >(srcRow + col * 4));      /* pixels 0-7 */
				const __m256i px1 = _mm256_loadu_si256(reinterpret_cast< const __m256i * >(srcRow + col * 4 + 32)); /* pixels 8-15 */

				/* Unpack BGRA bytes to 16-bit within each 128-bit lane. */
				const __m256i lo0 = _mm256_unpacklo_epi8(px0, zero256); /* lane0: pix 0,1 | lane1: pix 4,5 */
				const __m256i hi0 = _mm256_unpackhi_epi8(px0, zero256); /* lane0: pix 2,3 | lane1: pix 6,7 */
				const __m256i lo1 = _mm256_unpacklo_epi8(px1, zero256); /* lane0: pix 8,9 | lane1: pix 12,13 */
				const __m256i hi1 = _mm256_unpackhi_epi8(px1, zero256); /* lane0: pix 10,11 | lane1: pix 14,15 */

				/* _mm256_madd_epi16: signed 16×16→32 multiply-add pairs.
				 * Gives [25*B+129*G, 66*R+0] per pixel as 32-bit. */
				const __m256i mad0 = _mm256_madd_epi16(lo0, yCoef256);
				const __m256i mad1 = _mm256_madd_epi16(hi0, yCoef256);
				const __m256i mad2 = _mm256_madd_epi16(lo1, yCoef256);
				const __m256i mad3 = _mm256_madd_epi16(hi1, yCoef256);

				/* Horizontal add 32-bit pairs → per-pixel Y sums (lane-wise). */
				const __m256i hadd0 = _mm256_hadd_epi32(mad0, mad1); /* [Y0,Y1,Y2,Y3 | Y4,Y5,Y6,Y7] */
				const __m256i hadd1 = _mm256_hadd_epi32(mad2, mad3); /* [Y8,Y9,Y10,Y11 | Y12,Y13,Y14,Y15] */

				/* Add bias and arithmetic right shift by 8. */
				const __m256i shifted0 = _mm256_srai_epi32(_mm256_add_epi32(hadd0, yBias256), 8);
				const __m256i shifted1 = _mm256_srai_epi32(_mm256_add_epi32(hadd1, yBias256), 8);

				/* Extract 128-bit lanes and use SSE packing to avoid AVX2 lane-crossing issues.
				 * AVX2 packs_epi32/packus_epi16 are lane-wise — packing with zero256 leaves
				 * gaps of zeros (= black bands) instead of contiguous pixel data. */
				const __m128i s0_lo = _mm256_castsi256_si128(shifted0);      /* [Y0, Y1, Y2, Y3] */
				const __m128i s0_hi = _mm256_extracti128_si256(shifted0, 1); /* [Y4, Y5, Y6, Y7] */
				const __m128i s1_lo = _mm256_castsi256_si128(shifted1);      /* [Y8, Y9, Y10, Y11] */
				const __m128i s1_hi = _mm256_extracti128_si256(shifted1, 1); /* [Y12, Y13, Y14, Y15] */

				const __m128i pk16_a = _mm_packs_epi32(s0_lo, s0_hi); /* [Y0..Y7] as int16 */
				const __m128i pk16_b = _mm_packs_epi32(s1_lo, s1_hi); /* [Y8..Y15] as int16 */
				const __m128i result = _mm_packus_epi16(pk16_a, pk16_b); /* [Y0..Y15] as uint8 */

				/* Store 16 Y bytes. */
				_mm_storeu_si128(reinterpret_cast< __m128i * >(yRow + col), result);
			}

			/* Scalar tail. */
			for ( ; col < w; col++ )
			{
				const auto * px = srcRow + col * 4;

				yRow[col] = static_cast< uint8_t >(std::clamp(((66 * px[2] + 129 * px[1] + 25 * px[0] + 128) >> 8) + 16, 0, 255));
			}
		}

		/* Pass 2: UV planes (2x2 subsampled).
		 * Process 16 pixels (= 8 chroma blocks) per iteration. */
		for ( uint32_t row = 0; row < h; row += 2 )
		{
			const uint8_t * row0 = bgra + row * rowStride;
			const uint8_t * row1 = row0 + rowStride;
			uint8_t * uRow = u + (row >> 1) * uvW;
			uint8_t * vRow = v + (row >> 1) * uvW;
			uint32_t col = 0;

			for ( ; col + 16 <= w; col += 16 )
			{
				/* Load 16 BGRA pixels from each row (64 bytes each = 4 x 128-bit). */
				const __m128i r0_0 = _mm_loadu_si128(reinterpret_cast< const __m128i * >(row0 + col * 4));
				const __m128i r0_1 = _mm_loadu_si128(reinterpret_cast< const __m128i * >(row0 + col * 4 + 16));
				const __m128i r0_2 = _mm_loadu_si128(reinterpret_cast< const __m128i * >(row0 + col * 4 + 32));
				const __m128i r0_3 = _mm_loadu_si128(reinterpret_cast< const __m128i * >(row0 + col * 4 + 48));
				const __m128i r1_0 = _mm_loadu_si128(reinterpret_cast< const __m128i * >(row1 + col * 4));
				const __m128i r1_1 = _mm_loadu_si128(reinterpret_cast< const __m128i * >(row1 + col * 4 + 16));
				const __m128i r1_2 = _mm_loadu_si128(reinterpret_cast< const __m128i * >(row1 + col * 4 + 32));
				const __m128i r1_3 = _mm_loadu_si128(reinterpret_cast< const __m128i * >(row1 + col * 4 + 48));

				/* Process 8 chroma blocks using the same logic as SSSE3 but for 8 blocks.
				 * Each 128-bit register holds 4 BGRA pixels.
				 * Unpack to 16-bit, vertical sum, then extract per-block channel sums. */

				/* Unpack to 16-bit and vertical sum (avoids std::pair<__m128i> which triggers -Wignored-attributes). */
				__m128i vs0_lo, vs0_hi, vs1_lo, vs1_hi, vs2_lo, vs2_hi, vs3_lo, vs3_hi;

				{
					auto vertSum = [&zero](const __m128i & a, const __m128i & b, __m128i & outLo, __m128i & outHi)
					{
						outLo = _mm_add_epi16(_mm_unpacklo_epi8(a, zero), _mm_unpacklo_epi8(b, zero));
						outHi = _mm_add_epi16(_mm_unpackhi_epi8(a, zero), _mm_unpackhi_epi8(b, zero));
					};

					vertSum(r0_0, r1_0, vs0_lo, vs0_hi); /* blocks 0,1 */
					vertSum(r0_1, r1_1, vs1_lo, vs1_hi); /* blocks 2,3 */
					vertSum(r0_2, r1_2, vs2_lo, vs2_hi); /* blocks 4,5 */
					vertSum(r0_3, r1_3, vs3_lo, vs3_hi); /* blocks 6,7 */
				}

				/* For each block: extract B,G,R channel sums from vertical-summed 16-bit data. */
				/* Note: use shuffle+cvtsi128 (SSE2) instead of _mm_extract_epi32 (SSE4.1)
				 * because GCC does not propagate target attributes to lambdas. */
				auto extractBlockBGR = [&zero](const __m128i & vsum, int32_t & outB, int32_t & outG, int32_t & outR)
				{
					const __m128i even32 = _mm_unpacklo_epi16(vsum, zero);
					const __m128i odd32 = _mm_unpackhi_epi16(vsum, zero);
					const __m128i sum32 = _mm_add_epi32(even32, odd32);

					outB = _mm_cvtsi128_si32(sum32);
					outG = _mm_cvtsi128_si32(_mm_shuffle_epi32(sum32, _MM_SHUFFLE(1, 1, 1, 1)));
					outR = _mm_cvtsi128_si32(_mm_shuffle_epi32(sum32, _MM_SHUFFLE(2, 2, 2, 2)));
				};

				/* Extract B,G,R for all 8 blocks and pack into 256-bit vectors. */
				int32_t b0, g0, r0x, b1, g1, r1x, b2, g2, r2x, b3, g3, r3x;
				int32_t b4, g4, r4x, b5, g5, r5x, b6, g6, r6x, b7, g7, r7x;
				extractBlockBGR(vs0_lo, b0, g0, r0x);
				extractBlockBGR(vs0_hi, b1, g1, r1x);
				extractBlockBGR(vs1_lo, b2, g2, r2x);
				extractBlockBGR(vs1_hi, b3, g3, r3x);
				extractBlockBGR(vs2_lo, b4, g4, r4x);
				extractBlockBGR(vs2_hi, b5, g5, r5x);
				extractBlockBGR(vs3_lo, b6, g6, r6x);
				extractBlockBGR(vs3_hi, b7, g7, r7x);

				const __m256i bSums = _mm256_setr_epi32(b0, b1, b2, b3, b4, b5, b6, b7);
				const __m256i gSums = _mm256_setr_epi32(g0, g1, g2, g3, g4, g5, g6, g7);
				const __m256i rSums = _mm256_setr_epi32(r0x, r1x, r2x, r3x, r4x, r5x, r6x, r7x);

				/* U = (-38*R - 74*G + 112*B + 512) >> 10 + 128 */
				const __m256i uVal = _mm256_add_epi32(
					_mm256_add_epi32(
						_mm256_add_epi32(_mm256_mullo_epi32(uCoefR256, rSums), _mm256_mullo_epi32(uCoefG256, gSums)),
						_mm256_mullo_epi32(uCoefB256, bSums)
					),
					uvRound8
				);

				const __m256i vVal = _mm256_add_epi32(
					_mm256_add_epi32(
						_mm256_add_epi32(_mm256_mullo_epi32(vCoefR256, rSums), _mm256_mullo_epi32(vCoefG256, gSums)),
						_mm256_mullo_epi32(vCoefB256, bSums)
					),
					uvRound8
				);

				const __m256i uShifted = _mm256_add_epi32(_mm256_srai_epi32(uVal, 10), uvOffset256);
				const __m256i vShifted = _mm256_add_epi32(_mm256_srai_epi32(vVal, 10), uvOffset256);

				/* Pack 32→16→8 with saturation.
				 * _mm256_packs_epi32 works within 128-bit lanes, then permute to fix order. */
				const __m256i uPacked16 = _mm256_packs_epi32(uShifted, zero256);
				const __m256i vPacked16 = _mm256_packs_epi32(vShifted, zero256);
				const __m256i uPermuted = _mm256_permute4x64_epi64(uPacked16, _MM_SHUFFLE(3, 1, 2, 0));
				const __m256i vPermuted = _mm256_permute4x64_epi64(vPacked16, _MM_SHUFFLE(3, 1, 2, 0));
				const __m128i uLo = _mm256_castsi256_si128(uPermuted);
				const __m128i vLo = _mm256_castsi256_si128(vPermuted);
				const __m128i uPacked8 = _mm_packus_epi16(uLo, zero);
				const __m128i vPacked8 = _mm_packus_epi16(vLo, zero);

				/* Store 8 U and 8 V bytes. */
				const auto uvIdx = col >> 1;
				_mm_storel_epi64(reinterpret_cast< __m128i * >(uRow + uvIdx), uPacked8);
				_mm_storel_epi64(reinterpret_cast< __m128i * >(vRow + uvIdx), vPacked8);
			}

			/* Scalar tail for UV. */
			for ( ; col < w; col += 2 )
			{
				const auto * p00 = row0 + col * 4;
				const auto * p01 = p00 + 4;
				const auto * p10 = row1 + col * 4;
				const auto * p11 = p10 + 4;

				const auto avgR = (p00[2] + p01[2] + p10[2] + p11[2]) >> 2;
				const auto avgG = (p00[1] + p01[1] + p10[1] + p11[1]) >> 2;
				const auto avgB = (p00[0] + p01[0] + p10[0] + p11[0]) >> 2;

				const auto uvIdx = col >> 1;

				uRow[uvIdx] = static_cast< uint8_t >(std::clamp(((-38 * avgR - 74 * avgG + 112 * avgB + 128) >> 8) + 128, 0, 255));
				vRow[uvIdx] = static_cast< uint8_t >(std::clamp(((112 * avgR - 94 * avgG - 18 * avgB + 128) >> 8) + 128, 0, 255));
			}
		}
	}
#endif /* IS_X86_ARCH */
#else
	static constexpr auto RecordingDisabledMessage{"Video recording is not available. Recompile the engine with EMERAUDE_ENABLE_VIDEO_RECORDING=On to enable this feature."};

	Recorder::Recorder (PrimaryServices & primaryServices, Renderer & renderer) noexcept
		: ServiceInterface{ClassId}
	{

	}

	bool
	Recorder::startRecording (const std::filesystem::path & /*outputPath*/) noexcept
	{
		Tracer::warning(ClassId, RecordingDisabledMessage);

		return false;
	}

	bool
	Recorder::stopRecording () noexcept
	{
		Tracer::warning(ClassId, RecordingDisabledMessage);

		return false;
	}

	[[nodiscard]]
	bool
	Recorder::isRecording () const noexcept
	{
		return false;
	}

	[[nodiscard]]
	bool
	Recorder::shouldCaptureFrame () const noexcept
	{
		return false;
	}

	void
	Recorder::captureAndSubmitFrame () noexcept
	{

	}

	bool
	Recorder::onInitialize () noexcept
	{
		Tracer::info(ClassId, RecordingDisabledMessage);

		return false;
	}

	bool
	Recorder::onTerminate () noexcept
	{
		return true;
	}
#endif
}

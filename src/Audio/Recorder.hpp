/*
 * src/Audio/Recorder.hpp
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

#include <fstream>

/* STL inclusions. */
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

/* Third-party inclusions. */
#include "OpenALExtensions.hpp"

/* Local inclusions. */
#include "ServiceInterface.hpp"

/* Local inclusions for usages. */
#include "Libs/WaveFactory/Types.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Audio
	{
		class Manager;
	}

	class PrimaryServices;
}

namespace EmEn::Audio
{
	/**
	 * @class Recorder
	 * @brief Audio recording service that captures game audio using OpenAL Soft loopback passthrough.
	 *
	 * Provides real-time audio recording and speaker playback using an OpenAL Soft loopback device.
	 * The loopback pipeline operates in three stages:
	 * 1. Loopback device renders game audio into a buffer instead of speakers
	 * 2. Dedicated render thread continuously pulls samples via alcRenderSamplesSOFT()
	 * 3. Samples are forwarded to a real playback device for speaker output and optionally accumulated for WAV recording
	 *
	 * The render thread uses a streaming source with buffered playback to ensure smooth audio output
	 * while simultaneously capturing PCM samples. When recording is active, samples are accumulated
	 * into a vector and written to a WAV file on stopRecording() using WaveFactory.
	 *
	 * @note Requires OpenAL Soft extensions: ALC_SOFT_loopback and ALC_EXT_thread_local_context.
	 * @see EmEn::ServiceInterface, EmEn::Audio::Manager
	 * @version 0.8.51
	 */
	class Recorder final : public ServiceInterface
	{
		public:

			/** @brief Service identifier for logging and registration. */
			static constexpr auto ClassId{"AudioRecorderService"};

			/**
			 * @brief Constructs the audio recorder service.
			 * @param primaryServices Reference to primary services for settings and filesystem access.
			 * @param audioManager Reference to the audio manager.
			 */
			Recorder (PrimaryServices & primaryServices, Manager & audioManager) noexcept;

			/**
			 * @brief Returns the loopback device pointer (used by Manager as the output device).
			 *
			 * @return Pointer to the loopback device, or nullptr if not active.
			 */
			[[nodiscard]]
			ALCdevice *
			loopbackDevice () const noexcept
			{
				return m_loopbackDevice;
			}

			/**
			 * @brief Returns the game context created on the loopback device.
			 *
			 * @return Pointer to the game context, or nullptr if not active.
			 */
			[[nodiscard]]
			ALCcontext *
			gameContext () const noexcept
			{
				return m_gameContext;
			}

			/**
			 * @brief Starts recording the audio output to a WAV file.
			 *
			 * Clears any previously accumulated samples and begins capturing PCM data from the render thread.
			 * The render thread will accumulate stereo int16 samples into m_recordSamples while recording is active.
			 *
			 * @param outputPath The filesystem path for the output WAV file.
			 * @return True if recording started successfully, false if pipeline is not active or already recording.
			 * @pre Loopback pipeline must be active (isActive() returns true).
			 * @post Recording flag is set, sample buffer is cleared, and render thread begins accumulating samples.
			 */
			bool startRecording (const std::filesystem::path & outputPath) noexcept;

			/**
			 * @brief Stops audio recording and writes the accumulated samples to the WAV file.
			 *
			 * Signals the render thread to stop accumulating samples, moves the accumulated PCM data,
			 * initializes a WaveFactory Wave object with stereo int16 format, and writes the final
			 * WAV file to the output path specified in startRecording().
			 *
			 * @return True if recording was active and WAV file written successfully, false if not recording or write failed.
			 * @post Recording flag is cleared, sample buffer is empty, WAV file is written to disk.
			 */
			bool stopRecording () noexcept;

			/**
			 * @brief Returns the number of audio channels used for recording.
			 *
			 * @return Channel count (2 for stereo, 6 for 5.1 surround).
			 */
			[[nodiscard]]
			uint16_t
			channelCount () const noexcept
			{
				return m_channelCount;
			}

			/**
			 * @brief Returns whether audio recording is currently active.
			 *
			 * @return True if recording, false otherwise.
			 */
			[[nodiscard]]
			bool
			isRecording () const noexcept
			{
				return m_recording;
			}

		private:

			/**
			 * @brief Sets up the loopback device, game context, playback device/context, and render thread.
			 *
			 * Creates the full OpenAL Soft loopback passthrough pipeline:
			 * 1. Opens a loopback device (no hardware output) and verifies stereo int16 format support
			 * 2. Creates the game context on the loopback device with specified source limits
			 * 3. Opens the real output device and creates a playback context for speaker passthrough
			 * 4. Starts the dedicated render thread that continuously pulls samples from loopback and forwards to speakers
			 *
			 * @return True if all resources created successfully and render thread started, false otherwise.
			 * @pre OpenAL Soft extensions ALC_SOFT_loopback and ALC_EXT_thread_local_context must be available.
			 * @post Loopback pipeline is active, render thread is running, and game context is set as thread-local.
			 */
			bool onInitialize () noexcept override;

			/**
			 * @brief Shuts down the loopback pipeline, stops recording if active, and releases all resources.
			 *
			 * Stops any active recording, signals the render thread to terminate, waits for thread join,
			 * and releases all OpenAL contexts and devices in proper order. Thread-local context is unset
			 * on the main thread.
			 *
			 * @post All resources released, render thread joined, pipeline inactive.
			 */
			bool onTerminate () noexcept override;

			/**
			 * @brief Render thread entry point that continuously renders loopback samples and forwards to speakers.
			 *
			 * Sets the playback context as thread-local, creates a streaming source with 4 buffers on the playback
			 * context, and continuously:
			 * 1. Pulls rendered samples from the loopback device using alcRenderSamplesSOFT()
			 * 2. Optionally accumulates samples for WAV recording (if m_recording is true)
			 * 3. Queues samples to the streaming source for speaker playback using alSourceQueueBuffers()
			 *
			 * Uses a 1024-sample chunk size with 4-buffer double-buffering to ensure smooth playback.
			 * Runs until m_renderRunning becomes false, then drains and releases all resources.
			 *
			 * @note This function executes on the render thread, separate from the main thread.
			 * @post All OpenAL resources are released and thread-local context is unset.
			 */
			void renderThreadFunc () noexcept;

			/**
			 * @brief Writes a placeholder or final WAV header to the output stream.
			 *
			 * @param stream The output stream to write to.
			 * @param dataSize The total size of the audio data in bytes.
			 * @return True if successful, false otherwise.
			 */
			bool writeWAVHeader (std::ostream & stream, uint32_t dataSize) const noexcept;

			PrimaryServices & m_primaryServices; ///< Primary services for settings and filesystem access.
			Manager & m_audioManager; ///< Audio manager owning this recorder.
			ALCdevice * m_loopbackDevice{nullptr}; ///< OpenAL loopback device (renders to buffer instead of speakers).
			ALCdevice * m_playbackDevice{nullptr}; ///< Real output device for speaker passthrough.
			ALCcontext * m_gameContext{nullptr}; ///< Game audio context on the loopback device (set as global current).
			ALCcontext * m_playbackContext{nullptr}; ///< Playback context on the real output device (thread-local on render thread).
			ALCcontext * m_previousGlobalContext{nullptr}; ///< Saved previous global context to restore on terminate.
			std::thread m_renderThread{}; ///< Dedicated render thread that pulls loopback samples and forwards to speakers.
			Libs::WaveFactory::Frequency m_playbackFrequency{Libs::WaveFactory::Frequency::PCM48000Hz}; ///< Playback frequency (typically 48kHz).
			uint16_t m_channelCount{2}; ///< Number of audio channels (2 = stereo, 6 = 5.1 surround).
			std::atomic< bool > m_recording{false}; ///< True when actively recording audio to WAV.
			std::atomic< bool > m_renderRunning{false}; ///< True when render thread should continue running.
			std::filesystem::path m_outputPath{}; ///< Output path for the WAV file set in startRecording().

			std::ofstream m_outputFileStream{}; ///< Output file stream for writing WAV data.
			std::streampos m_dataSizePos{}; ///< Position in the file where the data chunk size is written.
			uint32_t m_streamByteCount{0}; ///< Total bytes written to the data chunk.
	};
}

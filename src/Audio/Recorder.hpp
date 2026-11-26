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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stop_token>

/* Third-party inclusions. */
#include "AL/alc.h"

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"

/* Local inclusions for usages. */
#include "ConditionVariable.hpp"
#include "Mutex.hpp"
#include "OpenALHandles.hpp"
#include "Thread.hpp"
#include "ThreadSafety.hpp"
#include "WaveFactory/Types.hpp"

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
	 * while simultaneously capturing PCM samples. When recording is active, each chunk is written to the WAV file,
	 * whose header is completed when the recording stops.
	 *
	 * @note The render thread OWNS the recording file: it opens it, writes it, completes its header and closes it.
	 * startRecording() and stopRecording() post a request (the recording state and the path, behind a mutex) and wait
	 * for the thread's answer; no file object nor byte count is touched by two threads.
	 * @note Requires OpenAL Soft extensions: ALC_SOFT_loopback and ALC_EXT_thread_local_context.
	 * @see EmEn::ServiceInterface, EmEn::Audio::Manager
	 * @version 0.8.51
	 */
	class EMEN_API Recorder final : public ServiceInterface
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
			 * @brief Destructs the recorder: when onTerminate() was skipped (an initialization failure path), the render
			 * thread is stopped and joined and the OpenAL contexts / devices are released here.
			 */
			~Recorder () override;

			/** @brief Copy constructor (deleted: the render thread and the OpenAL handles are owned). */
			Recorder (const Recorder & copy) noexcept = delete;

			/** @brief Move constructor (deleted: the render thread captures this). */
			Recorder (Recorder && copy) noexcept = delete;

			/** @brief Copy assignment (deleted). */
			Recorder & operator= (const Recorder & copy) noexcept = delete;

			/** @brief Move assignment (deleted). */
			Recorder & operator= (Recorder && copy) noexcept = delete;

			/**
			 * @brief Returns the loopback device pointer (used by Manager as the output device).
			 *
			 * @return Pointer to the loopback device, or nullptr if not active.
			 */
			[[nodiscard]]
			ALCdevice *
			loopbackDevice () const noexcept
			{
				return m_loopbackDevice.get();
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
				return m_gameContext.get();
			}

			/**
			 * @brief Starts recording the audio output to a WAV file.
			 *
			 * Posts the request to the render thread, which opens the file and writes its header, and waits for its
			 * answer (RequestTimeout at most).
			 *
			 * @param outputPath The filesystem path for the output WAV file.
			 * @return True when the render thread opened the file and records. False when the recorder is not
			 * initialized, a recording is already in progress or being started / stopped, the file could not be opened,
			 * or the thread did not answer in time (traced; a request the thread had not taken yet is withdrawn).
			 * @note Any thread but the render thread itself.
			 */
			bool startRecording (const std::filesystem::path & outputPath) noexcept;

			/**
			 * @brief Stops the audio recording: the render thread completes the WAV header and closes the file.
			 *
			 * Posts the request and waits for the thread's answer (RequestTimeout at most): the file is complete when
			 * this returns true.
			 *
			 * @return True when a recording was active and its file was completed. False when nothing was recording,
			 * the header could not be written, or the thread did not answer in time (traced: the thread still
			 * completes the file, which it owns).
			 * @note Any thread but the render thread itself.
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
			 * @brief Returns whether an audio recording is in progress, being started or being stopped.
			 * @note Any thread.
			 * @return True if recording, false otherwise.
			 */
			[[nodiscard]]
			bool isRecording () const noexcept;

		private:

			/** @brief The longest wait of startRecording() / stopRecording() for the render thread's answer. */
			static constexpr std::chrono::seconds RequestTimeout{5};

			/** @brief Where a recording stands. Written by the two sides in turn, always under m_recordingAccess. */
			enum class RecordingState : uint8_t
			{
				/** No recording, no request. */
				Idle,
				/** startRecording() posted a path; the render thread has not taken it yet. */
				StartRequested,
				/** The render thread is opening the file. */
				Starting,
				/** The render thread writes every chunk to the file. */
				Recording,
				/** stopRecording() asked for the end; the render thread has not taken it yet. */
				StopRequested,
				/** The render thread is completing the header and closing the file. */
				Stopping
			};

			/** @brief The recording file. A LOCAL of the render thread's function: no other thread ever sees it. */
			struct RecordingFile final
			{
				std::ofstream stream; ///< The WAV file being written.
				std::filesystem::path path; ///< Its path, for the traces.
				uint32_t byteCount{0}; ///< The bytes written to the data chunk (a WAV size field is 32-bit).
			};

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
			 * @brief Releases the contexts and devices in order: the audio manager's context made current again (when the
			 * game context is), each context before its device. Every failure branch of onInitialize() and onTerminate()
			 * end here.
			 * @pre The render thread is joined (or never started).
			 */
			void releaseContexts () noexcept;

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
			 * Runs until the thread's stop is requested, then drains and releases all resources.
			 *
			 * @note This function executes on the render thread, separate from the main thread.
			 * @post All OpenAL resources are released and thread-local context is unset.
			 * @param stopToken The thread's stop token (Base::Thread): checked once per chunk.
			 */
			void renderThreadFunc (const std::stop_token & stopToken) noexcept;

			/**
			 * @brief Takes a pending request of startRecording() / stopRecording() and answers it.
			 * @note Render thread only, once per loop turn. The file work is done WITHOUT the mutex: the lock is held
			 * to read the request and to publish the answer only.
			 * @param file A reference to the thread's recording file.
			 */
			void serveRecordingRequest (RecordingFile & file) noexcept;

			/**
			 * @brief Writes one rendered chunk to the recording file; a write failure or a full WAV (4 GiB) ends the
			 * recording (the file is completed and closed, the state goes back to Idle).
			 * @note Render thread only.
			 * @param file A reference to the thread's recording file. Nothing is done when it is not open.
			 * @param data A pointer to the chunk's bytes.
			 * @param bytes The chunk's size in bytes.
			 */
			void writeRecordingChunk (RecordingFile & file, const char * data, size_t bytes) noexcept;

			/**
			 * @brief Opens the recording file and writes its placeholder header.
			 * @note Render thread only.
			 * @param file A reference to the thread's recording file, closed.
			 * @param path A reference to the path of the file.
			 * @return bool False when the file could not be opened or its header written (traced, the file is closed).
			 */
			[[nodiscard]]
			bool openRecordingFile (RecordingFile & file, const std::filesystem::path & path) const noexcept;

			/**
			 * @brief Completes the two size fields of the WAV header and closes the recording file.
			 * @note Render thread only.
			 * @param file A reference to the thread's recording file. Closed at return, whatever happened.
			 * @return bool False when the file was not open or the header could not be written (traced).
			 */
			[[nodiscard]]
			static bool closeRecordingFile (RecordingFile & file) noexcept;

			/**
			 * @brief Publishes the state the render thread reached and wakes whoever waits for it.
			 * @note Render thread only.
			 * @param state The new state.
			 * @param succeeded Whether the request just served succeeded.
			 * @param savedBytes The data bytes of the file just closed (0 otherwise).
			 */
			void publishRecordingState (RecordingState state, bool succeeded, uint32_t savedBytes) noexcept;

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
			/* NOTE: Owned (Ave Robustus II P3): the devices before the contexts created on them, destroyed after them.
			 * releaseContexts() releases them in order; the holders close whatever is left. */
			ALCDeviceHandle m_loopbackDevice; ///< OpenAL loopback device (renders to buffer instead of speakers).
			ALCDeviceHandle m_playbackDevice; ///< Real output device for speaker passthrough.
			ALCContextHandle m_gameContext; ///< Game audio context on the loopback device (set as global current).
			ALCContextHandle m_playbackContext; ///< Playback context on the real output device (thread-local on render thread).
			ALCcontext * m_previousGlobalContext{nullptr}; ///< NOT owned: the audio manager's context, made current again at the release.
			Base::WaveFactory::Frequency m_playbackFrequency{Base::WaveFactory::Frequency::PCM48000Hz}; ///< Playback frequency (typically 48kHz).
			uint16_t m_channelCount{2}; ///< Number of audio channels (2 = stereo, 6 = 5.1 surround).
			/* NOTE: The whole of what the render thread shares with the callers of startRecording() / stopRecording():
			 * the file itself is the thread's (RecordingFile, a local of renderThreadFunc()). */
			mutable Base::Mutex m_recordingAccess; ///< Guards the four members below.
			Base::ConditionVariable m_recordingChanged; ///< Notified by the render thread at each state it publishes.
			std::filesystem::path m_requestedPath EMEN_GUARDED_BY(m_recordingAccess); ///< The path posted by startRecording().
			uint32_t m_savedByteCount EMEN_GUARDED_BY(m_recordingAccess){0}; ///< The data bytes of the last file closed.
			RecordingState m_recordingState EMEN_GUARDED_BY(m_recordingAccess){RecordingState::Idle}; ///< Where the recording stands.
			bool m_requestSucceeded EMEN_GUARDED_BY(m_recordingAccess){false}; ///< The answer to the last request served.
			/* NOTE: LAST on purpose (Ave Robustus II, member order): the render thread uses the members above, so it is
			 * destroyed — joined — first. The destructor also stops it explicitly. */
			Base::Thread m_renderThread; ///< Dedicated render thread that pulls loopback samples and forwards to speakers.
	};
}

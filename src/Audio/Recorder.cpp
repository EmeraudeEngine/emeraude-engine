/*
 * src/Audio/Recorder.cpp
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

#include "Recorder.hpp"

/* STL inclusions. */
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <vector>

/* Third-party inclusions. */
#include "AL/al.h"
#include "AL/alext.h"
#include "AL/efx.h"

/* Local inclusions. */
#include "Audio/OpenALExtensions.hpp"
#include "Audio/OpenALHandles.hpp"
#include "BlobTrait.hpp"
#include "Manager.hpp"
#include "PrimaryServices.hpp"
#include "SettingKeys.hpp"
#include "Settings.hpp"
#include "Tracer.hpp"

namespace EmEn::Audio
{
	using namespace Base;

	Recorder::Recorder (PrimaryServices & primaryServices, Manager & audioManager) noexcept
		: ServiceInterface{ClassId},
		m_primaryServices{primaryServices},
		m_audioManager{audioManager}
	{

	}

	bool
	Recorder::onInitialize () noexcept
	{
		auto & settings = m_primaryServices.settings();

		if ( !settings.getOrSetDefault< bool >(RushMakerEnableAudioKey, DefaultRushMakerEnabled) )
		{
			return false;
		}

		const auto & outputDeviceName = m_audioManager.selectedOutputDeviceName();
		const auto monoSources = settings.getOrSetDefaultInRange< int32_t >(OpenALMaxMonoSourceCountKey, DefaultOpenALMaxMonoSourceCount, MinOpenALMaxMonoSourceCount, MaxOpenALMaxMonoSourceCount);
		const auto stereoSources = settings.getOrSetDefaultInRange< int32_t >(OpenALMaxStereoSourceCountKey, DefaultOpenALMaxStereoSourceCount, MinOpenALMaxStereoSourceCount, MaxOpenALMaxStereoSourceCount);

		/* Check for required extensions. */
		if ( !OpenAL::installExtensionLoopback() || !OpenAL::installExtensionThreadLocalContext() )
		{
			return false;
		}

		m_playbackFrequency = m_audioManager.frequencyPlayback();

		/* Step 1: Create the loopback device (no hardware output). */
		m_loopbackDevice.reset(OpenAL::alcLoopbackOpenDeviceSOFT(nullptr));

		if ( m_loopbackDevice == nullptr )
		{
			Tracer::error(ClassId, "Failed to open loopback device !");

			return false;
		}

		/* Determine channel layout from the unified output mode setting. */
		const auto outputMode = settings.getOrSetDefault< std::string >(AudioOutputModeKey, DefaultAudioOutputMode);
		ALCenum channelFormat = ALC_STEREO_SOFT;

		if ( outputMode == "Surround51" )
		{
			if ( OpenAL::alcIsRenderFormatSupportedSOFT(m_loopbackDevice.get(), static_cast< ALCsizei >(m_playbackFrequency.hz()), ALC_5POINT1_SOFT, ALC_SHORT_SOFT) != ALC_FALSE )
			{
				channelFormat = ALC_5POINT1_SOFT;
				m_channelCount = 6;

				Tracer::success(ClassId, "5.1 surround loopback format supported, using 6 channels.");
			}
			else
			{
				Tracer::warning(ClassId, "5.1 surround loopback format not supported, falling back to stereo.");
			}
		}

		/* Check if the chosen render format is supported. */
		if ( OpenAL::alcIsRenderFormatSupportedSOFT(m_loopbackDevice.get(), static_cast< ALCsizei >(m_playbackFrequency.hz()), channelFormat, ALC_SHORT_SOFT) == ALC_FALSE )
		{
			TraceError{ClassId} << "Loopback render format (" << m_channelCount << "ch, int16, " << m_playbackFrequency.hz() << "Hz) not supported !";

			this->releaseContexts();

			return false;
		}

		TraceInfo{ClassId} << "Audio recorder using " << m_channelCount << " channel(s).";

		/* Create the game context on the loopback device. */
		const std::array loopbackAttrs{
			ALC_FORMAT_CHANNELS_SOFT, static_cast< int >(channelFormat),
			ALC_FORMAT_TYPE_SOFT, ALC_SHORT_SOFT,
			ALC_FREQUENCY, static_cast< int >(m_playbackFrequency.hz()),
			ALC_MONO_SOURCES, monoSources,
			ALC_STEREO_SOURCES, stereoSources,
			ALC_MAX_AUXILIARY_SENDS, static_cast< int >(OpenAL::RequestedAuxiliarySends),
			0
		};

		m_gameContext.reset(alcCreateContext(m_loopbackDevice.get(), loopbackAttrs.data()));

		if ( m_gameContext == nullptr )
		{
			Tracer::error(ClassId, "Failed to create loopback context !");

			this->releaseContexts();

			return false;
		}

		/* Set the loopback context as the global current context so that
		 * ALL threads (main, resource workers, event loops) create OpenAL
		 * objects on the loopback device.  The render thread overrides this
		 * with its own thread-local playback context. */
		m_previousGlobalContext = alcGetCurrentContext();
		alcMakeContextCurrent(m_gameContext.get());

		/* Step 2: Open the real output device for speaker passthrough. */
		m_playbackDevice.reset(alcOpenDevice(outputDeviceName.c_str()));

		if ( m_playbackDevice == nullptr )
		{
			Tracer::error(ClassId, "Failed to open real output device for loopback passthrough !");

			/* NOTE: The previous global context (the audio manager's) is made current again: this branch used to destroy
			 * the game context while current, leaving the whole audio without a context. */
			this->releaseContexts();

			return false;
		}

		/* Resolve the output mode for the passthrough playback context. */
		ALCint playbackOutputMode = ALC_ANY_SOFT;

		if ( outputMode == "Surround51" )
		{
			playbackOutputMode = ALC_SURROUND_5_1_SOFT;
		}
		else if ( outputMode == "Stereo" )
		{
			playbackOutputMode = ALC_STEREO_BASIC_SOFT;
		}

		const std::array playbackAttrs{
			ALC_FREQUENCY, static_cast< int >(m_playbackFrequency.hz()),
			ALC_MONO_SOURCES, 1,
			ALC_STEREO_SOURCES, 1,
			ALC_OUTPUT_MODE_SOFT, static_cast< int >(playbackOutputMode),
			0
		};

		m_playbackContext.reset(alcCreateContext(m_playbackDevice.get(), playbackAttrs.data()));

		if ( m_playbackContext == nullptr )
		{
			Tracer::error(ClassId, "Failed to create playback context for loopback passthrough !");

			this->releaseContexts();

			return false;
		}

		/* Start the render thread for audio passthrough (stopped through its stop token). */
		if ( !m_renderThread.start([this] (const std::stop_token & stopToken) { this->renderThreadFunc(stopToken); }) )
		{
			/* The system refused the thread (Base::Thread traced why): the recorder is refused (owner policy,
			 * 2026-10-07). onTerminate() releases everything created above and restores the previous context. */
			Tracer::error(ClassId, "Unable to start the loopback render thread: the audio recorder is disabled !");

			static_cast< void >(this->onTerminate());

			return false;
		}

		Tracer::success(ClassId, "ALC_SOFT_loopback available, using loopback passthrough.");

		return true;
	}

	Recorder::~Recorder ()
	{
		/* NOTE: Only when something is still held: a second onTerminate() would make a null context current. */
		if ( m_renderThread.joinable() || m_loopbackDevice != nullptr || m_playbackDevice != nullptr || m_gameContext != nullptr || m_playbackContext != nullptr )
		{
			static_cast< void >(this->onTerminate());
		}
	}

	bool
	Recorder::onTerminate () noexcept
	{
		/* NOTE: No stopRecording() here: the render thread owns the file and completes it by itself when it is asked
		 * to stop (the end of renderThreadFunc()). */

		/* Stop the render thread (its stop token: Ave Robustus II, the std::jthread model). */
		m_renderThread.requestStop();

		if ( m_renderThread.joinable() )
		{
			m_renderThread.join();
		}

		this->releaseContexts();

		Tracer::success(ClassId, "Loopback passthrough terminated.");

		return true;
	}

	void
	Recorder::releaseContexts () noexcept
	{
		/* NOTE: The previous global context (the audio manager's) first, when the game context is the current one: a
		 * current context cannot be destroyed, and the audio must keep a context. */
		if ( m_gameContext != nullptr && alcGetCurrentContext() == m_gameContext.get() )
		{
			alcMakeContextCurrent(m_previousGlobalContext);
		}

		m_previousGlobalContext = nullptr;

		/* NOTE: Each context before its device (the holders' order too). */
		m_playbackContext.reset();
		m_playbackDevice.reset();
		m_gameContext.reset();
		m_loopbackDevice.reset();
	}

	void
	Recorder::renderThreadFunc (const std::stop_token & stopToken) noexcept
	{
		/* Set the playback context on this thread. */
		OpenAL::alcSetThreadContext(m_playbackContext.get());

		constexpr ALsizei ChunkSamples = 1024;
		constexpr size_t NumBuffers = 4;

		const auto frequency = static_cast< ALsizei >(m_playbackFrequency.hz());
		const ALenum alFormat = (m_channelCount == 6) ? alGetEnumValue("AL_FORMAT_51CHN16") : AL_FORMAT_STEREO16;

		/* Create a streaming source and buffers on the playback context. */
		ALuint source = 0;
		alGenSources(1, &source);

		std::array< ALuint, NumBuffers > buffers{};
		alGenBuffers(static_cast< ALsizei >(NumBuffers), buffers.data());

		/* Temporary render buffer (channels * int16). */
		std::vector< int16_t > renderBuf(static_cast< size_t >(ChunkSamples) * m_channelCount);

		/* Prime all buffers. */
		for ( auto buf : buffers )
		{
			OpenAL::alcRenderSamplesSOFT(m_loopbackDevice.get(), renderBuf.data(), ChunkSamples);

			alBufferData(buf, alFormat, renderBuf.data(),
				static_cast< ALsizei >(renderBuf.size() * sizeof(int16_t)), frequency);

			alSourceQueueBuffers(source, 1, &buf);
		}

		alSourcePlay(source);

		/* NOTE: The recording file lives here: opened, written, completed and closed by this thread alone. */
		RecordingFile recordingFile{};

		while ( !stopToken.stop_requested() )
		{
			this->serveRecordingRequest(recordingFile);

			ALint processed = 0;
			alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);

			while ( processed > 0 )
			{
				ALuint buf = 0;
				alSourceUnqueueBuffers(source, 1, &buf);

				/* Render from the loopback device. */
				OpenAL::alcRenderSamplesSOFT(m_loopbackDevice.get(), renderBuf.data(), ChunkSamples);

				/* If recording, write samples to the file (this thread's own). */
				this->writeRecordingChunk(recordingFile, reinterpret_cast< const char * >(renderBuf.data()), renderBuf.size() * sizeof(int16_t));

				/* Re-fill the buffer and queue it back. */
				alBufferData(buf, alFormat, renderBuf.data(),
					static_cast< ALsizei >(renderBuf.size() * sizeof(int16_t)), frequency);

				alSourceQueueBuffers(source, 1, &buf);

				processed--;
			}

			/* Ensure the source keeps playing (may stop if we lag). */
			ALint state = 0;
			alGetSourcei(source, AL_SOURCE_STATE, &state);

			if ( state != AL_PLAYING )
			{
				alSourcePlay(source);
			}

			std::this_thread::sleep_for(std::chrono::milliseconds{5});
		}

		/* NOTE: A recording still running is completed here, and a request not taken yet is answered (refused): nobody
		 * waits on a thread that is gone. */
		{
			const auto wasOpen = recordingFile.stream.is_open();
			const auto savedBytes = recordingFile.byteCount;
			const auto closed = wasOpen && closeRecordingFile(recordingFile);

			this->publishRecordingState(RecordingState::Idle, closed, closed ? savedBytes : 0);
		}

		/* Drain: stop the source and delete resources. */
		alSourceStop(source);

		ALint queued = 0;
		alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);

		while ( queued > 0 )
		{
			ALuint buf = 0;
			alSourceUnqueueBuffers(source, 1, &buf);
			queued--;
		}

		alDeleteSources(1, &source);
		alDeleteBuffers(static_cast< ALsizei >(NumBuffers), buffers.data());

		/* Unset thread-local context. */
		OpenAL::alcSetThreadContext(nullptr);

		Tracer::success(ClassId, "[THREAD] Audio render thread terminated successfully !");
	}

	bool
	Recorder::startRecording (const std::filesystem::path & outputPath) noexcept
	{
		if ( !this->usable() )
		{
			Tracer::warning(ClassId, "The audio recorder was not initialized.");

			return false;
		}

		Base::MutexLock lock{m_recordingAccess};

		if ( m_recordingState != RecordingState::Idle )
		{
			Tracer::warning(ClassId, "Audio recording is already in progress !");

			return false;
		}

		m_requestedPath = outputPath;
		m_requestSucceeded = false;
		m_recordingState = RecordingState::StartRequested;

		const auto answered = m_recordingChanged.waitFor(lock, RequestTimeout, [this] () EMEN_REQUIRES(m_recordingAccess) {
			return m_recordingState == RecordingState::Recording || m_recordingState == RecordingState::Idle;
		});

		if ( !answered )
		{
			/* NOTE: A request the render thread has not taken is withdrawn. One it is serving stays its own: it
			 * publishes the state it reaches, and isRecording() says it. */
			if ( m_recordingState == RecordingState::StartRequested )
			{
				m_recordingState = RecordingState::Idle;
			}

			TraceError{ClassId} << "The render thread did not answer the recording request for " << outputPath << " in time !";

			return false;
		}

		if ( !m_requestSucceeded )
		{
			return false;
		}

		TraceSuccess{ClassId} << "Audio recording started -> " << outputPath;

		return true;
	}

	bool
	Recorder::stopRecording () noexcept
	{
		Base::MutexLock lock{m_recordingAccess};

		if ( m_recordingState != RecordingState::Recording )
		{
			return false;
		}

		m_requestSucceeded = false;
		m_recordingState = RecordingState::StopRequested;

		const auto answered = m_recordingChanged.waitFor(lock, RequestTimeout, [this] () EMEN_REQUIRES(m_recordingAccess) {
			return m_recordingState == RecordingState::Idle;
		});

		if ( !answered )
		{
			/* NOTE: Nothing is given up: the request stays posted and the render thread, which owns the file,
			 * completes it. */
			Tracer::error(ClassId, "The render thread did not complete the audio recording in time !");

			return false;
		}

		if ( !m_requestSucceeded )
		{
			return false;
		}

		TraceSuccess{ClassId} << "Audio recording saved : " << m_savedByteCount / (m_channelCount * sizeof(int16_t)) << " samples.";

		return true;
	}

	bool
	Recorder::isRecording () const noexcept
	{
		const Base::MutexLock lock{m_recordingAccess};

		return m_recordingState != RecordingState::Idle;
	}

	void
	Recorder::publishRecordingState (RecordingState state, bool succeeded, uint32_t savedBytes) noexcept
	{
		/* NOTE: Notified with the mutex held: a waiter cannot return, and its owner cannot go, between the state's
		 * change and the notification. */
		const Base::MutexLock lock{m_recordingAccess};

		m_recordingState = state;
		m_requestSucceeded = succeeded;
		m_savedByteCount = savedBytes;

		m_recordingChanged.notifyAll();
	}

	void
	Recorder::serveRecordingRequest (RecordingFile & file) noexcept
	{
		auto request = RecordingState::Idle;
		std::filesystem::path path{};

		{
			const Base::MutexLock lock{m_recordingAccess};

			request = m_recordingState;

			switch ( request )
			{
				case RecordingState::StartRequested :
					path = m_requestedPath;
					m_recordingState = RecordingState::Starting;
					break;

				case RecordingState::StopRequested :
					m_recordingState = RecordingState::Stopping;
					break;

				case RecordingState::Idle :
				case RecordingState::Starting :
				case RecordingState::Recording :
				case RecordingState::Stopping :
					return;
			}
		}

		/* NOTE: The file work, without the mutex. */
		if ( request == RecordingState::StartRequested )
		{
			const auto opened = this->openRecordingFile(file, path);

			this->publishRecordingState(opened ? RecordingState::Recording : RecordingState::Idle, opened, 0);

			return;
		}

		const auto savedBytes = file.byteCount;
		const auto closed = closeRecordingFile(file);

		this->publishRecordingState(RecordingState::Idle, closed, closed ? savedBytes : 0);
	}

	void
	Recorder::writeRecordingChunk (RecordingFile & file, const char * data, size_t bytes) noexcept
	{
		if ( !file.stream.is_open() )
		{
			return;
		}

		/* NOTE: The two size fields of a WAV header are 32-bit, and the first one counts 36 bytes of header more. */
		constexpr uint64_t MaxDataBytes{std::numeric_limits< uint32_t >::max() - 36U};

		const auto full = static_cast< uint64_t >(file.byteCount) + bytes > MaxDataBytes;

		if ( !full )
		{
			file.stream.write(data, static_cast< std::streamsize >(bytes));

			if ( file.stream.good() )
			{
				file.byteCount += static_cast< uint32_t >(bytes);

				return;
			}
		}

		TraceError{ClassId} <<
			( full ? "The WAV file is full (4 GiB)" : "Unable to write the audio samples" ) <<
			": the recording to " << file.path << " ends here !";

		/* NOTE: What was written is kept: the header is completed with the bytes that reached the file. The state
		 * goes back to Idle unless a stop request was posted meanwhile: that one finds the file closed at the next
		 * turn and is answered as failed. */
		const auto savedBytes = file.byteCount;
		const auto closed = closeRecordingFile(file);

		const Base::MutexLock lock{m_recordingAccess};

		if ( m_recordingState == RecordingState::Recording )
		{
			m_recordingState = RecordingState::Idle;
			m_requestSucceeded = false;
			m_savedByteCount = closed ? savedBytes : 0;

			m_recordingChanged.notifyAll();
		}
	}

	bool
	Recorder::openRecordingFile (RecordingFile & file, const std::filesystem::path & path) const noexcept
	{
		file.path = path;
		file.byteCount = 0;
		file.stream.clear();
		file.stream.open(path, std::ios::binary | std::ios::out | std::ios::trunc);

		if ( !file.stream.is_open() )
		{
			TraceError{ClassId} << "Unable to open output file " << path << " for writing !";

			return false;
		}

		/* Write placeholder WAV header. */
		if ( !this->writeWAVHeader(file.stream, 0) )
		{
			TraceError{ClassId} << "Unable to write WAV header to " << path << " !";

			file.stream.close();

			return false;
		}

		return true;
	}

	bool
	Recorder::closeRecordingFile (RecordingFile & file) noexcept
	{
		if ( !file.stream.is_open() )
		{
			return false;
		}

		/* NOTE: A failed write left the stream in a failed state: cleared, so the header can still be completed. */
		file.stream.clear();

		/* Patch the header sizes: the RIFF chunk size (byte 4, the file size - 8) and the data chunk size (byte 40). */
		const uint32_t riffSize = file.byteCount + 36U;

		/* NOTE: A WAV size field is little-endian: written byte by byte, whatever the machine's order. */
		const auto writeSize = [&file] (std::streamoff position, uint32_t value) {
			const std::array< char, 4 > bytes{
				static_cast< char >(value & 0xFFU),
				static_cast< char >((value >> 8U) & 0xFFU),
				static_cast< char >((value >> 16U) & 0xFFU),
				static_cast< char >((value >> 24U) & 0xFFU)
			};

			file.stream.seekp(position, std::ios::beg);
			file.stream.write(bytes.data(), static_cast< std::streamsize >(bytes.size()));
		};

		writeSize(4, riffSize);
		writeSize(40, file.byteCount);

		const auto written = file.stream.good();

		file.stream.close();

		if ( !written || file.stream.fail() )
		{
			TraceError{ClassId} << "Unable to complete the WAV header of " << file.path << " !";

			return false;
		}

		return true;
	}

	bool
	Recorder::writeWAVHeader (std::ostream & stream, uint32_t dataSize) const noexcept
	{
		/* Standard WAV Header (44 bytes for 16-bit PCM). */
		const uint32_t riffSize = dataSize + 36;
		const uint16_t audioFormat = 1; /* PCM */
		const uint16_t numChannels = m_channelCount;
		const auto sampleRate = m_playbackFrequency.hz();
		const uint32_t byteRate = sampleRate * numChannels * static_cast< uint32_t >(sizeof(int16_t)); /* The WAV header field is 32-bit. */
		const uint16_t blockAlign = numChannels * sizeof(int16_t);
		const uint16_t bitsPerSample = 16;

		stream.write("RIFF", 4);
		stream.write(reinterpret_cast< const char * >(&riffSize), 4);
		stream.write("WAVE", 4);

		stream.write("fmt ", 4);
		const uint32_t fmtChunkSize = 16;
		stream.write(reinterpret_cast< const char * >(&fmtChunkSize), 4);
		stream.write(reinterpret_cast< const char * >(&audioFormat), 2);
		stream.write(reinterpret_cast< const char * >(&numChannels), 2);
		stream.write(reinterpret_cast< const char * >(&sampleRate), 4);
		stream.write(reinterpret_cast< const char * >(&byteRate), 4);
		stream.write(reinterpret_cast< const char * >(&blockAlign), 2);
		stream.write(reinterpret_cast< const char * >(&bitsPerSample), 2);

		stream.write("data", 4);
		stream.write(reinterpret_cast< const char * >(&dataSize), 4);

		return stream.good();
	}
}

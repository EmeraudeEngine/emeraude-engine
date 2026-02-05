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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "Recorder.hpp"

/* STL inclusions. */
#include <chrono>

/* Local inclusions. */
#include "Libs/WaveFactory/Wave.hpp"
#include "Libs/WaveFactory/FileIO.hpp"
#include "Manager.hpp"
#include "Utility.hpp"
#include "PrimaryServices.hpp"
#include "SettingKeys.hpp"

namespace EmEn::Audio
{
	using namespace Libs;

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

		if ( !settings.getOrSetDefault< bool >(AudioRecorderEnableKey, DefaultAudioRecorderEnable) )
		{
			return false;
		}

		const auto & outputDeviceName = m_audioManager.selectedOutputDeviceName();
		const auto monoSources = settings.getOrSetDefault< int32_t >(OpenALMaxMonoSourceCountKey, DefaultOpenALMaxMonoSourceCount);
		const auto stereoSources = settings.getOrSetDefault< int32_t >(OpenALMaxStereoSourceCountKey, DefaultOpenALMaxStereoSourceCount);

		/* Check for required extensions. */
		if ( !OpenAL::installExtensionLoopback() || !OpenAL::installExtensionThreadLocalContext() )
		{
			return false;
		}

		m_playbackFrequency = m_audioManager.frequencyPlayback();

		/* Step 1: Create the loopback device (no hardware output). */
		m_loopbackDevice = OpenAL::alcLoopbackOpenDeviceSOFT(nullptr);

		if ( m_loopbackDevice == nullptr )
		{
			Tracer::error(ClassId, "Failed to open loopback device !");

			return false;
		}

		/* Check if the render format is supported. */
		if ( OpenAL::alcIsRenderFormatSupportedSOFT(m_loopbackDevice, static_cast< ALCsizei >(m_playbackFrequency), ALC_STEREO_SOFT, ALC_SHORT_SOFT) == ALC_FALSE )
		{
			Tracer::error(ClassId, "Loopback render format (Stereo, int16, 48kHz) not supported !");

			alcCloseDevice(m_loopbackDevice);
			m_loopbackDevice = nullptr;

			return false;
		}

		/* Create the game context on the loopback device. */
		const std::array loopbackAttrs{
			ALC_FORMAT_CHANNELS_SOFT, static_cast< int >(ALC_STEREO_SOFT),
			ALC_FORMAT_TYPE_SOFT, static_cast< int >(ALC_SHORT_SOFT),
			ALC_FREQUENCY, static_cast< int >(m_playbackFrequency),
			ALC_MONO_SOURCES, monoSources,
			ALC_STEREO_SOURCES, stereoSources,
			0
		};

		m_gameContext = alcCreateContext(m_loopbackDevice, loopbackAttrs.data());

		if ( m_gameContext == nullptr )
		{
			Tracer::error(ClassId, "Failed to create loopback context !");

			alcCloseDevice(m_loopbackDevice);
			m_loopbackDevice = nullptr;

			return false;
		}

		/* Set the loopback context as the global current context so that
		 * ALL threads (main, resource workers, event loops) create OpenAL
		 * objects on the loopback device.  The render thread overrides this
		 * with its own thread-local playback context. */
		m_previousGlobalContext = alcGetCurrentContext();
		alcMakeContextCurrent(m_gameContext);

		/* Step 2: Open the real output device for speaker passthrough. */
		m_playbackDevice = alcOpenDevice(outputDeviceName.c_str());

		if ( m_playbackDevice == nullptr )
		{
			Tracer::error(ClassId, "Failed to open real output device for loopback passthrough !");

			alcDestroyContext(m_gameContext);
			m_gameContext = nullptr;
			alcCloseDevice(m_loopbackDevice);
			m_loopbackDevice = nullptr;

			return false;
		}

		const std::array playbackAttrs{
			ALC_FREQUENCY, static_cast< int >(m_playbackFrequency),
			ALC_MONO_SOURCES, 1,
			ALC_STEREO_SOURCES, 1,
			0
		};

		m_playbackContext = alcCreateContext(m_playbackDevice, playbackAttrs.data());

		if ( m_playbackContext == nullptr )
		{
			Tracer::error(ClassId, "Failed to create playback context for loopback passthrough !");

			alcCloseDevice(m_playbackDevice);
			m_playbackDevice = nullptr;
			alcDestroyContext(m_gameContext);
			m_gameContext = nullptr;
			alcCloseDevice(m_loopbackDevice);
			m_loopbackDevice = nullptr;

			return false;
		}

		/* Start the render thread for audio passthrough. */
		m_renderRunning = true;

		m_renderThread = std::thread{[this] {
			this->renderThreadFunc();
		}};

		Tracer::success(ClassId, "ALC_SOFT_loopback available, using loopback passthrough.");

		return true;
	}

	bool
	Recorder::onTerminate () noexcept
	{
		/* Stop any active recording. */
		if ( m_recording )
		{
			this->stopRecording();
		}

		/* Stop the render thread. */
		m_renderRunning = false;

		if ( m_renderThread.joinable() )
		{
			m_renderThread.join();
		}

		/* Restore the previous global context before destroying loopback resources. */
		alcMakeContextCurrent(m_previousGlobalContext);
		m_previousGlobalContext = nullptr;

		/* Destroy playback context and device. */
		if ( m_playbackContext != nullptr )
		{
			alcDestroyContext(m_playbackContext);
			m_playbackContext = nullptr;
		}

		if ( m_playbackDevice != nullptr )
		{
			alcCloseDevice(m_playbackDevice);
			m_playbackDevice = nullptr;
		}

		/* Destroy loopback context and device. */
		if ( m_gameContext != nullptr )
		{
			alcDestroyContext(m_gameContext);
			m_gameContext = nullptr;
		}

		if ( m_loopbackDevice != nullptr )
		{
			alcCloseDevice(m_loopbackDevice);
			m_loopbackDevice = nullptr;
		}

		Tracer::success(ClassId, "Loopback passthrough terminated.");

		return true;
	}

	void
	Recorder::renderThreadFunc () noexcept
	{
		/* Set the playback context on this thread. */
		OpenAL::alcSetThreadContext(m_playbackContext);

		constexpr ALsizei ChunkSamples = 1024;
		constexpr size_t NumBuffers = 4;
		constexpr size_t StereoChannels = 2;

		const auto frequency = static_cast< ALsizei >(m_playbackFrequency);

		/* Create a streaming source and buffers on the playback context. */
		ALuint source = 0;
		alGenSources(1, &source);

		std::array< ALuint, NumBuffers > buffers{};
		alGenBuffers(static_cast< ALsizei >(NumBuffers), buffers.data());

		/* Temporary render buffer (stereo int16). */
		std::vector< int16_t > renderBuf(static_cast< size_t >(ChunkSamples) * StereoChannels);

		/* Prime all buffers. */
		for ( auto buf : buffers )
		{
			OpenAL::alcRenderSamplesSOFT(m_loopbackDevice, renderBuf.data(), ChunkSamples);

			alBufferData(buf, AL_FORMAT_STEREO16, renderBuf.data(),
				static_cast< ALsizei >(renderBuf.size() * sizeof(int16_t)), frequency);

			alSourceQueueBuffers(source, 1, &buf);
		}

		alSourcePlay(source);

		while ( m_renderRunning )
		{
			ALint processed = 0;
			alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);

			while ( processed > 0 )
			{
				ALuint buf = 0;
				alSourceUnqueueBuffers(source, 1, &buf);

				/* Render from the loopback device. */
				OpenAL::alcRenderSamplesSOFT(m_loopbackDevice, renderBuf.data(), ChunkSamples);

				/* If recording, accumulate samples. */
				if ( m_recording.load(std::memory_order_relaxed) )
				{
					const std::lock_guard< std::mutex > lock{m_recordMutex};

					m_recordSamples.insert(m_recordSamples.end(), renderBuf.begin(), renderBuf.end());
				}

				/* Re-fill the buffer and queue it back. */
				alBufferData(buf, AL_FORMAT_STEREO16, renderBuf.data(),
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

		if ( m_recording )
		{
			Tracer::warning(ClassId, "Audio recording is already in progress !");

			return false;
		}

		{
			const std::lock_guard< std::mutex > lock{m_recordMutex};
			m_recordSamples.clear();
		}

		m_outputPath = outputPath;
		m_recording = true;

		TraceSuccess{ClassId} << "Audio recording started -> " << m_outputPath;

		return true;
	}

	bool
	Recorder::stopRecording () noexcept
	{
		if ( !m_recording )
		{
			return false;
		}

		m_recording = false;

		/* Grab the accumulated samples. */
		std::vector< int16_t > samples;

		{
			const std::lock_guard< std::mutex > lock{m_recordMutex};
			samples = std::move(m_recordSamples);
		}

		if ( samples.empty() )
		{
			Tracer::warning(ClassId, "Audio recording stopped but no samples were captured.");

			return false;
		}

		/* Write the WAV file using WaveFactory. */
		WaveFactory::Wave< int16_t > wave;

		if ( !wave.initialize(samples, WaveFactory::Channels::Stereo, m_playbackFrequency) )
		{
			Tracer::error(ClassId, "Unable to initialize wave data for audio recording !");

			return false;
		}

		if ( !WaveFactory::FileIO::write(wave, m_outputPath) )
		{
			TraceError{ClassId} << "Unable to save audio recording to " << m_outputPath << " !";

			return false;
		}

		TraceSuccess{ClassId} << "Audio recording saved : " << samples.size() / 2 << " samples -> " << m_outputPath;

		return true;
	}
}

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

		if ( !settings.getOrSetDefault< bool >(RushMakerEnableAudioKey, DefaultRushMakerEnabled) )
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

		/* Determine channel layout from the unified output mode setting. */
		const auto outputMode = settings.getOrSetDefault< std::string >(AudioOutputModeKey, DefaultAudioOutputMode);
		ALCenum channelFormat = ALC_STEREO_SOFT;

		if ( outputMode == "Surround51" )
		{
			if ( OpenAL::alcIsRenderFormatSupportedSOFT(m_loopbackDevice, static_cast< ALCsizei >(m_playbackFrequency), ALC_5POINT1_SOFT, ALC_SHORT_SOFT) != ALC_FALSE )
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
		if ( OpenAL::alcIsRenderFormatSupportedSOFT(m_loopbackDevice, static_cast< ALCsizei >(m_playbackFrequency), channelFormat, ALC_SHORT_SOFT) == ALC_FALSE )
		{
			TraceError{ClassId} << "Loopback render format (" << m_channelCount << "ch, int16, " << static_cast< int >(m_playbackFrequency) << "Hz) not supported !";

			alcCloseDevice(m_loopbackDevice);
			m_loopbackDevice = nullptr;

			return false;
		}

		TraceInfo{ClassId} << "Audio recorder using " << m_channelCount << " channel(s).";

		/* Create the game context on the loopback device. */
		const std::array loopbackAttrs{
			ALC_FORMAT_CHANNELS_SOFT, static_cast< int >(channelFormat),
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
			ALC_FREQUENCY, static_cast< int >(m_playbackFrequency),
			ALC_MONO_SOURCES, 1,
			ALC_STEREO_SOURCES, 1,
			ALC_OUTPUT_MODE_SOFT, static_cast< int >(playbackOutputMode),
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

		const auto frequency = static_cast< ALsizei >(m_playbackFrequency);
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
			OpenAL::alcRenderSamplesSOFT(m_loopbackDevice, renderBuf.data(), ChunkSamples);

			alBufferData(buf, alFormat, renderBuf.data(),
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

				/* If recording, write samples to stream. */
				if ( m_recording.load(std::memory_order_relaxed) )
				{
					if ( m_outputFileStream.is_open() )
					{
						const auto bytes = renderBuf.size() * sizeof(int16_t);
						m_outputFileStream.write(reinterpret_cast< const char * >(renderBuf.data()), static_cast< std::streamsize >(bytes));
						m_streamByteCount += static_cast< uint32_t >(bytes);
					}
				}

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

		m_outputPath = outputPath;
		m_outputFileStream.open(m_outputPath, std::ios::binary | std::ios::out | std::ios::trunc);

		if ( !m_outputFileStream.is_open() )
		{
			TraceError{ClassId} << "Unable to open output file " << m_outputPath << " for writing !";

			return false;
		}

		/* Write placeholder WAV header. */
		if ( !this->writeWAVHeader(m_outputFileStream, 0) )
		{
			TraceError{ClassId} << "Unable to write WAV header to " << m_outputPath << " !";
			m_outputFileStream.close();

			return false;
		}

		/* Store the position of the data size field (Byte 40) for later patching. */
		m_dataSizePos = 40;
		m_streamByteCount = 0;

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

		if ( !m_outputFileStream.is_open() )
		{
			return false;
		}

		/* Patch the header sizes. */
		const auto totalFileSize = m_streamByteCount + 36; /* RIFF chunk size = file size - 8. */

		/* 1. Patch RIFF chunk size (Byte 4). */
		m_outputFileStream.seekp(4, std::ios::beg);
		m_outputFileStream.write(reinterpret_cast< const char * >(&totalFileSize), 4);

		/* 2. Patch data chunk size (Byte 40). */
		m_outputFileStream.seekp(m_dataSizePos, std::ios::beg);
		m_outputFileStream.write(reinterpret_cast< const char * >(&m_streamByteCount), 4);

		m_outputFileStream.close();

		TraceSuccess{ClassId} << "Audio recording saved : " << m_streamByteCount / (m_channelCount * sizeof(int16_t)) << " samples -> " << m_outputPath;

		return true;
	}

	bool
	Recorder::writeWAVHeader (std::ostream & stream, uint32_t dataSize) const noexcept
	{
		/* Standard WAV Header (44 bytes for 16-bit PCM). */
		const uint32_t riffSize = dataSize + 36;
		const uint16_t audioFormat = 1; /* PCM */
		const uint16_t numChannels = m_channelCount;
		const uint32_t sampleRate = static_cast< uint32_t >(m_playbackFrequency);
		const uint32_t byteRate = sampleRate * numChannels * sizeof(int16_t);
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

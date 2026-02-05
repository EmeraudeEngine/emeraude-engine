/*
 * src/Audio/ExternalInput.cpp
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

#include "ExternalInput.hpp"

/* Local inclusions. */
#include "Libs/WaveFactory/Wave.hpp"
#include "Libs/WaveFactory/FileIO.hpp"
#include "Manager.hpp"

namespace EmEn::Audio
{
	using namespace Libs;

	ExternalInput::ExternalInput (PrimaryServices & primaryServices, Manager & audioManager) noexcept
		: ServiceInterface{ClassId},
		m_primaryServices{primaryServices},
		m_audioManager{audioManager}
	{

	}

	bool
	ExternalInput::selectDevice () noexcept
	{
		/* NOTE: Check for the audio device enumeration. */
		if ( alcIsExtensionPresent(nullptr, "ALC_EXT_CAPTURE") == ALC_FALSE )
		{
			Tracer::error(ClassId, "OpenAL extension 'ALC_EXT_CAPTURE' not available!");

			return false;
		}

		m_availableDevices.clear();

		const auto * devices = alcGetString(nullptr, ALC_CAPTURE_DEVICE_SPECIFIER);

		if ( devices == nullptr )
		{
			Tracer::error(ClassId, "There is no capture audio devices!");

			return false;
		}

		while ( *devices != '\0' )
		{
			m_availableDevices.emplace_back(devices);

			devices += m_availableDevices.back().length() + 1;
		}

		if ( m_availableDevices.empty() )
		{
			return false;
		}

		const auto * defaultDeviceName = alcGetString(nullptr, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);

		if ( m_selectedDeviceName.empty() || m_selectedDeviceName == DefaultAudioCaptureDeviceName )
		{
			m_selectedDeviceName.assign(defaultDeviceName);
		}
		else
		{
			/* NOTE: Check if the previous selected device from settings is still available. */
			const auto deviceIt = std::ranges::find_if(m_availableDevices, [this] (const std::string & deviceName) {
				return deviceName == m_selectedDeviceName;
			});

			if ( deviceIt == m_availableDevices.cend() )
			{
				TraceWarning{ClassId} << "The selected input audio device '" << m_selectedDeviceName << "' is not available anymore!";

				m_selectedDeviceName.assign(defaultDeviceName);
			}
		}

		s_audioCaptureAvailable = true;

		return true;
	}

	bool
	ExternalInput::onInitialize () noexcept
	{
		auto & settings = m_primaryServices.settings();

		if ( !settings.getOrSetDefault< bool >(AudioCaptureEnableKey, DefaultAudioCaptureEnable) )
		{
			return false;
		}

		const auto & arguments = m_primaryServices.arguments();

		m_showInformation =
			settings.getOrSetDefault< bool >(AudioShowInformationKey, DefaultAudioShowInformation) ||
			arguments.isSwitchPresent("--show-all-infos") ||
			arguments.isSwitchPresent("--show-audio-infos");

		/* First, read setting for a desired input audio device. */
		m_selectedDeviceName = settings.getOrSetDefault< std::string >(AudioCaptureDeviceNameKey, DefaultAudioCaptureDeviceName);

		/* Then, check the audio capture system. */
		if ( !this->selectDevice() )
		{
			Tracer::error(ClassId, "There is no audio input device on the system!");

			return false;
		}

		if ( m_showInformation )
		{
			std::cout << "[OpenAL] Capture audio devices:" "\n";

			for ( const auto & deviceName : m_availableDevices )
			{
				std::cout << " - " << deviceName << '\n';
			}

			std::cout << "Default: " << m_selectedDeviceName << '\n';
		}

		/* Checks configuration file */
		const auto bufferSize = settings.getOrSetDefault< int32_t >(AudioCaptureBufferSizeKey, DefaultAudioCaptureBufferSize);
		m_frequency = WaveFactory::toFrequency(settings.getOrSetDefault< int32_t >(AudioCaptureFrequencyKey, DefaultAudioCaptureFrequency));

		if ( m_frequency == WaveFactory::Frequency::Invalid )
		{
			TraceWarning{ClassId} <<
				"Invalid recorder frequency in settings file! "
				"Leaving to default " << DefaultAudioCaptureFrequency << " Hz.";

			m_frequency = WaveFactory::Frequency::PCM48000Hz;
		}

		/* NOTE: Opening the output input device. */
		m_device = alcCaptureOpenDevice(
			m_selectedDeviceName.c_str(),
			static_cast< ALuint >(m_frequency),
			AL_FORMAT_MONO16,
			bufferSize * 1024
		);

		if ( m_device == nullptr )
		{
			TraceError{ClassId} << "Unable to open the input audio device '" << m_selectedDeviceName << "' !";

			return false;
		}

		TraceSuccess{ClassId} << "The input audio device '" << alcGetString(m_device, ALC_CAPTURE_DEVICE_SPECIFIER) << "' selected !";

		return true;
	}

	bool
	ExternalInput::onTerminate () noexcept
	{
		auto & settings = m_primaryServices.settings();

		if ( m_process.joinable() )
		{
			m_process.join();
		}

		/* NOTE: Release the output audio device. */
		if ( m_device != nullptr )
		{
			if ( alcCaptureCloseDevice(m_device) == ALC_TRUE )
			{
				TraceSuccess{ClassId} << "The input audio device '" << m_selectedDeviceName << "' closed !";

				settings.set< std::string >(AudioCaptureDeviceNameKey, m_selectedDeviceName);
			}
			else
			{
				TraceError{ClassId} << "Unable to close the input audio device '" << m_selectedDeviceName << "' !";
			}

			m_device = nullptr;
		}

		return true;
	}

	void
	ExternalInput::start () noexcept
	{
		if ( m_device == nullptr || m_isRecording )
		{
			return;
		}

		m_samples.clear();

		alcCaptureStart(m_device);

		m_isRecording = true;

		m_process = std::thread(&ExternalInput::recordingTask, this);
	}

	void
	ExternalInput::stop () noexcept
	{
		if ( m_device == nullptr || !m_isRecording )
		{
			return;
		}

		alcCaptureStop(m_device);

		m_isRecording = false;
	}

	bool
	ExternalInput::saveRecord (const std::filesystem::path & filepath) const noexcept
	{
		if ( m_isRecording )
		{
			Tracer::warning(ClassId, "The recorder is still running !");

			return false;
		}

		if ( m_samples.empty() )
		{
			Tracer::warning(ClassId, "There is no record to save !");

			return false;
		}

		WaveFactory::Wave< int16_t > wave;

		if ( !wave.initialize(m_samples, WaveFactory::Channels::Mono, m_frequency) )
		{
			Tracer::error(ClassId, "Unable to initialize wave data !");

			return false;
		}

		if ( !WaveFactory::FileIO::write(wave, filepath) )
		{
			Tracer::error(ClassId, "Unable to save the record to a file !");

			return false;
		}

		return true;
	}

	void
	ExternalInput::recordingTask () noexcept
	{
		while ( m_isRecording )
		{
			ALCint sampleCount = 0;

			alcGetIntegerv(m_device, ALC_CAPTURE_SAMPLES, 1, &sampleCount);

			if ( sampleCount > 0 )
			{
				const auto offset = m_samples.size();

				m_samples.resize(offset + static_cast< size_t >(sampleCount));

				alcCaptureSamples(m_device, &m_samples[offset], sampleCount);
			}
		}
	}
}

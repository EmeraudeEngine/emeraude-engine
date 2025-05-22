/*
 * src/Audio/ExternalInput.cpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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

/* STL inclusions. */
#include <cstring>
#include <iostream>

/* Local inclusions. */
#include "Libs/WaveFactory/Wave.hpp"
#include "Constants.hpp"
#include "PrimaryServices.hpp"
#include "Manager.hpp"

namespace EmEn::Audio
{
	using namespace EmEn::Libs;

	const size_t ExternalInput::ClassUID{getClassUID(ClassId)};

	ExternalInput::ExternalInput (PrimaryServices & primaryServices) noexcept
		: ServiceInterface(ClassId),
		m_primaryServices(primaryServices)
	{

	}

	ExternalInput::~ExternalInput ()
	{
		if ( m_process.joinable() )
		{
			m_process.join();
		}
	}

	size_t
	ExternalInput::classUID () const noexcept
	{
		return ClassUID;
	}

	bool
	ExternalInput::is (size_t classUID) const noexcept
	{
		return classUID == ClassUID;
	}

	bool
	ExternalInput::usable () const noexcept
	{
		return m_device != nullptr;
	}

	bool
	ExternalInput::onInitialize () noexcept
	{
		m_flags[ShowInformation] = m_primaryServices.settings().get< bool >(OpenALShowInformationKey, DefaultOpenALShowInformation);

		if ( !Manager::instance()->usable() )
		{
			Tracer::warning(ClassId, "Audio external input disabled at startup.");

			return true;
		}

		if ( alcIsExtensionPresent(nullptr, "ALC_EXT_CAPTURE") == AL_FALSE )
		{
			return false;
		}

		/* Checks configuration file */
		const auto frequency = WaveFactory::toFrequency(m_primaryServices.settings().get< int32_t >(RecorderFrequencyKey, DefaultRecorderFrequency));
		const auto bufferSize = m_primaryServices.settings().get< int32_t >(RecorderBufferSizeKey, DefaultRecorderBufferSize);

		if ( frequency == WaveFactory::Frequency::Invalid )
		{
			TraceWarning{ClassId} << "Invalid recorder frequency in configuration file ! Leaving to default (" << DefaultRecorderFrequency << " Hz).";
		}
		else
		{
			m_frequency = frequency;
		}

		if ( !this->queryDevices() )
		{
			Tracer::error(ClassId, "There is no capture device available !");

			return false;
		}

		m_device = alcCaptureOpenDevice(nullptr, static_cast< ALuint >(m_frequency), AL_FORMAT_MONO16, bufferSize * ComputerNumber1024< int >);

		if ( m_device == nullptr )
		{
			Tracer::error(ClassId, "Unable to open default capture device !");

			return false;
		}

		return true;
	}

	bool
	ExternalInput::onTerminate () noexcept
	{
		if ( m_device != nullptr )
		{
			alcCaptureCloseDevice(m_device);

			m_device = nullptr;
		}

		return true;
	}

	bool
	ExternalInput::isRecording () const noexcept
	{
		return m_flags[IsRecording];
	}

	void
	ExternalInput::start () noexcept
	{
		if ( !this->usable() )
		{
			Tracer::warning(ClassId, "The audio external input is unavailable !");

			return;
		}

		if ( m_flags[IsRecording] )
		{
			return;
		}

		m_samples.clear();

		alcCaptureStart(m_device);

		m_flags[IsRecording] = true;

		m_process = std::thread(&ExternalInput::process, this);
	}

	void
	ExternalInput::stop () noexcept
	{
		if ( !this->usable() )
		{
			Tracer::warning(ClassId, "The audio external input is unavailable !");

			return;
		}

		if ( !m_flags[IsRecording] )
		{
			return;
		}

		alcCaptureStop(m_device);

		m_flags[IsRecording] = false;
	}

	bool
	ExternalInput::saveRecord (const std::filesystem::path & filepath) const noexcept
	{
		if ( m_flags[IsRecording] )
		{
			Tracer::warning(ClassId, "The recorder is still running !");

			return false;
		}

		if ( m_samples.empty() )
		{
			Tracer::warning(ClassId, "There is no record to save !");

			return false;
		}

		WaveFactory::Wave< int16_t > file;

		if ( !file.initialize(m_samples, WaveFactory::Channels::Mono, m_frequency) )
		{
			Tracer::error(ClassId, "Unable to initialize wave data !");

			return false;
		}

		if ( !file.writeFile(filepath) )
		{
			Tracer::error(ClassId, "Unable to save the record to a file !");

			return false;
		}

		return true;
	}

	void
	ExternalInput::process () noexcept
	{
		while ( m_flags[IsRecording] )
		{
			ALCint sampleCount = 0;

			alcGetIntegerv(m_device, ALC_CAPTURE_SAMPLES, 1, &sampleCount);

			if ( sampleCount > 0 )
			{
				auto offset = m_samples.size();

				m_samples.resize(offset + static_cast< size_t >(sampleCount));

				alcCaptureSamples(m_device, &m_samples[offset], sampleCount);
			}
		}
	}

	bool
	ExternalInput::queryDevices () noexcept
	{
		if ( alcIsExtensionPresent(nullptr, "ALC_EXT_CAPTURE") == ALC_FALSE )
		{
			return false;
		}

		const auto * devices = alcGetString(nullptr, ALC_CAPTURE_DEVICE_SPECIFIER);

		if ( devices != nullptr )
		{
			while ( std::strlen(devices) > 0 )
			{
				m_availableCaptureDevices.emplace_back(devices);

				devices += std::strlen(devices) + 1;
			}

			if ( m_flags[ShowInformation] )
			{
				std::cout << "[OpenAL] Capture devices :" "\n";

				for ( auto & deviceName : m_availableCaptureDevices )
				{
					std::cout << " - " << deviceName << '\n';
				}

				std::cout << "[OpenAL] Default capture device : " << alcGetString(nullptr, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER) << "\n\n";
			}
		}

		return !m_availableCaptureDevices.empty();
	}
}

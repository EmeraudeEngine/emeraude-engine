/*
 * src/Audio/Manager.cpp
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

#include "Manager.hpp"

/* STL inclusions. */
#include <cstring>
#include <iostream>

/* Local inclusions. */
#include "Libs/Math/Base.hpp"
#include "Resources/Manager.hpp"
#include "PrimaryServices.hpp"
#include "SoundResource.hpp"
#include "Source.hpp"
#include "Utility.hpp"

namespace EmEn::Audio
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;

	const size_t Manager::ClassUID{getClassUID(ClassId)};
	Manager * Manager::s_instance{nullptr};
	bool Manager::s_audioDisabled{false};

	bool
	Manager::initializeSubServices () noexcept
	{
		/* Initialize the track mixer. */
		if ( m_trackMixer.initialize(m_subServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_trackMixer.name() << " service up !";
		}
		else
		{
			TraceWarning{ClassId} <<
				m_trackMixer.name() << " service failed to execute." "\n"
				"No music available !";
		}

		/* Initialize the audio external input. */
		if ( m_externalInput.initialize(m_subServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_externalInput.name() << " service up !";
		}
		else
		{
			TraceWarning{ClassId} <<
				m_externalInput.name() << " service failed to execute !" "\n"
				"No audio input available !";
		}

		return true;
	}

	bool
	Manager::onInitialize () noexcept
	{
		m_flags[ShowInformation] = m_primaryServices.settings().get< bool >(OpenALShowInformationKey, DefaultOpenALShowInformation);

		if ( m_primaryServices.arguments().get("--disable-audio").isPresent() || !m_primaryServices.settings().get< bool >(AudioEnableKey, DefaultAudioEnable) )
		{
			Tracer::warning(ClassId, "Audio manager disabled at startup.");

			m_flags[ServiceInitialized] = true;

			s_audioDisabled = true;
			m_flags[AudioDisabledAtStartup] = true;

			return true;
		}

		/* Sets the frequency playback. */
		m_playbackFrequency = WaveFactory::toFrequency(m_primaryServices.settings().get< int32_t >(AudioPlaybackFrequencyKey, DefaultAudioPlaybackFrequency));

		/* Sets the music chunk size in bytes. */
		m_musicChunkSize = m_primaryServices.settings().get< uint32_t >(AudioMusicChunkSizeKey, DefaultAudioMusicChunkSize);

		this->queryDevices();

		/* Take the default device. */
		m_device = alcOpenDevice(nullptr);

		if ( alcGetErrors(m_device, "alcOpenDevice()", __FILE__, __LINE__) || m_device == nullptr )
		{
			Tracer::error(ClassId, "Unable to get the default audio device !");

			return false;
		}

		std::array attrList{
			ALC_FREQUENCY, static_cast< int >(m_playbackFrequency),
			ALC_REFRESH, m_primaryServices.settings().get< int32_t >(OpenALRefreshRateKey, DefaultOpenALRefreshRate),
			ALC_SYNC, m_primaryServices.settings().get< int32_t >(OpenALSyncStateKey, DefaultOpenALSyncState),
			ALC_MONO_SOURCES, m_primaryServices.settings().get< int32_t >(OpenALMaxMonoSourceCountKey, DefaultOpenALMaxMonoSourceCount),
			ALC_STEREO_SOURCES, m_primaryServices.settings().get< int32_t >(OpenALMaxStereoSourceCountKey, DefaultOpenALMaxStereoSourceCount),
			0
		};

		/* Context creation and set it as default. */
		m_context = alcCreateContext(m_device, attrList.data());

		if ( alcGetErrors(m_device, "alcCreateContext()", __FILE__, __LINE__) || m_context == nullptr )
		{
			Tracer::error(ClassId, "Unable to load an audio context !");

			return false;
		}

		if ( alcMakeContextCurrent(m_context) == AL_FALSE || alcGetErrors(m_device, "alcMakeContextCurrent()", __FILE__, __LINE__) )
		{
			Tracer::error(ClassId, "Unable set the current audio context !");

			return false;
		}

		/* OpenAL EFX extensions. */
		if ( m_primaryServices.settings().get< bool >(OpenALUseEFXExtensionsKey, DefaultOpenALUseEFXExtensions) )
		{
			m_EFX = std::make_shared< EFX >(m_device);

			if ( EFX::isAvailable() )
			{
				Tracer::success(ClassId, "OpenAL EFX extension available.");
			}
			else
			{
				Tracer::warning(ClassId, "OpenAL EFX extension unavailable !");
			}
		}

		if ( !this->saveContextAttributes() )
		{
			return false;
		}

		m_flags[ServiceInitialized] = true;
		m_flags[Enabled] = true;

		/* NOTE: Be sure of the playback frequency allowed by this OpenAL context. */
		m_playbackFrequency = WaveFactory::toFrequency(m_contextAttributes[ALC_FREQUENCY]);

		this->setMetersPerUnit(1.0F);
		this->setMasterVolume(m_primaryServices.settings().get< float >(AudioMasterVolumeKey, DefaultAudioMasterVolume));

		/* Create all sources available minus the default one. */
		for ( int index = 0; index < m_contextAttributes[ALC_MONO_SOURCES]; index++ )
		{
			auto source = std::make_shared< Source >();

			if ( !source->isCreated() )
			{
				TraceWarning{ClassId} << "Unable to create the source #" << index << " !";

				break;
			}

			m_sources.emplace_back(source);
		}

		if ( m_sources.empty() )
		{
			Tracer::error(ClassId, "No audio source available at all ! Disable audio layer ...");

			m_flags[Enabled] = false;

			s_audioDisabled = true;
			m_flags[AudioDisabledAtStartup] = true;

			return false;
		}

		m_defaultSource = *m_sources.begin();

		/* NOTE: Check for missing errors from audio lib initialization. */
		if ( alcGetErrors(m_device, "AudioInit", __FILE__, __LINE__) )
		{
			Tracer::warning(ClassId, "There was unread problem with ALC during initialization !");
		}

		if ( alGetErrors("AudioInit", __FILE__, __LINE__) )
		{
			Tracer::warning(ClassId, "There was unread problem with AL during initialization !");
		}

		this->registerToConsole();

		/* NOTE: Initialize all sub-services */
		if ( !this->initializeSubServices() )
		{
			Tracer::fatal(ClassId, "Unable to initialize renderer sub-services properly !");

			return false;
		}

		if ( m_flags[ShowInformation] )
		{
			Tracer::info(ClassId, this->getAPIInformation());
		}

		return true;
	}

	bool
	Manager::onTerminate () noexcept
	{
		m_flags[ServiceInitialized] = false;
		m_flags[Enabled] = false;

		/* NOTE: The audio service wasn't inited. */
		if ( m_flags[AudioDisabledAtStartup] )
		{
			return true;
		}

		/* NOTE: Check for missing errors from audio lib execution. */
		if ( alGetErrors("AudioRelease", __FILE__, __LINE__) )
		{
			Tracer::warning(ClassId, "There was unread problem with AL during execution !");
		}

		if ( alcGetErrors(m_device, "AudioRelease", __FILE__, __LINE__) )
		{
			Tracer::warning(ClassId, "There was unread problem with ALC during execution !");
		}

		alcMakeContextCurrent(nullptr);

		if ( m_context != nullptr )
		{
			alcDestroyContext(m_context);

			m_context = nullptr;
		}

		if ( m_device != nullptr )
		{
			alcCloseDevice(m_device);

			m_device = nullptr;
		}

		return true;
	}

	void
	Manager::onRegisterToConsole () noexcept
	{

	}

	void
	Manager::enableAudio (bool state) noexcept
	{
		if ( m_flags[AudioDisabledAtStartup] )
		{
			Tracer::info(ClassId, "The audio sub-system has been disabled at startup !");

			return;
		}

		m_flags[Enabled] = state;
	}

	void
	Manager::play (const std::string & resourceName, Source::PlayMode mode, float gain) const noexcept
	{
		if ( !this->isAudioEnabled() )
		{
			return;
		}

		auto soundResource = m_resourceManager.container< SoundResource >()->getResource(resourceName);

		if ( !soundResource->isLoaded() )
		{
			TraceInfo{ClassId} << "The sound '" << soundResource->name() << "' is not yet loaded !";

			return;
		}

		this->play(soundResource, mode, gain);
	}

	void
	Manager::setMetersPerUnit (float meters) noexcept
	{
		if ( m_flags[AudioDisabledAtStartup] || !EFX::isAvailable() )
		{
			return;
		}

		if ( meters < 0.0F )
		{
			Tracer::warning(ClassId, "Meters per unit must more than zero !");

			return;
		}

		alListenerf(AL_METERS_PER_UNIT, meters);
	}

	float
	Manager::metersPerUnit () const noexcept
	{
		ALfloat meters = AL_DEFAULT_METERS_PER_UNIT;

		if ( !m_flags[AudioDisabledAtStartup] && EFX::isAvailable() )
		{
			alGetListenerf(AL_METERS_PER_UNIT, &meters);
		}

		return meters;
	}

	std::string
	Manager::getALCVersionString () const noexcept
	{
		ALCint major = 0;
		ALCint minor = 0;

		if ( !m_flags[AudioDisabledAtStartup] )
		{
			if ( m_contextAttributes.empty() )
			{
				alcGetIntegerv(m_device, ALC_MAJOR_VERSION, 1, &major);
				alcGetIntegerv(m_device, ALC_MINOR_VERSION, 1, &minor);
			}
			else
			{
				major = m_contextAttributes.at(ALC_MAJOR_VERSION);
				minor = m_contextAttributes.at(ALC_MINOR_VERSION);
			}
		}
		else
		{
			Tracer::info(ClassId, "The audio sub-system has been disabled at startup !");
		}

		return (std::stringstream{} << major << '.' << minor).str();
	}

	std::string
	Manager::getEFXVersionString () const noexcept
	{
		ALCint major = 0;
		ALCint minor = 0;

		if ( !m_flags[AudioDisabledAtStartup] )
		{
			/*if ( m_contextAttributes.empty() )
			{
				alcGetIntegerv(m_device, ALC_MAJOR_VERSION, 1, &major);
				alcGetIntegerv(m_device, ALC_MINOR_VERSION, 1, &minor);
			}
			else*/
			{
				major = m_contextAttributes.at(ALC_EFX_MAJOR_VERSION);
				minor = m_contextAttributes.at(ALC_EFX_MINOR_VERSION);
			}
		}
		else
		{
			Tracer::info(ClassId, "The audio sub-system has been disabled at startup !");
		}

		return (std::stringstream{} << major << '.' << minor).str();
	}

	size_t
	Manager::getAvailableSourceCount () const noexcept
	{
		size_t count = 0;

		for ( const auto & source : m_sources )
		{
			if ( source.use_count() == 1 )
			{
				count++;
			}
		}

		return count;
	}

	std::shared_ptr< Source >
	Manager::requestSource () const noexcept
	{
		/* FIXME: Find a better way to deliver quickly an available source. */
		for ( const auto & source : m_sources )
		{
			if ( source.use_count() == 1 )
			{
				return source;
			}
		}

		return nullptr;
	}

	void
	Manager::setMasterVolume (float gain) noexcept
	{
		if ( m_flags[AudioDisabledAtStartup] )
		{
			return;
		}

		alListenerf(AL_GAIN, Math::clampToUnit(gain));
	}

	float
	Manager::masterVolume () noexcept
	{
		ALfloat gain = 0.0F;

		if ( !m_flags[AudioDisabledAtStartup] )
		{
			alGetListenerf(AL_GAIN, &gain);
		}

		return gain;
	}

	void
	Manager::setSoundEnvironmentProperties (const SoundEnvironmentProperties & properties) noexcept
	{
		if ( m_flags[AudioDisabledAtStartup] )
		{
			return;
		}

		this->setDopplerFactor(properties.dopplerFactor());
		this->setSpeedOfSound(properties.speedOfSound());
		this->setDistanceModel(properties.distanceModel());
	}

	SoundEnvironmentProperties
	Manager::getSoundEnvironmentProperties () const noexcept
	{
		SoundEnvironmentProperties properties;
		properties.setDopplerFactor(this->dopplerFactor());
		properties.setSpeedOfSound(this->speedOfSound());
		properties.setDistanceModel(this->distanceModel());

		return properties;
	}

	void
	Manager::setDopplerFactor (float dopplerFactor) noexcept
	{
		/* Scale for source and listener velocities (default:1.0, range:0.0-INF+). */
		alDopplerFactor(dopplerFactor);
	}

	float
	Manager::dopplerFactor () const noexcept
	{
		return alGetFloat(AL_DOPPLER_FACTOR);
	}

	void
	Manager::setSpeedOfSound (float speed) noexcept
	{
		/* The speed at which sound waves are assumed to travel,
		 * when calculating the doppler effect.
		 * (default:343.3, range:0.0001-INF+) */
		alSpeedOfSound(speed);
	}

	float
	Manager::speedOfSound () const noexcept
	{
		return alGetFloat(AL_SPEED_OF_SOUND);
	}

	void
	Manager::setDistanceModel (DistanceModel model) noexcept
	{
		switch ( model )
		{
			case DistanceModel::ExponentClamped :
				alDistanceModel(AL_EXPONENT_DISTANCE_CLAMPED);
				break;

			case DistanceModel::Exponent :
				alDistanceModel(AL_EXPONENT_DISTANCE);
				break;

			case DistanceModel::LinearClamped :
				alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);
				break;

			case DistanceModel::Linear :
				alDistanceModel(AL_LINEAR_DISTANCE);
				break;

			case DistanceModel::InverseClamped :
				alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
				break;

			case DistanceModel::Inverse :
				alDistanceModel(AL_INVERSE_DISTANCE);
				break;
		}
	}

	DistanceModel
	Manager::distanceModel () const noexcept
	{
		switch ( alGetInteger(AL_DISTANCE_MODEL) )
		{
			case AL_INVERSE_DISTANCE :
				return DistanceModel::Inverse;

			case AL_INVERSE_DISTANCE_CLAMPED :
				return DistanceModel::InverseClamped;

			case AL_LINEAR_DISTANCE :
				return DistanceModel::Linear;

			case AL_LINEAR_DISTANCE_CLAMPED :
				return DistanceModel::LinearClamped;

			case AL_EXPONENT_DISTANCE :
				return DistanceModel::Exponent;

			case AL_EXPONENT_DISTANCE_CLAMPED :
				return DistanceModel::ExponentClamped;

			default:
				return DistanceModel::Inverse;
		}
	}

	void
	Manager::setListenerProperties (const std::array< ALfloat, 12 > & properties) noexcept
	{
		if ( !m_flags[AudioDisabledAtStartup] )
		{
			alListenerfv(AL_POSITION, properties.data());
			alListenerfv(AL_ORIENTATION, properties.data() + 3);
			alListenerfv(AL_VELOCITY, properties.data() + 9);
		}
	}

	void
	Manager::listenerProperties (std::array< ALfloat, 12 > & properties) const noexcept
	{
		if ( !m_flags[AudioDisabledAtStartup] )
		{
			alGetListenerfv(AL_POSITION, properties.data());
			alGetListenerfv(AL_ORIENTATION, properties.data() + 3);
			alGetListenerfv(AL_VELOCITY, properties.data() + 9);
		}
	}

	std::string
	Manager::getAPIInformation () const noexcept
	{
		if ( m_flags[AudioDisabledAtStartup] )
		{
			return "API not loaded !";
		}

		std::stringstream output;

		/* OpenAL basic information. */
		output <<
			"OpenAL API information" "\n"
			" - Vendor : " << alGetString(AL_VENDOR) << "\n"
			" - Renderer : " << alGetString(AL_RENDERER) << "\n"
			" - Version : " << alGetString(AL_VERSION) << "\n"
			" - ALC Version : " << this->getALCVersionString() << "\n"
			" - EFX Version : " << this->getEFXVersionString() << "\n";

		/* OpenAL extensions. */
		{
			auto extensions = String::explode(alGetString(AL_EXTENSIONS), ' ', false);

			if ( extensions.empty() )
			{
				output << "No AL extension available !" "\n";
			}
			else
			{
				output << "Available AL extensions :" "\n";

				for ( const auto & extension : extensions )
				{
					output << " - " << extension << '\n';
				}
			}
		}

		/* Advanced information via ALC. */
		output << "ALC information" "\n";

		/* ALC Capabilities (read before) */
		for ( const auto & pair : m_contextAttributes )
		{
			output << " - " << alcKeyToLabel(pair.first) << " : " << pair.second << "\n";
		}

		/* ALC extensions */
		{
			auto extensions = String::explode(alcGetString(nullptr, ALC_EXTENSIONS), ' ', false);

			if ( extensions.empty() )
			{
				output << "No ALC extension available !" "\n";
			}
			else
			{
				output << "Available ALC extensions :" "\n";

				for ( const auto & extension : extensions )
				{
					output << " - " << extension << '\n';
				}
			}
		}

		return output.str();
	}

	std::vector< std::string >
	Manager::getDeviceName (const ALCchar * list) noexcept
	{
		std::vector< std::string > devices{};

		if ( list != nullptr )
		{
			while ( std::strlen(list) > 0 )
			{
				devices.emplace_back(list);

				list += std::strlen(list) + 1;
			}
		}

		return devices;
	}

	void
	Manager::queryDevices () noexcept
	{
		const ALCchar * devices = alcGetString(nullptr, ALC_DEVICE_SPECIFIER);

		if ( devices != nullptr )
		{
			while ( std::strlen(devices) > 0 )
			{
				m_availableAudioDevices.emplace_back(devices);

				devices += std::strlen(devices) + 1;
			}

			if ( m_flags[ShowInformation] )
			{
				std::cout << "[OpenAL] Devices :" "\n";

				for ( const auto & deviceName : m_availableAudioDevices )
				{
					std::cout << " - " << deviceName << '\n';
				}

				std::cout << "[OpenAL] Default device : " << alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER) << '\n';
			}
		}

		if ( m_flags[ShowInformation] && alcIsExtensionPresent(nullptr, "ALC_ENUMERATE_ALL_EXT") == ALC_TRUE )
		{
			devices = alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);

			if ( devices != nullptr )
			{
				std::cout << "[OpenAL] All devices :\n";

				while ( std::strlen(devices) > 0 )
				{
					std::cout << " - " << devices << '\n';

					devices += std::strlen(devices) + 1;
				}

				std::cout << "[OpenAL] Default all devices : " << alcGetString(nullptr, ALC_DEFAULT_ALL_DEVICES_SPECIFIER) << '\n';
			}
		}

		alcFlushErrors(m_device);
	}

	std::vector< ALCint >
	Manager::getDeviceAttributes () const noexcept
	{
		std::vector< ALCint > attributes{};

		ALCint size = 0;

		alcGetIntegerv(m_device, ALC_ATTRIBUTES_SIZE, 1, &size);

		if ( size > 0 )
		{
			attributes.resize(size);

			alcGetIntegerv(m_device, ALC_ALL_ATTRIBUTES, size, attributes.data());
		}

		if ( alcGetErrors(m_device, "alcGetIntegerv", __FILE__, __LINE__) )
		{
			Tracer::warning(ClassId, "Unable to fetch device attributes correctly !");
		}

		return attributes;
	}

	bool
	Manager::saveContextAttributes () noexcept
	{
		const auto attributes = this->getDeviceAttributes();

		if ( attributes.empty() )
		{
			Tracer::error(ClassId, "Unable to retrieve context attributes !");

			return false;
		}

		for ( size_t index = 0; index < attributes.size(); index += 2 )
		{
			if ( attributes[index] == 0 )
			{
				break;
			}

			m_contextAttributes[attributes[index]] = attributes[index+1];
		}

		return true;
	}
}

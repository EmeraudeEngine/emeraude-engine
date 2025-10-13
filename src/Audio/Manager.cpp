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
	using namespace Libs;
	using namespace Libs::Math;

	bool
	Manager::queryOutputDevices (bool useExtendedAPI) noexcept
	{
		/* NOTE: Check for the audio device enumeration. */
		const char * extensionName = useExtendedAPI ? "ALC_ENUMERATE_ALL_EXT" : "ALC_ENUMERATION_EXT";

		if ( alcIsExtensionPresent(nullptr, extensionName) == ALC_FALSE )
		{
			TraceError{ClassId} << "OpenAL extension '" << extensionName << "' not available!";

			return false;
		}

		m_availableOutputDevices.clear();
		m_usingAdvancedEnumeration = useExtendedAPI;

		const auto * devices = alcGetString(nullptr, useExtendedAPI ? ALC_ALL_DEVICES_SPECIFIER : ALC_DEVICE_SPECIFIER);

		if ( devices == nullptr )
		{
			Tracer::error(ClassId, "There is no audio devices!");

			return false;
		}

		while ( *devices != '\0' )
		{
			m_availableOutputDevices.emplace_back(devices);

			devices += m_availableOutputDevices.back().length() + 1;
		}

		if ( m_availableOutputDevices.empty() )
		{
			/* No device at all... */
			m_selectedOutputDeviceName.clear();

			return false;
		}

		const auto * defaultDeviceName = alcGetString(nullptr, useExtendedAPI ? ALC_DEFAULT_ALL_DEVICES_SPECIFIER : ALC_DEFAULT_DEVICE_SPECIFIER);

		if ( m_selectedOutputDeviceName.empty() )
		{
			m_selectedOutputDeviceName.assign(defaultDeviceName);
		}
		else
		{
			/* NOTE: Check if the previous selected device from settings is still available. */
			const auto deviceIt = std::ranges::find_if(m_availableOutputDevices, [this] (const std::string & deviceName) {
				return deviceName == m_selectedOutputDeviceName;
			});

			if ( deviceIt == m_availableOutputDevices.cend() )
			{
				TraceWarning{ClassId} << "The selected output audio device '" << m_selectedOutputDeviceName << "' is not available anymore!";

				m_selectedOutputDeviceName.assign(defaultDeviceName);
			}
		}

		s_audioSystemAvailable = true;

		return true;
	}

	bool
	Manager::setupAudioOutputDevice () noexcept
	{
		auto & settings = m_primaryServices.settings();
		
		/* First, read setting for a desired output audio device. */
		m_selectedOutputDeviceName = settings.getOrSetDefault< std::string >(AudioDeviceNameKey, DefaultAudioDeviceName);

		/* Then, check the audio system. */
		bool forceDefaultDevice = false;

		if ( !settings.getOrSetDefault< bool >(AudioForceDefaultDeviceKey, DefaultAudioForceDefaultDevice) )
		{
			if ( this->queryOutputDevices(true) || this->queryOutputDevices(false) )
			{
				if ( m_showInformation )
				{
					std::cout << "[OpenAL] Audio devices:" "\n";

					for ( const auto & deviceName : m_availableOutputDevices )
					{
						std::cout << " - " << deviceName << '\n';
					}

					std::cout << "Default: " << m_selectedOutputDeviceName << '\n';
				}
			}
			else
			{
				forceDefaultDevice = true;

				Tracer::warning(ClassId, "There is no audio system found by querying! Let OpenAL open a default itself ...");
			}
		}
		else
		{
			forceDefaultDevice = true;
		}

		/* Checks configuration file */
		s_playbackFrequency = WaveFactory::toFrequency(settings.getOrSetDefault< int32_t >(AudioPlaybackFrequencyKey, DefaultAudioPlaybackFrequency));

		if ( s_playbackFrequency == WaveFactory::Frequency::Invalid )
		{
			TraceWarning{ClassId} <<
				"Invalid frequency in settings file! "
				"Leaving to default " << DefaultAudioPlaybackFrequency << " Hz.";

			s_playbackFrequency = WaveFactory::Frequency::PCM48000Hz;
		}

		/* NOTE: Opening the output audio device. */
		if ( forceDefaultDevice )
		{
			m_outputDevice = alcOpenDevice(nullptr);

			if ( alcGetErrors(m_outputDevice, "alcOpenDevice(NULL)", __FILE__, __LINE__) || m_outputDevice == nullptr )
			{
				TraceError{ClassId} << "Unable to open the default output audio device !";

				return false;
			}
		}
		else
		{
			m_outputDevice = alcOpenDevice(m_selectedOutputDeviceName.c_str());

			if ( alcGetErrors(m_outputDevice, "alcOpenDevice(deviceName)", __FILE__, __LINE__) || m_outputDevice == nullptr )
			{
				TraceError{ClassId} << "Unable to open the selected output audio device '" << m_selectedOutputDeviceName << "' !";

				return false;
			}
		}

		if ( m_usingAdvancedEnumeration )
		{
			TraceSuccess{ClassId} << "The output audio device '" << alcGetString(m_outputDevice, ALC_ALL_DEVICES_SPECIFIER) << "' selected !";
		}
		else
		{
			TraceSuccess{ClassId} << "The output audio device '" << alcGetString(m_outputDevice, ALC_DEVICE_SPECIFIER) << "' selected !";
		}

		const std::array attributeList{
			ALC_FREQUENCY, static_cast< int >(s_playbackFrequency),
			ALC_REFRESH, settings.getOrSetDefault< int32_t >(OpenALRefreshRateKey, DefaultOpenALRefreshRate),
			ALC_SYNC, settings.getOrSetDefault< int32_t >(OpenALSyncStateKey, DefaultOpenALSyncState),
			ALC_MONO_SOURCES, settings.getOrSetDefault< int32_t >(OpenALMaxMonoSourceCountKey, DefaultOpenALMaxMonoSourceCount),
			ALC_STEREO_SOURCES, settings.getOrSetDefault< int32_t >(OpenALMaxStereoSourceCountKey, DefaultOpenALMaxStereoSourceCount),
			0
		};

		/* Context creation and set it as default. */
		m_context = alcCreateContext(m_outputDevice, attributeList.data());

		if ( alcGetErrors(m_outputDevice, "alcCreateContext()", __FILE__, __LINE__) || m_context == nullptr )
		{
			Tracer::error(ClassId, "Unable to create an audio context !");

			return false;
		}

		if ( alcMakeContextCurrent(m_context) == AL_FALSE || alcGetErrors(m_outputDevice, "alcMakeContextCurrent()", __FILE__, __LINE__) )
		{
			Tracer::error(ClassId, "Unable set the current audio context !");

			return false;
		}

		OpenAL::installExtensionEvents();

		/* OpenAL EFX extensions. */
		if ( settings.getOrSetDefault< bool >(OpenALUseEFXExtensionsKey, DefaultOpenALUseEFXExtensions) )
		{
			OpenAL::installExtensionSystemEvents(m_outputDevice);

			OpenAL::installExtensionEFX(m_outputDevice);
		}

		return this->saveContextAttributes();
	}

	bool
	Manager::queryInputDevices () noexcept
	{
		/* NOTE: Check for the audio device enumeration. */
		if ( alcIsExtensionPresent(nullptr, "ALC_EXT_CAPTURE") == ALC_FALSE )
		{
			Tracer::error(ClassId, "OpenAL extension 'ALC_EXT_CAPTURE' not available!");

			return false;
		}

		m_availableInputDevices.clear();

		const auto * devices = alcGetString(nullptr, ALC_CAPTURE_DEVICE_SPECIFIER);

		if ( devices == nullptr )
		{
			Tracer::error(ClassId, "There is no capture audio devices!");

			return false;
		}

		while ( *devices != '\0' )
		{
			m_availableInputDevices.emplace_back(devices);

			devices += m_availableInputDevices.back().length() + 1;
		}

		if ( m_availableInputDevices.empty() )
		{
			/* No device at all... */
			m_selectedInputDeviceName.clear();

			return false;
		}

		const auto * defaultDeviceName = alcGetString(nullptr, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);

		if ( m_selectedInputDeviceName.empty() )
		{
			m_selectedInputDeviceName.assign(defaultDeviceName);
		}
		else
		{
			/* NOTE: Check if the previous selected device from settings is still available. */
			const auto deviceIt = std::ranges::find_if(m_availableInputDevices, [this] (const std::string & deviceName) {
				return deviceName == m_selectedInputDeviceName;
			});

			if ( deviceIt == m_availableInputDevices.cend() )
			{
				TraceWarning{ClassId} << "The selected input audio device '" << m_selectedInputDeviceName << "' is not available anymore!";

				m_selectedInputDeviceName.assign(defaultDeviceName);
			}
		}

		s_audioCaptureAvailable = true;

		return true;
	}

	bool
	Manager::setupAudioInputDevice () noexcept
	{
		auto & settings = m_primaryServices.settings();

		/* First, read setting for a desired input audio device. */
		m_selectedInputDeviceName = settings.getOrSetDefault< std::string >(AudioRecorderDeviceNameKey, DefaultAudioRecorderDeviceName);

		/* Then, check the audio capture system. */
		if ( !this->queryInputDevices() )
		{
			return false;
		}

		if ( m_showInformation )
		{
			std::cout << "[OpenAL] Capture audio devices:" "\n";

			for ( const auto & deviceName : m_availableInputDevices )
			{
				std::cout << " - " << deviceName << '\n';
			}

			std::cout << "Default: " << m_selectedInputDeviceName << '\n';
		}

		/* Checks configuration file */
		const auto bufferSize = settings.getOrSetDefault< int32_t >(RecorderBufferSizeKey, DefaultRecorderBufferSize);
		s_recordFrequency = WaveFactory::toFrequency(settings.getOrSetDefault< int32_t >(RecorderFrequencyKey, DefaultRecorderFrequency));

		if ( s_recordFrequency == WaveFactory::Frequency::Invalid )
		{
			TraceWarning{ClassId} <<
				"Invalid recorder frequency in settings file! "
				"Leaving to default " << DefaultRecorderFrequency << " Hz.";

			s_recordFrequency = WaveFactory::Frequency::PCM48000Hz;
		}

		/* NOTE: Opening the output input device. */
		m_inputDevice = alcCaptureOpenDevice(
			m_selectedInputDeviceName.c_str(),
			static_cast< ALuint >(s_recordFrequency),
			AL_FORMAT_MONO16,
			bufferSize * 1024
		);

		if ( m_inputDevice == nullptr )
		{
			TraceError{ClassId} << "Unable to open the input audio device '" << m_selectedOutputDeviceName << "' !";

			return false;
		}

		TraceSuccess{ClassId} << "The input audio device '" << alcGetString(m_inputDevice, ALC_CAPTURE_DEVICE_SPECIFIER) << "' selected !";

		return true;
	}

	bool
	Manager::onInitialize () noexcept
	{
		auto & settings = m_primaryServices.settings();

		m_showInformation = settings.getOrSetDefault< bool >(OpenALShowInformationKey, DefaultOpenALShowInformation);

		if ( m_primaryServices.arguments().isSwitchPresent("--disable-audio") || !settings.getOrSetDefault< bool >(AudioEnableKey, DefaultAudioEnable) )
		{
			Tracer::warning(ClassId, "Audio manager disabled at startup.");

			return true;
		}

		/* NOTE: Select an audio device. */
		if ( !this->setupAudioOutputDevice() )
		{
			Tracer::error(ClassId, "Unable to get an audio device or an audio context! Disabling audio layer.");

			/* NOTE: Disable the previously enabled state. */
			s_audioSystemAvailable = false;

			return false;
		}

		/* NOTE: Select a capture audio device. */
		if ( settings.getOrSetDefault< bool >(AudioRecorderEnableKey, DefaultAudioRecorderEnable) )
		{
			if ( this->setupAudioInputDevice() )
			{
				m_audioRecorder.configure(m_inputDevice, WaveFactory::Channels::Mono, s_recordFrequency);
			}
			else
			{
				Tracer::error(ClassId, "Unable to get a capture audio device! Disabling audio recording.");
			}
		}

		/* NOTE: Set up the audio configuration. */
		this->setMetersPerUnit(1.0F);
		this->setMainLevel(settings.getOrSetDefault< float >(AudioMasterVolumeKey, DefaultAudioMasterVolume));

		s_playbackFrequency = WaveFactory::toFrequency(m_contextAttributes[ALC_FREQUENCY]); /* NOTE: Be sure of the playback frequency allowed by this OpenAL context. */
		s_musicChunkSize = settings.getOrSetDefault< uint32_t >(AudioMusicChunkSizeKey, DefaultAudioMusicChunkSize);

		/* NOTE: Create a default source. */
		m_defaultSource = std::make_shared< Source >();

		/* Create all sources available minus the default one and two for the track mixer. */
		if ( m_contextAttributes[ALC_MONO_SOURCES] > 4 )
		{
			const auto maxMonoSources = static_cast< size_t >(m_contextAttributes[ALC_MONO_SOURCES]) - 3;

			m_allSources.reserve(maxMonoSources);
			m_availableSources.reserve(maxMonoSources);

			for ( size_t index = 0; index < maxMonoSources; index++ )
			{
				auto source = std::make_shared< Source >();

				if ( !source->isCreated() )
				{
					TraceWarning{ClassId} << "Unable to create the source #" << index << " !";

					break;
				}

				m_allSources.push_back(source);

				m_availableSources.push_back(source.get());
			}
		}

		if ( m_allSources.empty() )
		{
			Tracer::error(ClassId, "No audio source available at all! Disabling audio layer.");

			/* NOTE: Disable the previously enabled state. */
			s_audioSystemAvailable = false;

			return false;
		}

		s_audioEnabled = true;

		this->registerToConsole();

		if ( m_trackMixer.initialize() )
		{
			TraceSuccess{ClassId} << m_trackMixer.name() << " service up !";

			m_trackMixer.enableCrossFader(m_contextAttributes[ALC_STEREO_SOURCES] >= 2);
		}
		else
		{
			TraceWarning{ClassId} << m_trackMixer.name() << " service failed to execute !";
		}

		if ( m_showInformation )
		{
			Tracer::info(ClassId, this->getAPIInformation());
		}

		/* NOTE: Check for missing errors from audio lib initialization. */
		if ( alGetErrors("GlobalInitFlush", __FILE__, __LINE__) )
		{
			Tracer::warning(ClassId, "There was unread problem with AL during initialization !");
		}

		if ( alcGetErrors(m_outputDevice, "GlobalInitFlush", __FILE__, __LINE__) )
		{
			Tracer::warning(ClassId, "There was unread problem with ALC during initialization !");
		}

		return true;
	}

	bool
	Manager::onTerminate () noexcept
	{
		s_audioEnabled = false;

		/* NOTE: The audio service wasn't inited. */
		if ( !s_audioSystemAvailable )
		{
			return true;
		}

		if ( m_trackMixer.terminate() )
		{
			TraceSuccess{ClassId} << m_trackMixer.name() << " primary service terminated gracefully!";
		}
		else
		{
			TraceError{ClassId} << m_trackMixer.name() << " primary service failed to terminate properly!";
		}

		m_defaultSource.reset();

		/* NOTE: Check for missing errors from audio lib execution. */
		if ( alGetErrors("GlobalReleaseFlush", __FILE__, __LINE__) )
		{
			Tracer::warning(ClassId, "There was unread problem with AL during execution !");
		}

		if ( alcGetErrors(m_outputDevice, "GlobalReleaseFlush", __FILE__, __LINE__) )
		{
			Tracer::warning(ClassId, "There was unread problem with ALC during execution !");
		}

		auto & settings = m_primaryServices.settings();

		/* NOTE: Release the output audio device. */
		if ( m_inputDevice != nullptr )
		{
			if ( alcCaptureCloseDevice(m_inputDevice) == ALC_TRUE )
			{
				TraceSuccess{ClassId} << "The input audio device '" << m_selectedInputDeviceName << "' closed !";

				settings.set< std::string >(AudioRecorderDeviceNameKey, m_selectedInputDeviceName);
			}
			else
			{
				TraceError{ClassId} << "Unable to close the input audio device '" << m_selectedInputDeviceName << "' !";
			}

			m_inputDevice = nullptr;
		}

		/* NOTE: Release the audio context. */
		alcMakeContextCurrent(nullptr);

		if ( m_context != nullptr )
		{
			alcDestroyContext(m_context);

			m_context = nullptr;
		}

		/* NOTE: Release the output audio device. */
		if ( m_outputDevice != nullptr )
		{
			if ( alcCloseDevice(m_outputDevice) == ALC_TRUE )
			{
				TraceSuccess{ClassId} << "The output audio device '" << m_selectedOutputDeviceName << "' closed !";

				settings.set< std::string >(AudioDeviceNameKey, m_selectedOutputDeviceName);
			}
			else
			{
				TraceError{ClassId} << "Unable to close the output audio device '" << m_selectedOutputDeviceName << "' !";
			}

			m_outputDevice = nullptr;
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
		if ( !s_audioSystemAvailable )
		{
			Tracer::info(ClassId, "The audio sub-system has been disabled at startup !");

			return;
		}

		s_audioEnabled = state;
	}

	void
	Manager::play (const std::shared_ptr< PlayableInterface > & playable, PlayMode mode, float gain) const noexcept
	{
		if ( Manager::isAudioEnabled() && m_defaultSource != nullptr )
		{
			m_defaultSource->setGain(gain);
			m_defaultSource->play(playable, mode);
		}
	}

	void
	Manager::play (const std::string & resourceName, PlayMode mode, float gain) const noexcept
	{
		if ( Manager::isAudioEnabled() && m_defaultSource != nullptr )
		{
			const auto soundResource = m_resourceManager
				.container< SoundResource >()
				->getResource(resourceName);

			if ( !soundResource->isLoaded() )
			{
				TraceDebug{ClassId} << "The sound resource '" << resourceName << "' is not yet loaded ! Skipping ...";

				return;
			}

			m_defaultSource->setGain(gain);
			m_defaultSource->play(soundResource, mode);
		}
	}

	void
	Manager::setMetersPerUnit (float meters) noexcept
	{
		if ( !s_audioSystemAvailable || !OpenAL::isEFXAvailable() )
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

		if ( s_audioSystemAvailable && OpenAL::isEFXAvailable() )
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

		if ( s_audioSystemAvailable )
		{
			if ( m_contextAttributes.empty() )
			{
				alcGetIntegerv(m_outputDevice, ALC_MAJOR_VERSION, 1, &major);
				alcGetIntegerv(m_outputDevice, ALC_MINOR_VERSION, 1, &minor);
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

		if ( s_audioSystemAvailable )
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
		const std::lock_guard< std::mutex > lock{m_sourcePoolMutex};

		return m_availableSources.size();
	}

	SourceRequest
	Manager::requestSource () noexcept
	{
		const std::lock_guard< std::mutex > lock{m_sourcePoolMutex};

		if ( m_availableSources.empty() )
		{
			return nullptr;
		}

		auto * source = m_availableSources.back();

		m_availableSources.pop_back();

		return {source, [this](Source * sourceToRelease) {
			this->releaseSource(sourceToRelease);
		}};
	}

	void
	Manager::releaseSource (Source * source) noexcept
	{
		if ( source != nullptr )
		{
			const std::lock_guard< std::mutex > lock{m_sourcePoolMutex};

			m_availableSources.push_back(source);
		}
	}

	void
	Manager::setMainLevel (float gain) noexcept
	{
		if ( !s_audioSystemAvailable )
		{
			return;
		}

		alListenerf(AL_GAIN, Math::clampToUnit(gain));
	}

	float
	Manager::mainLevel () const noexcept
	{
		ALfloat gain = 0.0F;

		if ( s_audioSystemAvailable )
		{
			alGetListenerf(AL_GAIN, &gain);
		}

		return gain;
	}

	void
	Manager::setSoundEnvironmentProperties (const SoundEnvironmentProperties & properties) noexcept
	{
		if ( !s_audioSystemAvailable )
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
		if ( s_audioSystemAvailable )
		{
			alListenerfv(AL_POSITION, properties.data());
			alListenerfv(AL_ORIENTATION, properties.data() + 3);
			alListenerfv(AL_VELOCITY, properties.data() + 9);
		}
	}

	void
	Manager::listenerProperties (std::array< ALfloat, 12 > & properties) const noexcept
	{
		if ( s_audioSystemAvailable )
		{
			alGetListenerfv(AL_POSITION, properties.data());
			alGetListenerfv(AL_ORIENTATION, properties.data() + 3);
			alGetListenerfv(AL_VELOCITY, properties.data() + 9);
		}
	}

	std::string
	Manager::getAPIInformation () const noexcept
	{
		if ( !s_audioSystemAvailable )
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
		for ( const auto & [label, value] : m_contextAttributes )
		{
			output << " - " << alcKeyToLabel(label) << " : " << value << "\n";
		}

		/* ALC extensions */
		if ( const auto extensions = String::explode(alcGetString(nullptr, ALC_EXTENSIONS), ' ', false); extensions.empty() )
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

		return output.str();
	}

	std::vector< ALCint >
	Manager::getDeviceAttributes () const noexcept
	{
		std::vector< ALCint > attributes{};

		ALCint size = 0;

		alcGetIntegerv(m_outputDevice, ALC_ATTRIBUTES_SIZE, 1, &size);

		if ( size > 0 )
		{
			attributes.resize(size);

			alcGetIntegerv(m_outputDevice, ALC_ALL_ATTRIBUTES, size, attributes.data());
		}

		if ( alcGetErrors(m_outputDevice, "alcGetIntegerv", __FILE__, __LINE__) )
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

/*
 * src/Audio/Manager.cpp
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

#include "Manager.hpp"

/* STL inclusions. */
#include <cstring>
#include <iostream>

/* Local inclusions. */
#include "Libs/WaveFactory/Synthesizer.hpp"
#include "Libs/Math/Base.hpp"
#include "SettingKeys.hpp"
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
	Manager::selectedOutputDevice (bool useExtendedAPI) noexcept
	{
		/* NOTE: Check for the audio device enumeration. */
		const char * extensionName = useExtendedAPI ? "ALC_ENUMERATE_ALL_EXT" : "ALC_ENUMERATION_EXT";

		if ( alcIsExtensionPresent(nullptr, extensionName) == ALC_FALSE )
		{
			TraceError{ClassId} << "OpenAL extension '" << extensionName << "' not available!";

			return false;
		}

		m_availableDevices.clear();
		m_usingAdvancedEnumeration = useExtendedAPI;

		const auto * devices = alcGetString(nullptr, useExtendedAPI ? ALC_ALL_DEVICES_SPECIFIER : ALC_DEVICE_SPECIFIER);

		if ( devices == nullptr )
		{
			Tracer::error(ClassId, "There is no audio devices!");

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

		const auto * defaultDeviceName = alcGetString(nullptr, useExtendedAPI ? ALC_DEFAULT_ALL_DEVICES_SPECIFIER : ALC_DEFAULT_DEVICE_SPECIFIER);

		if ( m_selectedDeviceName.empty() || m_selectedDeviceName == DefaultAudioDeviceName )
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
				TraceWarning{ClassId} << "The selected output audio device '" << m_selectedDeviceName << "' is not available anymore!";

				m_selectedDeviceName.assign(defaultDeviceName);
			}
		}

		s_audioSystemAvailable = true;

		return true;
	}

	bool
	Manager::setupAudioOutputDevice (Settings & settings) noexcept
	{
		/* Read setting for a desired output audio device. */
		m_selectedDeviceName = settings.getOrSetDefault< std::string >(AudioDeviceNameKey, DefaultAudioDeviceName);

		/* NOTE: Check the requested device or fall back to default. */
		if ( !this->selectedOutputDevice(true) && !this->selectedOutputDevice(false) )
		{
			Tracer::error(ClassId, "There is no audio output device on the system!");

			return false;
		}

		if ( m_showInformation )
		{
			std::cout << "[OpenAL] Audio devices:" "\n";

			for ( const auto & deviceName : m_availableDevices )
			{
				std::cout << " - " << deviceName << '\n';
			}

			std::cout << "Default: " << m_selectedDeviceName << '\n';
		}

		/* Checks configuration file */
		m_playbackFrequency = WaveFactory::toFrequency(settings.getOrSetDefault< int32_t >(AudioPlaybackFrequencyKey, DefaultAudioPlaybackFrequency));

		if ( m_playbackFrequency == WaveFactory::Frequency::Invalid )
		{
			TraceWarning{ClassId} <<
				"Invalid frequency in settings file! "
				"Leaving to default " << DefaultAudioPlaybackFrequency << " Hz.";

			m_playbackFrequency = WaveFactory::Frequency::PCM48000Hz;
		}

		/* NOTE: Opening the output audio device. */
		m_device = alcOpenDevice(m_selectedDeviceName.c_str());

		if ( alcGetErrors(m_device, "alcOpenDevice()", __FILE__, __LINE__) || m_device == nullptr )
		{
			TraceError{ClassId} << "Unable to open the output audio device '" << m_selectedDeviceName << "' !";

			return false;
		}

		if ( m_usingAdvancedEnumeration )
		{
			TraceSuccess{ClassId} << "The output audio device '" << alcGetString(m_device, ALC_ALL_DEVICES_SPECIFIER) << "' selected !";
		}
		else
		{
			TraceSuccess{ClassId} << "The output audio device '" << alcGetString(m_device, ALC_DEVICE_SPECIFIER) << "' selected !";
		}

		const std::array attributeList{
			ALC_FREQUENCY, static_cast< int >(m_playbackFrequency),
			ALC_REFRESH, settings.getOrSetDefault< int32_t >(OpenALRefreshRateKey, DefaultOpenALRefreshRate),
			ALC_SYNC, settings.getOrSetDefault< int32_t >(OpenALSyncStateKey, DefaultOpenALSyncState),
			ALC_MONO_SOURCES, settings.getOrSetDefault< int32_t >(OpenALMaxMonoSourceCountKey, DefaultOpenALMaxMonoSourceCount),
			ALC_STEREO_SOURCES, settings.getOrSetDefault< int32_t >(OpenALMaxStereoSourceCountKey, DefaultOpenALMaxStereoSourceCount),
			0
		};

		/* Context creation and set it as default. */
		m_context = alcCreateContext(m_device, attributeList.data());

		if ( alcGetErrors(m_device, "alcCreateContext()", __FILE__, __LINE__) || m_context == nullptr )
		{
			Tracer::error(ClassId, "Unable to create an audio context !");

			return false;
		}

		if ( alcMakeContextCurrent(m_context) == AL_FALSE || alcGetErrors(m_device, "alcMakeContextCurrent()", __FILE__, __LINE__) )
		{
			Tracer::error(ClassId, "Unable set the current audio context !");

			return false;
		}

		OpenAL::installExtensionEvents();

		/* OpenAL EFX extensions. */
		if ( settings.getOrSetDefault< bool >(OpenALUseEFXExtensionsKey, DefaultOpenALUseEFXExtensions) )
		{
			OpenAL::installExtensionSystemEvents(m_device);

			OpenAL::installExtensionEFX(m_device);
		}

		return this->saveContextAttributes();
	}

	bool
	Manager::onInitialize () noexcept
	{
		const auto & arguments = m_primaryServices.arguments();
		auto & settings = m_primaryServices.settings();

		m_showInformation =
			settings.getOrSetDefault< bool >(AudioShowInformationKey, DefaultAudioShowInformation) ||
			arguments.isSwitchPresent("--show-all-infos") ||
			arguments.isSwitchPresent("--show-audio-infos");

		if ( arguments.isSwitchPresent("--disable-audio") || !settings.getOrSetDefault< bool >(AudioEnableKey, DefaultAudioEnable) )
		{
			Tracer::warning(ClassId, "Audio manager disabled at startup.");

			return true;
		}

		/* NOTE: Select an audio device. */
		if ( !this->setupAudioOutputDevice(settings) )
		{
			Tracer::error(ClassId, "Unable to get an audio device or an audio context! Disabling the audio layer.");

			/* NOTE: Disable the previously enabled state. */
			s_audioSystemAvailable = false;

			return false;
		}

		/* NOTE: Be sure of the playback frequency allowed by this OpenAL context. */
		m_playbackFrequency = WaveFactory::toFrequency(m_contextAttributes[ALC_FREQUENCY]);
		m_musicChunkSize = settings.getOrSetDefault< uint32_t >(AudioMusicChunkSizeKey, DefaultAudioMusicChunkSize);

		/* NOTE: The recorder uses a loopback device with a thread-local context.
		 * It MUST be initialized before any alGen/alListener calls so that
		 * sources, buffers, and listener state are created in the correct context. */
		if ( m_recorder.initialize(m_subServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_recorder.name() << " service up !";
		}
		else
		{
			TraceWarning{ClassId} << m_recorder.name() << " service failed or disabled at startup!";
		}

		/* NOTE: Set up the audio configuration (after recorder, so listener state
		 * goes to the loopback context when the recorder is active). */
		this->setMetersPerUnit(1.0F);
		this->setMainLevel(settings.getOrSetDefault< float >(AudioMasterVolumeKey, DefaultAudioMasterVolume));

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
			Tracer::error(ClassId, "No audio source available at all! Disabling th audio layer.");

			/* NOTE: Disable the previously enabled state. */
			s_audioSystemAvailable = false;

			return false;
		}

		s_audioEnabled = true;

		this->registerToConsole();

		if ( m_externalInput.initialize(m_subServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_externalInput.name() << " service up !";
		}
		else
		{
			TraceWarning{ClassId} << m_externalInput.name() << " service failed or disabled at startup!";
		}

		if ( m_trackMixer.initialize(m_subServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_trackMixer.name() << " service up !";

			m_trackMixer.enableCrossFader(m_contextAttributes[ALC_STEREO_SOURCES] >= 2);
		}
		else
		{
			TraceWarning{ClassId} << m_trackMixer.name() << " service failed to execute !";
		}

		if ( settings.getOrSetDefault< bool >(AudioEnablePrebuiltSoundsKey, DefaultAudioEnablePrebuiltSounds) )
		{
			this->generateBuiltinSounds();
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

		if ( alcGetErrors(m_device, "GlobalInitFlush", __FILE__, __LINE__) )
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

		size_t error = 0;

		/* Terminate sub-services. */
		{
			for ( auto * service : std::ranges::reverse_view(m_subServicesEnabled) )
			{
				if ( service->terminate() )
				{
					TraceSuccess{ClassId} << service->name() << " sub-service terminated gracefully!";
				}
				else
				{
					error++;

					TraceError{ClassId} << service->name() << " sub-service failed to terminate properly!";
				}
			}

			m_subServicesEnabled.clear();
		}

		m_defaultSource.reset();

		/* NOTE: Check for missing errors from audio lib execution. */
		if ( alGetErrors("GlobalReleaseFlush", __FILE__, __LINE__) )
		{
			Tracer::warning(ClassId, "There was unread problem with AL during execution !");
		}

		if ( alcGetErrors(m_device, "GlobalReleaseFlush", __FILE__, __LINE__) )
		{
			Tracer::warning(ClassId, "There was unread problem with ALC during execution !");
		}

		auto & settings = m_primaryServices.settings();

		/* NOTE: Release the audio context. */
		alcMakeContextCurrent(nullptr);

		if ( m_context != nullptr )
		{
			alcDestroyContext(m_context);

			m_context = nullptr;
		}

		/* NOTE: Release the output audio device. */
		if ( m_device != nullptr )
		{
			if ( alcCloseDevice(m_device) == ALC_TRUE )
			{
				TraceSuccess{ClassId} << "The output audio device '" << m_selectedDeviceName << "' closed !";

				settings.set< std::string >(AudioDeviceNameKey, m_selectedDeviceName);
			}
			else
			{
				TraceError{ClassId} << "Unable to close the output audio device '" << m_selectedDeviceName << "' !";
			}

			m_device = nullptr;
		}

		return error == 0;
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
	Manager::generateBuiltinSounds () noexcept
	{
		if ( m_prebuiltSoundsGenerated )
		{
			return;
		}

		auto * soundResources = m_resourceManager.container< SoundResource >();

		const auto frequencyPlayback = Manager::frequencyPlayback();
		const auto sampleRate = static_cast< size_t >(frequencyPlayback);

		/* INFO sound: A gentle, pleasant two-tone chime (ascending).
		 * Musical interval: Perfect fifth (C5 -> G5). */
		{
			auto infoSound = soundResources->createResource(InfoSound);

			if ( infoSound != nullptr )
			{
				const auto noteDuration = sampleRate / 5;  /* 200ms per note */
				const auto totalDuration = noteDuration * 2;

				auto & localData = infoSound->localData();

				WaveFactory::Synthesizer synth{localData, totalDuration, frequencyPlayback};

				/* First note: C5 (523.25 Hz) */
				synth.setRegion(0, noteDuration);
				synth.sineWave(523.25F, 0.6F);
				synth.applyADSR(0.01F, 0.05F, 0.6F, 0.1F);

				/* Second note: G5 (783.99 Hz) */
				synth.setRegion(noteDuration, noteDuration);
				synth.sineWave(783.99F, 0.6F);
				synth.applyADSR(0.01F, 0.05F, 0.6F, 0.1F);

				synth.resetRegion();
				synth.normalize();

				infoSound->setManualLoadSuccess(true);
			}
		}

		/* ERROR sound: An alarming descending tritone (the "devil's interval").
		 * Creates tension and urgency. */
		{
			auto errorSound = soundResources->createResource(ErrorSound);

			if ( errorSound != nullptr )
			{
				const auto totalDuration = sampleRate / 4;  /* 250ms */

				auto & localData = errorSound->localData();

				WaveFactory::Synthesizer synth{localData, totalDuration, frequencyPlayback};

				/* Descending pitch sweep with square wave for harsh sound. */
				synth.pitchSweep(880.0F, 220.0F, 0.7F);

				/* Add some grit with bit crushing. */
				synth.applyBitCrush(10);

				/* Sharp attack, quick decay. */
				synth.applyADSR(0.005F, 0.1F, 0.4F, 0.1F);

				/* Mix with a lower rumble for weight. */
				WaveFactory::Wave< int16_t > rumble;
				WaveFactory::Synthesizer synthRumble{rumble, totalDuration, frequencyPlayback};
				synthRumble.pitchSweep(150.0F, 80.0F, 0.5F);
				synthRumble.applyADSR(0.01F, 0.1F, 0.3F, 0.1F);

				synth.mix(rumble, 0.6F);
				synth.normalize();

				errorSound->setManualLoadSuccess(true);
			}
		}

		/* WARNING sound: A pulsing alert tone, like a gentle alarm.
		 * Two quick beeps at the same pitch. */
		{
			auto warningSound = soundResources->createResource(WarningSound);

			if ( warningSound != nullptr )
			{
				const auto beepDuration = sampleRate / 12;  /* ~83ms per beep */
				const auto gapDuration = sampleRate / 20;   /* 50ms gap */
				const auto totalDuration = beepDuration * 2 + gapDuration;

				auto & localData = warningSound->localData();

				WaveFactory::Synthesizer synth{localData, totalDuration, frequencyPlayback};

				/* First beep: A4 (440 Hz) with triangle wave for softer tone. */
				synth.setRegion(0, beepDuration);
				synth.triangleWave(440.0F, 0.7F);
				synth.applyADSR(0.005F, 0.02F, 0.8F, 0.02F);

				/* Second beep after the gap. */
				synth.setRegion(beepDuration + gapDuration, beepDuration);
				synth.triangleWave(440.0F, 0.7F);
				synth.applyADSR(0.005F, 0.02F, 0.8F, 0.02F);

				synth.resetRegion();
				synth.normalize();

				warningSound->setManualLoadSuccess(true);
			}
		}

		/* SUCCESS sound: A triumphant ascending arpeggio (major chord).
		 * C5 -> E5 -> G5 (C major triad). */
		{
			auto successSound = soundResources->createResource(SuccessSound);

			if ( successSound != nullptr )
			{
				const auto noteDuration = sampleRate / 8;   /* 125ms per note */
				const auto totalDuration = noteDuration * 3;

				auto & localData = successSound->localData();

				WaveFactory::Synthesizer synth{localData, totalDuration, frequencyPlayback};

				/* First note: C5 (523.25 Hz). */
				synth.setRegion(0, noteDuration);
				synth.sineWave(523.25F, 0.5F);
				synth.applyADSR(0.01F, 0.03F, 0.7F, 0.05F);

				/* Second note: E5 (659.25 Hz). */
				synth.setRegion(noteDuration, noteDuration);
				synth.sineWave(659.25F, 0.5F);
				synth.applyADSR(0.01F, 0.03F, 0.7F, 0.05F);

				/* Third note: G5 (783.99 Hz). */
				synth.setRegion(noteDuration * 2, noteDuration);
				synth.sineWave(783.99F, 0.5F);
				synth.applyADSR(0.01F, 0.03F, 0.7F, 0.05F);

				/* Add a subtle shimmer with high frequency over all. */
				WaveFactory::Wave< int16_t > shimmer;
				WaveFactory::Synthesizer synthShimmer{shimmer, totalDuration, frequencyPlayback};
				synthShimmer.sineWave(1046.5F, 0.2F);  /* C6 - octave above. */
				synthShimmer.applyADSR(0.05F, 0.1F, 0.3F, 0.2F);

				synth.resetRegion();
				synth.mix(shimmer, 0.3F);
				synth.normalize();

				successSound->setManualLoadSuccess(true);
			}
		}

		m_prebuiltSoundsGenerated = true;

		TraceSuccess{ClassId} << "Builtin sounds generated: " << InfoSound << ", " << ErrorSound << ", " << WarningSound << ", " << SuccessSound;
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
			if ( const auto it = m_contextAttributes.find(ALC_MAJOR_VERSION); it == m_contextAttributes.end() )
			{
				alcGetIntegerv(m_device, ALC_MAJOR_VERSION, 1, &major);
			}
			else
			{
				major = m_contextAttributes.at(ALC_MAJOR_VERSION);
			}

			if ( const auto it = m_contextAttributes.find(ALC_MINOR_VERSION); it == m_contextAttributes.end() )
			{
				alcGetIntegerv(m_device, ALC_MINOR_VERSION, 1, &minor);
			}
			else
			{
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
			if ( const auto it = m_contextAttributes.find(ALC_EFX_MAJOR_VERSION); it == m_contextAttributes.end() )
			{
				alcGetIntegerv(m_device, ALC_EFX_MAJOR_VERSION, 1, &major);
			}
			else
			{
				major = m_contextAttributes.at(ALC_EFX_MAJOR_VERSION);
			}

			if ( const auto it = m_contextAttributes.find(ALC_EFX_MINOR_VERSION); it == m_contextAttributes.end() )
			{
				alcGetIntegerv(m_device, ALC_EFX_MINOR_VERSION, 1, &minor);
			}
			else
			{
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
	Manager::setEnvironmentSoundProperties (const Physics::EnvironmentPhysicalProperties & properties) noexcept
	{
		if ( !s_audioSystemAvailable )
		{
			return;
		}

		this->setDopplerFactor(properties.dopplerFactor());
		this->setSpeedOfSound(properties.speedOfSound());
		this->setDistanceModel(properties.distanceModel());
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
		bool extensionFound = false;
		const auto rawExtensions = alcGetString(nullptr, ALC_EXTENSIONS);

		if ( rawExtensions != nullptr )
		{
			const auto extensions = String::explode(rawExtensions, ' ', false);

			if ( !extensions.empty() )
			{
				output << "Available ALC extensions :" "\n";

				for ( const auto & extension : extensions )
				{
					output << " - " << extension << '\n';
				}

				extensionFound = true;
			}
		}

		if ( !extensionFound )
		{
			output << "No ALC extension available !" "\n";
		}

		return output.str();
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

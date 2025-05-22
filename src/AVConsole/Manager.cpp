/*
 * src/AVConsole/Manager.cpp
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

/* Local inclusions. */
#include <ranges>
#include <sstream>

/* Local inclusions. */
#include "Audio/HardwareOutput.hpp"

namespace EmEn::AVConsole
{
	using namespace EmEn::Libs;
	using namespace Graphics;

	const size_t Manager::ClassUID{getClassUID(ClassId)};
	const std::string Manager::DefaultViewName{"DefaultView"};
	const std::string Manager::DefaultSpeakerName{"DefaultSpeaker"};

	Manager::Manager (const std::string & name) noexcept
		: NameableTrait(name + ClassId),
		Controllable(ClassId)
	{
		/* Console commands bindings. */
		this->bindCommand("listDevices", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			DeviceType deviceType{DeviceType::Both};
			ConnexionType directionType{ConnexionType::Both};

			if ( !arguments.empty() )
			{
				{
					const auto argument = arguments[0].asString();

					if ( argument == "video" )
					{
						deviceType = DeviceType::Video;
					}
					else if ( argument == "audio" )
					{
						deviceType = DeviceType::Audio;
					}
					else if ( argument == "both" )
					{
						deviceType = DeviceType::Both;
					}
				}

				if ( arguments.size() > 1 )
				{
					const auto argument = std::any_cast< std::string >(arguments[1]);

					if ( argument == "input" )
					{
						directionType = ConnexionType::Input;
					}
					else if ( argument == "output" )
					{
						directionType = ConnexionType::Output;
					}
					else if ( argument == "both" )
					{
						directionType = ConnexionType::Both;
					}
				}
			}

			outputs.emplace_back(Severity::Info, this->getDeviceList(deviceType, directionType));

			return 0;
		}, "Get a list of input/output audio/video devices.");

		this->bindCommand("registerRoute", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.size() != 3 )
			{
				outputs.emplace_back(Severity::Error, "This method need 3 parameters.");

				return 1;
			}

			const auto type = arguments[0].asString();
			const auto source = arguments[1].asString();
			const auto target = arguments[2].asString();

			if ( type == "video" )
			{
				if ( !this->connectVideoDevices(source, target) )
				{
					outputs.emplace_back(Severity::Error, "Unable to connect the video device.");

					return 3;
				}
			}
			else if ( type == "audio" )
			{
				if ( !this->connectAudioDevices(source, target) )
				{
					outputs.emplace_back(Severity::Error, "Unable to connect the audio device.");

					return 3;
				}
			}
			else
			{
				outputs.emplace_back(Severity::Error, "First parameter must be 'video' or 'audio'.");

				return 2;
			}

			return 0;
		}, "Register a route from input device to output device.");
	}

	bool
	Manager::addVideoDevice (const std::shared_ptr< AbstractVirtualDevice > & device, bool primaryDevice) noexcept
	{
		if ( device->deviceType() != DeviceType::Video )
		{
			TraceWarning{ClassId} << "The virtual device '" << device->id() << "' is not a video device !";

			return false;
		}

		if ( !m_virtualVideoDevices.contains(device->id()) )
		{
			if ( !m_virtualVideoDevices.emplace(device->id(), device).second )
			{
				TraceWarning{ClassId} << "Unable to register the virtual video device '" << device->id() << "' !";

				return false;
			}

			/* NOTE: Save the video device to a specific container for rendering processLogics. */
			switch ( device->videoType() )
			{
				case VideoType::View :
				{
					auto renderToView = std::static_pointer_cast< RenderTarget::Abstract >(device);

					if ( m_renderToViews.emplace(renderToView).second )
					{
						this->notify(RenderToViewAdded, renderToView);
					}
					else
					{
						TraceError{ClassId} << "Unable to store the render to view '" << renderToView->id() << "' for rendering !";
					}
				}
					break;

				case VideoType::Texture :
				{
					auto renderToTexture = std::static_pointer_cast< RenderTarget::Abstract >(device);

					if ( m_renderToTextures.emplace(renderToTexture).second )
					{
						this->notify(RenderToTextureAdded, renderToTexture);
					}
					else
					{
						TraceError{ClassId} << "Unable to store the render to texture '" << device->id() << "' for rendering !";
					}
				}
					break;

				case VideoType::ShadowMap :
				{
					auto renderToShadowMap = std::static_pointer_cast< RenderTarget::Abstract >(device);

					if ( m_renderToShadowMaps.emplace(renderToShadowMap).second )
					{
						this->notify(RenderToShadowMapAdded, renderToShadowMap);
					}
					else
					{
						TraceError{ClassId} << "Unable to store the render to shadow map '" << device->id() << "' for rendering !";
					}
				}
					break;

				default :
					/* Nothing to do with this type of device. */
					break;
			}

			TraceSuccess{ClassId} << "New virtual video device '" << device->id() << "' available !";

			this->notify(VideoDeviceAdded, device);
		}
		else
		{
			TraceInfo{ClassId} << "Virtual video device '" << device->id() << "' already registered !";
		}

		/* NOTE: Set the device as primary if requested and even if it is already registered. */
		if ( primaryDevice )
		{
			switch ( device->allowedConnexionType() )
			{
				case ConnexionType::Output :
					TraceInfo{ClassId} << "Virtual video device '" << device->id() << "' declared as primary input !";

					m_primaryInputVideoDeviceId = device->id();
					break;

				case ConnexionType::Input :
				case ConnexionType::Both :
					TraceInfo{ClassId} << "Virtual video device '" << device->id() << "' declared as primary output !";

					m_primaryOutputVideoDeviceId = device->id();
					break;
			}
		}

		return true;
	}

	bool
	Manager::addAudioDevice (const std::shared_ptr< AbstractVirtualDevice > & device, bool primaryDevice) noexcept
	{
		if ( device->deviceType() != DeviceType::Audio )
		{
			TraceWarning{ClassId} << "The virtual device '" << device->id() << "' is not an audio device !";

			return false;
		}

		if ( !m_virtualAudioDevices.contains(device->id()) )
		{
			if ( !m_virtualAudioDevices.emplace(device->id(), device).second )
			{
				TraceWarning{ClassId} << "Unable to register the virtual audio device '" << device->id() << "' !";

				return false;
			}

			TraceSuccess{ClassId} << "New virtual audio device '" << device->id() << "' available !";

			this->notify(AudioDeviceAdded, device);
		}
		else
		{
			TraceInfo{ClassId} << "Virtual audio device '" << device->id() << "' already registered !";
		}

		/* NOTE: Set the device as primary if requested and even if it is already registered. */
		if ( primaryDevice )
		{
			switch ( device->allowedConnexionType() )
			{
				case ConnexionType::Output :
					TraceInfo{ClassId} << "Virtual audio device '" << device->id() << "' declared as primary input !";

					m_primaryInputAudioDeviceId = device->id();
					break;

				case ConnexionType::Input :
				case ConnexionType::Both :
					TraceInfo{ClassId} << "Virtual audio device '" << device->id() << "' declared as primary output !";

					m_primaryOutputAudioDeviceId = device->id();
					break;
			}
		}

		return true;
	}

	bool
	Manager::removeVideoDevice (const std::shared_ptr< AbstractVirtualDevice > & device) noexcept
	{
		if ( device->deviceType() != DeviceType::Video )
		{
			TraceWarning{ClassId} << "The virtual device '" << device->id() << "' is not a video device !";

			return false;
		}

		/* FIXME: This is clearly shit ! */
		{
			const auto abstractView = std::static_pointer_cast< RenderTarget::Abstract >(device);

			if ( abstractView != nullptr )
			{
				m_renderToViews.erase(abstractView);
			}
			else
			{
				const auto abstractTextures = std::static_pointer_cast< RenderTarget::Abstract >(device);

				if ( abstractTextures != nullptr )
				{
					m_renderToTextures.erase(abstractTextures);
				}
				else
				{
					const auto abstractShadowMaps = std::static_pointer_cast< RenderTarget::Abstract >(device);

					if ( abstractShadowMaps != nullptr )
					{
						m_renderToShadowMaps.erase(abstractShadowMaps);
					}
				}
			}
		}

		if ( m_virtualVideoDevices.erase(device->id()) <= 0 )
		{
			TraceInfo{ClassId} << "There is no virtual video device '" << device->id() << "' registered !";

			return false;
		}

		TraceSuccess{ClassId} << "Virtual video device '" << device->id() << "' removed !";

		this->notify(VideoDeviceRemoved, device);

		return true;
	}

	bool
	Manager::removeAudioDevice (const std::shared_ptr< AbstractVirtualDevice > & device) noexcept
	{
		if ( device->deviceType() != DeviceType::Audio )
		{
			TraceWarning{ClassId} << "The virtual device '" << device->id() << "' is not an audio device !";

			return false;
		}

		if ( m_virtualAudioDevices.erase(device->id()) <= 0 )
		{
			TraceInfo{ClassId} << "There is no virtual audio device '" << device->id() << "' registered !";

			return false;
		}

		TraceSuccess{ClassId} << "Virtual audio device '" << device->id() << "' removed !";

		this->notify(AudioDeviceRemoved, device);

		return true;
	}

	std::shared_ptr< RenderTarget::View< ViewMatrices2DUBO > >
	Manager::createRenderToView (Renderer & renderer, const std::string & name, uint32_t width, uint32_t height, const FramebufferPrecisions & precisions, bool primaryDevice) noexcept
	{
		if ( m_virtualVideoDevices.contains(name) )
		{
			TraceError{ClassId} << "A virtual device named '" << name << "' already exists ! Render to view creation canceled ...";

			return {};
		}

		/* Create the render target. */
		auto renderTarget = std::make_shared< RenderTarget::View< ViewMatrices2DUBO > >(name, width, height, precisions);

		if ( !renderTarget->create(renderer) )
		{
			TraceError{ClassId} << "Unable to create the render to view '" << name << "' !";

			return {};
		}

		if ( !this->addVideoDevice(renderTarget, primaryDevice) )
		{
			TraceError{ClassId} << "Unable to add the render to view '" << name << "' as a virtual video device !";

			return {};
		}

		return renderTarget;
	}

	std::shared_ptr< RenderTarget::View< ViewMatrices3DUBO > >
	Manager::createRenderToCubicView (Renderer & renderer, const std::string & name, uint32_t size, const FramebufferPrecisions & precisions, bool primaryDevice) noexcept
	{
		/* Checks name availability. */
		if ( m_virtualVideoDevices.contains(name) )
		{
			TraceError{ClassId} << "A virtual device named '" << name << "' already exists ! Render to cubic view creation canceled ...";

			return {};
		}

		/* Create the render target. */
		auto renderTarget = std::make_shared< RenderTarget::View< ViewMatrices3DUBO > >(name, size, precisions);

		if ( !renderTarget->create(renderer) )
		{
			TraceError{ClassId} << "Unable to create the render to cubic view '" << name << "' !";

			return {};
		}

		if ( !this->addVideoDevice(renderTarget, primaryDevice) )
		{
			TraceError{ClassId} << "Unable to add the render to cubic view '" << name << "' as a virtual video device !";

			return {};
		}

		return renderTarget;
	}

	std::shared_ptr< RenderTarget::Texture< ViewMatrices2DUBO > >
	Manager::createRenderToTexture2D (Renderer & renderer, const std::string & name, uint32_t width, uint32_t height, uint32_t colorCount) noexcept
	{
		if ( m_virtualVideoDevices.contains(name) )
		{
			TraceError{ClassId} <<
				"A virtual video device named '" << name << "' already exists ! "
				"Render to texture 2D creation canceled ...";

			return {};
		}

		/* Create the render target. */
		auto renderTarget = std::make_shared< RenderTarget::Texture< ViewMatrices2DUBO > >(name, width, height, colorCount);

		if ( !renderTarget->enableManualLoading() )
		{
			return {};
		}

		auto success = renderTarget->createOnHardware(renderer);

		if ( !renderTarget->setManualLoadSuccess(success) )
		{
			return {};
		}

		if ( !this->addVideoDevice(renderTarget) )
		{
			TraceError{ClassId} << "Unable to add the render to texture 2D '" << name << "' as a virtual video device !";

			return {};
		}

		return renderTarget;
	}

	std::shared_ptr< RenderTarget::Texture< ViewMatrices3DUBO > >
	Manager::createRenderToCubemap (Renderer & renderer, const std::string & name, uint32_t size, uint32_t colorCount) noexcept
	{
		if ( m_virtualVideoDevices.contains(name) )
		{
			TraceError{ClassId} <<
				"A virtual video device named '" << name << "' already exists ! "
				"Render to cubemap creation canceled ...";

			return {};
		}

		/* Create the render target. */
		auto renderTarget = std::make_shared< RenderTarget::Texture< ViewMatrices3DUBO > >(name, size, colorCount);

		if ( !renderTarget->createOnHardware(renderer) )
		{
			TraceError{ClassId} << "Unable to create the render to cubemap '" << name << "' !";

			return {};
		}

		if ( !this->addVideoDevice(renderTarget) )
		{
			TraceError{ClassId} << "Unable to add the render to cubemap '" << name << "' as a virtual video device !";

			return {};
		}

		return renderTarget;
	}

	std::shared_ptr< RenderTarget::ShadowMap< ViewMatrices2DUBO > >
	Manager::createRenderToShadowMap (Renderer & renderer, const std::string & name, uint32_t resolution) noexcept
	{
		if ( m_virtualVideoDevices.contains(name) )
		{
			TraceError{ClassId} <<
				"A virtual video device named '" << name << "' already exists ! "
				"Render to shadow map creation canceled ...";

			return {};
		}

		/* Create the render target. */
		auto renderTarget = std::make_shared< RenderTarget::ShadowMap< ViewMatrices2DUBO > >(name, resolution);

		if ( !renderTarget->create(renderer) )
		{
			TraceError{ClassId} << "Unable to create the render to shadow map '" << name << "' !";

			return {};
		}

		if ( !this->addVideoDevice(renderTarget) )
		{
			TraceError{ClassId} << "Unable to add the render to shadow map '" << name << "' as a virtual video device !";

			return {};
		}

		return renderTarget;
	}

	std::shared_ptr< RenderTarget::ShadowMap< ViewMatrices3DUBO > >
	Manager::createRenderToCubicShadowMap (Renderer & renderer, const std::string & name, uint32_t resolution) noexcept
	{
		if ( m_virtualVideoDevices.contains(name) )
		{
			TraceError{ClassId} <<
				"A virtual video device named '" << name << "' already exists ! "
				"Render to cubic shadow map creation canceled ...";

			return {};
		}

		/* Create the render target. */
		auto renderTarget = std::make_shared< RenderTarget::ShadowMap< ViewMatrices3DUBO > >(name, resolution);

		if ( !renderTarget->create(renderer) )
		{
			TraceError{ClassId} << "Unable to create the render to cubic shadow map '" << name << "' !";

			return {};
		}

		if ( !this->addVideoDevice(renderTarget) )
		{
			TraceError{ClassId} << "Unable to add the render to cubic shadow map '" << name << "' as a virtual video device !";

			return {};
		}

		return renderTarget;
	}

	std::shared_ptr< AbstractVirtualDevice >
	Manager::getVideoDevice (const std::string & deviceId) const noexcept
	{
		const auto deviceIt = m_virtualVideoDevices.find(deviceId);

		if ( deviceIt == m_virtualVideoDevices.cend() )
		{
			return nullptr;
		}

		return deviceIt->second;
	}

	std::shared_ptr< AbstractVirtualDevice >
	Manager::getAudioDevice (const std::string & deviceId) const noexcept
	{
		const auto deviceIt = m_virtualAudioDevices.find(deviceId);

		if ( deviceIt == m_virtualAudioDevices.cend() )
		{
			return nullptr;
		}

		return deviceIt->second;
	}

	std::vector< std::shared_ptr< AbstractVirtualDevice > >
	Manager::getVideoDeviceSources () const noexcept
	{
		std::vector< std::shared_ptr< AbstractVirtualDevice > > list{};
		list.reserve(m_virtualVideoDevices.size());

		for ( const auto & device : std::ranges::views::values(m_virtualVideoDevices) )
		{
			if ( device->allowedConnexionType() == ConnexionType::Output )
			{
				list.emplace_back(device);
			}
		}

		return list;
	}

	std::vector< std::shared_ptr< AbstractVirtualDevice > >
	Manager::getAudioDeviceSources () const noexcept
	{
		std::vector< std::shared_ptr< AbstractVirtualDevice > > list{};
		list.reserve(m_virtualAudioDevices.size());

		for ( const auto & device : std::ranges::views::values(m_virtualAudioDevices) )
		{
			if ( device->allowedConnexionType() == ConnexionType::Output )
			{
				list.emplace_back(device);
			}
		}

		return list;
	}

	bool
	Manager::connectVideoDevices (const std::string & sourceDeviceId, const std::string & targetDeviceId) const noexcept
	{
		const auto sourceDevice = this->getVideoDevice(sourceDeviceId);

		if ( sourceDevice == nullptr )
		{
			TraceError{ClassId} << "Unable to find virtual video device '" << sourceDeviceId << "' as source device to connect !";

			return false;
		}

		const auto targetDevice = this->getVideoDevice(targetDeviceId);

		if ( targetDevice == nullptr )
		{
			TraceError{ClassId} << "Unable to find virtual video device '" << targetDeviceId << "' as target device to connect !";

			return false;
		}

		if ( sourceDevice->isConnectedWith(targetDevice, ConnexionType::Output) )
		{
			return true;
		}

		return sourceDevice->connect(targetDevice);
	}

	bool
	Manager::connectAudioDevices (const std::string & sourceDeviceId, const std::string & targetDeviceId) const noexcept
	{
		const auto sourceDevice = this->getAudioDevice(sourceDeviceId);

		if ( sourceDevice == nullptr )
		{
			TraceError{ClassId} << "Unable to find virtual audio device '" << sourceDeviceId << "' as source device to connect !";

			return false;
		}

		const auto targetDevice = this->getAudioDevice(targetDeviceId);

		if ( targetDevice == nullptr )
		{
			TraceError{ClassId} << "Unable to find virtual audio device '" << targetDeviceId << "' as target device to connect !";

			return false;
		}

		if ( sourceDevice->isConnectedWith(targetDevice, ConnexionType::Output) )
		{
			return true;
		}

		return sourceDevice->connect(targetDevice);
	}

	bool
	Manager::createDefaultView (Renderer & graphicsRenderer, Settings & settings) noexcept
	{
		const auto deviceIt = m_virtualVideoDevices.find(DefaultViewName);

		if ( deviceIt != m_virtualVideoDevices.cend() )
		{
			return true;
		}

		/* Read the default configuration. */
		const auto isFullscreen = settings.get< bool >(VideoFullscreenEnabledKey, DefaultVideoFullscreenEnabled);

		const auto width = isFullscreen ?
			settings.get< uint32_t >(VideoFullscreenWidthKey, DefaultVideoFullscreenWidth) :
			settings.get< uint32_t >(VideoWindowWidthKey, DefaultVideoWindowWidth);

		const auto height = isFullscreen ?
			settings.get< uint32_t >(VideoFullscreenHeightKey, DefaultVideoFullscreenHeight) :
			settings.get< uint32_t >(VideoWindowHeightKey, DefaultVideoWindowHeight);

		const FramebufferPrecisions precisions{
			settings.get< uint32_t >(VideoFramebufferRedBitsKey, DefaultVideoFramebufferRedBits),
			settings.get< uint32_t >(VideoFramebufferGreenBitsKey, DefaultVideoFramebufferGreenBits),
			settings.get< uint32_t >(VideoFramebufferBlueBitsKey, DefaultVideoFramebufferBlueBits),
			settings.get< uint32_t >(VideoFramebufferAlphaBitsKey, DefaultVideoFramebufferAlphaBits),
			settings.get< uint32_t >(VideoFramebufferDepthBitsKey, DefaultVideoFramebufferDepthBits),
			settings.get< uint32_t >(VideoFramebufferStencilBitsKey, DefaultVideoFramebufferStencilBits),
			settings.get< uint32_t >(VideoFramebufferSamplesKey, DefaultVideoFramebufferSamples)
		};

		return this->createRenderToView(graphicsRenderer, DefaultViewName, width, height, precisions, true) != nullptr;
	}

	bool
	Manager::createDefaultSpeaker (Audio::Manager & audioManager, Settings & /*settings*/) noexcept
	{
		const auto deviceIt = m_virtualAudioDevices.find(DefaultSpeakerName);

		if ( deviceIt != m_virtualAudioDevices.cend() )
		{
			return true;
		}

		if ( !audioManager.usable() )
		{
			return false;
		}

		return this->addAudioDevice(std::make_shared< Audio::HardwareOutput >(DefaultSpeakerName, audioManager), true);
	}

	bool
	Manager::autoConnectPrimaryVideoDevices (Renderer & renderer, Settings & settings) noexcept
	{
		if ( !this->autoSelectPrimaryInputVideoDevice() )
		{
			Tracer::error(ClassId, "There is no input primary video device declared !");

			return false;
		}

		if ( m_primaryOutputVideoDeviceId.empty() )
		{
			Tracer::info(ClassId, "There is no output primary video device declared ! Creating a view ...");

			if ( !this->createDefaultView(renderer, settings) )
			{
				Tracer::error(ClassId, "Unable to create the default view !");

				return false;
			}
		}

		return this->connectVideoDevices(m_primaryInputVideoDeviceId, m_primaryOutputVideoDeviceId);
	}

	bool
	Manager::autoConnectPrimaryAudioDevices (Audio::Manager & audioManager, Settings & settings) noexcept
	{
		if ( !this->autoSelectPrimaryInputAudioDevice() )
		{
			Tracer::error(ClassId, "There is no input primary audio device declared !");

			return false;
		}

		if ( m_primaryOutputAudioDeviceId.empty() )
		{
			Tracer::info(ClassId, "There is no output primary audio device declared ! Creating a speaker ...");

			if ( !this->createDefaultSpeaker(audioManager, settings) )
			{
				Tracer::error(ClassId, "Unable to create the default speaker !");

				return false;
			}
		}

		return this->connectAudioDevices(m_primaryInputAudioDeviceId, m_primaryOutputAudioDeviceId);
	}

	std::string
	Manager::getConnexionStates () const noexcept
	{
		std::stringstream string;

		string << "Video routes :" "\n";

		for ( const auto & device : this->getVideoDeviceSources() )
		{
			string << device->getConnexionState();
		}

		string << "Audio routes :" "\n";

		for ( const auto & device : this->getAudioDeviceSources() )
		{
			string << device->getConnexionState();
		}

		return string.str();
	}

	std::string
	Manager::getDeviceList (DeviceType deviceType, ConnexionType directionType) const noexcept
	{
		std::stringstream string;

		if ( deviceType == DeviceType::Video || deviceType == DeviceType::Both )
		{
			if ( directionType == ConnexionType::Input || directionType == ConnexionType::Both )
			{
				size_t count = 0;

				string << "Video input devices :" "\n";

				for ( const auto & [name, device] : m_virtualVideoDevices )
				{
					const auto deviceConnexionType = device->allowedConnexionType();

					if ( deviceConnexionType == ConnexionType::Input || deviceConnexionType == ConnexionType::Both )
					{
						string << " - '" << name << "'" "\n";

						count++;
					}
				}

				if ( count == 0 )
				{
					string << " None !" "\n";
				}
			}

			if ( directionType == ConnexionType::Output || directionType == ConnexionType::Both )
			{
				size_t count = 0;

				string << "Video output devices :" "\n";

				for ( const auto & [name, device] : m_virtualVideoDevices )
				{
					const auto deviceConnexionType = device->allowedConnexionType();

					if ( deviceConnexionType == ConnexionType::Output || deviceConnexionType == ConnexionType::Both )
					{
						string << " - '" << name << "'" "\n";

						count++;
					}
				}

				if ( count == 0 )
				{
					string << " None !" "\n";
				}
			}
		}

		if ( deviceType == DeviceType::Audio || deviceType == DeviceType::Both )
		{
			if ( directionType == ConnexionType::Input || directionType == ConnexionType::Both )
			{
				size_t count = 0;

				string << "Audio input devices :" "\n";

				for ( const auto & [name, device] : m_virtualAudioDevices )
				{
					const auto deviceConnexionType = device->allowedConnexionType();

					if ( deviceConnexionType == ConnexionType::Input || deviceConnexionType == ConnexionType::Both )
					{
						string << " - '" << name << "'" "\n";

						count++;
					}
				}

				if ( count == 0 )
				{
					string << " None !" "\n";
				}
			}

			if ( directionType == ConnexionType::Output || directionType == ConnexionType::Both )
			{
				size_t count = 0;

				string << "Audio output devices :" "\n";

				for ( const auto & [name, device] : m_virtualAudioDevices )
				{
					const auto deviceConnexionType = device->allowedConnexionType();

					if ( deviceConnexionType == ConnexionType::Output || deviceConnexionType == ConnexionType::Both )
					{
						string << " - '" << name << "'" "\n";

						count++;
					}
				}

				if ( count == 0 )
				{
					string << " None !" "\n";
				}
			}
		}

		return string.str();
	}

	void
	Manager::clear () noexcept
	{
		/* NOTE: Clearing the primary device names. */
		m_primaryOutputAudioDeviceId.clear();
		m_primaryInputAudioDeviceId.clear();
		m_primaryOutputVideoDeviceId.clear();
		m_primaryInputVideoDeviceId.clear();

		/* NOTE: Removing the specific devices. */
		m_renderToShadowMaps.clear();
		m_renderToTextures.clear();
		m_renderToViews.clear();

		for ( const auto & device : std::ranges::views::values(m_virtualAudioDevices) )
		{
			device->disconnectFromAll();
		}

		m_virtualAudioDevices.clear();

		for ( const auto & device : std::ranges::views::values(m_virtualVideoDevices) )
		{
			device->disconnectFromAll();
		}

		m_virtualVideoDevices.clear();
	}

	bool
	Manager::onNotification (const ObservableTrait * observable, int notificationCode, const std::any & /*data*/) noexcept
	{
		/* NOTE: Don't know what is it, goodbye! */
		TraceDebug{ClassId} <<
			"Received an unhandled notification (Code:" << notificationCode << ") from observable '" << whoIs(observable->classUID()) << "' (UID:" << observable->classUID() << ")  ! "
			"Forgetting it ...";

		return false;
	}

	void
	Manager::onRegisterToConsole () noexcept
	{

	}

	bool
	Manager::autoSelectPrimaryInputVideoDevice () noexcept
	{
		if ( m_primaryInputVideoDeviceId.empty() )
		{
			const auto selectedDeviceIt = std::ranges::find_if(m_virtualVideoDevices, [] (const auto & deviceIt) {
				return deviceIt.second->allowedConnexionType() == ConnexionType::Output;
			});

			if ( selectedDeviceIt == m_virtualVideoDevices.cend() )
			{
				return false;
			}

			m_primaryInputVideoDeviceId = selectedDeviceIt->first;
		}

		return true;
	}

	bool
	Manager::autoSelectPrimaryInputAudioDevice () noexcept
	{
		if ( m_primaryInputAudioDeviceId.empty() )
		{
			const auto selectedDeviceIt = std::ranges::find_if(m_virtualAudioDevices, [] (const auto & deviceIt) {
				return deviceIt.second->allowedConnexionType() == ConnexionType::Output;
			});

			if ( selectedDeviceIt == m_virtualAudioDevices.cend() )
			{
				return false;
			}

			m_primaryInputAudioDeviceId = selectedDeviceIt->first;
		}

		return true;
	}
}

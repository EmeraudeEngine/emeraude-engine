/*
 * src/Scenes/Component/AbstractLightEmitter.cpp
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

#include "AbstractLightEmitter.hpp"

/* Local inclusions. */
#include "Graphics/BindlessTextureManager.hpp"
#include "Graphics/SharedUniformBuffer.hpp"
#include "Resources/ResourceTrait.hpp"
#include "Scenes/AVConsole/Manager.hpp"
#include "Tracer.hpp"
#include "Vulkan/TextureInterface.hpp"

namespace EmEn::Scenes::Component
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Graphics;
	
	void
	AbstractLightEmitter::updateDeviceFromCoordinates (const CartesianFrame< float > & worldCoordinates, const Vector< 3, float > & worldVelocity) noexcept
	{
		if ( !this->hasOutputConnected() )
		{
			return;
		}

		/* NOTE: We send the new light coordinates to update the matrices of render targets. */
		this->forEachOutputs([&worldCoordinates, &worldVelocity] (const auto & output) {
			output->updateDeviceFromCoordinates(worldCoordinates, worldVelocity);
		});
	}

	void
	AbstractLightEmitter::onOutputDeviceConnected (EngineContext & /*engineContext*/, AbstractVirtualDevice & targetDevice) noexcept
	{
		/* When the shadow map is connected, we initialize it with coordinates and light properties. */
		targetDevice.updateVideoDeviceProperties(this->getFovOrNear(), this->getDistanceOrFar(), this->isOrthographicProjection());
		targetDevice.updateDeviceFromCoordinates(this->getWorldCoordinates(), this->getWorldVelocity());
	}

	bool
	AbstractLightEmitter::addToSharedUniformBuffer (const std::shared_ptr< SharedUniformBuffer > & sharedBufferUniform) noexcept
	{
		m_sharedUniformBuffer = sharedBufferUniform;

		if ( m_sharedUniformBuffer == nullptr )
		{
			TraceError{TracerTag} << "The shared uniform buffer smart pointer is null !";

			return false;
		}

		if ( !m_sharedUniformBuffer->addElement(this, m_sharedUBOIndex) )
		{
			TraceError{TracerTag} << "Unable to add the light in the shared uniform buffer !";

			return false;
		}

		return true;
	}

	void
	AbstractLightEmitter::removeFromSharedUniformBuffer () noexcept
	{
		if ( m_sharedUniformBuffer == nullptr )
		{
			return;
		}

		m_sharedUniformBuffer->removeElement(this);
		m_sharedUniformBuffer.reset();
	}

	bool
	AbstractLightEmitter::updateVideoMemory () noexcept
	{
		if ( this->isFlagEnabled(VideoMemoryUpdateRequested) )
		{
			this->disableFlag(VideoMemoryUpdateRequested);

			if ( m_sharedUniformBuffer == nullptr || !this->onVideoMemoryUpdate(*m_sharedUniformBuffer, m_sharedUBOIndex) )
			{
				return false;
			}
		}

		return true;
	}

	void
	AbstractLightEmitter::enable (bool state) noexcept
	{
		this->setFlag(Enabled, state);

		if ( state )
		{
			this->enableFlag(VideoMemoryUpdateRequested);
		}
	}

	bool
	AbstractLightEmitter::toggle () noexcept
	{
		if ( this->isFlagEnabled(Enabled) )
		{
			this->disableFlag(Enabled);

			return false;
		}

		this->enableFlag(Enabled);
		this->enableFlag(VideoMemoryUpdateRequested);

		return true;
	}

	void
	AbstractLightEmitter::setColor (const Libs::PixelFactory::Color< float > & color) noexcept
	{
		m_color = color;

		this->onColorChange(m_color);

		this->requestVideoMemoryUpdate();
	}

	void
	AbstractLightEmitter::setIntensity (float intensity) noexcept
	{
		m_intensity = intensity;

		this->onIntensityChange(m_intensity);

		this->requestVideoMemoryUpdate();
	}

	uint32_t
	AbstractLightEmitter::UBOAlignment () const noexcept
	{
		if ( m_sharedUniformBuffer == nullptr )
		{
			return 0;
		}

		return m_sharedUniformBuffer->blockAlignedSize();
	}

	uint32_t
	AbstractLightEmitter::UBOOffset () const noexcept
	{
		if ( m_sharedUniformBuffer == nullptr )
		{
			return 0;
		}

		return m_sharedUBOIndex * m_sharedUniformBuffer->blockAlignedSize();
	}

	const Vulkan::DescriptorSet *
	AbstractLightEmitter::descriptorSet ([[maybe_unused]] bool useShadowMap) const noexcept
	{
		if ( m_sharedUniformBuffer == nullptr )
		{
			return nullptr;
		}

		return m_sharedUniformBuffer->descriptorSet(m_sharedUBOIndex);
	}

	void
	AbstractLightEmitter::requestVideoMemoryUpdate () noexcept
	{
		if ( this->isEnabled() )
		{
			this->enableFlag(VideoMemoryUpdateRequested);
		}
	}

	void
	AbstractLightEmitter::setColorProjectionTexture (const std::shared_ptr< Vulkan::TextureInterface > & texture) noexcept
	{
		m_colorProjectionTexture = texture;

		if ( m_colorProjectionTexture != nullptr && m_bindlessTextureManager != nullptr )
		{
			auto * resource = dynamic_cast< Resources::ResourceTrait * >(m_colorProjectionTexture.get());

			if ( resource != nullptr && resource->isLoaded() )
			{
				this->registerColorProjectionInBindless();
			}
			else if ( resource != nullptr )
			{
				this->observe(resource);
			}
		}

		this->enableFlag(VideoMemoryUpdateRequested);
	}

	void
	AbstractLightEmitter::registerColorProjectionInBindless () noexcept
	{
		if ( m_bindlessTextureManager == nullptr || m_colorProjectionTexture == nullptr )
		{
			return;
		}

		if ( this->usesCubemapColorProjection() )
		{
			/* Check if the texture is a cube array (animated cubemap) or a regular cube. */
			if ( m_colorProjectionTexture->type() == Vulkan::TextureType::TextureCubeArray )
			{
				m_colorProjectionBindlessIndex = m_bindlessTextureManager->registerTextureCubeArray(*m_colorProjectionTexture);
				m_colorProjectionIsCubeArray = true;
				m_colorProjectionFrameIndex = 0;
			}
			else
			{
				m_colorProjectionBindlessIndex = m_bindlessTextureManager->registerTextureCube(*m_colorProjectionTexture);
				m_colorProjectionIsCubeArray = false;
				m_colorProjectionFrameIndex = NoColorProjectionTexture;
			}
		}
		else
		{
			m_colorProjectionBindlessIndex = m_bindlessTextureManager->registerTexture2D(*m_colorProjectionTexture);
			m_colorProjectionIsCubeArray = false;
			m_colorProjectionFrameIndex = NoColorProjectionTexture;
		}

		if ( m_colorProjectionBindlessIndex != NoColorProjectionTexture )
		{
			TraceDebug{TracerTag} << "Color projection texture registered in bindless manager at index " << m_colorProjectionBindlessIndex << (m_colorProjectionIsCubeArray ? " (animated cube array)." : ".");
		}
		else
		{
			TraceError{TracerTag} << "Failed to register color projection texture in bindless manager!";
		}

		this->requestVideoMemoryUpdate();
	}


	void
	AbstractLightEmitter::unregisterColorProjectionFromBindless (bool useCubemap) noexcept
	{
		if ( m_colorProjectionBindlessIndex != NoColorProjectionTexture && m_bindlessTextureManager != nullptr )
		{
			if ( useCubemap && m_colorProjectionIsCubeArray )
			{
				m_bindlessTextureManager->unregisterTextureCubeArray(m_colorProjectionBindlessIndex);
			}
			else if ( useCubemap )
			{
				m_bindlessTextureManager->unregisterTextureCube(m_colorProjectionBindlessIndex);
			}
			else
			{
				m_bindlessTextureManager->unregisterTexture2D(m_colorProjectionBindlessIndex);
			}

			m_colorProjectionBindlessIndex = NoColorProjectionTexture;
			m_colorProjectionFrameIndex = NoColorProjectionTexture;
			m_colorProjectionIsCubeArray = false;
		}

		if ( m_colorProjectionTexture != nullptr )
		{
			auto * resource = dynamic_cast< Resources::ResourceTrait * >(m_colorProjectionTexture.get());

			if ( resource != nullptr )
			{
				this->forget(resource);
			}
		}
	}
	bool
	AbstractLightEmitter::onNotification (const Libs::ObservableTrait * /*observable*/, int notificationCode, const std::any & /*data*/) noexcept
	{
		if ( notificationCode == Resources::ResourceTrait::LoadFinished )
		{
			this->registerColorProjectionInBindless();

			return false;
		}

		if ( notificationCode == Resources::ResourceTrait::LoadFailed )
		{
			TraceError{TracerTag} << "Color projection texture loading failed !";

			return false;
		}

		return true;
	}
}

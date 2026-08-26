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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "AbstractLightEmitter.hpp"

#include <algorithm>

/* Local inclusions. */
#include "Scenes/BindlessTextureSet.hpp"
#include "Graphics/SharedUniformBuffer.hpp"
#include "Resources/ResourceTrait.hpp"
#include "Scenes/AVConsole/Manager.hpp"
#include "Tracer.hpp"
#include "Vulkan/TextureInterface.hpp"

namespace EmEn::Scenes::Component
{
	using namespace Base;
	using namespace Base::Math;
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

	void
	AbstractLightEmitter::publishStateForRendering (uint32_t writeStateIndex) noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( writeStateIndex >= m_publishedBlocks.size() )
			{
				Tracer::error(this->getComponentType(), "Index overflow !");

				return;
			}
		}

		/* Nothing changed since this slot was last filled. */
		if ( m_publishedGeneration[writeStateIndex] == m_logicGeneration )
		{
			return;
		}

		this->writeUniformBlock(m_publishedBlocks[writeStateIndex].data());

		m_publishedGeneration[writeStateIndex] = m_logicGeneration;
	}

	bool
	AbstractLightEmitter::primeVideoMemory () noexcept
	{
		for ( uint32_t slot = 0; slot < m_publishedBlocks.size(); ++slot )
		{
			this->publishStateForRendering(slot);
		}

		if ( m_sharedUniformBuffer == nullptr )
		{
			return true;
		}

		/* ⚠️⚠️ Bound by the buffer's REAL region count, never by MaxFrameRegionCount — that constant
		 * only sizes the bookkeeping array. Looping to it wrote past the end of the buffer and left
		 * the cursor on region 7 of a 3-region buffer, so every subsequent bind carried a dynamic
		 * offset outside the allocation: caught by
		 * VUID-vkCmdBindDescriptorSets-pDescriptorSets-01979, on a frame that still looked correct.
		 * EVERY existing region must hold valid bytes from the first frame on, not just the one the
		 * next frame happens to use. */
		const auto regionCount = std::min< uint32_t >(m_sharedUniformBuffer->frameCount(), MaxFrameRegionCount);

		for ( uint32_t region = 0; region < regionCount; ++region )
		{
			if ( !this->updateVideoMemory(0, region) )
			{
				return false;
			}
		}

		/* Leave the cursor on a region the next frame can legally bind, whatever the loop ended on. */
		m_currentFrameRegion = 0;

		return true;
	}

	bool
	AbstractLightEmitter::updateVideoMemory (uint32_t readStateIndex, uint32_t frameIndex) noexcept
	{
		if ( readStateIndex >= m_publishedBlocks.size() || frameIndex >= MaxFrameRegionCount )
		{
			Tracer::error(this->getComponentType(), "Index overflow !");

			return false;
		}

		if ( m_sharedUniformBuffer != nullptr && frameIndex >= m_sharedUniformBuffer->frameCount() )
		{
			Tracer::error(this->getComponentType(), "Frame region beyond what the shared uniform buffer carries !");

			return false;
		}

		/* ⚠️ Set BEFORE the early returns: every bind of this frame reads it, whether or not this
		 * light needed an upload. Leaving it stale would point the dynamic offset at another
		 * frame's region — the very memory this partitioning exists to stop sharing. */
		m_currentFrameRegion = frameIndex;

		if ( m_sharedUniformBuffer == nullptr )
		{
			return true;
		}

		/* This region already holds exactly this generation. */
		if ( m_uploadedGeneration[frameIndex] == m_publishedGeneration[readStateIndex] )
		{
			return true;
		}

		if ( !m_sharedUniformBuffer->writeElementData(m_sharedUBOIndex, frameIndex, m_publishedBlocks[readStateIndex].data()) )
		{
			return false;
		}

		m_uploadedGeneration[frameIndex] = m_publishedGeneration[readStateIndex];

		return true;
	}

	void
	AbstractLightEmitter::enable (bool state) noexcept
	{
		this->setFlag(Enabled, state);

		if ( state )
		{
			++m_logicGeneration;
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

		++m_logicGeneration;

		return true;
	}

	void
	AbstractLightEmitter::setColor (const Base::PixelFactory::Color< float > & color) noexcept
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

		/* ⚠️ The offset is LOCAL to the bank the element lives in, exactly like
		 * SharedUniformBuffer::getByteOffsetForElement(). descriptorSet() already selects the bank
		 * from the GLOBAL index; feeding the global index to the offset too made the two disagree as
		 * soon as a scene held more lights of one type than a single 64 KiB bank can carry, pushing
		 * the dynamic offset past the end of the buffer (VUID-vkCmdBindDescriptorSets-pDynamicOffsets)
		 * or, worse, silently onto another light's block. */
		return static_cast< uint32_t >(m_sharedUniformBuffer->getByteOffsetForElement(m_sharedUBOIndex, m_currentFrameRegion));
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
			/* ⚠️ A generation, not a flag — see m_logicGeneration. Overflow is a non-issue: the
			 * comparison is for EQUALITY, so a wrap only ever costs one redundant re-publish. */
			++m_logicGeneration;
		}
	}

	void
	AbstractLightEmitter::setColorProjectionTexture (const std::shared_ptr< Vulkan::TextureInterface > & texture) noexcept
	{
		m_colorProjectionTexture = texture;

		if ( m_colorProjectionTexture != nullptr && m_bindlessTextureSet != nullptr )
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

		++m_logicGeneration;
	}

	void
	AbstractLightEmitter::registerColorProjectionInBindless () noexcept
	{
		if ( m_bindlessTextureSet == nullptr || m_colorProjectionTexture == nullptr )
		{
			return;
		}

		if ( this->usesCubemapColorProjection() )
		{
			/* Check if the texture is a cube array (animated cubemap) or a regular cube. */
			if ( m_colorProjectionTexture->type() == Vulkan::TextureType::TextureCubeArray )
			{
				m_colorProjectionBindlessIndex = m_bindlessTextureSet->registerTextureCubeArray(m_colorProjectionTexture);
				m_colorProjectionIsCubeArray = true;
				m_colorProjectionFrameIndex = 0;
			}
			else
			{
				m_colorProjectionBindlessIndex = m_bindlessTextureSet->registerTextureCube(m_colorProjectionTexture);
				m_colorProjectionIsCubeArray = false;
				m_colorProjectionFrameIndex = NoColorProjectionTexture;
			}
		}
		else
		{
			m_colorProjectionBindlessIndex = m_bindlessTextureSet->registerTexture2D(m_colorProjectionTexture);
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
		if ( m_colorProjectionBindlessIndex != NoColorProjectionTexture && m_bindlessTextureSet != nullptr && m_colorProjectionTexture != nullptr )
		{
			if ( useCubemap && m_colorProjectionIsCubeArray )
			{
				m_bindlessTextureSet->unregisterTextureCubeArray(m_colorProjectionTexture.get());
			}
			else if ( useCubemap )
			{
				m_bindlessTextureSet->unregisterTextureCube(m_colorProjectionTexture.get());
			}
			else
			{
				m_bindlessTextureSet->unregisterTexture2D(m_colorProjectionTexture.get());
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
	AbstractLightEmitter::onNotification (const Base::ObservableTrait * /*observable*/, int notificationCode, const std::any & /*data*/) noexcept
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

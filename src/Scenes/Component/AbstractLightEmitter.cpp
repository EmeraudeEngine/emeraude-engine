/*
 * src/Scenes/Component/AbstractLightEmitter.cpp
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

#include "AbstractLightEmitter.hpp"

/* Local inclusions. */
#include "AVConsole/Manager.hpp"

namespace EmEn::Scenes::Component
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Graphics;

	float AbstractLightEmitter::s_maxDistance{DefaultGraphicsShadowMappingMaxDistance};

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
	AbstractLightEmitter::onOutputDeviceConnected (AVConsole::AVManagers & /*managers*/, AbstractVirtualDevice & targetDevice) noexcept
	{
		/* When the shadow map is connected, we initialize it with coordinates and light properties. */
		targetDevice.updateVideoDeviceProperties(this->getFovOrNear(), this->getDistanceOrFar(), this->isOrthographicProjection());
		targetDevice.updateDeviceFromCoordinates(this->getWorldCoordinates(), this->getWorldVelocity());
	}

	Matrix< 4, float >
	AbstractLightEmitter::getLightSpaceMatrix () const noexcept
	{
		if ( !this->isShadowCastingEnabled() )
		{
			return Matrix< 4, float >::identity();
		}

		const auto & viewMatrices = this->shadowMap()->viewMatrices();

		return RenderTarget::ScaleBiasMatrix * viewMatrices.projectionMatrix() * viewMatrices.viewMatrix(false, 0);
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
}

/*
 * src/Scenes/Component/DirectionalLight.cpp
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

#include "DirectionalLight.hpp"

/* Local inclusions. */
#include "Saphir/LightGenerator.hpp"
#include "AVConsole/Manager.hpp"
#include "Scenes/Scene.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes::Component
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Animations;
	using namespace Saphir;
	using namespace Graphics;

	void
	DirectionalLight::onOutputDeviceConnected (AVConsole::AVManagers & /*managers*/, AbstractVirtualDevice * targetDevice) noexcept
	{
		targetDevice->updateDeviceFromCoordinates(this->getWorldCoordinates(), this->getWorldVelocity());
		targetDevice->updateProperties(false, s_maxDistance, 0.0F);
	}

	bool
	DirectionalLight::playAnimation (uint8_t animationID, const Variant & value, size_t /*cycle*/) noexcept
	{
		switch ( animationID )
		{
			case EmittingState :
				this->enable(value.asBool());
				return true;

			case Color :
				this->setColor(value.asColor());

				return true;

			case Intensity :
				this->setIntensity(value.asFloat());

				return true;

			default:
				return false;
		}
	}

	void
	DirectionalLight::processLogics (const Scene & scene) noexcept
	{
		if ( !this->isEnabled() )
		{
			return;
		}

		this->updateAnimations(scene.cycle());
	}

	void
	DirectionalLight::move (const CartesianFrame< float > & worldCoordinates) noexcept
	{
		if ( !this->isEnabled() )
		{
			return;
		}

		if ( this->isShadowCastingEnabled() )
		{
			this->updateDeviceFromCoordinates(worldCoordinates, this->getWorldVelocity());
		}

		const auto & direction = worldCoordinates.backwardVector();

		m_buffer[DirectionOffset+0] = direction.x();
		m_buffer[DirectionOffset+1] = direction.y();
		m_buffer[DirectionOffset+2] = direction.z();

		this->requestVideoMemoryUpdate();
	}

	void
	DirectionalLight::onColorChange (const PixelFactory::Color< float > & color) noexcept
	{
		m_buffer[ColorOffset+0] = color.red();
		m_buffer[ColorOffset+1] = color.green();
		m_buffer[ColorOffset+2] = color.blue();
	}

	void
	DirectionalLight::onIntensityChange (float intensity) noexcept
	{
		m_buffer[IntensityOffset] = intensity;
	}

	bool
	DirectionalLight::createOnHardware (Scene & scene) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} << "The directional light '" << this->name() << "' is already created !";

			return true;
		}

		/* Create and register the light to a shared uniform buffer. */
		if ( !this->addToSharedUniformBuffer(scene.lightSet().directionalLightBuffer()) )
		{
			Tracer::error(ClassId, "Unable to create the directional light shared uniform buffer !");

			return false;
		}

		/* Initialize the data buffer. */
		{
			const auto worldCoordinates = this->getWorldCoordinates();

			const auto & direction = worldCoordinates.backwardVector();

			m_buffer[DirectionOffset + 0] = direction.x();
			m_buffer[DirectionOffset + 1] = direction.y();
			m_buffer[DirectionOffset + 2] = direction.z();
		}

		const auto resolution = this->shadowMapResolution();

		if ( resolution > 0 )
		{
			/* [VULKAN-SHADOW] TODO: Reuse shadow maps + remove it from console on failure */
			m_shadowMap = scene.createRenderToShadowMap(this->name() + ShadowMapName, resolution);

			if ( m_shadowMap != nullptr )
			{
				if ( this->connect(scene.AVConsoleManager().managers(), m_shadowMap, true) != AVConsole::ConnexionResult::Success )
				{
					TraceSuccess{ClassId} << "2D shadow map (" << resolution << "px²) successfully created for directional light '" << this->name() << "'.";

					this->enableShadowCasting(true);
				}
				else
				{
					TraceError{ClassId} << "Unable to connect the 2D shadow map (" << resolution << "px²) to directional light '" << this->name() << "' !";

					m_shadowMap.reset();
				}
			}
			else
			{
				TraceError{ClassId} << "Unable to create a 2D shadow map (" << resolution << "px²) for directional light '" << this->name() << "' !";
			}
		}

		return this->updateVideoMemory();
	}

	void
	DirectionalLight::destroyFromHardware (Scene & scene) noexcept
	{
		if ( m_shadowMap != nullptr )
		{
			this->disconnect(scene.AVConsoleManager().managers(), m_shadowMap, true);

			m_shadowMap.reset();
		}

		this->removeFromSharedUniformBuffer();
	}

	Declaration::UniformBlock
	DirectionalLight::getUniformBlock (uint32_t set, uint32_t binding, bool useShadow) const noexcept
	{
		return LightGenerator::getUniformBlock(set, binding, LightType::Directional, useShadow);
	}
}

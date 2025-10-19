/*
 * src/Scenes/Component/SpotLight.cpp
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

#include "SpotLight.hpp"

/* Local inclusions. */
#include "Libs/Math/Space3D/Collisions/PointSphere.hpp"
#include "Saphir/LightGenerator.hpp"
#include "AVConsole/Manager.hpp"
#include "Scenes/Scene.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes::Component
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Animations;
	using namespace Graphics;
	using namespace Saphir;

	bool
	SpotLight::playAnimation (uint8_t animationID, const Variant & value, size_t /*cycle*/) noexcept
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

			case Radius :
				this->setRadius(value.asFloat());
				return true;

			case InnerAngle :
				this->setConeAngles(value.asFloat(), this->outerAngle());
				return true;

			case OuterAngle :
				this->setConeAngles(this->innerAngle(), value.asFloat());
				return true;

			default:
				return false;
		}
	}

	void
	SpotLight::processLogics (const Scene & scene) noexcept
	{
		if ( !this->isEnabled() )
		{
			return;
		}

		this->updateAnimations(scene.cycle());
	}

	void
	SpotLight::move (const CartesianFrame< float > & worldCoordinates) noexcept
	{
		if ( !this->isEnabled() )
		{
			return;
		}

		if ( this->isShadowCastingEnabled() )
		{
			this->updateDeviceFromCoordinates(worldCoordinates, this->getWorldVelocity());
		}

		const auto & position = worldCoordinates.position();

		m_buffer[PositionOffset+0] = position.x();
		m_buffer[PositionOffset+1] = position.y();
		m_buffer[PositionOffset+2] = position.z();

		const auto & direction = worldCoordinates.backwardVector();

		m_buffer[DirectionOffset+0] = direction.x();
		m_buffer[DirectionOffset+1] = direction.y();
		m_buffer[DirectionOffset+2] = direction.z();

		this->requestVideoMemoryUpdate();
	}

	bool
	SpotLight::touch (const Vector< 3, float > & position) const noexcept
	{
		const Space3D::Sphere< float > boundingSphere{m_radius, this->getWorldCoordinates().position()};

		/* TODO: Check for the cone ! */

		return Space3D::isColliding(position, boundingSphere);
	}

	bool
	SpotLight::createOnHardware (Scene & scene) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} << "The spot light '" << this->name() << "' is already created !";

			return true;
		}

		/* Create and register the light to a shared uniform buffer. */
		if ( !this->addToSharedUniformBuffer(scene.lightSet().spotLightBuffer()) )
		{
			Tracer::error(ClassId, "Unable to create the spotlight shared uniform buffer !");

			return false;
		}

		/* Initialize the data buffer. */
		{
			const auto worldCoordinates = this->getWorldCoordinates();

			const auto & position = worldCoordinates.position();

			m_buffer[PositionOffset + 0] = position.x();
			m_buffer[PositionOffset + 1] = position.y();
			m_buffer[PositionOffset + 2] = position.z();

			const auto & direction = worldCoordinates.backwardVector();

			m_buffer[DirectionOffset + 0] = direction.x();
			m_buffer[DirectionOffset + 1] = direction.y();
			m_buffer[DirectionOffset + 2] = direction.z();
		}

		if ( const auto resolution = this->shadowMapResolution(); resolution > 0 )
		{
			/* [VULKAN-SHADOW] TODO: Reuse shadow maps + remove it from console on failure */
			m_shadowMap = scene.createRenderToShadowMap(this->name() + ShadowMapName, resolution, this->isOrthographicProjection());

			if ( m_shadowMap != nullptr )
			{
				if ( this->connect(scene.AVConsoleManager().managers(), m_shadowMap, true) == AVConsole::ConnexionResult::Success )
				{
					TraceSuccess{ClassId} << "2D shadow map (" << resolution << "px²) successfully created for spotlight '" << this->name() << "'.";

					this->enableShadowCasting(true);
				}
				else
				{
					TraceError{ClassId} << "Unable to connect the 2D shadow map (" << resolution << "px²) to spotlight '" << this->name() << "' !";

					m_shadowMap.reset();
				}
			}
			else
			{
				TraceError{ClassId} << "Unable to create a 2D shadow map (" << resolution << "px²) for spotlight '" << this->name() << "' !";
			}
		}

		return this->updateVideoMemory();
	}

	void
	SpotLight::destroyFromHardware (Scene & scene) noexcept
	{
		if ( m_shadowMap != nullptr )
		{
			this->disconnect(scene.AVConsoleManager().managers(), m_shadowMap, true);

			m_shadowMap.reset();
		}

		this->removeFromSharedUniformBuffer();
	}

	Declaration::UniformBlock
	SpotLight::getUniformBlock (uint32_t set, uint32_t binding, bool useShadow) const noexcept
	{
		return LightGenerator::getUniformBlock(set, binding, LightType::Spot, useShadow);
	}

	void
	SpotLight::setRadius (float radius) noexcept
	{
		m_radius = std::abs(radius);

		m_buffer[RadiusOffset] = m_radius;

		if ( m_shadowMap != nullptr )
		{
			m_shadowMap->updateViewRangesProperties(this->getFovOrNear(), this->getDistanceOrFar());
		}

		this->requestVideoMemoryUpdate();
	}

	void
	SpotLight::setConeAngles (float innerAngle, float outerAngle) noexcept
	{
		if ( outerAngle <= 0.0F )
		{
			outerAngle = innerAngle;
		}
		else if ( innerAngle > outerAngle )
		{
			std::swap(innerAngle, outerAngle);
		}

		m_innerAngle = innerAngle;
		m_outerAngle = outerAngle;

		m_buffer[InnerCosAngleOffset] = std::cos(Radian(m_innerAngle));
		m_buffer[OuterCosAngleOffset] = std::cos(Radian(m_outerAngle));

		if ( m_shadowMap != nullptr )
		{
			m_shadowMap->updateViewRangesProperties(this->getFovOrNear(), this->getDistanceOrFar());
		}

		this->requestVideoMemoryUpdate();
	}
}

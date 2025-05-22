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
#include "Scenes/Scene.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes::Component
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace Animations;
	using namespace Graphics;
	using namespace Saphir;

	SpotLight::SpotLight (const std::string & name, const AbstractEntity & parentEntity, uint32_t shadowMapResolution) noexcept
		: AbstractLightEmitter(name, parentEntity, shadowMapResolution)
	{
		this->setConeAngles(30.0F, 35.0F);
	}

	SpotLight::~SpotLight ()
	{
		this->destroyFromHardware();
	}

	void
	SpotLight::onTargetConnected (AbstractVirtualDevice * targetDevice) noexcept
	{
		const auto maxDistance =
			m_radius > 0.0F ?
			m_radius :
			Settings::instance()->get< float >(GraphicsShadowMappingMaxDistanceKey, DefaultGraphicsShadowMappingMaxDistance);

		const auto fov = Degree(2.0F * Radian(m_outerAngle));

		targetDevice->updateDeviceFromCoordinates(this->getWorldCoordinates(), this->getWorldVelocity());
		targetDevice->updateProperties(true, maxDistance, fov);
	}

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
				this->setInnerAngle(value.asFloat());
				return true;

			case OuterAngle :
				this->setOuterAngle(value.asFloat());
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

	void
	SpotLight::onColorChange (const PixelFactory::Color< float > & color) noexcept
	{
		m_buffer[ColorOffset+0] = color.red();
		m_buffer[ColorOffset+1] = color.green();
		m_buffer[ColorOffset+2] = color.blue();
	}

	void
	SpotLight::onIntensityChange (float intensity) noexcept
	{
		m_buffer[IntensityOffset] = intensity;
	}

	bool
	SpotLight::touch (const Vector< 3, float > & position) const noexcept
	{
		const Space3D::Sphere< float > boundingSphere{m_radius, this->getWorldCoordinates().position()};

		/* TODO: Check for the cone ! */

		return Space3D::isColliding(position, boundingSphere);
	}

	bool
	SpotLight::createOnHardware (LightSet & lightSet, Renderer & renderer, AVConsole::Manager & AVConsoleManager) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} << "The spot light '" << this->name() << "' is already created !";

			return true;
		}

		/* Create and register the light to a shared uniform buffer. */
		if ( !this->addToSharedUniformBuffer(lightSet.spotLightBuffer()) )
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

		const auto resolution = this->shadowMapResolution();

		if ( resolution > 0 )
		{
			/* [VULKAN-SHADOW] TODO: Reuse shadow maps + remove it from console on failure */
			m_shadowMap = AVConsoleManager.createRenderToShadowMap(renderer, this->name() + ShadowMapName, resolution);

			if ( m_shadowMap != nullptr )
			{
				if ( this->connect(m_shadowMap) )
				{
					TraceSuccess{ClassId} << "2D shadow map successfully created for spotlight '" << this->name() << "'.";

					this->enableShadowCasting(true);
				}
				else
				{
					TraceError{ClassId} << "Unable to connect the 2D shadow map to spotlight '" << this->name() << "' !";

					m_shadowMap.reset();
				}
			}
			else
			{
				TraceError{ClassId} << "Unable to create a 2D shadow map for spotlight '" << this->name() << "' !";
			}
		}

		return this->updateVideoMemory();
	}

	void
	SpotLight::destroyFromHardware () noexcept
	{
		if ( m_shadowMap != nullptr )
		{
			this->disconnect(m_shadowMap);

			/* TODO: Check for automatic disconnection ! */
			//console.removeVideoDevice(m_shadowMap);

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

		this->setInnerAngle(innerAngle);

		this->setOuterAngle(outerAngle);

		this->requestVideoMemoryUpdate();
	}

	void
	SpotLight::setInnerAngle (float angle) noexcept
	{
		m_innerAngle = angle;

		m_buffer[InnerCosAngleOffset] = std::cos(Radian(m_innerAngle));
	}

	void
	SpotLight::setOuterAngle (float angle) noexcept
	{
		m_outerAngle = angle;

		m_buffer[OuterCosAngleOffset] = std::cos(Radian(m_outerAngle));

		if ( m_shadowMap != nullptr )
		{
			const auto fov = Degree(2.0F * Radian(m_outerAngle));
			const auto maxDistance = Settings::instance()->get< float >(GraphicsShadowMappingMaxDistanceKey, DefaultGraphicsShadowMappingMaxDistance);

			this->updateProperties(true, maxDistance, fov);
		}
	}

	std::ostream &
	operator<< (std::ostream & out, const SpotLight & obj)
	{
		const auto worldCoordinates = obj.getWorldCoordinates();

		return out << "Spot light data ;\n"
			"Position (World Space) : " << worldCoordinates.position() << "\n"
			"Direction (World Space) : " << worldCoordinates.forwardVector() << "\n"
			"Color : " << obj.color() << "\n"
			"Intensity : " << obj.intensity() << "\n"
			"Radius : " << obj.m_radius << "\n"
			"Inner angle : " << obj.m_innerAngle << "° (" << Radian(obj.m_innerAngle) << " rad) (cosine : " << std::cos(Radian(obj.m_innerAngle)) << ")\n"
			"Outer angle : " << obj.m_outerAngle << "° (" << Radian(obj.m_outerAngle) << " rad) (cosine : " << std::cos(Radian(obj.m_outerAngle)) << ")\n"
			"Activity : " << ( obj.isEnabled() ? "true" : "false" ) << "\n"
			"Shadow caster : " << ( obj.isShadowCastingEnabled() ? "true" : "false" ) << '\n';
	}

	std::string
	to_string (const SpotLight & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}

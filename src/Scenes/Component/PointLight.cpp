/*
 * src/Scenes/Component/PointLight.cpp
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

#include "PointLight.hpp"

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

	PointLight::PointLight (const std::string & name, const AbstractEntity & parentEntity, uint32_t shadowMapResolution) noexcept
		: AbstractLightEmitter(name, parentEntity, shadowMapResolution)
	{

	}

	PointLight::~PointLight ()
	{
		this->destroyFromHardware();
	}

	void
	PointLight::onTargetConnected (AbstractVirtualDevice * targetDevice) noexcept
	{
		const auto maxDistance =
			m_radius > 0.0F ?
			m_radius :
			Settings::instance()->get< float >(GraphicsShadowMappingMaxDistanceKey, DefaultGraphicsShadowMappingMaxDistance);

		targetDevice->updateDeviceFromCoordinates(this->getWorldCoordinates(), this->getWorldVelocity());
		targetDevice->updateProperties(true, maxDistance, 90.0F);
	}

	bool
	PointLight::playAnimation (uint8_t identifier, const Variant & value, size_t /*cycle*/) noexcept
	{
		switch ( identifier )
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

			default:
				return false;
		}
	}

	void
	PointLight::processLogics (const Scene & scene) noexcept
	{
		if ( !this->isEnabled() )
		{
			return;
		}

		this->updateAnimations(scene.cycle());
	}

	void
	PointLight::move (const CartesianFrame< float > & worldCoordinates) noexcept
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

		this->requestVideoMemoryUpdate();
	}

	void
	PointLight::onColorChange (const PixelFactory::Color< float > & color) noexcept
	{
		m_buffer[ColorOffset+0] = color.red();
		m_buffer[ColorOffset+1] = color.green();
		m_buffer[ColorOffset+2] = color.blue();
	}

	void
	PointLight::onIntensityChange (float intensity) noexcept
	{
		m_buffer[IntensityOffset] = intensity;
	}

	bool
	PointLight::touch (const Vector< 3, float > & position) const noexcept
	{
		const Space3D::Sphere< float > boundingSphere{m_radius, this->getWorldCoordinates().position()};

		return Space3D::isColliding(position, boundingSphere);
	}

	bool
	PointLight::createOnHardware (LightSet & lightSet, Renderer & renderer, AVConsole::Manager & AVConsoleManager) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} << "The point light '" << this->name() << "' is already created !";

			return true;
		}

		/* Create and register the light to a shared uniform buffer. */
		if ( !this->addToSharedUniformBuffer(lightSet.pointLightBuffer()) )
		{
			Tracer::error(ClassId, "Unable to create the point light shared uniform buffer !");

			return false;
		}

		/* Initialize the data buffer. */
		{
			const auto worldCoordinates = this->getWorldCoordinates();

			const auto & position = worldCoordinates.position();

			m_buffer[PositionOffset + 0] = position.x();
			m_buffer[PositionOffset + 1] = position.y();
			m_buffer[PositionOffset + 2] = position.z();
		}

		const auto resolution = this->shadowMapResolution();

		if ( resolution > 0 )
		{
			/* [VULKAN-SHADOW] TODO: Reuse shadow maps + remove it from console on failure */
			m_shadowMap = AVConsoleManager.createRenderToCubicShadowMap(renderer, this->name() + ShadowMapName, resolution);

			if ( m_shadowMap != nullptr )
			{
				if ( this->connect(m_shadowMap) )
				{
					TraceSuccess{ClassId} << "Cubic shadow map successfully created for point light '" << this->name() << "'.";

					this->enableShadowCasting(true);
				}
				else
				{
					TraceError{ClassId} << "Unable to connect the cubic shadow map to point light '" << this->name() << "' !";

					m_shadowMap.reset();
				}
			}
			else
			{
				TraceError{ClassId} << "Unable to create a cubic shadow map for point light '" << this->name() << "' !";
			}
		}

		return this->updateVideoMemory();
	}

	void
	PointLight::destroyFromHardware () noexcept
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
	PointLight::getUniformBlock (uint32_t set, uint32_t binding, bool useShadow) const noexcept
	{
		return LightGenerator::getUniformBlock(set, binding, LightType::Point, useShadow);
	}

	void
	PointLight::setRadius (float radius) noexcept
	{
		m_radius = std::abs(radius);

		m_buffer[RadiusOffset] = m_radius;

		this->requestVideoMemoryUpdate();
	}

	std::ostream &
	operator<< (std::ostream & out, const PointLight & obj)
	{
		const auto worldCoordinates = obj.getWorldCoordinates();

		return out << "Point light data :" "\n"
			"Position (World Space) : " << worldCoordinates.position() << "\n"
			"Color : " << obj.color() << "\n"
			"Intensity : " << obj.intensity() << "\n"
			"Radius : " << obj.m_radius << "\n"
			"Activity : " << ( obj.isEnabled() ? "true" : "false" ) << "\n"
			"Shadow caster : " << ( obj.isShadowCastingEnabled() ? "true" : "false" ) << '\n';
	}

	std::string
	to_string (const PointLight & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}

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
	PointLight::createOnHardware (Scene & scene) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} << "The point light '" << this->name() << "' is already created !";

			return true;
		}

		/* Create and register the light to a shared uniform buffer. */
		if ( !this->addToSharedUniformBuffer(scene.lightSet().pointLightBuffer()) )
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

		if ( const auto resolution = this->shadowMapResolution(); resolution > 0 )
		{
			/* [VULKAN-SHADOW] TODO: Reuse shadow maps + remove it from console on failure */
			m_shadowMap = scene.createRenderToCubicShadowMap(this->name() + ShadowMapName, resolution, this->getDistanceOrFar(), this->isOrthographicProjection());

			if ( m_shadowMap != nullptr )
			{
				if ( this->connect(scene.AVConsoleManager().managers(), m_shadowMap, true) == AVConsole::ConnexionResult::Success )
				{
					TraceSuccess{ClassId} << "Cubic shadow map (" << resolution << "px³) successfully created for point light '" << this->name() << "'.";

					this->enableShadowCasting(true);
				}
				else
				{
					TraceError{ClassId} << "Unable to connect the cubic shadow map (" << resolution << "px³) to point light '" << this->name() << "' !";

					m_shadowMap.reset();
				}
			}
			else
			{
				TraceError{ClassId} << "Unable to create a cubic shadow map (" << resolution << "px³) for point light '" << this->name() << "' !";
			}
		}

		return this->updateVideoMemory();
	}

	void
	PointLight::destroyFromHardware (Scene & scene) noexcept
	{
		if ( m_shadowMap != nullptr )
		{
			this->disconnect(scene.AVConsoleManager().managers(), m_shadowMap, true);

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

		if ( m_shadowMap != nullptr )
		{
			m_shadowMap->updateViewRangesProperties(this->getFovOrNear(), this->getDistanceOrFar());
		}

		this->requestVideoMemoryUpdate();
	}
}

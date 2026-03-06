/*
 * src/Scenes/Component/SpotLight.cpp
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

#include "SpotLight.hpp"

/* STL inclusions. */
#include <bit>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Resources/ResourceTrait.hpp"
#include "Libs/Math/Space3D/Collisions/PointSphere.hpp"
#include "Saphir/LightGenerator.hpp"
#include "Scenes/AVConsole/Manager.hpp"
#include "Scenes/LightSet.hpp"
#include "Scenes/Scene.hpp"
#include "Tracer.hpp"
#include "Vulkan/DescriptorSet.hpp"

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

		const auto direction = worldCoordinates.forwardVector();

		m_buffer[DirectionOffset+0] = direction.x();
		m_buffer[DirectionOffset+1] = direction.y();
		m_buffer[DirectionOffset+2] = direction.z();

		/* NOTE: Update light matrice for advanced light effects. */
		if ( this->isShadowCastingEnabled() || this->hasColorProjectionTexture() )
		{
			this->updateLightSpaceMatrix();
		}

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

		/* Store the bindless texture manager for color projection registration. */
		m_bindlessTextureManager = &scene.AVConsoleManager().graphicsRenderer().bindlessTextureManager();

		/* If a color projection texture is set, try to register it in bindless now. */
		if ( this->hasColorProjectionTexture() && this->colorProjectionBindlessIndex() == NoColorProjectionTexture )
		{
			auto * resource = dynamic_cast< Resources::ResourceTrait * >(this->colorProjectionTexture().get());

			if ( resource != nullptr && resource->isLoaded() )
			{
				this->registerColorProjectionInBindless();
			}
			else if ( resource != nullptr )
			{
				this->observe(resource);
			}
		}

		/* Initialize the data buffer. */
		{
			const auto worldCoordinates = this->getWorldCoordinates();

			const auto & position = worldCoordinates.position();

			m_buffer[PositionOffset + 0] = position.x();
			m_buffer[PositionOffset + 1] = position.y();
			m_buffer[PositionOffset + 2] = position.z();

			const auto direction = worldCoordinates.forwardVector();

			m_buffer[DirectionOffset + 0] = direction.x();
			m_buffer[DirectionOffset + 1] = direction.y();
			m_buffer[DirectionOffset + 2] = direction.z();
		}

		/* [VULKAN-SHADOW] TODO: Reuse shadow maps + remove it from console on failure */
		if ( const auto resolution = this->shadowMapResolution(); resolution > 0 )
		{
			m_shadowMap = scene.createRenderToShadowMap(this->name() + ShadowMapName, resolution, this->getDistanceOrFar(), this->isOrthographicProjection());

			if ( m_shadowMap != nullptr )
			{
				if ( this->connect(scene.AVConsoleManager().engineContext(), m_shadowMap, true) == AVConsole::ConnexionResult::Success )
				{
					TraceSuccess{ClassId} << "2D shadow map (" << resolution << "px²) successfully created for spotlight '" << this->name() << "'.";

					/* Create the shadow-enabled descriptor set (UBO + shadow map sampler). */
					if ( this->createShadowDescriptorSet(scene) )
					{
						this->enableShadowCasting(true);

						this->updateLightSpaceMatrix();
					}
					else
					{
						TraceError{ClassId} << "Unable to create shadow descriptor set for spotlight '" << this->name() << "' !";
					}
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
		/* Unregister the color projection texture from the bindless manager and stop observing. */
		this->unregisterColorProjectionFromBindless(false);

		/* Clean up descriptor set (shadow or color projection). */
		m_shadowDescriptorSet.reset();

		if ( m_shadowMap != nullptr )
		{
			this->disconnect(scene.AVConsoleManager().engineContext(), m_shadowMap, true);

			m_shadowMap.reset();
		}

		this->removeFromSharedUniformBuffer();
	}

	const Vulkan::DescriptorSet *
	SpotLight::descriptorSet (bool useShadowMap) const noexcept
	{
		/* If an enriched descriptor set exists (shadow map and/or color projection), use it. */
		if ( m_shadowDescriptorSet != nullptr )
		{
			return m_shadowDescriptorSet.get();
		}

		/* Otherwise, fall back to the base implementation (shared UBO descriptor set). */
		return AbstractLightEmitter::descriptorSet(useShadowMap);
	}

	Declaration::UniformBlock
	SpotLight::getUniformBlock (uint32_t set, uint32_t binding, bool useShadow, bool useColorProjection) const noexcept
	{
		return LightGenerator::getUniformBlock(set, binding, LightType::Spot, useShadow, useColorProjection);
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

		if ( this->isShadowCastingEnabled() || this->hasColorProjectionTexture() )
		{
			this->updateLightSpaceMatrix();
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

		if ( this->isShadowCastingEnabled() || this->hasColorProjectionTexture() )
		{
			this->updateLightSpaceMatrix();
		}

		this->requestVideoMemoryUpdate();
	}

	void
	SpotLight::setPCFRadius (float radius) noexcept
	{
		m_PCFRadius = std::abs(radius);

		m_buffer[PCFRadiusOffset] = m_PCFRadius;

		this->requestVideoMemoryUpdate();
	}

	void
	SpotLight::setShadowBias (float bias) noexcept
	{
		m_shadowBias = bias;

		m_buffer[ShadowBiasOffset] = m_shadowBias;

		this->requestVideoMemoryUpdate();
	}

	bool
	SpotLight::createShadowDescriptorSet (Scene & scene) noexcept
	{
		auto & renderer = scene.AVConsoleManager().graphicsRenderer();

		/* Get the shadow-enabled descriptor set layout. */
		const auto descriptorSetLayout = LightSet::getDescriptorSetLayoutWithShadow(renderer.layoutManager());

		if ( descriptorSetLayout == nullptr )
		{
			TraceError{ClassId} << "Unable to get the shadow descriptor set layout !";

			return false;
		}

		/* Create the descriptor set. */
		m_shadowDescriptorSet = std::make_unique< Vulkan::DescriptorSet >(renderer.descriptorPool(), descriptorSetLayout);

		if ( !m_shadowDescriptorSet->create() )
		{
			TraceError{ClassId} << "Unable to create the shadow descriptor set !";

			m_shadowDescriptorSet.reset();

			return false;
		}

		/* Get the UBO from the shared buffer. */
		const auto sharedUBO = scene.lightSet().spotLightBuffer();

		if ( !sharedUBO )
		{
			TraceError{ClassId} << "Unable to get the shared uniform buffer !";

			m_shadowDescriptorSet.reset();

			return false;
		}

		/* Write binding 0: Light UBO (dynamic offset). */
		if ( !m_shadowDescriptorSet->writeUniformBufferObjectDynamic(0, *sharedUBO->uniformBufferObject(this->UBOIndex())) )
		{
			TraceError{ClassId} << "Unable to write UBO to shadow descriptor set !";

			m_shadowDescriptorSet.reset();

			return false;
		}

		/* Write binding 1: Shadow map sampler.
		 * NOTE: ShadowMap inherits from TextureInterface and implements image/imageView/sampler methods. */
		if ( m_shadowMap == nullptr )
		{
			TraceError{ClassId} << "Shadow map is null, cannot bind to descriptor set !";

			m_shadowDescriptorSet.reset();

			return false;
		}

		/* Write binding 1: Shadow map sampler. */
		if ( m_shadowMap == nullptr || !m_shadowMap->writeCombinedImageSampler(*m_shadowDescriptorSet, 1) )
		{
			TraceError{ClassId} << "Shadow map is null, cannot bind to descriptor set !";

			m_shadowDescriptorSet.reset();

			return false;
		}

		TraceSuccess{ClassId} << "Shadow descriptor set created successfully for spotlight '" << this->name() << "'.";

		return true;
	}

	void
	SpotLight::updateLightSpaceMatrix () noexcept
	{
		const auto worldCoordinates = this->getWorldCoordinates();

		const auto viewMatrix = worldCoordinates.getViewMatrix();

		/* Compute near plane consistently with ViewMatrices2DUBO (1:1 aspect ratio). */
		const auto fov = this->getFovOrNear();
		const auto tanHalfFov = std::tan(Radian(fov) * 0.5F);
		const auto nearPlane = 0.1F / std::sqrt(1.0F + tanHalfFov * tanHalfFov * 2.0F);

		const auto projMatrix = Matrix< 4, float >::perspectiveProjection(fov, 1.0F, nearPlane, this->getDistanceOrFar());

		const auto matrix = RenderTarget::ScaleBiasMatrix * projMatrix * viewMatrix;

		std::copy_n(matrix.data(), 16, m_buffer.data() + LightMatrixOffset);
	}

	bool
	SpotLight::onVideoMemoryUpdate (SharedUniformBuffer & UBO, uint32_t index) noexcept
	{
		m_buffer[ColorProjectionIndexOffset] = std::bit_cast< float >(this->colorProjectionBindlessIndex());
		m_buffer[ColorProjectionBoostOffset] = this->colorProjectionBoost();

		return UBO.writeElementData(index, m_buffer.data());
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
}

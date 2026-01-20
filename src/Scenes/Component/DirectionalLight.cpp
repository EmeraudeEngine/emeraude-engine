/*
 * src/Scenes/Component/DirectionalLight.cpp
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

#include "DirectionalLight.hpp"

/* Local inclusions. */
#include "Saphir/LightGenerator.hpp"
#include "Scenes/AVConsole/Manager.hpp"
#include "Scenes/Scene.hpp"
#include "Tracer.hpp"
#include "Vulkan/DescriptorSet.hpp"

namespace EmEn::Scenes::Component
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Animations;
	using namespace Saphir;
	using namespace Graphics;

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
	DirectionalLight::setDirection (const CartesianFrame< float > & worldCoordinates) noexcept
	{
		const auto direction = m_useDirectionVector ?
			/* Use the world coordinates forward vector ... */
			worldCoordinates.forwardVector() :
			/* ... Or the inverse normalized position vector. */
			-worldCoordinates.position().normalized();

		m_buffer[DirectionOffset+0] = direction.x();
		m_buffer[DirectionOffset+1] = direction.y();
		m_buffer[DirectionOffset+2] = direction.z();
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
			/* NOTE: For directional lights (classic shadow map mode), the shadow map should be
			 * centered at the scene origin, not at the light position. The light is conceptually
			 * at infinity, so only the direction matters.
			 *
			 * The camera is positioned on the light side of origin, far enough back that
			 * the frustum covers the entire coverage area symmetrically around origin.
			 * With near=0 and far=coverage, placing camera at coverage/2 from origin
			 * means the far plane is only coverage/2 past origin. Instead, we use the
			 * full coverage as camera offset so that origin is at depth coverage/2.
			 */

			/* Compute the light direction (same logic as setDirection()).
			 * This is the direction light rays travel (from light toward scene). */
			const auto lightDirection = m_useDirectionVector ?
				worldCoordinates.forwardVector() :
				-worldCoordinates.position().normalized();

			CartesianFrame< float > shadowMapFrame;

			/* Position camera on the light side of origin at full coverage distance.
			 * This places origin at depth = coverage (middle when far = coverage * 2).
			 * Camera position = -lightDirection * coverage (opposite to ray direction). */
			const auto coverage = m_coverageSize > 0.0F ? m_coverageSize : this->getDistanceOrFar();
			shadowMapFrame.setPosition(-lightDirection * coverage);

			/* Set the shadow map frame to look in the light direction.
			 * CartesianFrame convention: camera looks in Z- (forward), Z+ is backward.
			 * So backward = -lightDirection to make the camera look in lightDirection. */
			shadowMapFrame.setBackwardVector(-lightDirection);

			this->updateDeviceFromCoordinates(shadowMapFrame, Vector< 3, float >::origin());

			/* Update the light space matrix in the buffer after changing shadow map coordinates. */
			this->updateLightSpaceMatrix();
		}

		this->setDirection(worldCoordinates);

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

	void
	DirectionalLight::setPCFRadius (float radius) noexcept
	{
		m_PCFRadius = std::abs(radius);

		m_buffer[PCFRadiusOffset] = m_PCFRadius;

		this->requestVideoMemoryUpdate();
	}

	void
	DirectionalLight::setShadowBias (float bias) noexcept
	{
		m_shadowBias = bias;

		m_buffer[ShadowBiasOffset] = m_shadowBias;

		this->requestVideoMemoryUpdate();
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
		this->setDirection(this->getWorldCoordinates());

		/* [VULKAN-SHADOW] Create shadow map if resolution is specified. */
		if ( const auto resolution = this->shadowMapResolution(); resolution > 0 )
		{
			if ( m_usesCSM )
			{
				/* CSM mode: coverageSize is 0 or not set. */
				TraceDebug{ClassId} << "Creating CSM (" << m_cascadeCount << " cascades, lambda=" << m_lambda << ") for directional light '" << this->name() << "'...";

				m_shadowMapCascaded = scene.createRenderToCascadedShadowMap(this->name() + ShadowMapName, resolution, this->getDistanceOrFar(), m_cascadeCount, m_lambda);

				if ( m_shadowMapCascaded != nullptr )
				{
					if ( this->connect(scene.AVConsoleManager().engineContext(), m_shadowMapCascaded, true) == AVConsole::ConnexionResult::Success )
					{
						TraceSuccess{ClassId} << "Cascaded shadow map (" << m_cascadeCount << " cascades, " << resolution << "px²) successfully created for directional light '" << this->name() << "'.";

						/* Create the descriptor set with the cascaded shadow map bound to binding 1. */
						if ( this->createShadowDescriptorSetCSM(scene) )
						{
							this->enableShadowCasting(true);
						}
						else
						{
							TraceError{ClassId} << "Unable to create CSM shadow descriptor set for directional light '" << this->name() << "' !";

							this->disconnect(scene.AVConsoleManager().engineContext(), m_shadowMapCascaded, true);

							m_shadowMapCascaded.reset();
						}
					}
					else
					{
						TraceError{ClassId} << "Unable to connect the cascaded shadow map to directional light '" << this->name() << "' !";

						m_shadowMapCascaded.reset();
					}
				}
				else
				{
					TraceError{ClassId} << "Unable to create a cascaded shadow map for directional light '" << this->name() << "' !";
				}
			}
			else
			{
				/* Classic shadow map mode: coverageSize defines the coverage dimension. */
				TraceInfo{ClassId} << "Creating classic shadow map (coverage=" << m_coverageSize << "m) for directional light '" << this->name() << "'...";

				m_shadowMap = scene.createRenderToShadowMap(this->name() + ShadowMapName, resolution, m_coverageSize, this->isOrthographicProjection());
			}

			if ( m_shadowMap != nullptr )
			{
				if ( this->connect(scene.AVConsoleManager().engineContext(), m_shadowMap, true) == AVConsole::ConnexionResult::Success )
				{
					TraceSuccess{ClassId} << "2D shadow map (" << resolution << "px²) successfully created for directional light '" << this->name() << "'.";

					/* Create the descriptor set with the shadow map bound to binding 1. */
					if ( this->createShadowDescriptorSet(scene) )
					{
						this->enableShadowCasting(true);

						/* NOTE: For directional lights, override the shadow map coordinates.
						 * The base class onOutputDeviceConnected() initialized it with the light's
						 * actual position, which is wrong for directional lights.
						 *
						 * The camera is positioned on the light side of origin at full coverage
						 * distance, so the frustum (with far = coverage * 2) extends coverage
						 * units past origin, creating a symmetric depth range.
						 */
						const auto worldCoordinates = this->getWorldCoordinates();

						/* Compute the light direction (same logic as setDirection()). */
						const auto lightDirection = m_useDirectionVector ?
							worldCoordinates.forwardVector() :
							-worldCoordinates.position().normalized();

						CartesianFrame< float > shadowMapFrame;

						/* Position camera on the light side of origin at full coverage distance. */
						const auto coverage = m_coverageSize > 0.0F ? m_coverageSize : this->getDistanceOrFar() * 0.5F;
						shadowMapFrame.setPosition(-lightDirection * coverage);

						/* Set the shadow map frame to look in the light direction. */
						shadowMapFrame.setBackwardVector(-lightDirection);

						this->updateDeviceFromCoordinates(shadowMapFrame, Vector< 3, float >::origin());

						this->updateLightSpaceMatrix();
					}
					else
					{
						TraceError{ClassId} << "Unable to create shadow descriptor set for directional light '" << this->name() << "' !";

						this->disconnect(scene.AVConsoleManager().engineContext(), m_shadowMap, true);

						m_shadowMap.reset();
					}
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
		/* Clean up shadow descriptor sets. */
		m_shadowDescriptorSet.reset();
		m_shadowDescriptorSetCSM.reset();

		/* Clean up classic shadow map. */
		if ( m_shadowMap != nullptr )
		{
			this->disconnect(scene.AVConsoleManager().engineContext(), m_shadowMap, true);

			m_shadowMap.reset();
		}

		/* Clean up cascaded shadow map. */
		if ( m_shadowMapCascaded != nullptr )
		{
			this->disconnect(scene.AVConsoleManager().engineContext(), m_shadowMapCascaded, true);

			m_shadowMapCascaded.reset();
		}

		this->removeFromSharedUniformBuffer();
	}

	const Vulkan::DescriptorSet *
	DirectionalLight::descriptorSet (bool useShadowMap) const noexcept
	{
		/* If shadow map is requested, check for CSM first, then classic shadow map. */
		if ( useShadowMap )
		{
			if ( m_usesCSM && m_shadowDescriptorSetCSM != nullptr )
			{
				return m_shadowDescriptorSetCSM.get();
			}

			if ( m_shadowDescriptorSet != nullptr )
			{
				return m_shadowDescriptorSet.get();
			}
		}

		/* Otherwise, fall back to the base implementation (shared UBO descriptor set). */
		return AbstractLightEmitter::descriptorSet(useShadowMap);
	}

	Declaration::UniformBlock
	DirectionalLight::getUniformBlock (uint32_t set, uint32_t binding, bool useShadow) const noexcept
	{
		return LightGenerator::getUniformBlock(set, binding, LightType::Directional, useShadow);
	}

	bool
	DirectionalLight::createShadowDescriptorSet (Scene & scene) noexcept
	{
		auto & renderer = scene.AVConsoleManager().graphicsRenderer();

		/* Get the unified descriptor set layout (same for shadow and non-shadow). */
		const auto descriptorSetLayout = LightSet::getDescriptorSetLayout(renderer.layoutManager());

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
		const auto sharedUBO = scene.lightSet().directionalLightBuffer();

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

		if ( !m_shadowMap->isCreated() )
		{
			TraceError{ClassId} << "Shadow map is not fully created yet !";

			m_shadowDescriptorSet.reset();

			return false;
		}

		if ( !m_shadowDescriptorSet->writeCombinedImageSampler(1, *m_shadowMap) )
		{
			TraceError{ClassId} << "Unable to write shadow map sampler to descriptor set !";

			m_shadowDescriptorSet.reset();

			return false;
		}

		TraceSuccess{ClassId} << "Shadow descriptor set created successfully for directional light '" << this->name() << "'.";

		return true;
	}

	bool
	DirectionalLight::createShadowDescriptorSetCSM (Scene & scene) noexcept
	{
		auto & renderer = scene.AVConsoleManager().graphicsRenderer();

		/* Get the unified descriptor set layout (same for shadow and non-shadow). */
		const auto descriptorSetLayout = LightSet::getDescriptorSetLayout(renderer.layoutManager());

		if ( descriptorSetLayout == nullptr )
		{
			TraceError{ClassId} << "Unable to get the CSM shadow descriptor set layout !";

			return false;
		}

		/* Create the descriptor set. */
		m_shadowDescriptorSetCSM = std::make_unique< Vulkan::DescriptorSet >(renderer.descriptorPool(), descriptorSetLayout);

		if ( !m_shadowDescriptorSetCSM->create() )
		{
			TraceError{ClassId} << "Unable to create the CSM shadow descriptor set !";

			m_shadowDescriptorSetCSM.reset();

			return false;
		}

		/* Get the UBO from the shared buffer. */
		const auto sharedUBO = scene.lightSet().directionalLightBuffer();

		if ( !sharedUBO )
		{
			TraceError{ClassId} << "Unable to get the shared uniform buffer for CSM !";

			m_shadowDescriptorSetCSM.reset();

			return false;
		}

		/* Write binding 0: Light UBO (dynamic offset). */
		if ( !m_shadowDescriptorSetCSM->writeUniformBufferObjectDynamic(0, *sharedUBO->uniformBufferObject(this->UBOIndex())) )
		{
			TraceError{ClassId} << "Unable to write UBO to CSM shadow descriptor set !";

			m_shadowDescriptorSetCSM.reset();

			return false;
		}

		/* Write binding 1: Cascaded shadow map array sampler.
		 * NOTE: ShadowMapCascaded inherits from TextureInterface and implements image/imageView/sampler methods. */
		if ( m_shadowMapCascaded == nullptr )
		{
			TraceError{ClassId} << "Cascaded shadow map is null, cannot bind to descriptor set !";

			m_shadowDescriptorSetCSM.reset();

			return false;
		}

		if ( !m_shadowMapCascaded->isCreated() )
		{
			TraceError{ClassId} << "Cascaded shadow map is not fully created yet !";

			m_shadowDescriptorSetCSM.reset();

			return false;
		}

		if ( !m_shadowDescriptorSetCSM->writeCombinedImageSampler(1, *m_shadowMapCascaded) )
		{
			TraceError{ClassId} << "Unable to write cascaded shadow map sampler to descriptor set !";

			m_shadowDescriptorSetCSM.reset();

			return false;
		}

		TraceSuccess{ClassId} << "CSM shadow descriptor set created successfully for directional light '" << this->name() << "'.";

		return true;
	}

	void
	DirectionalLight::updateCascades (const std::array< Vector< 3, float >, 8 > & cameraFrustumCorners, float nearPlane, float farPlane) const noexcept
	{
		if ( !m_usesCSM || m_shadowMapCascaded == nullptr )
		{
			return;
		}

		/* Get the light direction. */
		const auto worldCoordinates = this->getWorldCoordinates();
		const auto lightDirection = m_useDirectionVector ?
			worldCoordinates.forwardVector() :
			-worldCoordinates.position().normalized();

		/* Update the cascade matrices in the cascaded view matrices UBO. */
		m_shadowMapCascaded->cascadedViewMatrices().updateCascades(
			lightDirection,
			cameraFrustumCorners,
			nearPlane,
			farPlane
		);
	}

	void
	DirectionalLight::updateLightSpaceMatrix () noexcept
	{
		this->writeLightSpaceMatrix(m_buffer.data() + LightMatrixOffset);
	}

	bool
	DirectionalLight::onVideoMemoryUpdate (SharedUniformBuffer & UBO, uint32_t index) noexcept
	{
		return UBO.writeElementData(index, m_buffer.data());
	}
}

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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "DirectionalLight.hpp"

/* STL inclusions. */
#include <bit>
#include <cstring>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Resources/ResourceTrait.hpp"
#include "Saphir/LightGenerator.hpp"
#include "Scenes/AVConsole/Manager.hpp"
#include "Scenes/Scene.hpp"
#include "Tracer.hpp"
#include "Vulkan/DescriptorSet.hpp"

namespace EmEn::Scenes::Component
{
	using namespace Base;
	using namespace Base::Math;
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

	Vector< 3, float >
	DirectionalLight::resolveDirection (const CartesianFrame< float > & worldCoordinates) const noexcept
	{
		/* Use the world coordinates forward vector ... */
		if ( m_useDirectionVector )
		{
			return worldCoordinates.forwardVector();
		}

		/* ... Or the inverse normalized position vector. ⚠️ A light AT the origin has no
		 * position-to-origin direction: normalising the null vector would write NaN into the
		 * light buffer, and the vertex shader normalises it again — one NaN there poisons the
		 * whole frame through the bloom and the TAA history. Fall back to the forward axis. */
		const auto & position = worldCoordinates.position();

		if ( position.lengthSquared() < 1e-12F )
		{
			return worldCoordinates.forwardVector();
		}

		return -position.normalized();
	}

	void
	DirectionalLight::setDirection (const CartesianFrame< float > & worldCoordinates) noexcept
	{
		const auto direction = this->resolveDirection(worldCoordinates);

		m_buffer[DirectionOffset+0] = direction.x();
		m_buffer[DirectionOffset+1] = direction.y();
		m_buffer[DirectionOffset+2] = direction.z();
	}

	void
	DirectionalLight::move (const CartesianFrame< float > & worldCoordinates) noexcept
	{
		/* ⚠️ No early return on a DISABLED light. This method used to skip everything while the
		 * light was off, so a light that moved while disabled and was then enabled kept a STALE
		 * direction, shadow frame and light-space matrix — whatever its last enabled move() had
		 * staged, NaN included (Component::SunCourse, 2026-09-13: the light was built at the origin,
		 * staged a NaN direction, spent the night disabled and rose with it — black frames, GPU
		 * unresponsive). A light tracks its frame whether it emits or not; emitting is the render
		 * passes' decision (they skip a disabled light themselves). */

		/* NOTE: For CSM, the shadow map coordinates and matrices are computed in updateCascades()
		 * based on the camera frustum. The classic shadow map logic below doesn't apply. */
		if ( this->isShadowCastingEnabled() && !m_usesCSM )
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

			/* The direction light rays travel (from light toward scene) — the same resolution
			 * setDirection() writes to the buffer, null-position guard included. */
			const auto lightDirection = this->resolveDirection(worldCoordinates);

			CartesianFrame< float > shadowMapFrame;

			/* Position camera on the light side of origin at full coverage distance.
			 * This places origin at depth = coverage (middle when far = coverage * 2).
			 * Camera position = -lightDirection * coverage (opposite to ray direction). */
			shadowMapFrame.setPosition(-lightDirection * m_coverageSize);

			/* Set the shadow map frame to look in the light direction.
			 * CartesianFrame convention: camera looks in Z- (forward), Z+ is backward.
			 * So backward = -lightDirection to make the camera look in lightDirection. */
			shadowMapFrame.setBackwardVector(-lightDirection);

			this->updateDeviceFromCoordinates(shadowMapFrame, Vector< 3, float >::origin());
		}

		this->setDirection(worldCoordinates);

		/* NOTE: Update light matrice for advanced light effects. */
		if ( this->isShadowCastingEnabled() || this->hasColorProjectionTexture() )
		{
			this->updateLightSpaceMatrix();
		}

		this->requestVideoMemoryUpdate();
	}

	void
	DirectionalLight::onColorChange (const Math::Vector< 3, float > & chromaticity) noexcept
	{
		m_buffer[ColorOffset+0] = chromaticity[Math::X];
		m_buffer[ColorOffset+1] = chromaticity[Math::Y];
		m_buffer[ColorOffset+2] = chromaticity[Math::Z];
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

		/* Store the bindless texture manager for color projection registration. */
		m_bindlessTextureSet = &scene.bindlessTextureSet();

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
		this->setDirection(this->getWorldCoordinates());

		/* [VULKAN-SHADOW] Create shadow map if resolution is specified. */
		if ( const auto resolution = this->shadowMapResolution(); resolution > 0 )
		{
			if ( m_usesCSM )
			{
				TraceDebug{ClassId} << "Creating CSM (" << m_cascadeCount << " cascades, lambda=" << m_lambda << ") for directional light '" << this->name() << "'...";

				m_shadowMap = scene.createRenderToCascadedShadowMap(this->name() + ShadowMapName, resolution, this->getDistanceOrFar(), m_cascadeCount, m_lambda);
			}
			else
			{
				TraceDebug{ClassId} << "Creating classic shadow map (coverage=" << m_coverageSize << "m) for directional light '" << this->name() << "'...";

				m_shadowMap = scene.createRenderToShadowMap(this->name() + ShadowMapName, resolution, m_coverageSize, this->isOrthographicProjection());
			}

			if ( m_shadowMap == nullptr )
			{
				TraceError{ClassId} << "Unable to create shadow map for directional light '" << this->name() << "' !";

				return false;
			}

			if ( this->connect(scene.AVConsoleManager().engineContext(), m_shadowMap, true) != AVConsole::ConnexionResult::Success )
			{
				TraceError{ClassId} << "Unable to connect the shadow map to directional light '" << this->name() << "' !";

				m_shadowMap.reset();

				return false;
			}

			if ( !this->createShadowDescriptorSet(scene) )
			{
				TraceError{ClassId} << "Unable to create shadow descriptor set for directional light '" << this->name() << "' !";

				this->disconnect(scene.AVConsoleManager().engineContext(), m_shadowMap, true);

				m_shadowMap.reset();

				return false;
			}

			this->enableShadowCasting(true);

			TraceSuccess{ClassId} << "Shadow map (" << resolution << "px²) successfully created for directional light '" << this->name() << "'.";

			/* NOTE: For classic shadow maps, override the coordinates.
			 * The base class onOutputDeviceConnected() initialized it with the light's
			 * actual position, which is wrong for directional lights. */
			if ( !m_usesCSM )
			{
				const auto worldCoordinates = this->getWorldCoordinates();
				const auto lightDirection = m_useDirectionVector ? worldCoordinates.forwardVector() : -worldCoordinates.position().normalized();

				CartesianFrame< float > shadowMapFrame;
				const auto coverage = m_coverageSize > 0.0F ? m_coverageSize : this->getDistanceOrFar() * 0.5F;
				shadowMapFrame.setPosition(-lightDirection * coverage);
				shadowMapFrame.setBackwardVector(-lightDirection);

				this->updateDeviceFromCoordinates(shadowMapFrame, Vector< 3, float >::origin());

				this->updateLightSpaceMatrix();
			}
		}

		return this->primeVideoMemory();
	}

	void
	DirectionalLight::destroyFromHardware (Scene & scene) noexcept
	{
		/* Unregister the color projection texture from the bindless manager and stop observing. */
		this->unregisterColorProjectionFromBindless(false);

		/* Clean up descriptor set (shadow or color projection). */
		m_shadowDescriptorSet.reset();

		/* Clean up classic shadow map. */
		if ( m_shadowMap != nullptr )
		{
			this->disconnect(scene.AVConsoleManager().engineContext(), m_shadowMap, true);

			m_shadowMap.reset();
		}

		this->removeFromSharedUniformBuffer();
	}

	const Vulkan::DescriptorSet *
	DirectionalLight::descriptorSet (bool useShadowMap) const noexcept
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
	DirectionalLight::getUniformBlock (uint32_t set, uint32_t binding, bool useShadow, bool useColorProjection) const noexcept
	{
		return LightGenerator::getUniformBlock(set, binding, LightType::Directional, useShadow, useColorProjection);
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

		/* Write binding 1: Shadow map sampler. */
		if ( m_shadowMap == nullptr || !m_shadowMap->writeCombinedImageSampler(*m_shadowDescriptorSet, 1) )
		{
			TraceError{ClassId} << "Shadow map is null, cannot bind to descriptor set !";

			m_shadowDescriptorSet.reset();

			return false;
		}

		TraceSuccess{ClassId} << "Shadow descriptor set created successfully for directional light '" << this->name() << "'.";

		return true;
	}

	void
	DirectionalLight::updateCascades (const std::array< Vector< 3, float >, 8 > & cameraFrustumCorners, float nearPlane, float farPlane) noexcept
	{
		if ( !m_usesCSM || m_shadowMap == nullptr )
		{
			return;
		}

		/* Get the light direction. */
		const auto worldCoordinates = this->getWorldCoordinates();
		const auto lightDirection = m_useDirectionVector ? worldCoordinates.forwardVector() : -worldCoordinates.position().normalized();

		/* Scale CSM coverage based on m_csmScale.
		 * csmScale 1.0 = full frustum, 2.0 = zoom x2, 4.0 = zoom x4, etc.
		 * We must scale both the far corners AND the farPlane value. */
		const float CSMCoverageRatio = 1.0F / m_CSMScale;

		/* Scale far corners (indices 4-7) to match reduced coverage.
		 * New far corner = near corner + (far corner - near corner) * ratio */
		auto scaledCorners = cameraFrustumCorners;

		for ( size_t corner = 0; corner < 4; ++corner )
		{
			scaledCorners[corner + 4] = scaledCorners[corner] + (cameraFrustumCorners[corner + 4] - cameraFrustumCorners[corner]) * CSMCoverageRatio;
		}

		const float CSMFarPlane = nearPlane + ((farPlane - nearPlane) * CSMCoverageRatio);

		/* Update the cascade matrices in the cascaded view matrices UBO. */
		auto & viewMatrices = static_cast< ViewMatricesCascadedUBO & >(m_shadowMap->viewMatrices());

		viewMatrices.updateFromMainCameraFrustum(lightDirection, scaledCorners, nearPlane, CSMFarPlane);

		/* Copy cascade data to the Light UBO buffer for shader access during scene rendering. */
		for ( uint32_t cascadeIndex = 0; cascadeIndex < m_cascadeCount; ++cascadeIndex )
		{
			const auto & matrix = viewMatrices.cascadeViewProjectionMatrix(cascadeIndex);

			std::memcpy(&m_CSMBuffer[CSM_CascadeMatricesOffset + (cascadeIndex * 16)], matrix.data(), 16 * sizeof(float));
		}

		/* Copy split distances. */
		for ( uint32_t cascadeIndex = 0; cascadeIndex < m_cascadeCount; ++cascadeIndex )
		{
			m_CSMBuffer[CSM_SplitDistancesOffset + cascadeIndex] = viewMatrices.splitDistance(cascadeIndex);
		}

		/* Set cascade count and shadow bias. */
		m_CSMBuffer[CSM_CascadeCountOffset] = static_cast< float >(m_cascadeCount);
		m_CSMBuffer[CSM_ShadowBiasOffset] = m_shadowBias;

		/* Copy light properties. */
		/* ⚠️ The SECOND writer of a directional light's colour (onColorChange() fills the classic block only): the
		 * emitted chromaticity here too, or every CSM sun keeps the old convention. */
		m_CSMBuffer[CSM_ColorOffset + 0] = this->emissionChromaticity()[Math::X];
		m_CSMBuffer[CSM_ColorOffset + 1] = this->emissionChromaticity()[Math::Y];
		m_CSMBuffer[CSM_ColorOffset + 2] = this->emissionChromaticity()[Math::Z];
		m_CSMBuffer[CSM_ColorOffset + 3] = 1.0F;

		m_CSMBuffer[CSM_DirectionOffset + 0] = lightDirection.x();
		m_CSMBuffer[CSM_DirectionOffset + 1] = lightDirection.y();
		m_CSMBuffer[CSM_DirectionOffset + 2] = lightDirection.z();
		m_CSMBuffer[CSM_DirectionOffset + 3] = 0.0F;

		m_CSMBuffer[CSM_IntensityOffset] = this->intensity();

		/* ⚠️⚠️ WITHOUT THIS THE WHOLE FUNCTION IS A NO-OP AS FAR AS THE GPU IS CONCERNED, and that
		 * was the real cause of "a CSM light contributes NO light at all" — filed for months as an
		 * early return on a null shadow map, which it never was.
		 * updateVideoMemory() uploads only when this flag is set, and nothing else raises it for a
		 * light that does not MOVE: the only upload ever performed was the one from
		 * createOnHardware(), at a point where this function had not run yet and m_CSMBuffer was
		 * all zeros — black colour, zero intensity, null matrices. Measured on reflexion-debug
		 * (pinned exposure, one pose): ground mean 73.7 in classic mode against 16.2 in CSM, the
		 * remainder being the ambient term alone.
		 * Every frame is correct and not wasteful: the cascade matrices are re-fitted to the camera
		 * frustum on every logic tick, so their GPU copy is stale by construction otherwise. */
		this->requestVideoMemoryUpdate();
	}

	void
	DirectionalLight::updateLightSpaceMatrix () noexcept
	{
		const auto worldCoordinates = this->getWorldCoordinates();

		const auto lightDirection = m_useDirectionVector ?
			worldCoordinates.forwardVector() :
			-worldCoordinates.position().normalized();

		/* Build a view frame: camera positioned opposite to the light direction,
		 * at coverageSize distance from the origin, looking toward the scene. */
		CartesianFrame< float > frame;
		frame.setPosition(-lightDirection * m_coverageSize);
		frame.setBackwardVector(-lightDirection);

		const auto viewMatrix = frame.getViewMatrix();
		const auto projMatrix = Matrix< 4, float >::orthographicProjection(-m_coverageSize, m_coverageSize, -m_coverageSize, m_coverageSize, this->getFovOrNear(), this->getDistanceOrFar());

		const auto matrix = RenderTarget::ScaleBiasMatrix * projMatrix * viewMatrix;

		std::copy_n(matrix.data(), 16, m_buffer.data() + LightMatrixOffset);
	}

	void
	DirectionalLight::writeUniformBlock (float * destination) noexcept
	{
		if ( m_usesCSM )
		{
			std::copy_n(m_cloudShadowMatrix.data(), m_cloudShadowMatrix.size(), m_CSMBuffer.data() + CSM_CloudShadowMatrixOffset);
			m_CSMBuffer[CSM_CloudShadowIndexOffset] = std::bit_cast< float >(m_cloudShadowIndex);

			std::copy_n(m_CSMBuffer.data(), m_CSMBuffer.size(), destination);

			return;
		}

		m_buffer[ColorProjectionIndexOffset] = std::bit_cast< float >(this->colorProjectionBindlessIndex());
		m_buffer[ColorProjectionBoostOffset] = this->colorProjectionBoost();

		std::copy_n(m_cloudShadowMatrix.data(), m_cloudShadowMatrix.size(), m_buffer.data() + CloudShadowMatrixOffset);
		m_buffer[CloudShadowIndexOffset] = std::bit_cast< float >(m_cloudShadowIndex);

		std::copy_n(m_buffer.data(), m_buffer.size(), destination);
	}

	void
	DirectionalLight::updateCloudShadow (uint32_t bindlessIndex, const Vector< 3, float > & cameraPosition, float coverage, uint32_t resolution) noexcept
	{
		if ( coverage <= 0.0F || resolution == 0 )
		{
			this->disableCloudShadow();

			return;
		}

		const auto worldCoordinates = this->getWorldCoordinates();

		/* The direction the light PROPAGATES along — the same rule as the cascades. */
		const auto lightDirection = (m_useDirectionVector ? worldCoordinates.forwardVector() : -worldCoordinates.position().normalized()).normalized();

		/* An orthonormal frame around the light direction; a sun at the zenith takes X as its reference. */
		const auto reference = std::abs(lightDirection[Y]) < 0.99F ? Vector< 3, float >{0.0F, 1.0F, 0.0F} : Vector< 3, float >{1.0F, 0.0F, 0.0F};
		const auto right = Vector< 3, float >::crossProduct(lightDirection, reference).normalized();
		const auto up = Vector< 3, float >::crossProduct(right, lightDirection);

		/* ⚠️ Snapped to the texel grid: a map that slides with the camera by a fraction of a texel
		 * re-samples every cloud edge each frame, and the shadows crawl. */
		const auto texelSize = coverage / static_cast< float >(resolution);
		const auto centreRight = std::floor(Vector< 3, float >::dotProduct(cameraPosition, right) / texelSize) * texelSize;
		const auto centreUp = std::floor(Vector< 3, float >::dotProduct(cameraPosition, up) / texelSize) * texelSize;
		const auto centreDepth = Vector< 3, float >::dotProduct(cameraPosition, lightDirection);

		/* Rows: u = right.P / coverage + 0.5 - centre, v likewise, depth = light.P - camera depth.
		 * Stored COLUMN-major, as GLSL reads a mat4. */
		const std::array< float, 16 > matrix{
			right[X] / coverage, up[X] / coverage, lightDirection[X], 0.0F,
			right[Y] / coverage, up[Y] / coverage, lightDirection[Y], 0.0F,
			right[Z] / coverage, up[Z] / coverage, lightDirection[Z], 0.0F,
			0.5F - centreRight / coverage, 0.5F - centreUp / coverage, -centreDepth, 1.0F
		};

		/* Nothing to publish when the frame is the one already in the block (a camera inside one texel). */
		if ( matrix == m_cloudShadowMatrix && bindlessIndex == m_cloudShadowIndex )
		{
			return;
		}

		m_cloudShadowMatrix = matrix;
		m_cloudShadowIndex = bindlessIndex;

		this->requestVideoMemoryUpdate();
	}

	void
	DirectionalLight::disableCloudShadow () noexcept
	{
		if ( m_cloudShadowIndex == NoCloudShadow )
		{
			return;
		}

		m_cloudShadowIndex = NoCloudShadow;

		this->requestVideoMemoryUpdate();
	}

	Matrix< 4, float >
	DirectionalLight::cloudShadowMatrix (uint32_t readStateIndex) const noexcept
	{
		const auto & block = this->publishedBlock(readStateIndex);
		const auto offset = m_usesCSM ? CSM_CloudShadowMatrixOffset : CloudShadowMatrixOffset;

		Matrix< 4, float > matrix;

		std::copy_n(block.data() + offset, 16, matrix.data());

		return matrix;
	}

	uint32_t
	DirectionalLight::cloudShadowIndex (uint32_t readStateIndex) const noexcept
	{
		const auto & block = this->publishedBlock(readStateIndex);

		return std::bit_cast< uint32_t >(block[m_usesCSM ? CSM_CloudShadowIndexOffset : CloudShadowIndexOffset]);
	}

	std::ostream &
	operator<< (std::ostream & out, const DirectionalLight & obj)
	{
		const auto worldCoordinates = obj.getWorldCoordinates();

		return out << "Directional light data ;\n"
			"Direction (World Space) : " << worldCoordinates.forwardVector() << "\n"
			"Color : " << obj.authoredColor() << " (emitted chromaticity " << obj.emissionChromaticity() << ")" "\n"
			"Intensity : " << obj.intensity() << "\n"
			"Activity : " << ( obj.isEnabled() ? "true" : "false" ) << "\n"
			"Shadow caster : " << ( obj.isShadowCastingEnabled() ? "true" : "false" ) << '\n';
	}
}

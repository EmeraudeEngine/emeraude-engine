/*
 * src/Scenes/Component/DirectionalLight.hpp
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

#pragma once

/* STL inclusions. */
#include <array>

/* Local inclusions for inheritances. */
#include "AbstractLightEmitter.hpp"

/* Local inclusions for usages. */
#include "Graphics/ViewMatricesCascadedUBO.hpp"
#include "Graphics/RenderTarget/ShadowMapCascaded.hpp"

namespace EmEn::Scenes::Component
{
	/**
	 * @brief Defines a scene directional light like the sun.
	 * @extends EmEn::Scenes::Component::AbstractLightEmitter The base class for each light type.
	 */
	class DirectionalLight final : public AbstractLightEmitter
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"DirectionalLight"};

			/**
			 * @brief Constructs a directional light without shadow mapping.
			 * @param componentName A reference to a string.
			 * @param parentEntity A reference to the parent entity.
			 */
			DirectionalLight (const std::string & componentName, const AbstractEntity & parentEntity) noexcept
				: AbstractLightEmitter{componentName, parentEntity, 0}
			{

			}

			/**
			 * @brief Constructs a directional light with shadow mapping.
			 * @param componentName A reference to a string.
			 * @param parentEntity A reference to the parent entity.
			 * @param shadowMapResolution The shadow map resolution.
			 * @param coverageSize The coverage size in world units.
			 */
			DirectionalLight (const std::string & componentName, const AbstractEntity & parentEntity, uint32_t shadowMapResolution, float coverageSize) noexcept
				: AbstractLightEmitter{componentName, parentEntity, shadowMapResolution},
				m_coverageSize{coverageSize}
			{

			}

			/**
			 * @brief Constructs a directional light with cascaded shadow mapping.
			 * @param componentName A reference to a string.
			 * @param parentEntity A reference to the parent entity.
			 * @param shadowMapResolution The shadow map resolution.
			 * @param coverageSize The coverage size in world units.
			 * @param cascadeCount The number of cascades (1-4) when using CSM.
			 * @param lambda The split factor (0 = linear, 1 = logarithmic, 0.5 = balanced) when using CSM. Default is 0.5.
			 */
			DirectionalLight (const std::string & componentName, const AbstractEntity & parentEntity, uint32_t shadowMapResolution, float coverageSize, uint32_t cascadeCount, float lambda = Graphics::DefaultCascadeLambda) noexcept
				: AbstractLightEmitter{componentName, parentEntity, shadowMapResolution},
				m_coverageSize{coverageSize},
				m_lambda{std::clamp(lambda, 0.0F, 1.0F)},
				m_cascadeCount{std::clamp(cascadeCount, 1U, Graphics::MaxCascadeCount)},
				m_usesCSM{true}
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			DirectionalLight (const DirectionalLight & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			DirectionalLight (DirectionalLight && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return DirectionalLight &
			 */
			DirectionalLight & operator= (const DirectionalLight & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return DirectionalLight &
			 */
			DirectionalLight & operator= (DirectionalLight && copy) noexcept = delete;

			/**
			 * @brief Destructs a directional light.
			 */
			~DirectionalLight () override = default;

			/** @copydoc EmEn::Scenes::Component::Abstract::getComponentType() */
			[[nodiscard]]
			const char *
			getComponentType () const noexcept override
			{
				return ClassId;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::isComponent() */
			[[nodiscard]]
			bool
			isComponent (const char * classID) const noexcept override
			{
				return strcmp(ClassId, classID) == 0;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::processLogics() */
			void processLogics (const Scene & scene) noexcept override;

			/** @copydoc EmEn::Scenes::Component::Abstract::move() */
			void move (const Libs::Math::CartesianFrame< float > & worldCoordinates) noexcept override;

			/** @copydoc EmEn::Scenes::Component::Abstract::shouldBeRemoved() */
			[[nodiscard]]
			bool
			shouldBeRemoved () const noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::touch() */
			[[nodiscard]]
			bool
			touch (const Libs::Math::Vector< 3, float > & /*position*/) const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::createOnHardware() */
			[[nodiscard]]
			bool createOnHardware (Scene & scene) noexcept override;

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::destroyFromHardware() */
			void destroyFromHardware (Scene & scene) noexcept override;

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::shadowMap() */
			[[nodiscard]]
			std::shared_ptr< Graphics::RenderTarget::Abstract >
			shadowMap () const noexcept override
			{
				if ( m_usesCSM && m_shadowMapCascaded )
				{
					return std::static_pointer_cast< Graphics::RenderTarget::Abstract >(m_shadowMapCascaded);
				}

				return std::static_pointer_cast< Graphics::RenderTarget::Abstract >(m_shadowMap);
			}

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::descriptorSet() */
			[[nodiscard]]
			const Vulkan::DescriptorSet *
			descriptorSet (bool useShadowMap) const noexcept override;

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::hasShadowDescriptorSet() */
			[[nodiscard]]
			bool
			hasShadowDescriptorSet () const noexcept override
			{
				return m_shadowDescriptorSet != nullptr;
			}

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::getUniformBlock() */
			[[nodiscard]]
			Saphir::Declaration::UniformBlock getUniformBlock (uint32_t set, uint32_t binding, bool useShadow) const noexcept override;

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::setPCFRadius(float) */
			void setPCFRadius (float radius) noexcept override;

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::PCFRadius() */
			[[nodiscard]]
			float
			PCFRadius () const noexcept override
			{
				return m_PCFRadius;
			}

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::setShadowBias(float) */
			void setShadowBias (float bias) noexcept override;

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::shadowBias() */
			[[nodiscard]]
			float
			shadowBias () const noexcept override
			{
				return m_shadowBias;
			}

			/**
			 * @brief Sets the light direction from the coordinate direction instead of the position to origin.
			 * @param state The state.
			 * @brief void
			 */
			void
			useDirectionVector (bool state) noexcept
			{
				m_useDirectionVector = state;
			}

			/**
			 * @brief Returns whether the light direction is using the coordinate direction instead of the position to origin.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isUsingDirectionVector () const noexcept
			{
				return m_useDirectionVector;
			}

			/**
			 * @brief Returns whether this light uses Cascaded Shadow Maps.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			usesCSM () const noexcept
			{
				return m_usesCSM;
			}

			/**
			 * @brief Returns the number of cascades.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			cascadeCount () const noexcept
			{
				return m_cascadeCount;
			}

			/**
			 * @brief Returns the lambda value for cascade split calculation.
			 * @return float
			 */
			[[nodiscard]]
			float
			cascadeLambda () const noexcept
			{
				return m_lambda;
			}

			/**
			 * @brief Returns the coverage size for classic shadow mapping.
			 * @return float The coverage size in world units (0 = CSM mode).
			 */
			[[nodiscard]]
			float
			coverageSize () const noexcept
			{
				return m_coverageSize;
			}

			/**
			 * @brief Sets the coverage size for shadow mapping.
			 * @param size The coverage size in world units. If 0, uses CSM. If > 0, uses classic shadow map.
			 * @return void
			 */
			void
			setCoverageSize (float size) noexcept
			{
				m_coverageSize = std::max(0.0F, size);
				m_usesCSM = (m_coverageSize <= 0.0F && this->shadowMapResolution() > 0);
			}

			/**
			 * @brief Sets the cascade count (only effective when using CSM).
			 * @param count The number of cascades (1-4).
			 * @return void
			 */
			void
			setCascadeCount (uint32_t count) noexcept
			{
				m_cascadeCount = std::clamp(count, 1U, Graphics::MaxCascadeCount);
			}

			/**
			 * @brief Sets the cascade lambda value (only effective when using CSM).
			 * @param lambda The lambda value (0 = linear, 1 = logarithmic).
			 * @return void
			 */
			void
			setCascadeLambda (float lambda) noexcept
			{
				m_lambda = std::clamp(lambda, 0.0F, 1.0F);
			}

			/**
			 * @brief Returns the cascaded shadow map (CSM only).
			 * @return std::shared_ptr< Graphics::RenderTarget::ShadowMapCascaded >
			 */
			[[nodiscard]]
			std::shared_ptr< Graphics::RenderTarget::ShadowMapCascaded >
			shadowMapCascaded () const noexcept
			{
				return m_shadowMapCascaded;
			}

			/**
			 * @brief Updates the cascade matrices based on the camera frustum.
			 * @note This should be called every frame when CSM is enabled.
			 * @param cameraFrustumCorners Array of 8 corners of the camera frustum in world space.
			 * @param nearPlane The camera near plane distance.
			 * @param farPlane The camera far plane distance.
			 * @return void
			 */
			void updateCascades (const std::array< Libs::Math::Vector< 3, float >, 8 > & cameraFrustumCorners, float nearPlane, float farPlane) const noexcept;

		private:

			/** @copydoc EmEn::Animations::AnimatableInterface::playAnimation() */
			bool playAnimation (uint8_t animationID, const Libs::Variant & value, size_t cycle) noexcept override;

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::createShadowDescriptorSet() */
			bool createShadowDescriptorSet (Scene & scene) noexcept override;

			/**
			 * @brief Creates the descriptor set for cascaded shadow mapping.
			 * @param scene A reference to the scene.
			 * @return bool True if successful.
			 */
			bool createShadowDescriptorSetCSM (Scene & scene) noexcept;

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::updateLightSpaceMatrix() */
			void updateLightSpaceMatrix () noexcept override;

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::getFovOrNear() */
			[[nodiscard]]
			float
			getFovOrNear () const noexcept override
			{
				/* NOTE: A directional light returns the near value. */
				return 0.0F;
			}

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::getDistanceOrFar() */
			[[nodiscard]]
			float
			getDistanceOrFar () const noexcept override
			{
				/* NOTE: For classic shadow map, we need the far plane at coverage * 2.
				 * The camera is positioned at coverage distance from origin, so
				 * far = coverage * 2 means the frustum extends coverage units past origin,
				 * creating a symmetric depth range around the scene center. */
				return m_coverageSize > 0.0F ? m_coverageSize * 2.0F : DefaultGraphicsShadowMappingViewDistance;
			}

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::isOrthographicProjection() */
			[[nodiscard]]
			bool
			isOrthographicProjection () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::onVideoMemoryUpdate() */
			[[nodiscard]]
			bool onVideoMemoryUpdate (Graphics::SharedUniformBuffer & UBO, uint32_t index) noexcept override;

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::onColorChange() */
			void onColorChange (const Libs::PixelFactory::Color< float > & color) noexcept override;

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::onIntensityChange() */
			void onIntensityChange (float intensity) noexcept override;

			/**
			 * @brief Sets the light direction into the buffer.
			 * @brief A reference to a cartesian frame for the light location.
			 * @return void
			 */
			void setDirection (const Libs::Math::CartesianFrame< float > & worldCoordinates) noexcept;

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const DirectionalLight & obj);

			/* Uniform buffer object offset to write data (std140 layout).
			 * vec4 Color: floats 0-3
			 * vec4 Direction: floats 4-7
			 * float Intensity: float 8
			 * float PCFRadius: float 9
			 * float ShadowBias: float 10
			 * float padding: float 11
			 * mat4 ViewProjectionMatrix: floats 12-27
			 */
			static constexpr auto ColorOffset{0UL};
			static constexpr auto DirectionOffset{4UL};
			static constexpr auto IntensityOffset{8UL};
			static constexpr auto PCFRadiusOffset{9UL};
			static constexpr auto ShadowBiasOffset{10UL};
			static constexpr auto LightMatrixOffset{12UL};

			std::shared_ptr< Graphics::RenderTarget::ShadowMap< Graphics::ViewMatrices2DUBO > > m_shadowMap;
			std::shared_ptr< Graphics::RenderTarget::ShadowMapCascaded > m_shadowMapCascaded;
			std::unique_ptr< Vulkan::DescriptorSet > m_shadowDescriptorSet;
			std::unique_ptr< Vulkan::DescriptorSet > m_shadowDescriptorSetCSM;
			float m_coverageSize{0.0F}; /**< Coverage size in world units. 0 = CSM, > 0 = classic shadow map. */
			float m_PCFRadius{1.0F}; /**< PCF filter radius in normalized texture coordinates. */
			float m_shadowBias{0.0F}; /**< Shadow bias to prevent shadow acne. */
			float m_lambda{Graphics::DefaultCascadeLambda};
			uint32_t m_cascadeCount{Graphics::MaxCascadeCount};
			std::array< float, 4 + 4 + 4 + 16 > m_buffer{
				/* Light color. */
				this->color().red(), this->color().green(), this->color().blue(), 1.0F,
				/* Light direction (Directional). */
				0.0F, 1.0F, 0.0F, 0.0F,
				/* Light properties. */
				this->intensity(), m_PCFRadius, m_shadowBias, 0.0F,
				/* Light matrix. */
				1.0F, 0.0F, 0.0F, 0.0F,
				0.0F, 1.0F, 0.0F, 0.0F,
				0.0F, 0.0F, 1.0F, 0.0F,
				0.0F, 0.0F, 0.0F, 1.0F
			};
			bool m_useDirectionVector{false};
			bool m_usesCSM{false};
	};

	inline
	std::ostream &
	operator<< (std::ostream & out, const DirectionalLight & obj)
	{
		const auto worldCoordinates = obj.getWorldCoordinates();

		return out << "Directional light data ;\n"
			"Direction (World Space) : " << worldCoordinates.forwardVector() << "\n"
			"Color : " << obj.color() << "\n"
			"Intensity : " << obj.intensity() << "\n"
			"Activity : " << ( obj.isEnabled() ? "true" : "false" ) << "\n"
			"Shadow caster : " << ( obj.isShadowCastingEnabled() ? "true" : "false" ) << '\n';
	}

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	inline
	std::string
	to_string (const DirectionalLight & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}

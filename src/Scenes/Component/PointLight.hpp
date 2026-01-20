/*
 * src/Scenes/Component/PointLight.hpp
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

namespace EmEn::Scenes::Component
{
	/**
	 * @brief Defines a scene point light like a lamp bulb.
	 * @extends EmEn::Scenes::Component::AbstractLightEmitter The base class for each light type.
	 */
	class PointLight final : public AbstractLightEmitter
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"PointLight"};

			/**
			 * @brief Constructs a point light.
			 * @param componentName A reference to a string.
			 * @param parentEntity A reference to the parent entity.
			 */
			PointLight (const std::string & componentName, const AbstractEntity & parentEntity) noexcept
				: AbstractLightEmitter{componentName, parentEntity, 0}
			{

			}

			/**
			 * @brief Constructs a point light.
			 * @param componentName A reference to a string.
			 * @param parentEntity A reference to the parent entity.
			 * @param shadowMapResolution Enable the shadow map by specifying the resolution.
			 */
			PointLight (const std::string & componentName, const AbstractEntity & parentEntity, uint32_t shadowMapResolution) noexcept
				: AbstractLightEmitter{componentName, parentEntity, shadowMapResolution}
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			PointLight (const PointLight & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			PointLight (PointLight && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return PointLight &
			 */
			PointLight & operator= (const PointLight & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return PointLight &
			 */
			PointLight & operator= (PointLight && copy) noexcept = delete;

			/**
			 * @brief Destructs a point light.
			 */
			~PointLight () override = default;

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
			bool touch (const Libs::Math::Vector< 3, float > & position) const noexcept override;

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
			 * @brief Set the radius of light area.
			 * @param radius
			 */
			void setRadius (float radius) noexcept;

			/**
			 * @brief Returns the radius of light area.
			 * @return float
			 */
			[[nodiscard]]
			float
			radius () const noexcept
			{
				return m_radius;
			}

		private:

			/** @copydoc EmEn::Animations::AnimatableInterface::playAnimation() */
			bool playAnimation (uint8_t animationID, const Libs::Variant & value, size_t cycle) noexcept override;

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::createShadowDescriptorSet() */
			bool createShadowDescriptorSet (Scene & scene) noexcept override;

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::updateLightSpaceMatrix() */
			void updateLightSpaceMatrix () noexcept override;

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::getFovOrNear() */
			[[nodiscard]]
			float
			getFovOrNear () const noexcept override
			{
				/* NOTE: A point light returns the field of view in degrees. */
				return 90.0F;
			}

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::getDistanceOrFar() */
			[[nodiscard]]
			float
			getDistanceOrFar () const noexcept override
			{
				/* NOTE: A point light returns the distance. */
				return m_radius > 0.0F ? m_radius : DefaultGraphicsShadowMappingViewDistance;
			}

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::isOrthographicProjection() */
			[[nodiscard]]
			bool
			isOrthographicProjection () const noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::onVideoMemoryUpdate() */
			[[nodiscard]]
			bool onVideoMemoryUpdate (Graphics::SharedUniformBuffer & UBO, uint32_t index) noexcept override;

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::onColorChange() */
			void onColorChange (const Libs::PixelFactory::Color< float > & color) noexcept override;

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::onIntensityChange() */
			void onIntensityChange (float intensity) noexcept override;

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const PointLight & obj);

			/* Uniform buffer object offset to write data (std140 layout).
			 * vec4 Color: floats 0-3
			 * vec4 Position: floats 4-7
			 * float Intensity: float 8
			 * float Radius: float 9
			 * float PCFRadius: float 10
			 * float ShadowBias: float 11
			 * mat4 ViewProjectionMatrix: floats 12-27
			 */
			static constexpr auto ColorOffset{0UL};
			static constexpr auto PositionOffset{4UL};
			static constexpr auto IntensityOffset{8UL};
			static constexpr auto RadiusOffset{9UL};
			static constexpr auto PCFRadiusOffset{10UL};
			static constexpr auto ShadowBiasOffset{11UL};
			static constexpr auto LightMatrixOffset{12UL};

			std::shared_ptr< Graphics::RenderTarget::ShadowMap< Graphics::ViewMatrices3DUBO > > m_shadowMap;
			std::unique_ptr< Vulkan::DescriptorSet > m_shadowDescriptorSet;
			float m_radius{DefaultRadius};
			float m_PCFRadius{1.0F}; /**< PCF filter radius in normalized texture coordinates. */
			float m_shadowBias{0.0F}; /**< Shadow bias to prevent shadow acne. */
			std::array< float, 4 + 4 + 4 + 16 > m_buffer{
				/* Light color. */
				this->color().red(), this->color().green(), this->color().blue(), 1.0F,
				/* Light position (Point) */
				0.0F, 0.0F, 0.0F, 1.0F, // NOTE: Put W to zero and the light will follows the camera.
				/* Light properties. */
				this->intensity(), m_radius, m_PCFRadius, m_shadowBias,
				/* Light matrix. */
				1.0F, 0.0F, 0.0F, 0.0F,
				0.0F, 1.0F, 0.0F, 0.0F,
				0.0F, 0.0F, 1.0F, 0.0F,
				0.0F, 0.0F, 0.0F, 1.0F
			};
	};

	inline
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

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	inline
	std::string
	to_string (const PointLight & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}

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
			 * @brief Constructs a directional light.
			 * @param componentName A reference to a string.
			 * @param parentEntity A reference to the parent entity.
			 * @param shadowMapResolution Enable the shadow map by specifying the resolution. Default, no shadow map.
			 */
			DirectionalLight (const std::string & componentName, const AbstractEntity & parentEntity, uint32_t shadowMapResolution = 0) noexcept
				: AbstractLightEmitter{componentName, parentEntity, shadowMapResolution}
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
				return std::static_pointer_cast< Graphics::RenderTarget::Abstract >(m_shadowMap);
			}

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::getUniformBlock() */
			[[nodiscard]]
			Saphir::Declaration::UniformBlock getUniformBlock (uint32_t set, uint32_t binding, bool useShadow) const noexcept override;

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

		private:

			/** @copydoc EmEn::Animations::AnimatableInterface::playAnimation() */
			bool playAnimation (uint8_t animationID, const Libs::Variant & value, size_t cycle) noexcept override;

			/**
			 * @brief Sets the light direction into the buffer.
			 * @brief A reference to a cartesian frame for the light location.
			 * @return void
			 */
			void setDirection (const Libs::Math::CartesianFrame< float > & worldCoordinates) noexcept;

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
				/* NOTE: A directional light returns the far value. */
				return DefaultGraphicsShadowMappingViewDistance;
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
			bool
			onVideoMemoryUpdate (Graphics::SharedUniformBuffer & UBO, uint32_t index) noexcept override
			{
				return UBO.writeElementData(index, m_buffer.data());
			}

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
			friend std::ostream & operator<< (std::ostream & out, const DirectionalLight & obj);

			/* Uniform buffer object offset to write data. */
			static constexpr auto ColorOffset{0UL};
			static constexpr auto DirectionOffset{4UL};
			static constexpr auto IntensityOffset{8UL};
			static constexpr auto LightMatrixOffset{16UL};

			std::shared_ptr< Graphics::RenderTarget::ShadowMap< Graphics::ViewMatrices2DUBO > > m_shadowMap;
			std::array< float, 4 + 4 + 4 + 16 > m_buffer{
				/* Light color. */
				this->color().red(), this->color().green(), this->color().blue(), 1.0F,
				/* Light direction (Directional). */
				0.0F, 1.0F, 0.0F, 0.0F,
				/* Light properties. */
				this->intensity(), 0.0F, 0.0F, 0.0F,
				/* Light matrix. */
				1.0F, 0.0F, 0.0F, 0.0F,
				0.0F, 1.0F, 0.0F, 0.0F,
				0.0F, 0.0F, 1.0F, 0.0F,
				0.0F, 0.0F, 0.0F, 1.0F
			};
			bool m_useDirectionVector{false};
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

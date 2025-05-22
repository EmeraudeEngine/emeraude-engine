/*
 * src/Scenes/Component/SpotLight.hpp
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

#pragma once

/* STL inclusions. */
#include <array>

/* Local inclusions for inheritances. */
#include "AbstractLightEmitter.hpp"

namespace EmEn::Scenes::Component
{
	/**
	 * @brief Defines a scene spotlight like an electrical light torch.
	 * @extends EmEn::Scenes::Component::AbstractLightEmitter The base class for each light type.
	 */
	class SpotLight final : public AbstractLightEmitter
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SpotLight"};

			/**
			 * @brief Constructs a spotlight.
			 * @param name The name of the component.
			 * @param parentEntity A reference to the parent entity.
			 * @param shadowMapResolution Enable the shadow map by specifying the resolution. Default, no shadow map.
			 */
			SpotLight (const std::string & name, const AbstractEntity & parentEntity, uint32_t shadowMapResolution = 0) noexcept;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			SpotLight (const SpotLight & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			SpotLight (SpotLight && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return SpotLight &
			 */
			SpotLight & operator= (const SpotLight & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return SpotLight &
			 */
			SpotLight & operator= (SpotLight && copy) noexcept = delete;

			/**
			 * @brief Destructs a spotlight.
			 */
			~SpotLight () override;

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
			bool createOnHardware (LightSet & lightSet, Graphics::Renderer & renderer, AVConsole::Manager & AVConsoleManager) noexcept override;

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::destroyFromHardware() */
			void destroyFromHardware () noexcept override;

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
			 * @brief Set the radius of light area.
			 * @param radius
			 */
			void setRadius (float radius) noexcept;

			/**
			 * @brief Sets the inner and the outer angles of the light cone.
			 * @param innerAngle The inner angle in degree of the cone where light is 100%.
			 * @param outerAngle The outer angle in degree of the cone until the light is off.
			 */
			void setConeAngles (float innerAngle, float outerAngle = 0.0F) noexcept;

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

			/**
			 * @brief Returns the inner angle in degree of the cone where light is 100%.
			 * @return float
			 */
			[[nodiscard]]
			float
			innerAngle () const noexcept
			{
				return m_innerAngle;
			}

			/**
			 * @brief Returns the outer angle in degree of the cone until the light is off.
			 * @return float
			 */
			[[nodiscard]]
			float
			outerAngle () const noexcept
			{
				return m_outerAngle;
			}

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const SpotLight & obj);

			/**
			 * @brief Stringifies the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend std::string to_string (const SpotLight & obj) noexcept;

		private:

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::onTargetConnected() */
			void onTargetConnected (AbstractVirtualDevice * targetDevice) noexcept override;

			/** @copydoc EmEn::Animations::AnimatableInterface::playAnimation() */
			bool playAnimation (uint8_t animationID, const Libs::Variant & value, size_t cycle) noexcept override;

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::onVideoMemoryUpdate() */
			[[nodiscard]]
			bool
			onVideoMemoryUpdate (Vulkan::SharedUniformBuffer & UBO, uint32_t index) noexcept override
			{
				return UBO.writeElementData(index, m_buffer.data());
			}

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::onColorChange() */
			void onColorChange (const Libs::PixelFactory::Color< float > & color) noexcept override;

			/** @copydoc EmEn::Scenes::Component::AbstractLightEmitter::onIntensityChange() */
			void onIntensityChange (float intensity) noexcept override;

			/**
			 * @brief Sets the inner angle of the cone where light is 100%.
			 * @param angle The angle in degree.
			 */
			void setInnerAngle (float angle) noexcept;

			/**
			 * @brief Sets the outer angle of the cone until the light is off.
			 * @param angle The angle in degree.
			 */
			void setOuterAngle (float angle) noexcept;

			/* Uniform buffer object offset to write data. */
			static constexpr auto ColorOffset{0UL};
			static constexpr auto PositionOffset{4UL};
			static constexpr auto DirectionOffset{8UL};
			static constexpr auto IntensityOffset{12UL};
			static constexpr auto RadiusOffset{13UL};
			static constexpr auto InnerCosAngleOffset{14UL};
			static constexpr auto OuterCosAngleOffset{15UL};
			static constexpr auto LightMatrixOffset{16UL};

			std::shared_ptr< Graphics::RenderTarget::ShadowMap< Graphics::ViewMatrices2DUBO > > m_shadowMap;
			float m_radius{DefaultRadius};
			float m_innerAngle{DefaultInnerAngle};
			float m_outerAngle{DefaultOuterAngle};
			std::array< float, 4 + 4 + 4 + 4 + 16 > m_buffer{
				/* Light color. */
				this->color().red(), this->color().green(), this->color().blue(), 1.0F,
				/* Light position (Spot) */
				0.0F, 0.0F, 0.0F, 1.0F, // NOTE: Put W to zero and the light will follows the camera.
				/* Light direction (Spot). */
				0.0F, 1.0F, 0.0F, 0.0F,
				/* Light properties. */
				this->intensity(), m_radius, std::cos(Libs::Math::Radian(m_innerAngle)), std::cos(Libs::Math::Radian(m_outerAngle)),
				/* Light matrix. */
				1.0F, 0.0F, 0.0F, 0.0F,
				0.0F, 1.0F, 0.0F, 0.0F,
				0.0F, 0.0F, 1.0F, 0.0F,
				0.0F, 0.0F, 0.0F, 1.0F
			};
	};
}

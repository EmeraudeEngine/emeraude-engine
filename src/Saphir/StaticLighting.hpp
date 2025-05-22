/*
 * src/Saphir/StaticLighting.hpp
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
#include <cmath>
#include <string>
#include <sstream>

/* Local inclusions for usages. */
#include "Libs/Math/Base.hpp"
#include "Libs/Math/Vector.hpp"
#include "Libs/PixelFactory/Color.hpp"
#include "Graphics/Types.hpp"

namespace EmEn::Saphir
{
	/**
	 * @brief The static lighting class.
	 */
	class StaticLighting final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"StaticLighting"};

			/* Default variables. */
			static constexpr auto DefaultAmbientIntensity{0.10F};
			static constexpr auto DefaultLightIntensity{1.0F};
			static constexpr auto DefaultLightRadius{0.0F};
			static constexpr auto DefaultSpotLightInnerAngle{30.0F};
			static constexpr auto DefaultSpotLightOuterAngle{45.0F};

			/** 
			 * @brief Constructs a static lighting.
			 */
			StaticLighting () noexcept = default;

			/**
			 * @brief Configures the ambient part of the static lighting.
			 * @param color A reference to a color.
			 * @param intensity The intensity of the ambient. Default 0.10.
			 * @return StaticLighting &
			 */
			StaticLighting &
			setAmbientParameters (const Libs::PixelFactory::Color< float > & color, float intensity = DefaultAmbientIntensity) noexcept
			{
				m_ambientColor = color;
				m_ambientIntensity = intensity;

				return *this;
			}

			/**
			 * @brief Configures the common parameters of the static lighting.
			 * @param color A reference to a color.
			 * @param intensity The intensity of the light. Default 1.0.
			 * @return StaticLighting &
			 */
			StaticLighting &
			setLightParameters (const Libs::PixelFactory::Color< float > & color, float intensity = DefaultLightIntensity) noexcept
			{
				m_color = color;
				m_intensity = intensity;

				return *this;
			}

			/**
			 * @brief Configures the static light as a directional.
			 * @param direction A reference to a vector for the light direction in world space.
			 * @return void
			 */
			void
			setAsDirectionalLight (const Libs::Math::Vector< 3, float > & direction) noexcept
			{
				m_type = Graphics::LightType::Directional;
				m_direction = direction.normalized();
			}

			/**
			 * @brief Configures the static light as a point.
			 * @param position A reference to a vector for the light position in world space.
			 * @param radius The radius of the light emission. Default 0.0 (Infinite).
			 * @return void
			 */
			void
			setAsPointLight (const Libs::Math::Vector< 3, float > & position, float radius = DefaultLightRadius) noexcept
			{
				m_type = Graphics::LightType::Point;
				m_position = position;
				m_radius = std::max(0.0F, radius);
			}

			/**
			 * @brief Configures the static light as a spot.
			 * @param position A reference to a vector for the light position in world space.
			 * @param pointTo A reference to a vector for computing the light direction in world space.
			 * @param innerAngle The inner angle of the spot in degree. Default 30°.
			 * @param outerAngle The inner angle of the spot in degree. Default 45°.
			 * @param radius The radius of the light emission. Default 0.0 (Infinite).
			 * @return StaticLighting &
			 */
			void
			setAsSpotLight (const Libs::Math::Vector< 3, float > & position, const Libs::Math::Vector< 3, float > & pointTo, float innerAngle = DefaultSpotLightInnerAngle, float outerAngle = DefaultSpotLightOuterAngle, float radius = DefaultLightRadius) noexcept
			{
				m_type = Graphics::LightType::Spot;
				m_position = position;
				m_direction = (pointTo - position).normalized();
				m_radius = std::max(0.0F, radius);

				if ( innerAngle > outerAngle )
				{
					/* NOTE: Swap angles if the outer angle is bigger than the inner angle. */
					m_innerCosAngle = std::cos(Libs::Math::Radian(outerAngle));
					m_outerCosAngle = std::cos(Libs::Math::Radian(innerAngle));
				}
				else
				{
					m_innerCosAngle = std::cos(Libs::Math::Radian(innerAngle));
					m_outerCosAngle = std::cos(Libs::Math::Radian(outerAngle));
				}
			}

			/**
			 * @brief Returns the static light type.
			 * @return Graphics::LightType
			 */
			[[nodiscard]]
			Graphics::LightType
			type () const noexcept
			{
				return m_type;
			}

			/**
			 * @brief Returns the position as a GLSL vec3.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string
			positionVec3 () const noexcept
			{
				std::stringstream output;

				output << "vec3(" << m_position[Libs::Math::X] << ", " << m_position[Libs::Math::Y] << ", " << m_position[Libs::Math::Z] << ")";

				return output.str();
			}

			/**
			 * @brief Returns the position as a GLSL vec4.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string
			positionVec4 () const noexcept
			{
				std::stringstream output;

				output << "vec4(" << m_position[Libs::Math::X] << ", " << m_position[Libs::Math::Y] << ", " << m_position[Libs::Math::Z] << ", 1.0)";

				return output.str();
			}

			/**
			 * @brief Returns the direction as a GLSL vec3.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string
			directionVec3 () const noexcept
			{
				std::stringstream output;

				output << "vec3(" << m_direction[Libs::Math::X] << ", " << m_direction[Libs::Math::Y] << ", " << m_direction[Libs::Math::Z] << ")";

				return output.str();
			}

			/**
			 * @brief Returns the direction as a GLSL vec4.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string
			directionVec4 () const noexcept
			{
				std::stringstream output;

				output << "vec4(" << m_direction[Libs::Math::X] << ", " << m_direction[Libs::Math::Y] << ", " << m_direction[Libs::Math::Z] << ", 0.0)";

				return output.str();
			}

			/**
			 * @brief Returns the ambient color as a GLSL vec4.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string
			ambientColorVec4 () const noexcept
			{
				std::stringstream output;

				output << "vec4(" << m_ambientColor.red() << ", " << m_ambientColor.green() << ", " << m_ambientColor.blue() << ", 1.0)";

				return output.str();
			}

			/**
			 * @brief Returns the light color as a GLSL vec4.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string
			colorVec4 () const noexcept
			{
				std::stringstream output;

				output << "vec4(" << m_color.red() << ", " << m_color.green() << ", " << m_color.blue() << ", 1.0)";

				return output.str();
			}

			/**
			 * @brief Returns the ambient intensity.
			 * @return float
			 */
			[[nodiscard]]
			float
			ambientIntensity () const noexcept
			{
				return m_ambientIntensity;
			}

			/**
			 * @brief Returns the light intensity.
			 * @return float
			 */
			[[nodiscard]]
			float
			intensity () const noexcept
			{
				return m_intensity;
			}

			/**
			 * @brief Returns the radius for point and spotlight.
			 * @return float
			 */
			[[nodiscard]]
			float
			radius () const noexcept
			{
				return m_radius;
			}

			/**
			 * @brief Returns the cosine of the spot inner angle.
			 * @return float
			 */
			[[nodiscard]]
			float
			innerCosAngle () const noexcept
			{
				return m_innerCosAngle;
			}

			/**
			 * @brief Returns the cosine of the spot outer angle.
			 * @return float
			 */
			[[nodiscard]]
			float
			outerCosAngle () const noexcept
			{
				return m_outerCosAngle;
			}

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend
			std::ostream &
			operator<< (std::ostream & out, const StaticLighting & obj)
			{
				return out <<
					"Static lighting data:" "\n"
					"Type: " << to_string(obj.m_type) << "\n"
					"Position (Spot & Point): " << obj.m_position << "\n"
					"Direction (Directional & Spot): " << obj.m_direction << "\n"
					"Ambient color: " << obj.m_ambientColor << "\n"
					"Light color: " << obj.m_color << "\n"
					"Ambient intensity: " << obj.m_ambientIntensity << "\n"
					"Light intensity: " << obj.m_intensity << "\n"
					"Radius (Spot & Point): " << obj.m_radius << "\n"
					"Inner cosine angle (Spot): " << obj.m_innerCosAngle << "\n"
					"Outer cosine angler (Spot): " << obj.m_outerCosAngle << "\n";
			}

			/**
			 * @brief Stringifies the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */

			friend
			std::string
			to_string (const StaticLighting & obj) noexcept
			{
				std::stringstream output;

				output << obj;

				return output.str();
			}

		private:

			Graphics::LightType m_type{Graphics::LightType::Directional};
			Libs::Math::Vector< 3, float > m_position{250.0F, -500.0F, 250.0F}; /* (Spot & Point) */
			Libs::Math::Vector< 3, float > m_direction{-0.333F, 0.333F, -0.333F}; /* (Directional & Spot) */
			Libs::PixelFactory::Color< float > m_ambientColor{Libs::PixelFactory::DarkCyan};
			Libs::PixelFactory::Color< float > m_color{Libs::PixelFactory::White}; /* (All) */
			float m_ambientIntensity{DefaultAmbientIntensity};
			float m_intensity{DefaultLightIntensity}; /* (All) */
			float m_radius{DefaultLightRadius}; /* (Spot & Point) */
			float m_innerCosAngle{std::cos(Libs::Math::Radian(DefaultSpotLightInnerAngle))}; /* (Spot) */
			float m_outerCosAngle{std::cos(Libs::Math::Radian(DefaultSpotLightOuterAngle))}; /* (Spot) */
	};
}

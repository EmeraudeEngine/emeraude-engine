/*
 * src/Graphics/CelestialBody.hpp
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

#pragma once

/* STL inclusions. */
#include <algorithm>
#include <cstdint>
#include <string>

/* Local inclusions for usages. */
#include "Graphics/Photometry.hpp"
#include "Math/Vector.hpp"
#include "PixelFactory/Color.hpp"

namespace EmEn::Graphics
{
	/**
	 * @brief Describes a distant light emitter identified in a background (a sun, a moon, a
	 * bright star, ...), carrying the photometric parameters needed to derive an analytic
	 * directional light from the sky.
	 * @note The contract: the direction points TOWARD the body in the engine frame (UP = +Y),
	 * the illuminance is measured on a surface facing the body, in lux, and the color is
	 * resolved in sRGB — either authored directly or derived from a color temperature in
	 * kelvins, the industry-standard authoring (UE "Use Temperature", Unity HDRP
	 * "Color Temperature"). A background can declare zero (pure ambiance), one, or several
	 * bodies (binary suns, multiple moons).
	 */
	class CelestialBody final
	{
		public:

			/** @brief The kind of celestial body, for semantics and authoring defaults. */
			enum class Type : uint8_t
			{
				Sun,
				Moon,
				Star
			};

			/** @brief Angular diameter of the sun AND the full moon seen from Earth, in degrees. */
			static constexpr auto EarthlikeAngularDiameter{0.53F};

			/** @brief Default color temperature, in kelvins (D65-like, resolves to white). */
			static constexpr auto DefaultTemperature{6500.0F};

			/**
			 * @brief Constructs a celestial body with neutral defaults (a white sun at the zenith).
			 */
			CelestialBody () noexcept = default;

			/**
			 * @brief Sets the kind of celestial body.
			 * @param type The type.
			 * @return void
			 */
			void
			setType (Type type) noexcept
			{
				m_type = type;
			}

			/**
			 * @brief Returns the kind of celestial body.
			 * @return Type
			 */
			[[nodiscard]]
			Type
			type () const noexcept
			{
				return m_type;
			}

			/**
			 * @brief Sets the direction pointing TOWARD the body, in the engine frame (UP = +Y).
			 * @param direction A reference to a vector. It is normalized on store.
			 * @return void
			 */
			void
			setDirection (const Base::Math::Vector< 3, float > & direction) noexcept
			{
				m_direction = direction.normalized();
			}

			/**
			 * @brief Returns the normalized direction pointing TOWARD the body.
			 * @return const Base::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			const Base::Math::Vector< 3, float > &
			direction () const noexcept
			{
				return m_direction;
			}

			/**
			 * @brief Sets the illuminance produced on a surface facing the body, in lux.
			 * @note References: direct sun 100000 lx, overcast daylight 10000 lx, full moon 0.25 lx.
			 * @param lux The illuminance, in lux.
			 * @return void
			 */
			void
			setIlluminance (float lux) noexcept
			{
				m_illuminance = std::max(0.0F, lux);
			}

			/**
			 * @brief Returns the illuminance produced on a surface facing the body, in lux.
			 * @return float
			 */
			[[nodiscard]]
			float
			illuminance () const noexcept
			{
				return m_illuminance;
			}

			/**
			 * @brief Sets the color temperature, in kelvins, and resolves the sRGB color from it.
			 * @note References: noon sun ~5500 K, golden hour 2500-3500 K, moonlight ~4100 K.
			 * @param kelvin The color temperature, in kelvins (clamped to [1667, 25000]).
			 * @return void
			 */
			void
			setTemperature (float kelvin) noexcept
			{
				m_temperature = std::clamp(kelvin, 1667.0F, 25000.0F);
				m_color = Photometry::colorFromTemperature(m_temperature);
			}

			/**
			 * @brief Returns the color temperature, in kelvins.
			 * @return float
			 */
			[[nodiscard]]
			float
			temperature () const noexcept
			{
				return m_temperature;
			}

			/**
			 * @brief Sets the color directly, in sRGB, bypassing the color temperature.
			 * @note When a manifest declares both, the temperature wins (owner decision).
			 * @param color A reference to a color.
			 * @return void
			 */
			void
			setColor (const Base::PixelFactory::Color< float > & color) noexcept
			{
				m_color = color;
			}

			/**
			 * @brief Returns the resolved color of the body, in sRGB.
			 * @return const Base::PixelFactory::Color< float > &
			 */
			[[nodiscard]]
			const Base::PixelFactory::Color< float > &
			color () const noexcept
			{
				return m_color;
			}

			/**
			 * @brief Sets the apparent angular diameter of the body disc, in degrees.
			 * @param degrees The angular diameter, in degrees.
			 * @return void
			 */
			void
			setAngularDiameter (float degrees) noexcept
			{
				m_angularDiameter = std::max(0.0F, degrees);
			}

			/**
			 * @brief Returns the apparent angular diameter of the body disc, in degrees.
			 * @return float
			 */
			[[nodiscard]]
			float
			angularDiameter () const noexcept
			{
				return m_angularDiameter;
			}

			/**
			 * @brief Sets whether the body disc is painted in the background texture.
			 * @note This is the anti-double-counting flag: when the body is in the texture AND
			 * an analytic directional light is derived from it, an HDR pipeline must exclude
			 * the disc from the IBL. With the current LDR sources the clamped disc is a
			 * negligible IBL contributor, so the flag is informative.
			 * @param state The state.
			 * @return void
			 */
			void
			setInTexture (bool state) noexcept
			{
				m_inTexture = state;
			}

			/**
			 * @brief Returns whether the body disc is painted in the background texture.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isInTexture () const noexcept
			{
				return m_inTexture;
			}

			/**
			 * @brief Returns the name of a body type.
			 * @param type The type.
			 * @return const char *
			 */
			[[nodiscard]]
			static
			const char *
			typeName (Type type) noexcept
			{
				switch ( type )
				{
					case Type::Moon :
						return "Moon";

					case Type::Star :
						return "Star";

					case Type::Sun :
					default:
						return "Sun";
				}
			}

			/**
			 * @brief Returns a body type from a string ("Sun", "Moon", "Star").
			 * @param name A reference to a string. Unknown names resolve to Type::Sun.
			 * @return Type
			 */
			[[nodiscard]]
			static
			Type
			parseType (const std::string & name) noexcept
			{
				if ( name == "Moon" )
				{
					return Type::Moon;
				}

				if ( name == "Star" )
				{
					return Type::Star;
				}

				return Type::Sun;
			}

		private:

			Base::Math::Vector< 3, float > m_direction{0.0F, 1.0F, 0.0F}; /**< Toward the body, normalized. Default: zenith (UP = +Y). */
			Base::PixelFactory::Color< float > m_color{Base::PixelFactory::White}; /**< Resolved sRGB color. */
			float m_illuminance{0.0F}; /**< Illuminance facing the body, in lux. */
			float m_temperature{DefaultTemperature}; /**< Color temperature, in kelvins. */
			float m_angularDiameter{EarthlikeAngularDiameter}; /**< Apparent disc diameter, in degrees. */
			Type m_type{Type::Sun};
			bool m_inTexture{true};
	};
}

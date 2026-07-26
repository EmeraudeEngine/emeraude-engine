/*
 * src/Graphics/Photometry.hpp
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
#include <cmath>
#include <numbers>

/**
 * @brief Photometric unit conversions — the SINGLE place where physical light and exposure
 * units are converted in the engine.
 *
 * @note UNIT CONTRACT. Every light emitter stores the quantity its type is physically
 * measured in, and that is what reaches the GPU:
 * - directional (sun, moon): ILLUMINANCE in lux (lm/m²) — the distance is constant, so an
 *   illuminance is the natural description;
 * - point and spot: LUMINOUS INTENSITY in candela (lm/sr) — authored as luminous power in
 *   lumens and converted here, because that is what a bulb is sold as;
 * - emissive surfaces, sky, screens: LUMINANCE in nits (cd/m²) — constant along a direction,
 *   independent of distance.
 * Reference values to sanity-check content: direct sun 100 000 lx, overcast daylight
 * 10 000 lx, office interior ~500 lx, full moon 0.25-1 lx; a 60 W-equivalent bulb 800 lm;
 * a monitor 200-300 nits, a candle flame 5-10 nits.
 *
 * @note These conversions are only meaningful with a PHYSICAL attenuation
 * (`illuminanceFromIntensity()` below, i.e. inverse square). A radius-bounded artistic
 * falloff makes a lumen value arbitrary again — see `TODO.md` § "Photometric lighting".
 */
namespace EmEn::Graphics::Photometry
{
	/** @brief Reference illuminances, in lux, to author and sanity-check outdoor lighting. */
	constexpr float FullMoonIlluminance{0.25F};
	constexpr float OvercastDaylightIlluminance{10000.0F};
	constexpr float DirectSunlightIlluminance{100000.0F};

	/** @brief Saturation-based calibration of the exposure multiplier (Frostbite form):
	 * 78 / (S · q) with S = ISO 100 as the reference and q = 0.65 the lens/sensor factor. */
	constexpr float MeterCalibration{1.2F};

	/** @brief Reflected-light meter constant K (ISO 2720): the scene luminance in nits that a
	 * meter maps to a "correct" exposure at EV100 = 0 is K / (t·S/N²)… in practice, K relates
	 * the metered average luminance to the exposure the triad should produce. */
	constexpr float ReflectedMeterK{12.5F};

	/** @brief Display-referred value the METERED average luminance lands on after the manual
	 * APEX exposure: K / (MeterCalibration · 100) ≈ 0.104. This is the value the AUTO exposure
	 * must key on (`autoExposure = MeteredMiddleGrey / avgLuminance`) for the auto and manual
	 * paths to land a correctly metered scene identically — keying on Reinhard's 0.18 instead
	 * put the two conventions 0.79 EV apart, and shifted the auto-ISO window by as much
	 * relative to the sensor bounds computed with the APEX semantics. */
	constexpr float MeteredMiddleGrey{ReflectedMeterK / (MeterCalibration * 100.0F)};

	/**
 * @brief Converts the luminous power of a POINT light to a luminous intensity.
 * @note A point source radiates into the whole sphere, 4pi steradians: a 800 lm bulb is
	 * 800 / 4pi = 63.7 cd in every direction.
 * @param lumens The luminous power, in lumens.
 * @return float The luminous intensity, in candela.
	 */
	[[nodiscard]]
	inline
	float
	candelaFromPointLumens (float lumens) noexcept
	{
		constexpr auto SphereSolidAngle = 4.0F * std::numbers::pi_v< float >;

		return lumens / SphereSolidAngle;
	}

	/**
 * @brief Converts the luminous power of a SPOT light to a luminous intensity.
 * @note A spot concentrates the same power into a cone, so a narrower beam is BRIGHTER
	 * for the same wattage: the solid angle of a cone of half-angle theta is
	 * 2pi(1 - cos(theta)). A 100 lm flashlight with a 20 degree full beam yields ~1047 cd.
 * @warning The angle is the OUTER half-angle of the cone, in degrees — the angle beyond
	 * which the light is off, not the full-intensity inner one.
 * @param lumens The luminous power, in lumens.
 * @param outerHalfAngleDegrees The outer half-angle of the cone, in degrees.
 * @return float The luminous intensity, in candela.
	 */
	[[nodiscard]]
	inline
	float
	candelaFromSpotLumens (float lumens, float outerHalfAngleDegrees) noexcept
	{
		constexpr auto DegreesToRadians = std::numbers::pi_v< float > / 180.0F;
		constexpr auto MinSolidAngle{1e-4F};

		const auto solidAngle = 2.0F * std::numbers::pi_v< float > * (1.0F - std::cos(outerHalfAngleDegrees * DegreesToRadians));

		/* A degenerate cone would divide by zero and make the light infinitely bright. */
		return lumens / std::max(solidAngle, MinSolidAngle);
	}

	/**
 * @brief Converts a luminous intensity to the illuminance it produces at a distance.
 * @note The inverse-square law: 1000 cd measures 1000 lx at one meter, 250 lx at two.
 * @param candela The luminous intensity, in candela.
 * @param meters The distance to the surface, in meters.
 * @return float The illuminance, in lux.
	 */
	[[nodiscard]]
	inline
	float
	illuminanceFromIntensity (float candela, float meters) noexcept
	{
		constexpr auto MinDistance{1e-3F};

		const auto distance = std::max(meters, MinDistance);

		return candela / (distance * distance);
	}

	/**
 * @brief Returns the exposure value at ISO 100 of a camera setting (the APEX equation).
 * @note `EV100 = log2(N² / t * 100 / S)`, after Lagarde & de Rousiers, "Moving Frostbite
	 * to Physically Based Rendering" (SIGGRAPH 2014). Halving the shutter speed, opening one
	 * full f-stop or doubling the ISO each move the result by exactly one EV.
 * @param apertureFStop The aperture as an f-number (N).
 * @param shutterSpeedSeconds The exposure time, in seconds (t).
 * @param sensitivityISO The sensor sensitivity, in ISO (S).
 * @return float The exposure value, referenced to ISO 100.
	 */
	[[nodiscard]]
	inline
	float
	exposureValue100 (float apertureFStop, float shutterSpeedSeconds, float sensitivityISO) noexcept
	{
		constexpr auto MinValue{1e-6F};

		const auto aperture = std::max(apertureFStop, MinValue);
		const auto shutterSpeed = std::max(shutterSpeedSeconds, MinValue);
		const auto sensitivity = std::max(sensitivityISO, MinValue);

		return std::log2((aperture * aperture) / shutterSpeed * 100.0F / sensitivity);
	}

	/**
 * @brief Returns the linear exposure multiplier of an exposure value.
 * @note `exposure = 1 / (1.2 * 2^EV100)`, the Frostbite form: the 1.2 comes from the
	 * ISO-to-luminance calibration constant of a reflected-light meter (78 / (S * q) with
	 * q = 0.65). Multiplying a scene luminance in nits by this maps the metered mid-grey to
	 * the display range.
 * @param exposureValue100 The exposure value, referenced to ISO 100.
 * @return float The linear multiplier to apply to scene luminance.
	 */
	[[nodiscard]]
	inline
	float
	exposureFromValue100 (float exposureValue100) noexcept
	{
		return 1.0F / (MeterCalibration * std::exp2(exposureValue100));
	}
}

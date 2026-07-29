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

/* Local inclusions for usages. */
#include "PixelFactory/Color.hpp"

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

	/** @brief Illuminance factor of a UNIFORM sky dome: `E = pi x L`. Real skies measure lower
	 * (see CubemapResource::hemisphereIlluminanceFactor()). */
	constexpr float UniformDomeIlluminanceFactor{std::numbers::pi_v< float >};

	/**
 * @brief Returns the illuminance received on the ground from a uniform sky dome.
 * @note For a hemisphere of uniform luminance L, the horizontal illuminance is `E = pi * L`
	 * (Lambertian integral of L·cosθ over the upper hemisphere). Used as the default ambient
	 * illuminance when a sky manifest declares a luminance but no explicit ambient value: an
	 * 8000-nit clear sky yields ~25000 lx, consistent with the ~20000 lx measured in open shade
	 * under a clear sky.
 * @param nits The sky luminance, in candela per square meter.
 * @return float The illuminance, in lux.
	 */
	[[nodiscard]]
	inline
	float
	illuminanceFromSkyLuminance (float nits) noexcept
	{
		return std::numbers::pi_v< float > * std::max(0.0F, nits);
	}

	/**
 * @brief Returns the sRGB color of a black body at a given color temperature.
 * @note Planckian locus approximation in CIE 1931 (x, y) after Kang et al., "Design of
	 * advanced color temperature control system for HDTV applications", J. Korean Phys. Soc. 41
	 * (2002) — the approximation documented by the Wikipedia "Planckian locus" article. The
	 * chromaticity goes xyY (Y = 1) -> XYZ -> linear sRGB (D65 matrix), is normalized to a max
	 * component of 1, then sRGB-encoded ("Color" suffix convention: sRGB). Valid from 1667 K to
	 * 25000 K, the input is clamped. ~6500 K resolves to white.
 * @param kelvin The color temperature, in kelvins.
 * @return Base::PixelFactory::Color< float > The color, in sRGB.
	 */
	[[nodiscard]]
	inline
	Base::PixelFactory::Color< float >
	colorFromTemperature (float kelvin) noexcept
	{
		const auto temperature = std::clamp(kelvin, 1667.0F, 25000.0F);

		/* CIE 1931 chromaticity (x, y) on the Planckian locus. */
		const auto invT = 1000.0F / temperature;
		const auto invT2 = invT * invT;
		const auto invT3 = invT2 * invT;

		const auto x = temperature <= 4000.0F
			? -0.2661239F * invT3 - 0.2343589F * invT2 + 0.8776956F * invT + 0.179910F
			: -3.0258469F * invT3 + 2.1070379F * invT2 + 0.2226347F * invT + 0.240390F;

		const auto x2 = x * x;
		const auto x3 = x2 * x;

		float y;

		if ( temperature <= 2222.0F )
		{
			y = -1.1063814F * x3 - 1.34811020F * x2 + 2.18555832F * x - 0.20219683F;
		}
		else if ( temperature <= 4000.0F )
		{
			y = -0.9549476F * x3 - 1.37418593F * x2 + 2.09137015F * x - 0.16748867F;
		}
		else
		{
			y = 3.0817580F * x3 - 5.87338670F * x2 + 3.75112997F * x - 0.37001483F;
		}

		/* xyY (Y = 1) -> XYZ -> linear sRGB (D65, IEC 61966-2-1 matrix). */
		const auto bigX = x / y;
		const auto bigZ = (1.0F - x - y) / y;

		auto red = 3.2404542F * bigX - 1.5371385F - 0.4985314F * bigZ;
		auto green = -0.9692660F * bigX + 1.8760108F + 0.0415560F * bigZ;
		auto blue = 0.0556434F * bigX - 0.2040259F + 1.0572252F * bigZ;

		red = std::max(0.0F, red);
		green = std::max(0.0F, green);
		blue = std::max(0.0F, blue);

		const auto maxComponent = std::max(red, std::max(green, blue));

		if ( maxComponent > 0.0F )
		{
			red /= maxComponent;
			green /= maxComponent;
			blue /= maxComponent;
		}

		const auto encode = [] (float component) {
			return component <= 0.0031308F ? 12.92F * component : 1.055F * std::pow(component, 1.0F / 2.4F) - 0.055F;
		};

		return {encode(red), encode(green), encode(blue), 1.0F};
	}
}

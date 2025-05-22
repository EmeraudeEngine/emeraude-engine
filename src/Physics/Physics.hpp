/*
 * src/Physics/Physics.hpp
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

/* Local inclusions for usages. */
#include "Constants.hpp"
#include "Libs/Math/Vector.hpp"

namespace EmEn::Physics
{
	namespace SI
	{
		/**
		 * @brief Constant for one kilometer.
		 * @tparam precision_t The precision of floating point number.
		 */
		template< typename precision_t = float >
		constexpr precision_t Kilometer = static_cast< precision_t >(1000.0);

		/**
		 * @brief Convenient helper to express values in kilometers.
		 * @tparam precision_t The precision of floating point number.
		 * @param n The number of units.
		 * @return precision_t
		 */
		template< typename precision_t = float >
		constexpr
		precision_t
		kilometers (precision_t n) noexcept
		{
			return n * Kilometer< precision_t >;
		}

		/**
		 * @brief Constant for one meter.
		 * @tparam precision_t The precision of floating point number.
		 */
		template< typename precision_t = float >
		constexpr precision_t Meter = static_cast< precision_t >(1.0);

		/**
		 * @brief Convenient helper to express values in meters.
		 * @tparam precision_t The precision of floating point number.
		 * @param n The number of units.
		 * @return precision_t
		 */
		template< typename precision_t = float >
		constexpr
		precision_t
		meters (precision_t n) noexcept
		{
			return n * Meter< precision_t >;
		}

		/**
		 * @brief Constant for one centimeter.
		 * @tparam precision_t The precision of floating point number.
		 */
		template< typename precision_t = float >
		constexpr precision_t Centimeter = static_cast< precision_t >(0.01);

		/**
		 * @brief Convenient helper to express values in centimeters.
		 * @tparam precision_t The precision of floating point number.
		 * @param n The number of units.
		 * @return precision_t
		 */
		template< typename precision_t = float >
		constexpr
		precision_t
		centimeters (precision_t n) noexcept
		{
			return n * Centimeter< precision_t >;
		}

		/**
		 * @brief Constant for one millimeter.
		 * @tparam precision_t The precision of floating point number.
		 */
		template< typename precision_t = float >
		constexpr precision_t Millimeter = static_cast< precision_t >(0.001);

		/**
		 * @brief Convenient helper to express values in millimeters.
		 * @tparam precision_t The precision of floating point number.
		 * @param n The number of units.
		 * @return precision_t
		 */
		template< typename precision_t = float >
		constexpr
		precision_t
		millimeters (precision_t n) noexcept
		{
			return n * Millimeter< precision_t >;
		}

		/**
		 * @brief Constant for one tonne.
		 * @tparam precision_t The precision of floating point number.
		 */
		template< typename precision_t = float >
		constexpr precision_t Tonne = static_cast< precision_t >(1000.0);

		/**
		 * @brief Convenient helper to express values in tonnes.
		 * @tparam precision_t The precision of floating point number.
		 * @param n The number of units.
		 * @return precision_t
		 */
		template< typename precision_t = float >
		constexpr
		precision_t
		tonnes (precision_t n) noexcept
		{
			return n * Tonne< precision_t >;
		}

		/**
		 * @brief Constant for one kilogram.
		 * @tparam precision_t The precision of floating point number.
		 */
		template< typename precision_t = float >
		constexpr precision_t Kilogram = static_cast< precision_t >(1.0);

		/**
		 * @brief Convenient helper to express values in kilograms.
		 * @tparam precision_t The precision of floating point number.
		 * @param n The number of units.
		 * @return precision_t
		 */
		template< typename precision_t = float >
		constexpr
		precision_t
		kilograms (precision_t n) noexcept
		{
			return n * Kilogram< precision_t >;
		}

		/**
		 * @brief Constant for one gram.
		 * @tparam precision_t The precision of floating point number.
		 */
		template< typename precision_t = float >
		constexpr precision_t Gram = static_cast< precision_t >(0.001);

		/**
		 * @brief Convenient helper to express values in grams.
		 * @tparam precision_t The precision of floating point number.
		 * @param n The number of units.
		 * @return precision_t
		 */
		template< typename precision_t = float >
		constexpr
		precision_t
		grams (precision_t n) noexcept
		{
			return n * Gram< precision_t >;
		}

		/**
		 * @brief Constant for one milligram.
		 * @tparam precision_t The precision of floating point number.
		 */
		template< typename precision_t = float >
		constexpr precision_t Milligram = static_cast< precision_t >(0.000001);

		/**
		 * @brief Convenient helper to express values in milligrams.
		 * @tparam precision_t The precision of floating point number.
		 * @param n The number of units.
		 * @return precision_t
		 */
		template< typename precision_t = float >
		constexpr
		precision_t
		milligrams (precision_t n) noexcept
		{
			return n * Milligram< precision_t >;
		}

		/**
		 * @brief Constant for one hour.
		 * @tparam precision_t The precision of floating point number.
		 */
		template< typename precision_t = float >
		constexpr precision_t Hour = static_cast< precision_t >(3600.0);

		/**
		 * @brief Convenient helper to express values in hours.
		 * @tparam precision_t The precision of floating point number.
		 * @param n The number of units.
		 * @return precision_t
		 */
		template< typename precision_t = float >
		constexpr
		precision_t
		hours (precision_t n) noexcept
		{
			return n * Hour< precision_t >;
		}

		/**
		 * @brief Constant for one minute.
		 * @tparam precision_t The precision of floating point number.
		 */
		template< typename precision_t = float >
		constexpr precision_t Minute = static_cast< precision_t >(60.0);

		/**
		 * @brief Convenient helper to express values in minutes.
		 * @tparam precision_t The precision of floating point number.
		 * @param n The number of units.
		 * @return precision_t
		 */
		template< typename precision_t = float >
		constexpr
		precision_t
		minutes (precision_t n) noexcept
		{
			return n * Minute< precision_t >;
		}

		/**
		 * @brief Constant for one second.
		 * @tparam precision_t The precision of floating point number.
		 */
		template< typename precision_t = float >
		constexpr precision_t Second = static_cast< precision_t >(1.0);

		/**
		 * @brief Convenient helper to express values in seconds.
		 * @tparam precision_t The precision of floating point number.
		 * @param n The number of units.
		 * @return precision_t
		 */
		template< typename precision_t = float >
		constexpr
		precision_t
		seconds (precision_t n) noexcept
		{
			return n * Second< precision_t >;
		}

		/**
		 * @brief Constant for one millisecond.
		 * @tparam precision_t The precision of floating point number.
		 */
		template< typename precision_t = float >
		constexpr precision_t Millisecond = static_cast< precision_t >(0.001);

		/**
		 * @brief Convenient helper to express values in milliseconds.
		 * @tparam precision_t The precision of floating point number.
		 * @param n The number of units.
		 * @return precision_t
		 */
		template< typename precision_t = float >
		constexpr
		precision_t
		milliseconds (precision_t n) noexcept
		{
			return n * Millisecond< precision_t >;
		}

		/**
		 * @brief Constant for one microsecond.
		 * @tparam precision_t The precision of floating point number.
		 */
		template< typename precision_t = float >
		constexpr precision_t Microsecond = static_cast< precision_t >(0.000001);

		/**
		 * @brief Convenient helper to express values in microseconds.
		 * @tparam precision_t The precision of floating point number.
		 * @param n The number of units.
		 * @return precision_t
		 */
		template< typename precision_t = float >
		constexpr
		precision_t
		microseconds (precision_t n) noexcept
		{
			return n * Microsecond< precision_t >;
		}

		/**
		 * @brief Constant for one nanosecond.
		 * @tparam precision_t The precision of floating point number.
		 */
		template< typename precision_t = float >
		constexpr precision_t Nanosecond = static_cast< precision_t >(0.000000001);

		/**
		 * @brief Convenient helper to express values in nanoseconds.
		 * @tparam precision_t The precision of floating point number.
		 * @param n The number of units.
		 * @return precision_t
		 */
		template< typename precision_t = float >
		constexpr
		precision_t
		nanoseconds (precision_t n) noexcept
		{
			return n * Nanosecond< precision_t >;
		}
	}

	namespace DragCoefficient
	{
		/** @brief Drag coefficient for a sphere. */
		template< typename precision_t = float >
		constexpr precision_t Sphere = static_cast< precision_t >(0.45);

		/** @brief Drag coefficient for a half sphere. */
		template< typename precision_t = float >
		constexpr precision_t HalfSphere = static_cast< precision_t >(0.42);

		/** @brief Drag coefficient for a cone. */
		template< typename precision_t = float >
		constexpr precision_t Cone = static_cast< precision_t >(0.50);

		/** @brief Drag coefficient for a cube. */
		template< typename precision_t = float >
		constexpr precision_t Cube = static_cast< precision_t >(1.05);

		/** @brief Drag coefficient for an angled cube. */
		template< typename precision_t = float >
		constexpr precision_t AngledCube = static_cast< precision_t >(0.80);

		/** @brief Drag coefficient for a long cylinder. */
		template< typename precision_t = float >
		constexpr precision_t LongCylinder = static_cast< precision_t >(1.82);

		/** @brief Drag coefficient for a short cylinder. */
		template< typename precision_t = float >
		constexpr precision_t ShortCylinder = static_cast< precision_t >(0.82);

		/** @brief Drag coefficient for a streamlined body. */
		template< typename precision_t = float >
		constexpr precision_t StreamlinedBody = static_cast< precision_t >(0.04);

		/** @brief Drag coefficient for a streamlined half body. */
		template< typename precision_t = float >
		constexpr precision_t StreamlinedHalfBody = static_cast< precision_t >(0.09);
	}

	namespace Gravity
	{
		/** @brief Earth gravity expressed in m/s² */
		template< typename precision_t = float >
		constexpr precision_t Earth = static_cast< precision_t >(9.807);

		/** @brief Moon gravity expressed in m/s² */
		template< typename precision_t = float >
		constexpr precision_t Moon = static_cast< precision_t >(1.622);

		/** @brief Mars gravity expressed in m/s² */
		template< typename precision_t = float >
		constexpr precision_t Mars = static_cast< precision_t >(3.711);

		/** @brief Jupiter gravity expressed in m/s² */
		template< typename precision_t = float >
		constexpr precision_t Jupiter = static_cast< precision_t >(24.79);
	}

	namespace SpeedOfSound
	{
		/** @brief Speed of sound in vacuum expressed in m/s² */
		template< typename precision_t = float >
		constexpr precision_t Vacuum = static_cast< precision_t >(0.0);

		/** @brief Speed of sound in air expressed in m/s² */
		template< typename precision_t = float >
		constexpr precision_t Air = static_cast< precision_t >(343.3);

		/** @brief Speed of sound in water of sound expressed in m/s² */
		template< typename precision_t = float >
		constexpr precision_t Water = static_cast< precision_t >(1480.0);
	}

	namespace RefractiveIndex
	{
		/** @brief Refractive index of the vacuum. */
		template< typename precision_t = float >
		constexpr precision_t Vacuum = static_cast< precision_t >(1.0);

		/** @brief Refractive index of the air. */
		template< typename precision_t = float >
		constexpr precision_t Air = static_cast< precision_t >(1.000293);

		/** @brief Refractive index of the helium. */
		template< typename precision_t = float >
		constexpr precision_t Helium = static_cast< precision_t >(1.000036);

		/** @brief Refractive index of the hydrogen. */
		template< typename precision_t = float >
		constexpr precision_t Hydrogen = static_cast< precision_t >(1.000132);

		/** @brief Refractive index of the carbon dioxide. */
		template< typename precision_t = float >
		constexpr precision_t CarbonDioxide = static_cast< precision_t >(1.00045);

		/** @brief Refractive index of the water at 20 °C. */
		template< typename precision_t = float >
		constexpr precision_t Water = static_cast< precision_t >(1.333);

		/** @brief Refractive index of the ethanol at 20 °C. */
		template< typename precision_t = float >
		constexpr precision_t Ethanol = static_cast< precision_t >(1.36);

		/** @brief Refractive index of the olive oil at 20 °C. */
		template< typename precision_t = float >
		constexpr precision_t OliveOil = static_cast< precision_t >(1.47);

		/** @brief Refractive index of the ice. */
		template< typename precision_t = float >
		constexpr precision_t Ice = static_cast< precision_t >(1.31);

		/** @brief Refractive index of the Fused Silica (quartz). */
		template< typename precision_t = float >
		constexpr precision_t FusedSilica = static_cast< precision_t >(1.46);

		/** @brief Refractive index of the PMMA (acrylic, plexiglas, lucite, perspex). */
		template< typename precision_t = float >
		constexpr precision_t PMMA = static_cast< precision_t >(1.49);

		/** @brief Refractive index of the window glass. */
		template< typename precision_t = float >
		constexpr precision_t WindowGlass = static_cast< precision_t >(1.52);

		/** @brief Refractive index of the polycarbonate (Lexan™). */
		template< typename precision_t = float >
		constexpr precision_t Polycarbonate  = static_cast< precision_t >(1.58);

		/** @brief Refractive index of the flint glass (typical). */
		template< typename precision_t = float >
		constexpr precision_t FlintGlass = static_cast< precision_t >(1.69);

		/** @brief Refractive index of the sapphire. */
		template< typename precision_t = float >
		constexpr precision_t Sapphire = static_cast< precision_t >(1.77);

		/** @brief Refractive index of the cubic zirconia. */
		template< typename precision_t = float >
		constexpr precision_t CubicZirconia = static_cast< precision_t >(2.15);

		/** @brief Refractive index of the diamond. */
		template< typename precision_t = float >
		constexpr precision_t Diamond = static_cast< precision_t >(2.417);

		/** @brief Refractive index of the moissanite. */
		template< typename precision_t = float >
		constexpr precision_t Moissanite = static_cast< precision_t >(2.65);
	}

	namespace SpeedOfLight
	{
		/** @brief Speed of light in vacuum expressed in m/s² */
		template< typename precision_t = float >
		constexpr precision_t Vacuum = static_cast< precision_t >(299792458.0);

		/** @brief Speed of light in air expressed in m/s² */
		template< typename precision_t = float >
		constexpr precision_t Air = static_cast< precision_t >(SpeedOfLight::Vacuum< precision_t > * 1.0003); // Should be 299702547.0

		/** @brief Speed of light in water of sound expressed in m/s² */
		template< typename precision_t = float >
		constexpr precision_t Water = static_cast< precision_t >(SpeedOfLight::Vacuum< precision_t > / RefractiveIndex::Water< precision_t >); // Should be 224900569.0
	}

	/** @brief Each density is expressed in kg/m³. */
	namespace Density
	{
		/** @brief Air density expressed in kg/m³ (Sea level a 15 degree). */
		template< typename precision_t = float >
		constexpr precision_t EarthStandardAir = static_cast< precision_t >(1.225);

		/** @brief Water density expressed in kg/m³ (20 degree).
		 * @note References table (g/cm³) :
			0 0,999841
			1 0,9999
			2 0,999941
			3 0,999965
			4 0,999973
			5 0,999965
			6 0,999941
			7 0,999902
			8 0,999849
			9 0,999782
			10 0,999701
			11 0,999606
			12 0,999498
			13 0,999377
			14 0,999244
			15 0,999099
			16 0,998943
			17 0,998775
			18 0,998596
			19 0,998406
			20 0,998205 <- Chosen constant
			21 0,997994
			22 0,997772
			23 0,99754
			24 0,997299
			25 0,997047
			26 0,996785
			27 0,996515
			28 0,996235
			29 0,995946
			30 0,995649
			40 0,99222
			50 0,98803
			60 0,98320
			70 0,97778
			80 0,97182
			90 0,96535
			100 0,95840
		*/
		template< typename precision_t = float >
		constexpr precision_t EarthStandardWater = static_cast< precision_t >(998.205);

		template< typename precision_t = float >
		constexpr precision_t AlfalfaGround = static_cast< precision_t >(256);

		template< typename precision_t = float >
		constexpr precision_t Alum = static_cast< precision_t >(817);

		template< typename precision_t = float >
		constexpr precision_t Alumina = static_cast< precision_t >(961);

		template< typename precision_t = float >
		constexpr precision_t AluminumOxide = static_cast< precision_t >(1522);

		template< typename precision_t = float >
		constexpr precision_t AmmoniaGas = static_cast< precision_t >(0.77);

		template< typename precision_t = float >
		constexpr precision_t AmmoniumNitrate = static_cast< precision_t >(730);

		template< typename precision_t = float >
		constexpr precision_t AmmoniumSulphateDry = static_cast< precision_t >(1130);

		template< typename precision_t = float >
		constexpr precision_t AmmoniumSulphateWet = static_cast< precision_t >(1290);

		template< typename precision_t = float >
		constexpr precision_t AndesiteSolid = static_cast< precision_t >(2771);

		template< typename precision_t = float >
		constexpr precision_t Antimony = static_cast< precision_t >(6696);

		template< typename precision_t = float >
		constexpr precision_t Apples = static_cast< precision_t >(641);

		template< typename precision_t = float >
		constexpr precision_t Arsenic = static_cast< precision_t >(5671);

		template< typename precision_t = float >
		constexpr precision_t AsbestosShredded = static_cast< precision_t >(360);

		template< typename precision_t = float >
		constexpr precision_t AsbestosRock = static_cast< precision_t >(1600);

		template< typename precision_t = float >
		constexpr precision_t AshesWet = static_cast< precision_t >(810);

		template< typename precision_t = float >
		constexpr precision_t AshesDry = static_cast< precision_t >(610);

		template< typename precision_t = float >
		constexpr precision_t AsphaltCrushed = static_cast< precision_t >(721);

		template< typename precision_t = float >
		constexpr precision_t Babbitt = static_cast< precision_t >(7272);

		template< typename precision_t = float >
		constexpr precision_t Bagasse = static_cast< precision_t >(120);

		template< typename precision_t = float >
		constexpr precision_t BakeliteSolid = static_cast< precision_t >(1362);

		template< typename precision_t = float >
		constexpr precision_t BakingPowder = static_cast< precision_t >(721);

		template< typename precision_t = float >
		constexpr precision_t Barium = static_cast< precision_t >(3780);

		template< typename precision_t = float >
		constexpr precision_t Bark = static_cast< precision_t >(240);

		template< typename precision_t = float >
		constexpr precision_t Barley = static_cast< precision_t >(609);

		template< typename precision_t = float >
		constexpr precision_t BariteCrushed = static_cast< precision_t >(2883);

		template< typename precision_t = float >
		constexpr precision_t BasaltBroken = static_cast< precision_t >(1954);

		template< typename precision_t = float >
		constexpr precision_t BasaltSolid = static_cast< precision_t >(3011);

		template< typename precision_t = float >
		constexpr precision_t BauxiteCrushed = static_cast< precision_t >(1281);

		template< typename precision_t = float >
		constexpr precision_t BeansCastor = static_cast< precision_t >(577);

		template< typename precision_t = float >
		constexpr precision_t BeansCocoa = static_cast< precision_t >(593);

		template< typename precision_t = float >
		constexpr precision_t BeansNavy = static_cast< precision_t >(801);

		template< typename precision_t = float >
		constexpr precision_t BeansSoy = static_cast< precision_t >(721);

		template< typename precision_t = float >
		constexpr precision_t Beeswax = static_cast< precision_t >(961);

		template< typename precision_t = float >
		constexpr precision_t Beets = static_cast< precision_t >(721);

		template< typename precision_t = float >
		constexpr precision_t Bentonite = static_cast< precision_t >(593);

		template< typename precision_t = float >
		constexpr precision_t BicarbonateOSoda = static_cast< precision_t >(689);

		template< typename precision_t = float >
		constexpr precision_t Bismuth = static_cast< precision_t >(9787);

		template< typename precision_t = float >
		constexpr precision_t BonesPulverized = static_cast< precision_t >(881);

		template< typename precision_t = float >
		constexpr precision_t Borax = static_cast< precision_t >(849);

		template< typename precision_t = float >
		constexpr precision_t Bran = static_cast< precision_t >(256);

		template< typename precision_t = float >
		constexpr precision_t BrewersGgrain = static_cast< precision_t >(432);

		template< typename precision_t = float >
		constexpr precision_t BrickCommonRed = static_cast< precision_t >(1922);

		template< typename precision_t = float >
		constexpr precision_t BrickFireClay = static_cast< precision_t >(2403);

		template< typename precision_t = float >
		constexpr precision_t BrickSilica = static_cast< precision_t >(2050);

		template< typename precision_t = float >
		constexpr precision_t BrickChrome = static_cast< precision_t >(2803);

		template< typename precision_t = float >
		constexpr precision_t BrickMagnesia = static_cast< precision_t >(2563);

		template< typename precision_t = float >
		constexpr precision_t Buckwheat = static_cast< precision_t >(657);

		template< typename precision_t = float >
		constexpr precision_t Butter = static_cast< precision_t >(865);

		template< typename precision_t = float >
		constexpr precision_t Cadmium = static_cast< precision_t >(8650);

		template< typename precision_t = float >
		constexpr precision_t CalciumCarbide = static_cast< precision_t >(1201);

		template< typename precision_t = float >
		constexpr precision_t Caliche = static_cast< precision_t >(1442);

		template< typename precision_t = float >
		constexpr precision_t CarbonSolid = static_cast< precision_t >(2146);

		template< typename precision_t = float >
		constexpr precision_t CarbonPowdered = static_cast< precision_t >(80);

		template< typename precision_t = float >
		constexpr precision_t CarbonDioxide = static_cast< precision_t >(1.98);

		template< typename precision_t = float >
		constexpr precision_t CarbonMonoxide = static_cast< precision_t >(1.25);

		template< typename precision_t = float >
		constexpr precision_t Cardboard = static_cast< precision_t >(689);

		template< typename precision_t = float >
		constexpr precision_t CementClinker = static_cast< precision_t >(1415);

		template< typename precision_t = float >
		constexpr precision_t CementPortland = static_cast< precision_t >(1506);

		template< typename precision_t = float >
		constexpr precision_t CementMortar = static_cast< precision_t >(2162);

		template< typename precision_t = float >
		constexpr precision_t CementSlurry = static_cast< precision_t >(1442);

		template< typename precision_t = float >
		constexpr precision_t ChalkSolid = static_cast< precision_t >(2499);

		template< typename precision_t = float >
		constexpr precision_t ChalkLumpy = static_cast< precision_t >(1442);

		template< typename precision_t = float >
		constexpr precision_t ChalkFine = static_cast< precision_t >(1121);

		template< typename precision_t = float >
		constexpr precision_t Charcoal = static_cast< precision_t >(208);

		template< typename precision_t = float >
		constexpr precision_t Chloroform = static_cast< precision_t >(1522);

		template< typename precision_t = float >
		constexpr precision_t ChocolatePowder = static_cast< precision_t >(641);

		template< typename precision_t = float >
		constexpr precision_t ChromicAcidFlake = static_cast< precision_t >(1201);

		template< typename precision_t = float >
		constexpr precision_t Chromium = static_cast< precision_t >(6856);

		template< typename precision_t = float >
		constexpr precision_t ChromiumOre = static_cast< precision_t >(2162);

		template< typename precision_t = float >
		constexpr precision_t CindersFurnace = static_cast< precision_t >(913);

		template< typename precision_t = float >
		constexpr precision_t CindersCoalAsh = static_cast< precision_t >(641);

		template< typename precision_t = float >
		constexpr precision_t ClayDryExcavated = static_cast< precision_t >(1089);

		template< typename precision_t = float >
		constexpr precision_t ClayWetExcavated = static_cast< precision_t >(1826);

		template< typename precision_t = float >
		constexpr precision_t ClayDryLump = static_cast< precision_t >(1073);

		template< typename precision_t = float >
		constexpr precision_t ClayFire = static_cast< precision_t >(1362);

		template< typename precision_t = float >
		constexpr precision_t ClayWetLump = static_cast< precision_t >(1602);

		template< typename precision_t = float >
		constexpr precision_t ClayCompacted = static_cast< precision_t >(1746);

		template< typename precision_t = float >
		constexpr precision_t CloverSeed = static_cast< precision_t >(769);

		template< typename precision_t = float >
		constexpr precision_t CoalAnthraciteSolid = static_cast< precision_t >(1506);

		template< typename precision_t = float >
		constexpr precision_t CoalAnthraciteBroken = static_cast< precision_t >(1105);

		template< typename precision_t = float >
		constexpr precision_t CoalBituminousSolid = static_cast< precision_t >(1346);

		template< typename precision_t = float >
		constexpr precision_t CoalBituminousBroken = static_cast< precision_t >(833);

		template< typename precision_t = float >
		constexpr precision_t Cobaltite = static_cast< precision_t >(6295);

		template< typename precision_t = float >
		constexpr precision_t CoconutMeal = static_cast< precision_t >(513);

		template< typename precision_t = float >
		constexpr precision_t CoconutShredded = static_cast< precision_t >(352);

		template< typename precision_t = float >
		constexpr precision_t CoffeeFreshBeans = static_cast< precision_t >(561);

		template< typename precision_t = float >
		constexpr precision_t CoffeeRoastBeans = static_cast< precision_t >(432);

		template< typename precision_t = float >
		constexpr precision_t Coke = static_cast< precision_t >(610);

		template< typename precision_t = float >
		constexpr precision_t ConcreteAsphalt = static_cast< precision_t >(2243);

		template< typename precision_t = float >
		constexpr precision_t ConcreteGravel = static_cast< precision_t >(2403);

		template< typename precision_t = float >
		constexpr precision_t ConcreteLimestoneWithPortland = static_cast< precision_t >(2371);

		template< typename precision_t = float >
		constexpr precision_t CopperOre = static_cast< precision_t >(2265);

		template< typename precision_t = float >
		constexpr precision_t CopperSulfateGround = static_cast< precision_t >(3604);

		template< typename precision_t = float >
		constexpr precision_t CopraMediumSize = static_cast< precision_t >(529);

		template< typename precision_t = float >
		constexpr precision_t CopraMealGround = static_cast< precision_t >(641);

		template< typename precision_t = float >
		constexpr precision_t CopraExpellerCakeGround = static_cast< precision_t >(513);

		template< typename precision_t = float >
		constexpr precision_t CopraExpellerCakeChopped = static_cast< precision_t >(465);

		template< typename precision_t = float >
		constexpr precision_t CorkSolid = static_cast< precision_t >(240);

		template< typename precision_t = float >
		constexpr precision_t CorkGround = static_cast< precision_t >(160);

		template< typename precision_t = float >
		constexpr precision_t CornOnTheCob = static_cast< precision_t >(721);

		template< typename precision_t = float >
		constexpr precision_t CornShelled = static_cast< precision_t >(721);

		template< typename precision_t = float >
		constexpr precision_t CornGrits = static_cast< precision_t >(673);

		template< typename precision_t = float >
		constexpr precision_t CottonseedDryDeLinted = static_cast< precision_t >(561);

		template< typename precision_t = float >
		constexpr precision_t CottonseedDryNotDeLinted = static_cast< precision_t >(320);

		template< typename precision_t = float >
		constexpr precision_t CottonseedCakeLumpy = static_cast< precision_t >(673);

		template< typename precision_t = float >
		constexpr precision_t CottonseedHulls = static_cast< precision_t >(192);

		template< typename precision_t = float >
		constexpr precision_t CottonseedMeal = static_cast< precision_t >(593);

		template< typename precision_t = float >
		constexpr precision_t CottonseedMeats = static_cast< precision_t >(641);

		template< typename precision_t = float >
		constexpr precision_t Cottonwood = static_cast< precision_t >(416);

		template< typename precision_t = float >
		constexpr precision_t Cryolite = static_cast< precision_t >(1602);

		template< typename precision_t = float >
		constexpr precision_t Cullet = static_cast< precision_t >(1602);

		template< typename precision_t = float >
		constexpr precision_t Culm = static_cast< precision_t >(753);

		template< typename precision_t = float >
		constexpr precision_t DolomiteSolid = static_cast< precision_t >(2899);

		template< typename precision_t = float >
		constexpr precision_t DolomitePulverized = static_cast< precision_t >(737);

		template< typename precision_t = float >
		constexpr precision_t DolomiteLumpy = static_cast< precision_t >(1522);

		template< typename precision_t = float >
		constexpr precision_t EarthLoamDryExcavated = static_cast< precision_t >(1249);

		template< typename precision_t = float >
		constexpr precision_t EarthMoistExcavated = static_cast< precision_t >(1442);

		template< typename precision_t = float >
		constexpr precision_t EarthWetExcavated = static_cast< precision_t >(1602);

		template< typename precision_t = float >
		constexpr precision_t EarthDense = static_cast< precision_t >(2002);

		template< typename precision_t = float >
		constexpr precision_t EarthSoftLooseMud = static_cast< precision_t >(1730);

		template< typename precision_t = float >
		constexpr precision_t EarthPacked = static_cast< precision_t >(1522);

		template< typename precision_t = float >
		constexpr precision_t EarthFullersEaw = static_cast< precision_t >(673);

		template< typename precision_t = float >
		constexpr precision_t Emery = static_cast< precision_t >(4005);

		template< typename precision_t = float >
		constexpr precision_t Ether = static_cast< precision_t >(737);

		template< typename precision_t = float >
		constexpr precision_t FeldsparSolid = static_cast< precision_t >(2563);

		template< typename precision_t = float >
		constexpr precision_t FeldsparPulverized = static_cast< precision_t >(1233);

		template< typename precision_t = float >
		constexpr precision_t FertilizerAcidPhosphate = static_cast< precision_t >(961);

		template< typename precision_t = float >
		constexpr precision_t FishScrap = static_cast< precision_t >(721);

		template< typename precision_t = float >
		constexpr precision_t FishMeal = static_cast< precision_t >(593);

		template< typename precision_t = float >
		constexpr precision_t FlaxseedWhole = static_cast< precision_t >(721);

		template< typename precision_t = float >
		constexpr precision_t FlintSilica = static_cast< precision_t >(1390);

		template< typename precision_t = float >
		constexpr precision_t FlourWheat = static_cast< precision_t >(593);

		template< typename precision_t = float >
		constexpr precision_t FlueDust = static_cast< precision_t >(1735);

		template< typename precision_t = float >
		constexpr precision_t FluorsparSolid = static_cast< precision_t >(3204);

		template< typename precision_t = float >
		constexpr precision_t FluorsparLumps = static_cast< precision_t >(1602);

		template< typename precision_t = float >
		constexpr precision_t FluorsparPulverized = static_cast< precision_t >(1442);

		template< typename precision_t = float >
		constexpr precision_t FullersEarthEawOrBurnt = static_cast< precision_t >(650);

		template< typename precision_t = float >
		constexpr precision_t GalenaLeadOre = static_cast< precision_t >(7500);

		template< typename precision_t = float >
		constexpr precision_t GarbageHouseholdRubbish = static_cast< precision_t >(481);

		template< typename precision_t = float >
		constexpr precision_t GlassBroken = static_cast< precision_t >(1615);

		template< typename precision_t = float >
		constexpr precision_t GlassWindow = static_cast< precision_t >(2579);

		template< typename precision_t = float >
		constexpr precision_t GlueAnimalFlaked = static_cast< precision_t >(561);

		template< typename precision_t = float >
		constexpr precision_t GlueVegetablePowdered = static_cast< precision_t >(641);

		template< typename precision_t = float >
		constexpr precision_t GlutenMeal = static_cast< precision_t >(625);

		template< typename precision_t = float >
		constexpr precision_t GneissBedInPlace = static_cast< precision_t >(2867);

		template< typename precision_t = float >
		constexpr precision_t GneissBroken = static_cast< precision_t >(1858);

		template< typename precision_t = float >
		constexpr precision_t GraniteSolid = static_cast< precision_t >(2691);

		template< typename precision_t = float >
		constexpr precision_t GraniteBroken = static_cast< precision_t >(1650);

		template< typename precision_t = float >
		constexpr precision_t GraphiteFlake = static_cast< precision_t >(641);

		template< typename precision_t = float >
		constexpr precision_t GrainMaize = static_cast< precision_t >(760);

		template< typename precision_t = float >
		constexpr precision_t GrainBarley = static_cast< precision_t >(600);

		template< typename precision_t = float >
		constexpr precision_t GrainMillet = static_cast< precision_t >(780);

		template< typename precision_t = float >
		constexpr precision_t GrainWheat = static_cast< precision_t >(790);

		template< typename precision_t = float >
		constexpr precision_t GravelLooseDry = static_cast< precision_t >(1522);

		template< typename precision_t = float >
		constexpr precision_t GravelWithNaturalSand = static_cast< precision_t >(1922);

		template< typename precision_t = float >
		constexpr precision_t GummiteUraniumOre = static_cast< precision_t >(5145);

		template< typename precision_t = float >
		constexpr precision_t GypsumSolid = static_cast< precision_t >(2787);

		template< typename precision_t = float >
		constexpr precision_t GypsumBroken = static_cast< precision_t >(1445);

		template< typename precision_t = float >
		constexpr precision_t GypsumCrushed = static_cast< precision_t >(1602);

		template< typename precision_t = float >
		constexpr precision_t GypsumPulverized = static_cast< precision_t >(1121);

		template< typename precision_t = float >
		constexpr precision_t HaliteSaltSolid = static_cast< precision_t >(2323);

		template< typename precision_t = float >
		constexpr precision_t HaliteSaltBroken = static_cast< precision_t >(1506);

		template< typename precision_t = float >
		constexpr precision_t HematiteIronOre = static_cast< precision_t >(5150);

		template< typename precision_t = float >
		constexpr precision_t HemimorphiteZincOre = static_cast< precision_t >(3442.5);

		template< typename precision_t = float >
		constexpr precision_t HydrochloricAcid40 = static_cast< precision_t >(1201);

		template< typename precision_t = float >
		constexpr precision_t IceSolid = static_cast< precision_t >(919);

		template< typename precision_t = float >
		constexpr precision_t IceCrushed = static_cast< precision_t >(593);

		template< typename precision_t = float >
		constexpr precision_t Ilmenite = static_cast< precision_t >(2307);

		template< typename precision_t = float >
		constexpr precision_t Iridium = static_cast< precision_t >(22154);

		template< typename precision_t = float >
		constexpr precision_t IronOreCrushed = static_cast< precision_t >(2500);

		template< typename precision_t = float >
		constexpr precision_t IronOxidePigment = static_cast< precision_t >(400);

		template< typename precision_t = float >
		constexpr precision_t IronPyrites = static_cast< precision_t >(2400);

		template< typename precision_t = float >
		constexpr precision_t IronSulphateDry = static_cast< precision_t >(1200);

		template< typename precision_t = float >
		constexpr precision_t IronSulphateWet = static_cast< precision_t >(1290);

		template< typename precision_t = float >
		constexpr precision_t Ivory = static_cast< precision_t >(1842);

		template< typename precision_t = float >
		constexpr precision_t KaolinGreenCrushed = static_cast< precision_t >(1025);

		template< typename precision_t = float >
		constexpr precision_t KaolinPulverized = static_cast< precision_t >(352);

		template< typename precision_t = float >
		constexpr precision_t LeadRolled = static_cast< precision_t >(11389);

		template< typename precision_t = float >
		constexpr precision_t LeadRed = static_cast< precision_t >(3684);

		template< typename precision_t = float >
		constexpr precision_t LeadWhitePigment = static_cast< precision_t >(4085);

		template< typename precision_t = float >
		constexpr precision_t Leather = static_cast< precision_t >(945);

		template< typename precision_t = float >
		constexpr precision_t LigniteDry = static_cast< precision_t >(801);

		template< typename precision_t = float >
		constexpr precision_t LimeQuickLump = static_cast< precision_t >(849);

		template< typename precision_t = float >
		constexpr precision_t LimeQuickFine = static_cast< precision_t >(1201);

		template< typename precision_t = float >
		constexpr precision_t LimeStoneLarge = static_cast< precision_t >(2691);

		template< typename precision_t = float >
		constexpr precision_t LimeStoneLump = static_cast< precision_t >(1538);

		template< typename precision_t = float >
		constexpr precision_t LimeHydrated = static_cast< precision_t >(481);

		template< typename precision_t = float >
		constexpr precision_t LimeWetOrMortar = static_cast< precision_t >(1540);

		template< typename precision_t = float >
		constexpr precision_t LimoniteSolid = static_cast< precision_t >(3796);

		template< typename precision_t = float >
		constexpr precision_t LimoniteBroken = static_cast< precision_t >(2467);

		template< typename precision_t = float >
		constexpr precision_t LimestoneSolid = static_cast< precision_t >(2611);

		template< typename precision_t = float >
		constexpr precision_t LimestoneBroken = static_cast< precision_t >(1554);

		template< typename precision_t = float >
		constexpr precision_t LimestonePulverized = static_cast< precision_t >(1394);

		template< typename precision_t = float >
		constexpr precision_t LinseedWhole = static_cast< precision_t >(753);

		template< typename precision_t = float >
		constexpr precision_t LinseedMeal = static_cast< precision_t >(513);

		template< typename precision_t = float >
		constexpr precision_t LocustDry = static_cast< precision_t >(705);

		template< typename precision_t = float >
		constexpr precision_t MagnesiteSolid = static_cast< precision_t >(3011);

		template< typename precision_t = float >
		constexpr precision_t MagnesiumOxide = static_cast< precision_t >(1940);

		template< typename precision_t = float >
		constexpr precision_t MagnesiumSulphateCrystal = static_cast< precision_t >(1121);

		template< typename precision_t = float >
		constexpr precision_t MagnetiteSolidIronOre = static_cast< precision_t >(5046);

		template< typename precision_t = float >
		constexpr precision_t MagnetiteBroken = static_cast< precision_t >(3284);

		template< typename precision_t = float >
		constexpr precision_t MalachiteCopperOre = static_cast< precision_t >(3855);

		template< typename precision_t = float >
		constexpr precision_t Malt = static_cast< precision_t >(336);

		template< typename precision_t = float >
		constexpr precision_t ManganeseSolid = static_cast< precision_t >(7609);

		template< typename precision_t = float >
		constexpr precision_t ManganeseOxide = static_cast< precision_t >(1922);

		template< typename precision_t = float >
		constexpr precision_t Manure = static_cast< precision_t >(400);

		template< typename precision_t = float >
		constexpr precision_t MarbleSolid = static_cast< precision_t >(2563);

		template< typename precision_t = float >
		constexpr precision_t MarbleBroken = static_cast< precision_t >(1570);

		template< typename precision_t = float >
		constexpr precision_t MarlWetExcavated = static_cast< precision_t >(2243);

		template< typename precision_t = float >
		constexpr precision_t MicaSolid = static_cast< precision_t >(2883);

		template< typename precision_t = float >
		constexpr precision_t MicaBroken = static_cast< precision_t >(1602);

		template< typename precision_t = float >
		constexpr precision_t MicaFlake = static_cast< precision_t >(520);

		template< typename precision_t = float >
		constexpr precision_t MicaPowder = static_cast< precision_t >(986);

		template< typename precision_t = float >
		constexpr precision_t MilkPowdered = static_cast< precision_t >(449);

		template< typename precision_t = float >
		constexpr precision_t MolybdenumOre = static_cast< precision_t >(1600);

		template< typename precision_t = float >
		constexpr precision_t MortarWet = static_cast< precision_t >(2403);

		template< typename precision_t = float >
		constexpr precision_t MudPacked = static_cast< precision_t >(1906);

		template< typename precision_t = float >
		constexpr precision_t MudFluid = static_cast< precision_t >(1730);

		template< typename precision_t = float >
		constexpr precision_t NickelOre = static_cast< precision_t >(1600);

		template< typename precision_t = float >
		constexpr precision_t NickelRolled = static_cast< precision_t >(8666);

		template< typename precision_t = float >
		constexpr precision_t NitricAcid91 = static_cast< precision_t >(1506);

		template< typename precision_t = float >
		constexpr precision_t Nitrogen = static_cast< precision_t >(1.26);

		template< typename precision_t = float >
		constexpr precision_t WoodOakRed = static_cast< precision_t >(705);

		template< typename precision_t = float >
		constexpr precision_t Oats = static_cast< precision_t >(432);

		template< typename precision_t = float >
		constexpr precision_t OatsRolled = static_cast< precision_t >(304);

		template< typename precision_t = float >
		constexpr precision_t OilCake = static_cast< precision_t >(785);

		template< typename precision_t = float >
		constexpr precision_t OilLinseed = static_cast< precision_t >(942);

		template< typename precision_t = float >
		constexpr precision_t OilPetroleum = static_cast< precision_t >(881);

		template< typename precision_t = float >
		constexpr precision_t Oxygen = static_cast< precision_t >(1.43);

		template< typename precision_t = float >
		constexpr precision_t OysterShellsGround = static_cast< precision_t >(849);

		template< typename precision_t = float >
		constexpr precision_t PaperStandard = static_cast< precision_t >(1201);

		template< typename precision_t = float >
		constexpr precision_t PeanutsShelled = static_cast< precision_t >(641);

		template< typename precision_t = float >
		constexpr precision_t PeanutsNotShelled = static_cast< precision_t >(272);

		template< typename precision_t = float >
		constexpr precision_t PeatDry = static_cast< precision_t >(400);

		template< typename precision_t = float >
		constexpr precision_t PeatMoist = static_cast< precision_t >(801);

		template< typename precision_t = float >
		constexpr precision_t PeatWet = static_cast< precision_t >(1121);

		template< typename precision_t = float >
		constexpr precision_t PecanWood = static_cast< precision_t >(753);

		template< typename precision_t = float >
		constexpr precision_t PhosphateRockBroken = static_cast< precision_t >(1762);

		template< typename precision_t = float >
		constexpr precision_t Phosphorus = static_cast< precision_t >(2339);

		template< typename precision_t = float >
		constexpr precision_t Pitch = static_cast< precision_t >(1153);

		template< typename precision_t = float >
		constexpr precision_t Plaster = static_cast< precision_t >(849);

		template< typename precision_t = float >
		constexpr precision_t PlatinumOre = static_cast< precision_t >(2600);

		template< typename precision_t = float >
		constexpr precision_t Porcelain = static_cast< precision_t >(2403);

		template< typename precision_t = float >
		constexpr precision_t PorphyrySolid = static_cast< precision_t >(2547);

		template< typename precision_t = float >
		constexpr precision_t PorphyryBroken = static_cast< precision_t >(1650);

		template< typename precision_t = float >
		constexpr precision_t Potash = static_cast< precision_t >(1281);

		template< typename precision_t = float >
		constexpr precision_t PotassiumChloride = static_cast< precision_t >(2002);

		template< typename precision_t = float >
		constexpr precision_t PotatoesWhite = static_cast< precision_t >(769);

		template< typename precision_t = float >
		constexpr precision_t PumiceStone = static_cast< precision_t >(641);

		template< typename precision_t = float >
		constexpr precision_t Pyrite = static_cast< precision_t >(3707.5);

		template< typename precision_t = float >
		constexpr precision_t QuartzSolid = static_cast< precision_t >(2643);

		template< typename precision_t = float >
		constexpr precision_t QuartzLump = static_cast< precision_t >(1554);

		template< typename precision_t = float >
		constexpr precision_t QuartzSand = static_cast< precision_t >(1201);

		template< typename precision_t = float >
		constexpr precision_t ResinSyntheticCrushed = static_cast< precision_t >(561);

		template< typename precision_t = float >
		constexpr precision_t RiceHulled = static_cast< precision_t >(753);

		template< typename precision_t = float >
		constexpr precision_t RiceRough = static_cast< precision_t >(577);

		template< typename precision_t = float >
		constexpr precision_t RiceGrits = static_cast< precision_t >(689);

		template< typename precision_t = float >
		constexpr precision_t RipRap = static_cast< precision_t >(1602);

		template< typename precision_t = float >
		constexpr precision_t RockSoft = static_cast< precision_t >(1690);

		template< typename precision_t = float >
		constexpr precision_t Rosin = static_cast< precision_t >(1073);

		template< typename precision_t = float >
		constexpr precision_t RubberCaoutchouc = static_cast< precision_t >(945);

		template< typename precision_t = float >
		constexpr precision_t RubberManufactured = static_cast< precision_t >(1522);

		template< typename precision_t = float >
		constexpr precision_t RubberGroundScrap = static_cast< precision_t >(481);

		template< typename precision_t = float >
		constexpr precision_t Rye = static_cast< precision_t >(705);

		template< typename precision_t = float >
		constexpr precision_t SaltCake = static_cast< precision_t >(1442);

		template< typename precision_t = float >
		constexpr precision_t SaltCourse = static_cast< precision_t >(801);

		template< typename precision_t = float >
		constexpr precision_t SaltFine = static_cast< precision_t >(1201);

		template< typename precision_t = float >
		constexpr precision_t Saltpeter = static_cast< precision_t >(1201);

		template< typename precision_t = float >
		constexpr precision_t SandWet = static_cast< precision_t >(1922);

		template< typename precision_t = float >
		constexpr precision_t SandWetPacked = static_cast< precision_t >(2082);

		template< typename precision_t = float >
		constexpr precision_t SandDry = static_cast< precision_t >(1602);

		template< typename precision_t = float >
		constexpr precision_t SandLoose = static_cast< precision_t >(1442);

		template< typename precision_t = float >
		constexpr precision_t SandRammed = static_cast< precision_t >(1682);

		template< typename precision_t = float >
		constexpr precision_t SandWaterFilled = static_cast< precision_t >(1922);

		template< typename precision_t = float >
		constexpr precision_t SandWithGravelDry = static_cast< precision_t >(1650);

		template< typename precision_t = float >
		constexpr precision_t SandWithGravelWet = static_cast< precision_t >(2020);

		template< typename precision_t = float >
		constexpr precision_t SandstoneSolid = static_cast< precision_t >(2323);

		template< typename precision_t = float >
		constexpr precision_t SandstoneBroken = static_cast< precision_t >(1410);

		template< typename precision_t = float >
		constexpr precision_t Sawdust = static_cast< precision_t >(210);

		template< typename precision_t = float >
		constexpr precision_t SewageSludge = static_cast< precision_t >(721);

		template< typename precision_t = float >
		constexpr precision_t ShaleSolid = static_cast< precision_t >(2675);

		template< typename precision_t = float >
		constexpr precision_t ShaleBroken = static_cast< precision_t >(1586);

		template< typename precision_t = float >
		constexpr precision_t ShellsOyster = static_cast< precision_t >(800);

		template< typename precision_t = float >
		constexpr precision_t Sinter = static_cast< precision_t >(1890);

		template< typename precision_t = float >
		constexpr precision_t SlagSolid = static_cast< precision_t >(2114);

		template< typename precision_t = float >
		constexpr precision_t SlagBroken = static_cast< precision_t >(1762);

		template< typename precision_t = float >
		constexpr precision_t SlagCrushed = static_cast< precision_t >(1185);

		template< typename precision_t = float >
		constexpr precision_t SlagFurnGranulated = static_cast< precision_t >(961);

		template< typename precision_t = float >
		constexpr precision_t SlateSolid = static_cast< precision_t >(2691);

		template< typename precision_t = float >
		constexpr precision_t SlateBroken = static_cast< precision_t >(1370);

		template< typename precision_t = float >
		constexpr precision_t SlatePulverized = static_cast< precision_t >(1362);

		template< typename precision_t = float >
		constexpr precision_t SmithsoniteZincOre = static_cast< precision_t >(4300);

		template< typename precision_t = float >
		constexpr precision_t SnowFreshlyFallen = static_cast< precision_t >(160);

		template< typename precision_t = float >
		constexpr precision_t SnowCompacted = static_cast< precision_t >(481);

		template< typename precision_t = float >
		constexpr precision_t SoapSolid = static_cast< precision_t >(801);

		template< typename precision_t = float >
		constexpr precision_t SoapChips = static_cast< precision_t >(160);

		template< typename precision_t = float >
		constexpr precision_t SoapFlakes = static_cast< precision_t >(160);

		template< typename precision_t = float >
		constexpr precision_t SoapPowdered = static_cast< precision_t >(368);

		template< typename precision_t = float >
		constexpr precision_t SoapstoneTalc = static_cast< precision_t >(2400);

		template< typename precision_t = float >
		constexpr precision_t SodaAshHeavy = static_cast< precision_t >(1080);

		template< typename precision_t = float >
		constexpr precision_t SodaAshLight = static_cast< precision_t >(432);

		template< typename precision_t = float >
		constexpr precision_t Sodium = static_cast< precision_t >(977);

		template< typename precision_t = float >
		constexpr precision_t SodiumAluminateGround = static_cast< precision_t >(1153);

		template< typename precision_t = float >
		constexpr precision_t SodiumNitrateGround = static_cast< precision_t >(1201);

		template< typename precision_t = float >
		constexpr precision_t SoyBeansWhole = static_cast< precision_t >(753);

		template< typename precision_t = float >
		constexpr precision_t StarchPowdered = static_cast< precision_t >(561);

		template< typename precision_t = float >
		constexpr precision_t StoneCrushed = static_cast< precision_t >(1602);

		template< typename precision_t = float >
		constexpr precision_t StoneCommonGeneric = static_cast< precision_t >(2515);

		template< typename precision_t = float >
		constexpr precision_t SugarBrown = static_cast< precision_t >(721);

		template< typename precision_t = float >
		constexpr precision_t SugarPowdered = static_cast< precision_t >(801);

		template< typename precision_t = float >
		constexpr precision_t SugarGranulated = static_cast< precision_t >(849);

		template< typename precision_t = float >
		constexpr precision_t SugarRawCane = static_cast< precision_t >(961);

		template< typename precision_t = float >
		constexpr precision_t SugarbeetPulpDry = static_cast< precision_t >(208);

		template< typename precision_t = float >
		constexpr precision_t SugarbeetPulpWet = static_cast< precision_t >(561);

		template< typename precision_t = float >
		constexpr precision_t Sugarcane = static_cast< precision_t >(272);

		template< typename precision_t = float >
		constexpr precision_t SulphurSolid = static_cast< precision_t >(2002);

		template< typename precision_t = float >
		constexpr precision_t SulphurLump = static_cast< precision_t >(1314);

		template< typename precision_t = float >
		constexpr precision_t SulphurPulverized = static_cast< precision_t >(961);

		template< typename precision_t = float >
		constexpr precision_t Taconite = static_cast< precision_t >(2803);

		template< typename precision_t = float >
		constexpr precision_t TalcSolid = static_cast< precision_t >(2691);

		template< typename precision_t = float >
		constexpr precision_t TalcBroken = static_cast< precision_t >(1746);

		template< typename precision_t = float >
		constexpr precision_t TanbarkGround = static_cast< precision_t >(881);

		template< typename precision_t = float >
		constexpr precision_t Tankage = static_cast< precision_t >(961);

		template< typename precision_t = float >
		constexpr precision_t Tar = static_cast< precision_t >(1153);

		template< typename precision_t = float >
		constexpr precision_t Tobacco = static_cast< precision_t >(320);

		template< typename precision_t = float >
		constexpr precision_t TrapRockSolid = static_cast< precision_t >(2883);

		template< typename precision_t = float >
		constexpr precision_t TrapRockBroken = static_cast< precision_t >(1746);

		template< typename precision_t = float >
		constexpr precision_t Turf = static_cast< precision_t >(400);

		template< typename precision_t = float >
		constexpr precision_t Turpentine = static_cast< precision_t >(865);

		template< typename precision_t = float >
		constexpr precision_t WalnutBlackDry = static_cast< precision_t >(609);

		template< typename precision_t = float >
		constexpr precision_t WaterPure = static_cast< precision_t >(1000);

		template< typename precision_t = float >
		constexpr precision_t WaterSea = static_cast< precision_t >(1026);

		template< typename precision_t = float >
		constexpr precision_t Wheat = static_cast< precision_t >(769);

		template< typename precision_t = float >
		constexpr precision_t WheatCracked = static_cast< precision_t >(673);

		template< typename precision_t = float >
		constexpr precision_t Wool = static_cast< precision_t >(1314);

		template< typename precision_t = float >
		constexpr precision_t ZincOxide = static_cast< precision_t >(400);

		template< typename precision_t = float >
		constexpr precision_t AluminiumMelted = static_cast< precision_t >(2600);

		template< typename precision_t = float >
		constexpr precision_t AluminiumBronze = static_cast< precision_t >(8200);

		template< typename precision_t = float >
		constexpr precision_t AluminiumFoil = static_cast< precision_t >(2725);

		template< typename precision_t = float >
		constexpr precision_t AntifrictionMetal = static_cast< precision_t >(9865);

		template< typename precision_t = float >
		constexpr precision_t Beryllium = static_cast< precision_t >(1840);

		template< typename precision_t = float >
		constexpr precision_t BerylliumCopper = static_cast< precision_t >(8175);

		template< typename precision_t = float >
		constexpr precision_t BrassCasting = static_cast< precision_t >(8550);

		template< typename precision_t = float >
		constexpr precision_t BrassRolledAndDrawn = static_cast< precision_t >(8580);

		template< typename precision_t = float >
		constexpr precision_t BronzeLead = static_cast< precision_t >(8200);

		template< typename precision_t = float >
		constexpr precision_t BronzePhosphorous = static_cast< precision_t >(8850);

		template< typename precision_t = float >
		constexpr precision_t BronzeTin = static_cast< precision_t >(8150);

		template< typename precision_t = float >
		constexpr precision_t CastIron = static_cast< precision_t >(7300);

		template< typename precision_t = float >
		constexpr precision_t Cobolt = static_cast< precision_t >(8746);

		template< typename precision_t = float >
		constexpr precision_t Copper = static_cast< precision_t >(8930);

		template< typename precision_t = float >
		constexpr precision_t DeltaMetal = static_cast< precision_t >(8600);

		template< typename precision_t = float >
		constexpr precision_t Electrum = static_cast< precision_t >(8650);

		template< typename precision_t = float >
		constexpr precision_t Gold = static_cast< precision_t >(19320);

		template< typename precision_t = float >
		constexpr precision_t Iron = static_cast< precision_t >(7850);

		template< typename precision_t = float >
		constexpr precision_t Lead = static_cast< precision_t >(11340);

		template< typename precision_t = float >
		constexpr precision_t Magnesium = static_cast< precision_t >(1738);

		template< typename precision_t = float >
		constexpr precision_t Mercury = static_cast< precision_t >(13593);

		template< typename precision_t = float >
		constexpr precision_t Molybdenum = static_cast< precision_t >(10188);

		template< typename precision_t = float >
		constexpr precision_t Monel = static_cast< precision_t >(8600);

		template< typename precision_t = float >
		constexpr precision_t Nickel = static_cast< precision_t >(8800);

		template< typename precision_t = float >
		constexpr precision_t NickelSilver = static_cast< precision_t >(8442);

		template< typename precision_t = float >
		constexpr precision_t Platinum = static_cast< precision_t >(21400);

		template< typename precision_t = float >
		constexpr precision_t Plutonium = static_cast< precision_t >(19800);

		template< typename precision_t = float >
		constexpr precision_t Silver = static_cast< precision_t >(10490);

		template< typename precision_t = float >
		constexpr precision_t SteelRolled = static_cast< precision_t >(7850);

		template< typename precision_t = float >
		constexpr precision_t SteelStainless = static_cast< precision_t >(7740);

		template< typename precision_t = float >
		constexpr precision_t Tin = static_cast< precision_t >(7280);

		template< typename precision_t = float >
		constexpr precision_t Titanium = static_cast< precision_t >(4500);

		template< typename precision_t = float >
		constexpr precision_t Tungsten = static_cast< precision_t >(19600);

		template< typename precision_t = float >
		constexpr precision_t Uranium = static_cast< precision_t >(18900);

		template< typename precision_t = float >
		constexpr precision_t Vanadium = static_cast< precision_t >(5494);

		template< typename precision_t = float >
		constexpr precision_t WhiteMetal = static_cast< precision_t >(7100);

		template< typename precision_t = float >
		constexpr precision_t Zinc = static_cast< precision_t >(7135);

		template< typename precision_t = float >
		constexpr precision_t WoodChipsDry = static_cast< precision_t >(380);

		template< typename precision_t = float >
		constexpr precision_t WoodAfromosia = static_cast< precision_t >(705);

		template< typename precision_t = float >
		constexpr precision_t WoodApple = static_cast< precision_t >(745);

		template< typename precision_t = float >
		constexpr precision_t WoodAshBlack = static_cast< precision_t >(540);

		template< typename precision_t = float >
		constexpr precision_t WoodAshWhite = static_cast< precision_t >(670);

		template< typename precision_t = float >
		constexpr precision_t WoodAspen = static_cast< precision_t >(420);

		template< typename precision_t = float >
		constexpr precision_t WoodBalsa = static_cast< precision_t >(170);

		template< typename precision_t = float >
		constexpr precision_t WoodBamboo = static_cast< precision_t >(350);

		template< typename precision_t = float >
		constexpr precision_t WoodBirch = static_cast< precision_t >(670);

		template< typename precision_t = float >
		constexpr precision_t WoodCedarRed = static_cast< precision_t >(380);

		template< typename precision_t = float >
		constexpr precision_t WoodCypress = static_cast< precision_t >(510);

		template< typename precision_t = float >
		constexpr precision_t WoodDouglasFir = static_cast< precision_t >(530);

		template< typename precision_t = float >
		constexpr precision_t WoodEbony = static_cast< precision_t >(1040);

		template< typename precision_t = float >
		constexpr precision_t WoodElm = static_cast< precision_t >(707.5);

		template< typename precision_t = float >
		constexpr precision_t WoodIroko = static_cast< precision_t >(655);

		template< typename precision_t = float >
		constexpr precision_t WoodLarch = static_cast< precision_t >(590);

		template< typename precision_t = float >
		constexpr precision_t WoodLignumVitae = static_cast< precision_t >(1325);

		template< typename precision_t = float >
		constexpr precision_t WoodMahogany = static_cast< precision_t >(697.5);

		template< typename precision_t = float >
		constexpr precision_t WoodMaple = static_cast< precision_t >(755);

		template< typename precision_t = float >
		constexpr precision_t WoodOak = static_cast< precision_t >(760);

		template< typename precision_t = float >
		constexpr precision_t WoodPine = static_cast< precision_t >(505);

		template< typename precision_t = float >
		constexpr precision_t WoodRedwood = static_cast< precision_t >(480);

		template< typename precision_t = float >
		constexpr precision_t WoodSpruce = static_cast< precision_t >(450);

		template< typename precision_t = float >
		constexpr precision_t WoodSycamore = static_cast< precision_t >(590);

		template< typename precision_t = float >
		constexpr precision_t WoodTeak = static_cast< precision_t >(675);

		template< typename precision_t = float >
		constexpr precision_t WoodWillow = static_cast< precision_t >(420);
	}

	namespace Radius
	{
		/** @brief Earth radius expressed in meters. */
		template< typename precision_t = float >
		constexpr precision_t Earth = SI::Meter< precision_t > * static_cast< precision_t >(6371 * 1000);

		/** @brief Moon radius expressed in meters. */
		template< typename precision_t = float >
		constexpr precision_t Moon = SI::Meter< precision_t > * static_cast< precision_t >(1737.4 * 1000);

		/** @brief Mars radius expressed in meters. */
		template< typename precision_t = float >
		constexpr precision_t Mars = SI::Meter< precision_t > * static_cast< precision_t >(3389.5 * 1000);

		/** @brief Jupiter radius expressed in meters. */
		template< typename precision_t = float >
		constexpr precision_t Jupiter = SI::Meter< precision_t > * static_cast< precision_t >(69911 * 1000);
	}

	/** @brief Standard air pressure expressed in Pa at sea level. */
	template< typename precision_t = float >
	constexpr precision_t AirPressure = static_cast< precision_t >(101325);

	/** @brief Absolute zero in celsius temperature. */
	template< typename precision_t = float >
	constexpr precision_t AbsoluteZero = static_cast< precision_t >(-273.15);

	/**
	 * @brief Geo-potential ...
	 * @param altitude The altitude in meters.
	 * @return precision_t
	 */
	template< typename precision_t = float >
	constexpr
	precision_t
	getGeoPotential (precision_t altitude) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return Radius::Earth< precision_t > * altitude / (Radius::Earth< precision_t > + altitude);
	}

	/**
	 * @brief Returns standard atmospheric temperature Air temperature in K°
	 * @note Temperature is in kelvins = 273.15 + Celsius
	 * @param geoPotentailHeight The value returned by getGeoPotential().
	 * @return precision_t
	 */
	template< typename precision_t = float >
	constexpr
	precision_t
	getStandardTemperature (precision_t geoPotentailHeight) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		/* Troposphere */
		if ( geoPotentailHeight <= static_cast< precision_t >(11.0) )
		{
			return static_cast< precision_t >(288.15) - (static_cast< precision_t >(6.5) * geoPotentailHeight);
		}

		/* Stratosphere starts */
		if ( geoPotentailHeight <= static_cast< precision_t >(20.0) )
		{
			return static_cast< precision_t >(216.65);
		}

		if ( geoPotentailHeight <= static_cast< precision_t >(32.0) )
		{
			return static_cast< precision_t >(196.65) + geoPotentailHeight;
		}

		if ( geoPotentailHeight <= static_cast< precision_t >(47.0) )
		{
			return static_cast< precision_t >(228.65) + static_cast< precision_t >(2.8) * (geoPotentailHeight - static_cast< precision_t >(32.0));
		}

		/* Mesosphere starts */
		if ( geoPotentailHeight <= static_cast< precision_t >(51.0) )
		{
			return static_cast< precision_t >(270.65);
		}

		if ( geoPotentailHeight <= static_cast< precision_t >(71.0) )
		{
			return static_cast< precision_t >(270.65) - static_cast< precision_t >(2.8) * (geoPotentailHeight - static_cast< precision_t >(51.0));
		}

		if ( geoPotentailHeight <= static_cast< precision_t >(84.85) )
		{
			return static_cast< precision_t >(214.65) - static_cast< precision_t >(2.0) * (geoPotentailHeight - static_cast< precision_t >(71.0));
		}

		/* NOTE : Thermosphere has high kinetic temperature (500 C to 2000 C) but temperature
		 * as measured by a thermometer would be very low because of almost vacuum. */

		return 0;
	}

	/**
	 * @brief Returns standard atmospheric air pressure on earth in Pa (Pascals)
	 * @param altitude The altitude in meters.
	 * @return precision_t
	 */
	template< typename precision_t = float >
	constexpr
	precision_t
	getStandardPressure (precision_t altitude) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		const auto geoPotentialHeight = Physics::getGeoPotential< precision_t >(altitude);

		const auto temperature = Physics::getStandardTemperature< precision_t >(geoPotentialHeight);

		if ( geoPotentialHeight <= static_cast< precision_t >(11.0) )
		{
			return static_cast< precision_t >(101325.0) * std::pow(static_cast< precision_t >(288.15) / temperature, static_cast< precision_t >(-5.255877));
		}

		if ( geoPotentialHeight <= static_cast< precision_t >(20.0) )
		{
			return static_cast< precision_t >(22632.06) * std::exp(static_cast< precision_t >(-0.1577) * (geoPotentialHeight - static_cast< precision_t >(11.0)));
		}

		if ( geoPotentialHeight <= static_cast< precision_t >(32.0) )
		{
			return static_cast< precision_t >(5474.889) * std::pow(static_cast< precision_t >(216.65) / temperature, static_cast< precision_t >(34.16319));
		}

		if ( geoPotentialHeight <= static_cast< precision_t >(47.0) )
		{
			return static_cast< precision_t >(868.0187) * std::pow(static_cast< precision_t >(228.65) / temperature, static_cast< precision_t >(12.2011));
		}

		if ( geoPotentialHeight <= static_cast< precision_t >(51.0) )
		{
			return static_cast< precision_t >(110.9063) * std::exp(static_cast< precision_t >(-0.1262) * (geoPotentialHeight - static_cast< precision_t >(47.0)));
		}

		if ( geoPotentialHeight <= static_cast< precision_t >(71.0) )
		{
			return static_cast< precision_t >(66.93887) * std::pow(static_cast< precision_t >(270.65) / temperature, static_cast< precision_t >(-12.2011));
		}

		if ( geoPotentialHeight <= static_cast< precision_t >(84.85) )
		{
			return static_cast< precision_t >(3.956420) * std::pow(static_cast< precision_t >(214.65) / temperature, static_cast< precision_t >(-17.0816));
		}

		return 0;
	}

	/**
	 * @brief Returns air density based on earth environment in kg/m³.
	 * @note Below 51 km: Practical Meteorology by Roland Stull, pg 12.
	 * Above 51 km: http://www.braeunig.us/space/atmmodel.htm.
	 * Validation data: https://www.avs.org/AVS/files/c7/c7edaedb-95b2-438f-adfb-36de54f87b9e.pdf
	 * Example : density = 101325 / (287.05 * (15 + 273.15)) = 1.225 kg/m3 (http://barani.biz/apps/air-density/)
	 * @param altitude The altitude in meters.
	 * @param temperature The ambient temperature in degree C°.
	 * @return float
	 */
	template< typename precision_t = float >
	precision_t
	getAirDensity (precision_t altitude = 0, precision_t temperature = 20) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		const auto kelvin = std::abs(AbsoluteZero< precision_t >) + temperature;

		return Physics::getStandardPressure< precision_t >(altitude) / (static_cast< precision_t >(287.05) * kelvin);
	}

	/**
	 * @brief Returns a force vector from a mass and an acceleration.
	 * @note 1. Every object persists in its state of rest or uniform motion
	 * in a straight line unless it is compelled to change that state
	 * by forces impressed on it.
	 * 2. Force is equal to the change in momentum (mV) per change
	 * in time. For a constant mass, force equals mass times acceleration.
	 * "F = m * a".
	 * 3. For every action, there is an equal and opposite re-action.
	 * @tparam precision_t The precision of parameters. Default float.
	 * @param mass The mass of the object in Kg.
	 * @param acceleration A vector 3 that define the direction of the acceleration.
	 * @return precision_t
	 */
	template< typename precision_t = float >
	precision_t
	force (precision_t mass, const Libs::Math::Vector< 3, precision_t > & acceleration) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return mass * acceleration;
	}

	/**
	 * @brief Computes terminal velocity of a moving object in a fluid or air. Terminal velocity in m/s.
	 * @tparam precision_t The precision of parameters. Default float.
	 * @param mass The mass of the object in Kg.
	 * @param surface The object surface in m².
	 * @param dragCoefficient The drag coefficient of the object. Use constants.
	 * @param gravity The gravity acceleration of the object in m/s².
	 * @param density The density of the environment where the object is moving in kg/m³.
	 * @return precision_t
	 */
	template< typename precision_t = float >
	precision_t
	computeTerminalVelocity (precision_t mass, precision_t surface, precision_t dragCoefficient, precision_t gravity, precision_t density) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return std::sqrt((static_cast< precision_t >(2.0) * mass * gravity) / (density * surface * dragCoefficient));
	}

	/**
	 * @brief Computes the drag magnitude for an object in movement in a density.
	 * @note Formula : Drag = [Drag coefficient] * 0.5 * [density "kg/m³"] * [Velocity "m/s²"]² * [Area "m²"]
	 * @tparam precision_t The precision of parameters. Default float.
	 * @param dragCoefficient The object volumetric drag coefficient. You can use Physics::DragCoefficient constants.
	 * @param density The air or water density in kg/m³.
	 * @param velocity The object velocity in m/s².
	 * @param surface The object surface facing the direction of the movement in m².
	 * @return precision_t
	 */
	template< typename precision_t = float >
	precision_t
	getDragMagnitude (precision_t dragCoefficient, precision_t density, precision_t velocity, precision_t surface) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return dragCoefficient * static_cast< precision_t >(0.5) * density * (velocity * velocity) * surface;
	}

	/**
	 * @brief Modulates a scalar according to a factor. If the factor is 0, the return value will be 1.
	 * @tparam precision_t
	 * @param value A normalized value to modulate.
	 * @param factor A scalar number like a dot product between two vectors.
	 * @return precision_t
	 */
	template< class precision_t >
	precision_t
	modulateNormalizedValue (precision_t value, precision_t factor)
	{
		return value + ((static_cast< precision_t >(1) - value) * factor);
	}
}

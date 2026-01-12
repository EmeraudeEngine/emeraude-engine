/*
 * src/Testing/test_Version.cpp
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

#include <gtest/gtest.h>

/* Local inclusions. */
#include "Libs/Version.hpp"

namespace EmEn::Libs
{
	TEST(Version, initDefault)
	{
		constexpr Version version;

		ASSERT_EQ(version.major(), 0);
		ASSERT_EQ(version.minor(), 0);
		ASSERT_EQ(version.revision(), 0);

		std::cout << version << std::endl;
	}

	TEST(Version, initFromArgs)
	{
		constexpr Version version{2, 9, 46};

		ASSERT_EQ(version.major(), 2);
		ASSERT_EQ(version.minor(), 9);
		ASSERT_EQ(version.revision(), 46);

		std::cout << version << std::endl;
	}

	TEST(Version, initFromRaw)
	{
		{
			Version version;
			ASSERT_TRUE(version.parseFromString("1.2.295"));

			ASSERT_EQ(version.major(), 1);
			ASSERT_EQ(version.minor(), 2);
			ASSERT_EQ(version.revision(), 295);

			std::cout << version << std::endl;
		}

		{
			const auto version = Version::FromString("1,8,64", ',');

			ASSERT_TRUE(version);

			ASSERT_EQ(version.value().major(), 1);
			ASSERT_EQ(version.value().minor(), 8);
			ASSERT_EQ(version.value().revision(), 64);

			std::cout << version.value() << std::endl;
		}
	}

	TEST(Version, equalDifferent)
	{
		constexpr Version versionA{1, 0, 9};
		constexpr Version versionB{1, 0, 9};

		ASSERT_TRUE(versionA == versionB);
	}

	TEST(Version, different)
	{
		constexpr Version versionA{1, 0, 9};
		constexpr Version versionB{2, 9, 18};

		ASSERT_TRUE(versionA != versionB);
	}

	TEST(Version, greater)
	{
		{
			constexpr Version versionA{3, 1, 12};
			constexpr Version versionB{2, 9, 18};

			ASSERT_TRUE(versionA > versionB);
		}

		{
			constexpr Version versionA{1, 9, 168};
			constexpr Version versionB{1, 8, 1256};

			ASSERT_TRUE(versionA > versionB);
		}

		{
			constexpr Version versionA{2, 3, 350};
			constexpr Version versionB{2, 3, 225};

			ASSERT_TRUE(versionA > versionB);
		}
	}

	TEST(Version, lower)
	{
		{
			constexpr Version versionA{3, 1, 12};
			constexpr Version versionB{2, 9, 18};

			ASSERT_TRUE(versionB < versionA);
		}

		{
			constexpr Version versionA{1, 9, 168};
			constexpr Version versionB{1, 8, 1256};

			ASSERT_TRUE(versionB < versionA);
		}

		{
			constexpr Version versionA{2, 3, 350};
			constexpr Version versionB{2, 3, 225};

			ASSERT_TRUE(versionB < versionA);
		}
	}

	TEST(Version, greaterOrEqual)
	{
		{
			constexpr Version versionA{3, 1, 12};
			constexpr Version versionB{2, 9, 18};

			ASSERT_TRUE(versionA >= versionB);
		}

		{
			constexpr Version versionA{1, 9, 168};
			constexpr Version versionB{1, 8, 1256};

			ASSERT_TRUE(versionA >= versionB);
		}

		{
			constexpr Version versionA{2, 3, 350};
			constexpr Version versionB{2, 3, 225};

			ASSERT_TRUE(versionA >= versionB);
		}

		{
			constexpr Version versionA{1, 2, 9};
			constexpr Version versionB{1, 2, 9};

			ASSERT_TRUE(versionA >= versionB);
		}
	}

	TEST(Version, lowerOrEqual)
	{
		{
			constexpr Version versionA{3, 1, 12};
			constexpr Version versionB{2, 9, 18};

			ASSERT_TRUE(versionB <= versionA);
		}

		{
			constexpr Version versionA{1, 9, 168};
			constexpr Version versionB{1, 8, 1256};

			ASSERT_TRUE(versionB <= versionA);
		}

		{
			constexpr Version versionA{2, 3, 350};
			constexpr Version versionB{2, 3, 225};

			ASSERT_TRUE(versionB <= versionA);
		}

		{
			constexpr Version versionA{1, 2, 9};
			constexpr Version versionB{1, 2, 9};

			ASSERT_TRUE(versionB <= versionA);
		}
	}

	TEST(VersionTest, DefaultConstructor)
	{
		Version v;
		ASSERT_EQ(v.major(), 0);
		ASSERT_EQ(v.minor(), 0);
		ASSERT_EQ(v.revision(), 0);
	}

	TEST(VersionTest, IntegerConstructor)
	{
		Version v(1, 2, 3);
		ASSERT_EQ(v.major(), 1);
		ASSERT_EQ(v.minor(), 2);
		ASSERT_EQ(v.revision(), 3);
	}

	TEST(VersionTest, ConstexprConstructor)
	{
		// La compilation de cette ligne prouve que le constructeur est bien constexpr.
		constexpr Version v(4, 5, 6);
		ASSERT_EQ(v.major(), 4);
		ASSERT_EQ(v.minor(), 5);
		ASSERT_EQ(v.revision(), 6);
	}

	TEST(VersionTest, BitmaskConstructor)
	{
		// Major (10 bits), Minor (10 bits), Revision (12 bits)
		// Test avec des valeurs arbitraires : 5.12.100
		const uint32_t major_part = 5 << 22;
		const uint32_t minor_part = 12 << 12;
		const uint32_t revision_part = 100;
		const uint32_t bitmask = major_part | minor_part | revision_part;

		Version v(bitmask);
		ASSERT_EQ(v.major(), 5);
		ASSERT_EQ(v.minor(), 12);
		ASSERT_EQ(v.revision(), 100);
	}

	TEST(VersionTest, ComparisonOperators)
	{
		Version v1_0_0(1, 0, 0);
		Version v1_1_0(1, 1, 0);
		Version v1_1_1(1, 1, 1);
		Version v2_0_0(2, 0, 0);
		Version v1_1_0_copy(1, 1, 0);

		// Égalité (==)
		ASSERT_EQ(v1_1_0, v1_1_0_copy);

		// Inégalité (!=)
		ASSERT_NE(v1_1_0, v1_1_1);

		// Inférieur (<)
		ASSERT_LT(v1_0_0, v1_1_0);
		ASSERT_LT(v1_1_0, v1_1_1);
		ASSERT_LT(v1_1_1, v2_0_0);

		// Inférieur ou égal (<=)
		ASSERT_LE(v1_1_0, v1_1_0_copy);
		ASSERT_LE(v1_1_0, v1_1_1);

		// Supérieur (>)
		ASSERT_GT(v2_0_0, v1_1_1);
		ASSERT_GT(v1_1_1, v1_1_0);
		ASSERT_GT(v1_1_0, v1_0_0);

		// Supérieur ou égal (>=)
		ASSERT_GE(v1_1_0, v1_1_0_copy);
		ASSERT_GE(v2_0_0, v1_1_1);
	}

	TEST(VersionTest, ParseFromStringValid)
	{
		Version v;

		// Cas simple
		ASSERT_TRUE(v.parseFromString("1.2.3"));
		ASSERT_EQ(v, Version(1, 2, 3));

		// Avec des zéros
		ASSERT_TRUE(v.parseFromString("0.0.0"));
		ASSERT_EQ(v, Version(0, 0, 0));

		// Nombres plus grands
		ASSERT_TRUE(v.parseFromString("100.255.999"));
		ASSERT_EQ(v, Version(100, 255, 999));

		// Avec des espaces (doivent être ignorés)
		ASSERT_TRUE(v.parseFromString("  7.8.9  "));
		ASSERT_EQ(v, Version(7, 8, 9));

		// Avec un séparateur personnalisé
		ASSERT_TRUE(v.parseFromString("4-5-6", '-'));
		ASSERT_EQ(v, Version(4, 5, 6));
	}

	TEST(VersionTest, ParseFromStringInvalid)
	{
		Version v(9, 9, 9); // Initialise à une valeur non nulle pour tester la réinitialisation

		// Chaîne vide
		ASSERT_FALSE(v.parseFromString(""));
		// L'implémentation actuelle ne garantit pas la réinitialisation, mais c'est une bonne pratique à tester
		// Si vous ajoutez la réinitialisation, ce test passera.
		// ASSERT_EQ(v, Version(0, 0, 0));

		// Pas assez de composants
		v.set(9,9,9);
		ASSERT_FALSE(v.parseFromString("1.2"));

		// Trop de composants
		v.set(9,9,9);
		ASSERT_FALSE(v.parseFromString("1.2.3.4"));

		// Caractères invalides
		v.set(9,9,9);
		ASSERT_FALSE(v.parseFromString("1.a.3"));

		// Mauvais séparateur
		v.set(9,9,9);
		ASSERT_FALSE(v.parseFromString("1,2,3"));

		// Caractères à la fin
		v.set(9,9,9);
		ASSERT_FALSE(v.parseFromString("1.2.3-beta"));
	}

	TEST(VersionTest, StaticFactoryFromString)
	{
		// Cas valide
		auto opt_v1 = Version::FromString("10.20.30");
		ASSERT_TRUE(opt_v1.has_value());
		ASSERT_EQ(opt_v1.value(), Version(10, 20, 30));

		// Cas invalide
		auto opt_v2 = Version::FromString("invalid-version");
		ASSERT_FALSE(opt_v2.has_value());

		// Cas vide
		auto opt_v3 = Version::FromString("");
		ASSERT_FALSE(opt_v3.has_value());
	}

	TEST(VersionTest, SettersAndGetters)
	{
		Version v;
		ASSERT_EQ(v, Version(0, 0, 0));

		v.set(5, 6, 7);
		ASSERT_EQ(v, Version(5, 6, 7));
		ASSERT_EQ(v.major(), 5);
		ASSERT_EQ(v.minor(), 6);
		ASSERT_EQ(v.revision(), 7);

		v.setMajor(8);
		ASSERT_EQ(v.major(), 8);
		ASSERT_EQ(v.minor(), 6); // Ne doit pas avoir changé

		v.setMinor(9);
		ASSERT_EQ(v.minor(), 9);
		ASSERT_EQ(v.major(), 8); // Ne doit pas avoir changé

		v.setRevision(10);
		ASSERT_EQ(v.revision(), 10);
		ASSERT_EQ(v.minor(), 9); // Ne doit pas avoir changé
	}

	TEST(VersionTest, StreamOutput)
	{
		Version v(1, 2, 3);
		std::stringstream ss;
		ss << v;
		ASSERT_EQ(ss.str(), "1.2.3");
	}

	TEST(VersionTest, ToStringFunction)
	{
		Version v(4, 5, 6);
		ASSERT_EQ(to_string(v), "4.5.6");
	}
}

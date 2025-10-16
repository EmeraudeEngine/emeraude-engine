/*
 * src/Testing/test_String.cpp
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

#include <gtest/gtest.h>

/* Local inclusions. */
#include "Libs/Randomizer.hpp"
#include "Libs/String.hpp"
#include "Libs/Utility.hpp"

using namespace Libs;

TEST(String, numericLabel)
{
	ASSERT_EQ(String::numericLabel(std::string("toto"), 3), "toto3");
}

TEST(String, incrementalLabel)
{
	int identifier = 4;

	ASSERT_EQ(String::incrementalLabel(std::string("toto"), identifier), "toto4");
	ASSERT_EQ(identifier, 5);

	ASSERT_EQ(String::incrementalLabel(std::string("toto"), identifier), "toto5");
	ASSERT_EQ(identifier, 6);
}

TEST(String, trim)
{
	{
		const std::string source{"\t\nDummySTR \t   "};

		ASSERT_EQ(String::trim(source, String::WhiteCharsList, String::Side::Right), "\t\nDummySTR");
		ASSERT_EQ(String::trim(source, String::WhiteCharsList, String::Side::Left), "DummySTR \t   ");
		ASSERT_EQ(String::trim(source, String::WhiteCharsList, String::Side::Both), "DummySTR");
	}

	{
		const std::string source(" \f\n\r\t\vtoto \f\n\r\t\v");

		EXPECT_EQ(String::trim(source), std::string("toto"));
		EXPECT_EQ(String::trim(source, String::WhiteCharsList, String::Side::Left), std::string("toto \f\n\r\t\v"));
		EXPECT_EQ(String::trim(source, String::WhiteCharsList, String::Side::Right), std::string(" \f\n\r\t\vtoto"));
	}
}

TEST(String, pad)
{
	const std::string source{"DummySTR!"};

	ASSERT_EQ(String::pad(source, 16, '-', String::Side::Right), "DummySTR!-------");
	ASSERT_EQ(String::pad(source, 16, '-', String::Side::Left), "-------DummySTR!");
	ASSERT_EQ(String::pad(source, 16, '-', String::Side::Both), "---DummySTR!----");
}

TEST(String, explode)
{
	{
		const auto source = String::explode("aaaa---bb--cccc----ffffrrrr------", "--");

		ASSERT_EQ(source.size(), 7);

		ASSERT_EQ(source[0], std::string("aaaa"));
		ASSERT_EQ(source[1], std::string("-bb"));
		ASSERT_EQ(source[2], std::string("cccc"));
		ASSERT_EQ(source[3], std::string(""));
		ASSERT_EQ(source[4], std::string("ffffrrrr"));
		ASSERT_EQ(source[5], std::string(""));
		ASSERT_EQ(source[6], std::string(""));
	}

	{
		const auto source = String::explode("aaaa---bb--cccc----ffffrrrr------", "--", false);

		ASSERT_EQ(source.size(), 4);

		ASSERT_EQ(source[0], std::string("aaaa"));
		ASSERT_EQ(source[1], std::string("-bb"));
		ASSERT_EQ(source[2], std::string("cccc"));
		ASSERT_EQ(source[3], std::string("ffffrrrr"));
	}

	{
		const auto source = String::explode("Hello marvelous world !", ' ');

		ASSERT_EQ(source.size(), 4);

		ASSERT_EQ(source[0], std::string("Hello"));
		ASSERT_EQ(source[1], std::string("marvelous"));
		ASSERT_EQ(source[2], std::string("world"));
		ASSERT_EQ(source[3], std::string("!"));
	}
}

TEST(String, implode)
{
	const std::vector< std::string > words{"Hello", "bad", "world", "!"};

	ASSERT_EQ(String::implode(words), "Hellobadworld!");
	ASSERT_EQ(String::implode(words, ' '), "Hello bad world !");
	ASSERT_EQ(String::implode(words, 99.3F), "Hello99.3bad99.3world99.3!");
	ASSERT_EQ(String::implode(words, "---"), "Hello---bad---world---!");
}

TEST(String, caseChange)
{
	ASSERT_EQ(String::toUpper("DummySTR!"), "DUMMYSTR!");

	ASSERT_EQ(String::toLower("DummySTR!"), "dummystr!");

	ASSERT_EQ(String::ucfirst("titanic resurection !"), "Titanic resurection !");
	ASSERT_EQ(String::ucfirst("TEST"), "TEST");
	ASSERT_EQ(String::ucfirst(" wilson"), " wilson");
}

TEST(String, replace)
{
	const std::string source{"This is a huge test sentence, with a lot of tests to test if the test is correct !"};

	ASSERT_EQ(String::replace("test", "change", source), "This is a huge change sentence, with a lot of changes to change if the change is correct !");
	ASSERT_EQ(String::replace("lol", "XPtdr,", "lollollollol", 3), "XPtdr,XPtdr,XPtdr,lol");
}

TEST(String, removeChars)
{
	ASSERT_EQ(String::removeChars("Hellow world !", 'w'), "Hello orld !");
	ASSERT_EQ(String::removeChars("Abracadabra :)", "abc"), "Ardr :)");
}

TEST(String, removeFileExtension)
{
	ASSERT_EQ(String::removeFileExtension("sample.text"), "sample");
	ASSERT_EQ(String::removeFileExtension("/mydisk/tmp/test.mp3"), "/mydisk/tmp/test");
	ASSERT_EQ(String::removeFileExtension("/mydisk//tmp/my-file.INVALID"), "/mydisk//tmp/my-file");
}

TEST(String, extractFilename)
{
	ASSERT_EQ(String::extractFilename("sample.text"), "sample.text");
	ASSERT_EQ(String::extractFilename("/mydisk/tmp/test.mp3"), "test.mp3");
	ASSERT_EQ(String::extractFilename("/mydisk//tmp/my-file.INVALID"), "my-file.INVALID");
}

TEST(String, extractNumbers)
{
	ASSERT_EQ(String::extractNumbers(" 1: Hello 3 tims for 4 friend inside a 0.5 house ! 3"), "1 3 4 0.5 3");
}

TEST(String, extractTags)
{
	{
		const auto list = String::extractTags("This is a balized {NICE} string for {DYNAMIC} replacement !");

		ASSERT_EQ(list.size(), 2);
		ASSERT_EQ(list.at(0), "{NICE}");
		ASSERT_EQ(list.at(1), "{DYNAMIC}");
	}

	{
		const auto list = String::extractTags("My name is [NAME] and I live in [CITY]. I'm [YEARS]", {'[', ']'}, true);

		ASSERT_EQ(list.size(), 3);
		ASSERT_EQ(list.at(0), "NAME");
		ASSERT_EQ(list.at(1), "CITY");
		ASSERT_EQ(list.at(2), "YEARS");
	}
}

TEST(String, leftOrRight)
{
	const std::string source{"Check who is at the left or the right in the this string !"};

	ASSERT_EQ(String::left(source, "left or"), "Check who is at the ");

	ASSERT_EQ(String::right(source, "right in"), " the this string !");
}

#if __cplusplus < 202002L /* C++20 feature */
TEST(String, startsOrEndsWith)
{
	const std::string source{"Is the a number 365 in this sentence ?"};

	ASSERT_EQ(String::startsWith(source, "Is"), true);
	ASSERT_EQ(String::startsWith(source, "number 365"), false);

	ASSERT_EQ(String::endsWith(source, "sentence ?"), true);
	ASSERT_EQ(String::endsWith(source, "lol"), false);
}
#endif

#if __cplusplus < 202302L /* C++23 feature */
TEST(String, contains)
{
	const std::string source{"Is the a number 365 in this sentence ?"};

	ASSERT_EQ(String::contains(source, "365"), true);
	ASSERT_EQ(String::contains(source, "arzepuhf"), false);
}
#endif

TEST(String, unicodeToUTF8)
{
	ASSERT_EQ(String::unicodeToUTF8(1136), "Ѱ");
}

TEST(String, toNumber)
{
	/* Check 8-bit integers. */
	ASSERT_EQ(String::toNumber< char >("-128"), (char)-128);
	ASSERT_EQ(String::toNumber< char >("127"), (char)127);
	ASSERT_EQ(String::toNumber< unsigned char >("255"), (unsigned char)255);
	ASSERT_EQ(String::toNumber< int8_t >("-128"), (int8_t)-128);
	ASSERT_EQ(String::toNumber< int8_t >("127"), (int8_t)127);
	ASSERT_EQ(String::toNumber< uint8_t >("255"), (uint8_t)255);

	/* Check 16-bit integers. */
	ASSERT_EQ(String::toNumber< short int >("-32768"), (short int)-32768);
	ASSERT_EQ(String::toNumber< short int >("32767"), (short int)32767);
	ASSERT_EQ(String::toNumber< unsigned short int >("65535"), (unsigned short int)65535);
	ASSERT_EQ(String::toNumber< int16_t >("-32768"), (int16_t)-32768);
	ASSERT_EQ(String::toNumber< int16_t >("32767"), (int16_t)32767);
	ASSERT_EQ(String::toNumber< uint16_t >("65535"), (uint16_t)65535);

	/* Check 32-bit integers. */
	ASSERT_EQ(String::toNumber< int >("-2147483648"), -2147483648);
	ASSERT_EQ(String::toNumber< int >("2147483647"), 2147483647);
	ASSERT_EQ(String::toNumber< unsigned int >("4294967295"), 4294967295U);
	ASSERT_EQ(String::toNumber< int32_t >("-2147483648"), (int32_t)-2147483648);
	ASSERT_EQ(String::toNumber< int32_t >("2147483647"), (int32_t)2147483647);
	ASSERT_EQ(String::toNumber< uint32_t >("4294967295"), (uint32_t)4294967295);

	/* Check 64-bit integers. */
#if IS_WINDOWS
	// FIXME: Not real 64 number
	EXPECT_EQ(String::toNumber< long int >("-2147483648"), LONG_MIN);
	EXPECT_EQ(String::toNumber< long int >("2147483647"), LONG_MAX);
	EXPECT_EQ(String::toNumber< unsigned long int >("4294967295"), ULONG_MAX);
#else
	ASSERT_EQ(String::toNumber< long int >("-9223372036854775808"), -9223372036854775808L);
	ASSERT_EQ(String::toNumber< long int >("9223372036854775807"), 9223372036854775807L);
	ASSERT_EQ(String::toNumber< unsigned long int >("18446744073709551615"), 18446744073709551615UL);
#endif
	ASSERT_EQ(String::toNumber< int64_t >("-9223372036854775808"), (int64_t)-9223372036854775808);
	ASSERT_EQ(String::toNumber< int64_t >("9223372036854775807"), (int64_t)9223372036854775807);
	ASSERT_EQ(String::toNumber< uint64_t >("18446744073709551615"), (uint64_t)18446744073709551615);

	/* Check floating point numbers. */
	ASSERT_EQ(String::toNumber< float >("754.125"), 754.125F);
	ASSERT_EQ(String::toNumber< float >("-1847.057"), -1847.057F);
	ASSERT_EQ(String::toNumber< double >("755465465844.1564674968725"), 755465465844.1564674968725);
	ASSERT_EQ(String::toNumber< double >("-6546478.564185678746"), -6546478.564185678746);
	ASSERT_EQ(String::toNumber< long double >("7554654696849861895844.156467498916987678968725"), 7554654696849861895844.156467498916987678968725L);
	ASSERT_EQ(String::toNumber< long double >("-42.56418561798676658688764578127878746"), -42.56418561798676658688764578127878746L);
}

TEST(String, concat)
{
	ASSERT_EQ(String::concat("Year ", 2023), "Year 2023");
	ASSERT_EQ(String::concat("Result : ", 93.5F), "Result : 93.500000");
	ASSERT_EQ(String::concat("Hello", " world !"), "Hello world !");
}

TEST(String, to_string)
{
	ASSERT_EQ(String::to_string(127), "127");
	ASSERT_EQ(String::to_string(-985.25), "-985.250000");
	ASSERT_EQ(String::to_string(true), "true");
	ASSERT_EQ(String::to_string(false), "false");
}

TEST(String, floatVectorSerialization)
{
	Randomizer< int32_t > randomizer{0};

	const auto sourceData = randomizer.vector(20, -32000, 64000);

	const auto serialized = String::serializeVector(sourceData);

	ASSERT_EQ(serialized.empty(), false);

	const auto recoveredData = String::deserializeVector< int >(serialized);

	ASSERT_EQ(sourceData.size(), recoveredData.size());

	for ( size_t index = 0; index < sourceData.size(); ++index )
	{
		ASSERT_EQ(sourceData.at(index), recoveredData.at(index));
	}
}

TEST(String, doubleVectorSerialization)
{
	Randomizer< float > randomizer{0};

	const auto sourceData = randomizer.vector(20, -32000.0F, 64000.0F);

	const auto serialized = String::serializeVector(sourceData);

	ASSERT_EQ(serialized.empty(), false);

	const auto recoveredData = String::deserializeVector< float >(serialized);

	ASSERT_EQ(sourceData.size(), recoveredData.size());

	for ( size_t index = 0; index < sourceData.size(); ++index )
	{
		ASSERT_EQ(sourceData.at(index), recoveredData.at(index));
	}
}

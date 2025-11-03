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

using namespace EmEn::Libs;

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

TEST(String, trimEdgeCases)
{
	/* Empty string */
	ASSERT_EQ(String::trim(""), "");

	/* Only whitespace */
	ASSERT_EQ(String::trim("   \t\n  "), "");

	/* No whitespace */
	ASSERT_EQ(String::trim("NoSpaces"), "NoSpaces");

	/* Whitespace in middle only */
	ASSERT_EQ(String::trim("Hello World"), "Hello World");
}

TEST(String, padEdgeCases)
{
	/* Target length smaller than source */
	ASSERT_EQ(String::pad("LongString", 5, '-', String::Side::Right), "LongString");

	/* Target length equal to source */
	ASSERT_EQ(String::pad("Exact", 5, '-', String::Side::Left), "Exact");

	/* Empty string - Should fill entirely with padding */
	ASSERT_EQ(String::pad("", 5, '*', String::Side::Both), "*****");

	/* Single character padding on both sides */
	ASSERT_EQ(String::pad("X", 5, '-', String::Side::Both), "--X--");

	/* Odd padding distribution */
	ASSERT_EQ(String::pad("AB", 7, '*', String::Side::Both), "**AB***");
}

TEST(String, explodeEdgeCases)
{
	/* Delimiter not found */
	const auto result1 = String::explode("NoDelimiterHere", "XXX");
	ASSERT_EQ(result1.size(), 1);
	ASSERT_EQ(result1[0], "NoDelimiterHere");

	/* Empty string */
	const auto result2 = String::explode("", ",");
	ASSERT_EQ(result2.size(), 1);
	ASSERT_EQ(result2[0], "");

	/* Three dashes with -- delimiter */
	const auto result3 = String::explode("---", "--");
	ASSERT_EQ(result3.size(), 2);
	ASSERT_EQ(result3[0], "");
	ASSERT_EQ(result3[1], "-");

	/* Delimiter at start and end - final empty segment not included when delimiter is at end */
	const auto result4 = String::explode("--content--", "--");
	ASSERT_EQ(result4.size(), 2);
	ASSERT_EQ(result4[0], "");
	ASSERT_EQ(result4[1], "content");
}

TEST(String, implodeEdgeCases)
{
	/* Empty vector */
	const std::vector< std::string > empty;
	ASSERT_EQ(String::implode(empty), "");
	ASSERT_EQ(String::implode(empty, ' '), "");

	/* Single element */
	const std::vector< std::string > single{"Alone"};
	ASSERT_EQ(String::implode(single, ','), "Alone");

	/* Empty strings in vector - default behavior (keep empty) */
	const std::vector< std::string > withEmpty{"", "middle", ""};
	ASSERT_EQ(String::implode(withEmpty, '|'), "|middle|");

	/* Empty strings in vector - with ignoreEmpty = true */
	ASSERT_EQ(String::implode(withEmpty, '|', true), "middle");

	/* All empty strings - default behavior */
	const std::vector< std::string > allEmpty{"", "", ""};
	ASSERT_EQ(String::implode(allEmpty, '-'), "--");

	/* All empty strings - with ignoreEmpty = true */
	ASSERT_EQ(String::implode(allEmpty, '-', true), "");

	/* Mixed content with ignoreEmpty = true */
	const std::vector< std::string > mixed{"first", "", "second", "", "", "third"};
	ASSERT_EQ(String::implode(mixed, ' ', false), "first  second   third");
	ASSERT_EQ(String::implode(mixed, ' ', true), "first second third");
}

TEST(String, replaceEdgeCases)
{
	/* Pattern not found */
	ASSERT_EQ(String::replace("notfound", "new", "test string"), "test string");

	/* Count = 0 (replace all) */
	ASSERT_EQ(String::replace("a", "X", "aaa", 0), "XXX");

	/* Replace with empty string */
	ASSERT_EQ(String::replace("bad", "", "bad bad bad"), "  ");

	/* Empty source string */
	ASSERT_EQ(String::replace("any", "thing", ""), "");
}

TEST(String, removeCharsEdgeCases)
{
	/* Character not in string */
	ASSERT_EQ(String::removeChars("Hello", 'z'), "Hello");

	/* Empty string */
	ASSERT_EQ(String::removeChars("", 'a'), "");

	/* All characters removed */
	ASSERT_EQ(String::removeChars("aaaa", 'a'), "");

	/* Multiple character set removal */
	ASSERT_EQ(String::removeChars("Test123", "Test"), "123");
}

TEST(String, fileOperationsEdgeCases)
{
	/* Empty path */
	ASSERT_EQ(String::removeFileExtension(""), "");
	ASSERT_EQ(String::extractFilename(""), "");

	/* No extension */
	ASSERT_EQ(String::removeFileExtension("file_without_ext"), "file_without_ext");

	/* Multiple dots */
	ASSERT_EQ(String::removeFileExtension("archive.tar.gz"), "archive.tar");
	ASSERT_EQ(String::extractFilename("/path/to/archive.tar.gz"), "archive.tar.gz");

	/* Dot at start (hidden file) */
	ASSERT_EQ(String::extractFilename("/home/.hidden"), ".hidden");

	/* Only filename, no path */
	ASSERT_EQ(String::extractFilename("justfile.txt"), "justfile.txt");
}

TEST(String, extractNumbersEdgeCases)
{
	/* No numbers */
	ASSERT_EQ(String::extractNumbers("No digits here!"), "");

	/* Only numbers */
	ASSERT_EQ(String::extractNumbers("123 456 789"), "123 456 789");

	/* Negative numbers */
	ASSERT_EQ(String::extractNumbers("Temperature is -15.5 degrees"), "15.5");
}

TEST(String, extractTagsEdgeCases)
{
	/* No tags */
	const auto empty = String::extractTags("No tags in this string");
	ASSERT_EQ(empty.size(), 0);

	/* Nested tags */
	const auto nested = String::extractTags("Text {OUTER{INNER}} end");
	ASSERT_TRUE(nested.size() > 0);

	/* Unclosed tag */
	const auto unclosed = String::extractTags("Start {UNCLOSED string");
	ASSERT_EQ(unclosed.size(), 0);

	/* Adjacent tags */
	const auto adjacent = String::extractTags("{TAG1}{TAG2}{TAG3}");
	ASSERT_EQ(adjacent.size(), 3);
}

TEST(String, leftRightEdgeCases)
{
	const std::string source{"Sample text for testing"};

	/* Pattern not found */
	ASSERT_EQ(String::left(source, "NOTFOUND"), source);
	ASSERT_EQ(String::right(source, "NOTFOUND"), "");

	/* Pattern at start */
	ASSERT_EQ(String::left(source, "Sample"), "");

	/* Pattern at end */
	ASSERT_EQ(String::right(source, "testing"), "");

	/* Empty pattern */
	ASSERT_EQ(String::left(source, ""), source);
}

TEST(String, toNumberInvalid)
{
	/* Invalid integer string should return 0 or throw depending on implementation */
	ASSERT_EQ(String::toNumber< int >("not_a_number"), 0);
	ASSERT_EQ(String::toNumber< int >(""), 0);

	/* Overflow values */
	ASSERT_EQ(String::toNumber< int8_t >("999"), 0); // Would overflow int8_t

	/* Mixed content */
	ASSERT_EQ(String::toNumber< int >("123abc"), 123); // May parse partial
}

TEST(String, concatMultipleTypes)
{
	/* More than 2 arguments via chaining */
	const auto result1 = String::concat(String::concat("Count: ", 42), " items");
	ASSERT_EQ(result1, "Count: 42 items");

	/* Boolean values */
	ASSERT_EQ(String::concat("Result: ", true), "Result: 1");
	ASSERT_EQ(String::concat("Failed: ", false), "Failed: 0");

	/* Mixed precision floats */
	ASSERT_EQ(String::concat("Pi: ", 3.14159F).substr(0, 9), "Pi: 3.141");
}

TEST(String, vectorSerializationEdgeCases)
{
	/* Empty vector */
	const std::vector< int > emptyVec;
	const auto serializedEmpty = String::serializeVector(emptyVec);
	const auto recoveredEmpty = String::deserializeVector< int >(serializedEmpty);
	ASSERT_EQ(recoveredEmpty.size(), 0);

	/* Single element */
	const std::vector< int > singleVec{42};
	const auto serializedSingle = String::serializeVector(singleVec);
	const auto recoveredSingle = String::deserializeVector< int >(serializedSingle);
	ASSERT_EQ(recoveredSingle.size(), 1);
	ASSERT_EQ(recoveredSingle[0], 42);

	/* Negative and zero values */
	const std::vector< int > mixedVec{-100, 0, 100};
	const auto serializedMixed = String::serializeVector(mixedVec);
	const auto recoveredMixed = String::deserializeVector< int >(serializedMixed);
	ASSERT_EQ(recoveredMixed.size(), 3);
	ASSERT_EQ(recoveredMixed[0], -100);
	ASSERT_EQ(recoveredMixed[1], 0);
	ASSERT_EQ(recoveredMixed[2], 100);
}

TEST(String, ucfirstEdgeCases)
{
	/* Already uppercase */
	ASSERT_EQ(String::ucfirst("ALLCAPS"), "ALLCAPS");

	/* Single character */
	ASSERT_EQ(String::ucfirst("a"), "A");
	ASSERT_EQ(String::ucfirst("Z"), "Z");

	/* Empty string */
	ASSERT_EQ(String::ucfirst(""), "");

	/* Special characters at start */
	ASSERT_EQ(String::ucfirst("123test"), "123test");
	ASSERT_EQ(String::ucfirst("!hello"), "!hello");
}

TEST(String, unicodeToUTF8Range)
{
	/* ASCII range */
	ASSERT_EQ(String::unicodeToUTF8(65), "A");
	ASSERT_EQ(String::unicodeToUTF8(97), "a");

	/* Latin extended */
	ASSERT_EQ(String::unicodeToUTF8(233), "é");

	/* Emoji range (if supported) */
	ASSERT_EQ(String::unicodeToUTF8(128512).empty(), false); // 😀
}

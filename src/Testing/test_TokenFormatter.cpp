/*
 * src/Testing/test_TokenFormatter.cpp
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
#include "Libs/TokenFormatter.hpp"

using namespace EmEn::Libs;

/* ============================================================================
 * Detection Tests
 * ============================================================================ */

TEST(TokenFormatter, DetectCamelCase)
{
	EXPECT_EQ(TokenFormatter::detect("myVariableName"), CaseStyle::CamelCase);
	EXPECT_EQ(TokenFormatter::detect("someValue"), CaseStyle::CamelCase);
	EXPECT_EQ(TokenFormatter::detect("aBC"), CaseStyle::CamelCase);
}

TEST(TokenFormatter, DetectPascalCase)
{
	EXPECT_EQ(TokenFormatter::detect("MyVariableName"), CaseStyle::PascalCase);
	EXPECT_EQ(TokenFormatter::detect("SomeValue"), CaseStyle::PascalCase);
	EXPECT_EQ(TokenFormatter::detect("XMLParser"), CaseStyle::PascalCase);
}

TEST(TokenFormatter, DetectSnakeCase)
{
	EXPECT_EQ(TokenFormatter::detect("my_variable_name"), CaseStyle::SnakeCase);
	EXPECT_EQ(TokenFormatter::detect("some_value"), CaseStyle::SnakeCase);
	EXPECT_EQ(TokenFormatter::detect("a_b_c"), CaseStyle::SnakeCase);
}

TEST(TokenFormatter, DetectScreamingSnake)
{
	EXPECT_EQ(TokenFormatter::detect("MY_VARIABLE_NAME"), CaseStyle::ScreamingSnake);
	EXPECT_EQ(TokenFormatter::detect("SOME_VALUE"), CaseStyle::ScreamingSnake);
	EXPECT_EQ(TokenFormatter::detect("MAX_SIZE"), CaseStyle::ScreamingSnake);
}

TEST(TokenFormatter, DetectKebabCase)
{
	EXPECT_EQ(TokenFormatter::detect("my-variable-name"), CaseStyle::KebabCase);
	EXPECT_EQ(TokenFormatter::detect("some-value"), CaseStyle::KebabCase);
	EXPECT_EQ(TokenFormatter::detect("a-b-c"), CaseStyle::KebabCase);
}

TEST(TokenFormatter, DetectTrainCase)
{
	EXPECT_EQ(TokenFormatter::detect("MY-VARIABLE-NAME"), CaseStyle::TrainCase);
	EXPECT_EQ(TokenFormatter::detect("SOME-VALUE"), CaseStyle::TrainCase);
}

TEST(TokenFormatter, DetectFlatCase)
{
	EXPECT_EQ(TokenFormatter::detect("myvariablename"), CaseStyle::FlatCase);
	EXPECT_EQ(TokenFormatter::detect("somevalue"), CaseStyle::FlatCase);
}

TEST(TokenFormatter, DetectUpperFlatCase)
{
	EXPECT_EQ(TokenFormatter::detect("MYVARIABLENAME"), CaseStyle::UpperFlatCase);
	EXPECT_EQ(TokenFormatter::detect("SOMEVALUE"), CaseStyle::UpperFlatCase);
}

TEST(TokenFormatter, DetectLowerSpaced)
{
	EXPECT_EQ(TokenFormatter::detect("my variable name"), CaseStyle::LowerSpaced);
	EXPECT_EQ(TokenFormatter::detect("some value"), CaseStyle::LowerSpaced);
}

TEST(TokenFormatter, DetectUpperSpaced)
{
	EXPECT_EQ(TokenFormatter::detect("MY VARIABLE NAME"), CaseStyle::UpperSpaced);
	EXPECT_EQ(TokenFormatter::detect("SOME VALUE"), CaseStyle::UpperSpaced);
}

TEST(TokenFormatter, DetectTitleCase)
{
	EXPECT_EQ(TokenFormatter::detect("My Variable Name"), CaseStyle::TitleCase);
	EXPECT_EQ(TokenFormatter::detect("Some Value"), CaseStyle::TitleCase);
}

TEST(TokenFormatter, DetectEmpty)
{
	EXPECT_EQ(TokenFormatter::detect(""), CaseStyle::Unknown);
}

/* ============================================================================
 * Conversion Tests - From camelCase
 * ============================================================================ */

TEST(TokenFormatter, FromCamelCaseToAll)
{
	const std::string source = "myVariableName";

	EXPECT_EQ(TokenFormatter::toCamelCase(source), "myVariableName");
	EXPECT_EQ(TokenFormatter::toPascalCase(source), "MyVariableName");
	EXPECT_EQ(TokenFormatter::toSnakeCase(source), "my_variable_name");
	EXPECT_EQ(TokenFormatter::toScreamingSnake(source), "MY_VARIABLE_NAME");
	EXPECT_EQ(TokenFormatter::toKebabCase(source), "my-variable-name");
	EXPECT_EQ(TokenFormatter::toTrainCase(source), "MY-VARIABLE-NAME");
	EXPECT_EQ(TokenFormatter::toFlatCase(source), "myvariablename");
	EXPECT_EQ(TokenFormatter::toUpperFlatCase(source), "MYVARIABLENAME");
	EXPECT_EQ(TokenFormatter::toLowerSpaced(source), "my variable name");
	EXPECT_EQ(TokenFormatter::toUpperSpaced(source), "MY VARIABLE NAME");
	EXPECT_EQ(TokenFormatter::toTitleCase(source), "My Variable Name");
}

/* ============================================================================
 * Conversion Tests - From snake_case
 * ============================================================================ */

TEST(TokenFormatter, FromSnakeCaseToAll)
{
	const std::string source = "my_variable_name";

	EXPECT_EQ(TokenFormatter::toCamelCase(source), "myVariableName");
	EXPECT_EQ(TokenFormatter::toPascalCase(source), "MyVariableName");
	EXPECT_EQ(TokenFormatter::toSnakeCase(source), "my_variable_name");
	EXPECT_EQ(TokenFormatter::toScreamingSnake(source), "MY_VARIABLE_NAME");
	EXPECT_EQ(TokenFormatter::toKebabCase(source), "my-variable-name");
	EXPECT_EQ(TokenFormatter::toTrainCase(source), "MY-VARIABLE-NAME");
	EXPECT_EQ(TokenFormatter::toFlatCase(source), "myvariablename");
	EXPECT_EQ(TokenFormatter::toUpperFlatCase(source), "MYVARIABLENAME");
	EXPECT_EQ(TokenFormatter::toLowerSpaced(source), "my variable name");
	EXPECT_EQ(TokenFormatter::toUpperSpaced(source), "MY VARIABLE NAME");
	EXPECT_EQ(TokenFormatter::toTitleCase(source), "My Variable Name");
}

/* ============================================================================
 * Conversion Tests - From PascalCase
 * ============================================================================ */

TEST(TokenFormatter, FromPascalCaseToAll)
{
	const std::string source = "MyVariableName";

	EXPECT_EQ(TokenFormatter::toCamelCase(source), "myVariableName");
	EXPECT_EQ(TokenFormatter::toPascalCase(source), "MyVariableName");
	EXPECT_EQ(TokenFormatter::toSnakeCase(source), "my_variable_name");
	EXPECT_EQ(TokenFormatter::toScreamingSnake(source), "MY_VARIABLE_NAME");
	EXPECT_EQ(TokenFormatter::toKebabCase(source), "my-variable-name");
	EXPECT_EQ(TokenFormatter::toTrainCase(source), "MY-VARIABLE-NAME");
	EXPECT_EQ(TokenFormatter::toFlatCase(source), "myvariablename");
	EXPECT_EQ(TokenFormatter::toUpperFlatCase(source), "MYVARIABLENAME");
	EXPECT_EQ(TokenFormatter::toLowerSpaced(source), "my variable name");
	EXPECT_EQ(TokenFormatter::toUpperSpaced(source), "MY VARIABLE NAME");
	EXPECT_EQ(TokenFormatter::toTitleCase(source), "My Variable Name");
}

/* ============================================================================
 * Conversion Tests - From kebab-case
 * ============================================================================ */

TEST(TokenFormatter, FromKebabCaseToAll)
{
	const std::string source = "my-variable-name";

	EXPECT_EQ(TokenFormatter::toCamelCase(source), "myVariableName");
	EXPECT_EQ(TokenFormatter::toPascalCase(source), "MyVariableName");
	EXPECT_EQ(TokenFormatter::toSnakeCase(source), "my_variable_name");
	EXPECT_EQ(TokenFormatter::toScreamingSnake(source), "MY_VARIABLE_NAME");
	EXPECT_EQ(TokenFormatter::toKebabCase(source), "my-variable-name");
	EXPECT_EQ(TokenFormatter::toTrainCase(source), "MY-VARIABLE-NAME");
	EXPECT_EQ(TokenFormatter::toFlatCase(source), "myvariablename");
	EXPECT_EQ(TokenFormatter::toUpperFlatCase(source), "MYVARIABLENAME");
	EXPECT_EQ(TokenFormatter::toLowerSpaced(source), "my variable name");
	EXPECT_EQ(TokenFormatter::toUpperSpaced(source), "MY VARIABLE NAME");
	EXPECT_EQ(TokenFormatter::toTitleCase(source), "My Variable Name");
}

/* ============================================================================
 * Conversion Tests - From spaced formats
 * ============================================================================ */

TEST(TokenFormatter, FromLowerSpacedToAll)
{
	const std::string source = "my variable name";

	EXPECT_EQ(TokenFormatter::toCamelCase(source), "myVariableName");
	EXPECT_EQ(TokenFormatter::toPascalCase(source), "MyVariableName");
	EXPECT_EQ(TokenFormatter::toSnakeCase(source), "my_variable_name");
	EXPECT_EQ(TokenFormatter::toScreamingSnake(source), "MY_VARIABLE_NAME");
	EXPECT_EQ(TokenFormatter::toKebabCase(source), "my-variable-name");
	EXPECT_EQ(TokenFormatter::toFlatCase(source), "myvariablename");
	EXPECT_EQ(TokenFormatter::toUpperFlatCase(source), "MYVARIABLENAME");
	EXPECT_EQ(TokenFormatter::toLowerSpaced(source), "my variable name");
	EXPECT_EQ(TokenFormatter::toUpperSpaced(source), "MY VARIABLE NAME");
	EXPECT_EQ(TokenFormatter::toTitleCase(source), "My Variable Name");
}

TEST(TokenFormatter, FromTitleCaseToAll)
{
	const std::string source = "My Variable Name";

	EXPECT_EQ(TokenFormatter::toCamelCase(source), "myVariableName");
	EXPECT_EQ(TokenFormatter::toPascalCase(source), "MyVariableName");
	EXPECT_EQ(TokenFormatter::toSnakeCase(source), "my_variable_name");
	EXPECT_EQ(TokenFormatter::toScreamingSnake(source), "MY_VARIABLE_NAME");
	EXPECT_EQ(TokenFormatter::toKebabCase(source), "my-variable-name");
	EXPECT_EQ(TokenFormatter::toFlatCase(source), "myvariablename");
	EXPECT_EQ(TokenFormatter::toUpperFlatCase(source), "MYVARIABLENAME");
	EXPECT_EQ(TokenFormatter::toLowerSpaced(source), "my variable name");
	EXPECT_EQ(TokenFormatter::toUpperSpaced(source), "MY VARIABLE NAME");
	EXPECT_EQ(TokenFormatter::toTitleCase(source), "My Variable Name");
}

/* ============================================================================
 * Instance Method Tests
 * ============================================================================ */

TEST(TokenFormatter, InstanceMethods)
{
	TokenFormatter fmt("myVariableName");

	EXPECT_EQ(fmt.detectedStyle(), CaseStyle::CamelCase);
	EXPECT_EQ(fmt.wordCount(), 3U);
	EXPECT_FALSE(fmt.empty());

	EXPECT_EQ(fmt.toCamelCase(), "myVariableName");
	EXPECT_EQ(fmt.toPascalCase(), "MyVariableName");
	EXPECT_EQ(fmt.toSnakeCase(), "my_variable_name");
	EXPECT_EQ(fmt.toKebabCase(), "my-variable-name");
}

TEST(TokenFormatter, WordExtraction)
{
	TokenFormatter fmt("myVariableName");

	EXPECT_EQ(fmt.wordCount(), 3U);

	const auto & words = fmt.words();
	EXPECT_EQ(words[0], "my");
	EXPECT_EQ(words[1], "Variable");
	EXPECT_EQ(words[2], "Name");
}

TEST(TokenFormatter, WordExtractionSnakeCase)
{
	TokenFormatter fmt("my_variable_name");

	EXPECT_EQ(fmt.wordCount(), 3U);

	const auto & words = fmt.words();
	EXPECT_EQ(words[0], "my");
	EXPECT_EQ(words[1], "variable");
	EXPECT_EQ(words[2], "name");
}

/* ============================================================================
 * Acronym Handling Tests
 * ============================================================================ */

TEST(TokenFormatter, AcronymHandling)
{
	/* XMLParser should become xml_parser, not x_m_l_parser */
	EXPECT_EQ(TokenFormatter::toSnakeCase("XMLParser"), "xml_parser");
	EXPECT_EQ(TokenFormatter::toCamelCase("XMLParser"), "xmlParser");

	/* HTTPRequest */
	EXPECT_EQ(TokenFormatter::toSnakeCase("HTTPRequest"), "http_request");
	EXPECT_EQ(TokenFormatter::toKebabCase("HTTPRequest"), "http-request");

	/* getHTTPResponse */
	EXPECT_EQ(TokenFormatter::toSnakeCase("getHTTPResponse"), "get_http_response");
	EXPECT_EQ(TokenFormatter::toPascalCase("getHTTPResponse"), "GetHttpResponse");
}

/* ============================================================================
 * Edge Cases Tests
 * ============================================================================ */

TEST(TokenFormatter, EmptyString)
{
	TokenFormatter fmt("");

	EXPECT_TRUE(fmt.empty());
	EXPECT_EQ(fmt.wordCount(), 0U);
	EXPECT_EQ(fmt.detectedStyle(), CaseStyle::Unknown);
	EXPECT_EQ(fmt.toCamelCase(), "");
	EXPECT_EQ(fmt.toSnakeCase(), "");
}

TEST(TokenFormatter, SingleWord)
{
	TokenFormatter fmt("hello");

	EXPECT_EQ(fmt.wordCount(), 1U);
	EXPECT_EQ(fmt.toCamelCase(), "hello");
	EXPECT_EQ(fmt.toPascalCase(), "Hello");
	EXPECT_EQ(fmt.toSnakeCase(), "hello");
	EXPECT_EQ(fmt.toScreamingSnake(), "HELLO");
}

TEST(TokenFormatter, SingleUpperWord)
{
	TokenFormatter fmt("HELLO");

	EXPECT_EQ(fmt.wordCount(), 1U);
	EXPECT_EQ(fmt.toCamelCase(), "hello");
	EXPECT_EQ(fmt.toPascalCase(), "Hello");
	EXPECT_EQ(fmt.toSnakeCase(), "hello");
}

TEST(TokenFormatter, ConsecutiveSeparators)
{
	/* Multiple underscores should be treated as single separator */
	TokenFormatter fmt("my__variable___name");

	EXPECT_EQ(fmt.wordCount(), 3U);
	EXPECT_EQ(fmt.toSnakeCase(), "my_variable_name");
}

TEST(TokenFormatter, LeadingTrailingSeparators)
{
	TokenFormatter fmt("_my_variable_name_");

	EXPECT_EQ(fmt.wordCount(), 3U);
	EXPECT_EQ(fmt.toSnakeCase(), "my_variable_name");
}

/* ============================================================================
 * Generic Convert Method Tests
 * ============================================================================ */

TEST(TokenFormatter, ConvertMethod)
{
	const std::string source = "myVariableName";

	EXPECT_EQ(TokenFormatter::convert(source, CaseStyle::CamelCase), "myVariableName");
	EXPECT_EQ(TokenFormatter::convert(source, CaseStyle::PascalCase), "MyVariableName");
	EXPECT_EQ(TokenFormatter::convert(source, CaseStyle::SnakeCase), "my_variable_name");
	EXPECT_EQ(TokenFormatter::convert(source, CaseStyle::ScreamingSnake), "MY_VARIABLE_NAME");
	EXPECT_EQ(TokenFormatter::convert(source, CaseStyle::KebabCase), "my-variable-name");
	EXPECT_EQ(TokenFormatter::convert(source, CaseStyle::TrainCase), "MY-VARIABLE-NAME");
	EXPECT_EQ(TokenFormatter::convert(source, CaseStyle::FlatCase), "myvariablename");
	EXPECT_EQ(TokenFormatter::convert(source, CaseStyle::UpperFlatCase), "MYVARIABLENAME");
	EXPECT_EQ(TokenFormatter::convert(source, CaseStyle::LowerSpaced), "my variable name");
	EXPECT_EQ(TokenFormatter::convert(source, CaseStyle::UpperSpaced), "MY VARIABLE NAME");
	EXPECT_EQ(TokenFormatter::convert(source, CaseStyle::TitleCase), "My Variable Name");
}

TEST(TokenFormatter, ConvertUnknownReturnsOriginal)
{
	TokenFormatter fmt("myVariableName");

	/* Unknown style should return concatenated words */
	EXPECT_EQ(fmt.to(CaseStyle::Unknown), "myVariableName");
}

/* ============================================================================
 * StyleName Tests
 * ============================================================================ */

TEST(TokenFormatter, StyleName)
{
	EXPECT_EQ(TokenFormatter::styleName(CaseStyle::CamelCase), "camelCase");
	EXPECT_EQ(TokenFormatter::styleName(CaseStyle::PascalCase), "PascalCase");
	EXPECT_EQ(TokenFormatter::styleName(CaseStyle::SnakeCase), "snake_case");
	EXPECT_EQ(TokenFormatter::styleName(CaseStyle::ScreamingSnake), "SCREAMING_SNAKE_CASE");
	EXPECT_EQ(TokenFormatter::styleName(CaseStyle::KebabCase), "kebab-case");
	EXPECT_EQ(TokenFormatter::styleName(CaseStyle::TrainCase), "TRAIN-CASE");
	EXPECT_EQ(TokenFormatter::styleName(CaseStyle::FlatCase), "flatcase");
	EXPECT_EQ(TokenFormatter::styleName(CaseStyle::UpperFlatCase), "UPPERFLATCASE");
	EXPECT_EQ(TokenFormatter::styleName(CaseStyle::LowerSpaced), "lower spaced");
	EXPECT_EQ(TokenFormatter::styleName(CaseStyle::UpperSpaced), "UPPER SPACED");
	EXPECT_EQ(TokenFormatter::styleName(CaseStyle::TitleCase), "Title Case");
	EXPECT_EQ(TokenFormatter::styleName(CaseStyle::Unknown), "Unknown");
}

/* ============================================================================
 * TotalWordLength Tests
 * ============================================================================ */

TEST(TokenFormatter, TotalWordLength)
{
	TokenFormatter fmt("myVariableName");

	/* "my" (2) + "Variable" (8) + "Name" (4) = 14 */
	EXPECT_EQ(fmt.totalWordLength(), 14U);
}

/* ============================================================================
 * Real-World Examples
 * ============================================================================ */

TEST(TokenFormatter, RealWorldExamples)
{
	/* Class names */
	EXPECT_EQ(TokenFormatter::toSnakeCase("ResourceManager"), "resource_manager");
	EXPECT_EQ(TokenFormatter::toKebabCase("AudioBuffer"), "audio-buffer");

	/* Function names */
	EXPECT_EQ(TokenFormatter::toPascalCase("get_user_name"), "GetUserName");
	EXPECT_EQ(TokenFormatter::toCamelCase("SET_VALUE"), "setValue");

	/* Constants */
	EXPECT_EQ(TokenFormatter::toScreamingSnake("maxBufferSize"), "MAX_BUFFER_SIZE");

	/* CSS class to C++ */
	EXPECT_EQ(TokenFormatter::toPascalCase("btn-primary-large"), "BtnPrimaryLarge");

	/* Database column to display */
	EXPECT_EQ(TokenFormatter::toTitleCase("first_name"), "First Name");
	EXPECT_EQ(TokenFormatter::toTitleCase("created_at"), "Created At");
}

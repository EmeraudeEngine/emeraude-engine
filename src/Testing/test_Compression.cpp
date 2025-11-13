/*
 * src/Testing/test_Compression.cpp
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

/* STL inclusions. */
#include <cstring>

/* Local inclusions. */
#include "Libs/Compression/LZMA.hpp"
#include "Libs/Compression/LZMA/Compressor.hpp"
#include "Libs/Compression/LZMA/Decompressor.hpp"
#include "Libs/Compression/ZLIB.hpp"
#include "Libs/Randomizer.hpp"
#include "Libs/Time/Elapsed/PrintScopeRealTime.hpp"
#include "Libs/Utility.hpp"

using namespace EmEn::Libs;
using namespace EmEn::Libs::Compression;
using namespace EmEn::Libs::Time::Elapsed;

[[nodiscard]]
std::string
createSource (size_t size) noexcept
{
	std::string source;

	Randomizer< float > randomizer;

	const auto coordinates = randomizer.vector(512 * 3 * size, -32000.0F, 32000.0F);

	const auto bytes = coordinates.size() * sizeof(float);

	source.resize(bytes);

	std::memcpy(source.data(), coordinates.data(), bytes);

	return source;
}

TEST(Compression, ZLIBString)
{
	const auto source = createSource(2048UL);

	std::string compressed;

	{
		PrintScopeRealTime stat{"ZLIB::compressString()"};

		ASSERT_TRUE(ZLIB::compressString(source, compressed));
	}

	std::string recovered;

	{
		PrintScopeRealTime stat{"ZLIB::decompressString()"};

		ASSERT_TRUE(ZLIB::decompressString(compressed, recovered));
	}

	ASSERT_EQ(source, recovered);
}

TEST(Compression, LZMAStringBasic)
{
	const auto source = createSource(2048UL);

	std::string compressed;

	{
		PrintScopeRealTime stat{"LZMA::compressString()"};

		ASSERT_TRUE(LZMA::compressString(source, compressed));
	}

	std::string recovered;

	{
		PrintScopeRealTime stat{"LZMA::decompressString()"};

		ASSERT_TRUE(LZMA::decompressString(compressed, recovered));
	}

	ASSERT_EQ(source, recovered);
}

TEST(Compression, LZMAStringMT)
{
	const auto source = createSource(2048UL);

	std::string compressed;

	{
		PrintScopeRealTime stat{"LZMA::Compressor::compressString() [MT:8]"};

		LZMA::Compressor proc{8, 9};
		ASSERT_TRUE(proc.compressString(source, compressed));
	}

	std::string recovered;

	{
		PrintScopeRealTime stat{"LZMA::Decompressor::decompressString() [MT:8]"};

		LZMA::Decompressor proc{8};
		ASSERT_TRUE(proc.decompressString(compressed, recovered));
	}

	ASSERT_EQ(source, recovered);
}

TEST(Compression, LZMAStringMTDisabled)
{
	const auto source = createSource(2048UL);

	std::string compressed;

	{
		PrintScopeRealTime stat{"LZMA::Compressor::compressString() [MT:0]"};

		LZMA::Compressor proc{0, 9};

		ASSERT_TRUE(proc.compressString(source, compressed));
	}

	std::string recovered;

	{
		PrintScopeRealTime stat{"LZMA::Decompressor::decompressString() [MT:0]"};

		LZMA::Decompressor proc{0};

		ASSERT_TRUE(proc.decompressString(compressed, recovered));
	}

	ASSERT_EQ(source, recovered);
}

/*
 * src/Testing/test_PixelFactoryPixmapFormat.cpp
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
#include "Libs/Time/Elapsed/PrintScopeRealTime.hpp"
#include "Libs/PixelFactory/FileIO.hpp"
#include "Libs/PixelFactory/FileFormatTarga.hpp"
#include "Constants.hpp"

using namespace EmEn::Libs;
using namespace EmEn::Libs::PixelFactory;
using namespace EmEn::Libs::Time::Elapsed;

TEST(PixelFactoryPixmapFormat, readTarga)
{
	Pixmap< uint8_t > image;

	ASSERT_TRUE(FileIO::read(FixedFont, image));

	ASSERT_EQ(image.width(), 512);
	ASSERT_EQ(image.height(), 512);
	ASSERT_EQ(image.colorCount(), 1);
}

TEST(PixelFactoryPixmapFormat, writeTarga)
{
	Pixmap< uint8_t > image;

	ASSERT_TRUE(FileIO::read(LargeRGB, image));

	ASSERT_TRUE(FileIO::write(image, {"./test-assets/tmp_writeTarga.tga"}, true));
}

TEST(PixelFactoryPixmapFormat, readTargaRLE)
{
	Pixmap< uint8_t > image;

	ASSERT_TRUE(FileIO::read(LargeRGB_RLE_Targa, image));

	ASSERT_EQ(image.width(), 1700);
	ASSERT_EQ(image.height(), 1280);
	ASSERT_EQ(image.colorCount(), 3);
}

TEST(PixelFactoryPixmapFormat, writeTargaWithRLE)
{
	/* Read a source image */
	Pixmap< uint8_t > sourceImage;
	ASSERT_TRUE(FileIO::read(MediumRGB, sourceImage));

	/* Write with RLE compression using FileFormatTarga directly */
	FileFormatTarga< uint8_t > targaWriter;
	targaWriter.setRLECompression(true);

	ASSERT_TRUE(targaWriter.writeFile("./test-assets/tmp_writeTargaRLE.tga", sourceImage));

	/* Verify we can read it back */
	Pixmap< uint8_t > readBackImage;
	ASSERT_TRUE(FileIO::read({"./test-assets/tmp_writeTargaRLE.tga"}, readBackImage));

	ASSERT_EQ(readBackImage.width(), sourceImage.width());
	ASSERT_EQ(readBackImage.height(), sourceImage.height());
	ASSERT_EQ(readBackImage.colorCount(), sourceImage.colorCount());
}

TEST(PixelFactoryPixmapFormat, writeTargaWithoutRLE)
{
	/* Read a source image */
	Pixmap< uint8_t > sourceImage;
	ASSERT_TRUE(FileIO::read(MediumRGB, sourceImage));

	/* Write without RLE compression using FileFormatTarga directly */
	FileFormatTarga< uint8_t > targaWriter;
	targaWriter.setRLECompression(false);

	ASSERT_TRUE(targaWriter.writeFile("./test-assets/tmp_writeTargaNoRLE.tga", sourceImage));

	/* Verify we can read it back */
	Pixmap< uint8_t > readBackImage;
	ASSERT_TRUE(FileIO::read({"./test-assets/tmp_writeTargaNoRLE.tga"}, readBackImage));

	ASSERT_EQ(readBackImage.width(), sourceImage.width());
	ASSERT_EQ(readBackImage.height(), sourceImage.height());
	ASSERT_EQ(readBackImage.colorCount(), sourceImage.colorCount());
}

TEST(PixelFactoryPixmapFormat, targaRLERoundTrip)
{
	/* Read original RLE-compressed TARGA */
	Pixmap< uint8_t > originalImage;
	ASSERT_TRUE(FileIO::read(LargeRGB_RLE_Targa, originalImage));

	/* Write it back with RLE */
	FileFormatTarga< uint8_t > targaWriter;
	targaWriter.setRLECompression(true);
	ASSERT_TRUE(targaWriter.writeFile("./test-assets/tmp_targaRoundTrip.tga", originalImage));

	/* Read it back again */
	Pixmap< uint8_t > roundTripImage;
	ASSERT_TRUE(FileIO::read({"./test-assets/tmp_targaRoundTrip.tga"}, roundTripImage));

	/* Verify dimensions match */
	ASSERT_EQ(roundTripImage.width(), originalImage.width());
	ASSERT_EQ(roundTripImage.height(), originalImage.height());
	ASSERT_EQ(roundTripImage.colorCount(), originalImage.colorCount());

	/* Verify pixel data matches */
	const auto & origData = originalImage.data();
	const auto & rtData = roundTripImage.data();
	ASSERT_EQ(origData.size(), rtData.size());

	for ( size_t i = 0; i < origData.size(); ++i )
	{
		ASSERT_EQ(origData[i], rtData[i]) << "Pixel mismatch at index " << i;
	}
}

TEST(PixelFactoryPixmapFormat, targaGrayscaleRLE)
{
	/* Read a grayscale image */
	Pixmap< uint8_t > grayscaleImage;
	ASSERT_TRUE(FileIO::read(MediumGrayscale, grayscaleImage));

	/* Write as TARGA with RLE */
	FileFormatTarga< uint8_t > targaWriter;
	targaWriter.setRLECompression(true);
	ASSERT_TRUE(targaWriter.writeFile("./test-assets/tmp_grayscaleRLE.tga", grayscaleImage));

	/* Read it back */
	Pixmap< uint8_t > readBackImage;
	ASSERT_TRUE(FileIO::read({"./test-assets/tmp_grayscaleRLE.tga"}, readBackImage));

	ASSERT_EQ(readBackImage.width(), grayscaleImage.width());
	ASSERT_EQ(readBackImage.height(), grayscaleImage.height());
	ASSERT_EQ(readBackImage.colorCount(), 1);
}

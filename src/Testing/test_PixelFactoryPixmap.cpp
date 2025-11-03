/*
 * src/Testing/test_PixelFactoryPixmap.cpp
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
#include "Libs/Time/Elapsed/PrintScopeRealTime.hpp"
#include "Libs/PixelFactory/FileIO.hpp"
#include "Libs/PixelFactory/Pixmap.hpp"
#include "Libs/PixelFactory/Gradient.hpp"
#include "Constants.hpp"

using namespace EmEn::Libs;
using namespace EmEn::Libs::PixelFactory;
using namespace EmEn::Libs::Time::Elapsed;

TEST(PixelFactoryPixmap, constructor)
{
	PrintScopeRealTime stat{"Pixmap::Pixmap(1024, 768, ChannelMode::RGBA)"};

	Pixmap< uint8_t > image{1024, 768, ChannelMode::RGBA};

	ASSERT_EQ(image.width(), 1024);
	ASSERT_EQ(image.height(), 768);
	ASSERT_EQ(image.colorCount(), 4);
	ASSERT_TRUE(image.isValid());

	ASSERT_EQ(image.pixel(32, 32), Black);
}

TEST(PixelFactoryPixmap, readPNG_Palette)
{
	PrintScopeRealTime stat{"Pixmap::read(PNG_Palette)"};

	Pixmap< uint8_t > image;

	ASSERT_TRUE(FileIO::read(MediumPalette, image));

	ASSERT_EQ(image.width(), 512);
	ASSERT_EQ(image.height(), 512);
	ASSERT_EQ(image.colorCount(), 3);

	ASSERT_TRUE(FileIO::write(image, {"./test-assets/tmp_512x512-Palette.png"}, true));
}

TEST(PixelFactoryPixmap, readPNG_Grayscale)
{
	PrintScopeRealTime stat{"Pixmap::read(PNG_Grayscale)"};

	Pixmap< uint8_t > image;

	ASSERT_TRUE(FileIO::read(MediumGrayscale, image));

	ASSERT_EQ(image.width(), 512);
	ASSERT_EQ(image.height(), 512);
	ASSERT_EQ(image.colorCount(), 1);

	ASSERT_TRUE(FileIO::write(image, {"./test-assets/tmp_512x512-Grayscale.png"}, true));
}

TEST(PixelFactoryPixmap, readPNG_GrayscaleAlpha)
{
	PrintScopeRealTime stat{"Pixmap::read(PNG_GrayscaleAlpha)"};

	Pixmap< uint8_t > image;

	ASSERT_TRUE(FileIO::read(MediumGrayscaleAlpha, image));

	ASSERT_EQ(image.width(), 512);
	ASSERT_EQ(image.height(), 512);
	ASSERT_EQ(image.colorCount(), 2);

	ASSERT_TRUE(FileIO::write(image, {"./test-assets/tmp_512x512-GrayscaleAlpha.png"}, true));
}

TEST(PixelFactoryPixmap, readPNG_RGB)
{
	PrintScopeRealTime stat{"Pixmap::read(PNG_RGB)"};

	Pixmap< uint8_t > image;

	ASSERT_TRUE(FileIO::read(MediumRGB, image));

	ASSERT_EQ(image.width(), 512);
	ASSERT_EQ(image.height(), 512);
	ASSERT_EQ(image.colorCount(), 3);

	ASSERT_TRUE(FileIO::write(image, {"./test-assets/tmp_512x512-RGB.png"}, true));
}

TEST(PixelFactoryPixmap, readPNG_RGBA)
{
	PrintScopeRealTime stat{"Pixmap::read(PNG_RGBA)"};

	Pixmap< uint8_t > image;

	ASSERT_TRUE(FileIO::read(MediumRGBA, image));

	ASSERT_EQ(image.width(), 512);
	ASSERT_EQ(image.height(), 512);
	ASSERT_EQ(image.colorCount(), 4);

	ASSERT_TRUE(FileIO::write(image, {"./test-assets/tmp_512x512-RGBA.png"}, true));
}

TEST(PixelFactoryPixmap, fillValue)
{
	Pixmap< uint8_t > image{340, 120, ChannelMode::RGBA};

	{
		PrintScopeRealTime stat{"Pixmap::fill(127)"};

		ASSERT_TRUE(image.fill(127));
	}

	ASSERT_EQ(image.pixel(32, 32), ColorFromInteger< uint8_t >(127, 127, 127, 255));

	ASSERT_TRUE(FileIO::write(image, {"./test-assets/tmp_fillValue.png"}, true));
}

TEST(PixelFactoryPixmap, fillSequence)
{
	Randomizer< uint8_t > randomizer;

	const auto random = randomizer.vector(100, 0, 255);

	Pixmap< uint8_t > image{340, 120, ChannelMode::RGBA};

	{
		PrintScopeRealTime stat{"Pixmap::fill(sequence)"};

		ASSERT_TRUE(image.fill(random));
	}

	ASSERT_TRUE(FileIO::write(image, {"./test-assets/tmp_fillSequence.png"}, true));
}

TEST(PixelFactoryPixmap, fillColor)
{
	const auto color = ColorFromInteger< uint8_t >(127, 255, 255, 255);

	Pixmap< uint8_t > image{340, 120, ChannelMode::RGBA};

	{
		PrintScopeRealTime stat{"Pixmap::fill(color)"};

		ASSERT_TRUE(image.fill(color));
	}

	ASSERT_EQ(image.pixel(32, 32), color);

	ASSERT_TRUE(FileIO::write(image, {"./test-assets/tmp_fillColor.png"}, true));
}

TEST(PixelFactoryPixmap, fillPattern)
{
	Pixmap< uint8_t > pattern;

	ASSERT_TRUE(FileIO::read(SmallRGBA, pattern));
	ASSERT_EQ(pattern.width(), 64);
	ASSERT_EQ(pattern.height(), 64);
	ASSERT_EQ(pattern.colorCount(), 4);

	Pixmap< uint8_t > image{340, 120, ChannelMode::RGBA};

	{
		PrintScopeRealTime stat{"Pixmap::fill(pattern)"};

		ASSERT_TRUE(image.fill(pattern));
	}

	ASSERT_TRUE(FileIO::write(image, {"./test-assets/tmp_fillPattern.png"}, true));
}

TEST(PixelFactoryPixmap, fillHorizontalGradient)
{
	Pixmap< uint8_t > image{340, 120, ChannelMode::RGBA};

	{
		PrintScopeRealTime stat{"Pixmap::fillHorizontal(gradient2)"};

		ASSERT_TRUE(image.fillHorizontal({Red, Green}));
	}

	ASSERT_TRUE(FileIO::write(image, {"./test-assets/tmp_fillHorizontalGradient2.png"}, true));

	{
		PrintScopeRealTime stat{"Pixmap::fillHorizontal(gradient3)"};

		Gradient< float, float > gradient;
		gradient.addColorAt(0.0F, Red);
		gradient.addColorAt(0.5F, Blue);
		gradient.addColorAt(1.0F, Green);

		ASSERT_TRUE(image.fillHorizontal(gradient));
	}

	ASSERT_TRUE(FileIO::write(image, {"./test-assets/tmp_fillHorizontalGradient3.png"}, true));
}

TEST(PixelFactoryPixmap, fillVerticalGradient)
{
	Pixmap< uint8_t > image{340, 120, ChannelMode::RGBA};

	{
		PrintScopeRealTime stat{"Pixmap::fillVertical(gradient2)"};

		ASSERT_TRUE(image.fillVertical({Green, Cyan}));
	}

	ASSERT_TRUE(FileIO::write(image, {"./test-assets/tmp_fillVerticalGradient2.png"}, true));

	{
		PrintScopeRealTime stat{"Pixmap::fillVertical(gradient3)"};

		Gradient< float, float > gradient;
		gradient.addColorAt(0.0F, LightBlue);
		gradient.addColorAt(0.5F, DarkRed);
		gradient.addColorAt(1.0F, Grey);

		ASSERT_TRUE(image.fillVertical(gradient));
	}

	ASSERT_TRUE(FileIO::write(image, {"./test-assets/tmp_fillVerticalGradient3.png"}, true));
}

TEST(PixelFactoryPixmap, fillChannelValue)
{
	Pixmap< uint8_t > image{256, 256, ChannelMode::RGB};

	{
		PrintScopeRealTime stat{"Pixmap::fillChannel(Red, 255)"};

		ASSERT_TRUE(image.fillChannel(Channel::Red, 255));

		ASSERT_EQ(image.pixel(32, 32), Red);

		ASSERT_TRUE(FileIO::write(image, {"./test-assets/tmp_fillChannelRedValue.png"}, true));

		ASSERT_TRUE(image.fillChannel(Channel::Red, 0));
	}

	{
		PrintScopeRealTime stat{"Pixmap::fillChannel(Green, 255)"};

		ASSERT_TRUE(image.fillChannel(Channel::Green, 255));

		ASSERT_EQ(image.pixel(32, 32), Green);

		ASSERT_TRUE(FileIO::write(image, {"./test-assets/tmp_fillChannelGreenValue.png"}, true));

		ASSERT_TRUE(image.fillChannel(Channel::Green, 0));
	}

	{
		PrintScopeRealTime stat{"Pixmap::fillChannel(Blue, 255)"};

		ASSERT_TRUE(image.fillChannel(Channel::Blue, 255));

		ASSERT_EQ(image.pixel(32, 32), Blue);

		ASSERT_TRUE(FileIO::write(image, {"./test-assets/tmp_fillChannelBlueValue.png"}, true));

		ASSERT_TRUE(image.fillChannel(Channel::Blue, 0));
	}
}

TEST(PixelFactoryPixmap, fillChannelSequence)
{
	Randomizer< uint8_t > randomizer;

	const auto random = randomizer.vector(100, 0, 255);

	Pixmap< uint8_t > image{256, 256, ChannelMode::RGBA};

	{
		PrintScopeRealTime stat{"Pixmap::fillChannel(sequence)"};

		ASSERT_TRUE(image.fillChannel(Channel::Alpha, random));
	}

	ASSERT_TRUE(FileIO::write(image, {"./test-assets/tmp_fillChannelAlphaSequence.png"}, true));
}

TEST(PixelFactoryPixmap, fillChannelPattern)
{
	Pixmap< uint8_t > redChannel;
	Pixmap< uint8_t > greenChannel;
	Pixmap< uint8_t > blueChannel;
	Pixmap< uint8_t > alphaChannel;

	ASSERT_TRUE(FileIO::read(SmallPatternRBG_1, redChannel));
	ASSERT_TRUE(FileIO::read(SmallPatternRBG_2, greenChannel));
	ASSERT_TRUE(FileIO::read(SmallPatternRBG_3, blueChannel));
	ASSERT_TRUE(FileIO::read(MediumPatternRBG_1, alphaChannel));

	Pixmap< uint8_t > image{1024, 768, ChannelMode::RGBA};

	{
		PrintScopeRealTime stat{"Pixmap::fillChannel(PatternOnRed)"};

		ASSERT_TRUE(image.fillChannel(Channel::Red, redChannel));
	}

	{
		PrintScopeRealTime stat{"Pixmap::fillChannel(PatternOnGreen)"};

		ASSERT_TRUE(image.fillChannel(Channel::Green, greenChannel));
	}

	{
		PrintScopeRealTime stat{"Pixmap::fillChannel(PatternOnBlue)"};

		ASSERT_TRUE(image.fillChannel(Channel::Blue, blueChannel));
	}

	{
		PrintScopeRealTime stat{"Pixmap::fillChannelPattern(PatternOnAlpha)"};

		ASSERT_TRUE(image.fillChannel(Channel::Alpha, alphaChannel));
	}

	ASSERT_TRUE(FileIO::write(image, {"./test-assets/tmp_fillChannelPattern.png"}, true));
}

TEST(PixelFactoryPixmap, fillChannelGradient)
{
	Pixmap< uint8_t > image{800, 600, ChannelMode::RGB};

	{
		PrintScopeRealTime stat{"Pixmap::fillVerticalChannel(Gradient2)"};

		ASSERT_TRUE(image.fillVerticalChannel(Channel::Red, {LightBlue, DarkMagenta}));
	}

	{
		PrintScopeRealTime stat{"Pixmap::fillHorizontalChannel(Gradient5)"};

		Gradient< float, float > gradient;
		gradient.addColorAt(0.0F, DarkGreen);
		gradient.addColorAt(0.25F, LightYellow);
		gradient.addColorAt(0.5F, White);
		gradient.addColorAt(0.75F, LightYellow);
		gradient.addColorAt(1.0F, DarkRed);

		ASSERT_TRUE(image.fillHorizontalChannel(Channel::Green, gradient));
	}

	{
		PrintScopeRealTime stat{"Pixmap::fillVerticalChannel(Gradient3)"};

		Gradient< float, float > gradient;
		gradient.addColorAt(0.0F, LightBlue);
		gradient.addColorAt(0.5F, DarkRed);
		gradient.addColorAt(1.0F, Grey);

		ASSERT_TRUE(image.fillVerticalChannel(Channel::Blue, gradient));
	}

	ASSERT_TRUE(FileIO::write(image, {"./test-assets/tmp_fillChannelGradient.png"}, true));
}

TEST(PixelFactoryPixmap, noiseGeneration)
{
	Pixmap< uint8_t > image{1024, 1024, ChannelMode::RGB};

	{
		PrintScopeRealTime stat{"Pixmap::noise()"};

		ASSERT_TRUE(image.noise());

		ASSERT_TRUE(FileIO::write(image, {"./test-assets/tmp_1024x1024-noise.png"}, true));
	}

	{
		PrintScopeRealTime stat{"Pixmap::perlinNoise(9.0F, false)"};

		ASSERT_TRUE(image.perlinNoise(9.0F, false));

		ASSERT_TRUE(FileIO::write(image, {"./test-assets/tmp_1024x1024-perlinNoise.png"}, true));
	}

	{
		PrintScopeRealTime stat{"Pixmap::perlinNoise(9.0F, true)"};

		ASSERT_TRUE(image.perlinNoise(9.0F, true));

		ASSERT_TRUE(FileIO::write(image, {"./test-assets/tmp_1024x1024-perlinNoiseGray.png"}, true));
	}
}

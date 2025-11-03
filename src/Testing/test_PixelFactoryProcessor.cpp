/*
 * src/Testing/test_PixelFactoryProcessor.cpp
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
#include "Constants.hpp"

using namespace EmEn::Libs;
using namespace EmEn::Libs::PixelFactory;
using namespace EmEn::Libs::Time::Elapsed;

TEST(PixelFactoryProcessor, scale)
{
	PrintScopeRealTime globalStat{"Processor::scale(2.0F) [OVERALL]"};

	Pixmap< uint8_t > source;

	ASSERT_TRUE(FileIO::read(ExtraLargeRGB, source));

	ASSERT_EQ(source.width(), 8160);
	ASSERT_EQ(source.height(), 6144);
	ASSERT_EQ(source.colorCount(), 3);

	auto output = source;

	{
		PrintScopeRealTime localStat{"Processor::scale(2.0F)"};

		Processor< uint8_t > processor{output};

		ASSERT_TRUE(processor.scaleValue(2.0F));
	}

	ASSERT_EQ(output.width(), 8160);
	ASSERT_EQ(output.height(), 6144);
	ASSERT_EQ(output.colorCount(), 3);

	ASSERT_TRUE(FileIO::write(output, {"./test-assets/tmp_scaleAllValue.png"}, true));
}

TEST(PixelFactoryProcessor, scaleRed)
{
	PrintScopeRealTime globalStat{"Processor::scale(1.5F, Channel::Red) [OVERALL]"};

	Pixmap< uint8_t > source;

	ASSERT_TRUE(FileIO::read(ExtraLargeRGB, source));

	ASSERT_EQ(source.width(), 8160);
	ASSERT_EQ(source.height(), 6144);
	ASSERT_EQ(source.colorCount(), 3);

	auto output = source;

	{
		PrintScopeRealTime localStat{"Processor::scale(1.5F, Channel::Red)"};

		Processor< uint8_t > processor{output};

		ASSERT_TRUE(processor.scaleValue(2.0F, Channel::Red));
	}

	ASSERT_EQ(source.width(), 8160);
	ASSERT_EQ(source.height(), 6144);
	ASSERT_EQ(source.colorCount(), 3);

	ASSERT_TRUE(FileIO::write(output, {"./test-assets/tmp_scaleRedValue.png"}, true));
}

TEST(PixelFactoryProcessor, drawing)
{
	PrintScopeRealTime globalStat{"Processor::drawXXX() [OVERALL]"};

	Pixmap< uint8_t > emptyImage{800, 600};

	ASSERT_EQ(emptyImage.width(), 800);
	ASSERT_EQ(emptyImage.height(), 600);
	ASSERT_EQ(emptyImage.colorCount(), 3);

	Processor< uint8_t > processor{emptyImage};

	{
		PrintScopeRealTime localState{"Processor::drawSegment({10, 16}, {85, 503}, LightBlue)"};

		ASSERT_TRUE(processor.drawSegment({10, 16}, {85, 503}, LightBlue));
	}

	{
		PrintScopeRealTime localState{"Processor::drawCircle({578, 250}, 58, DarkRed)"};

		ASSERT_TRUE(processor.drawCircle({578, 250}, 58, DarkRed));
	}

	{
		PrintScopeRealTime localState{"Processor::drawCircle({64, 128}, 800, Red)"};

		ASSERT_TRUE(processor.drawCircle({64, 128}, 500, Red));
	}

	{
		PrintScopeRealTime localState{"Processor::drawSquare({256, 502, 125, 98}, Green)"};

		ASSERT_TRUE(processor.drawSquare({256, 502, 125, 98}, Green));
	}

	{
		PrintScopeRealTime localState{"Processor::drawCross({64, 96, 256, 128}, Yellow)"};

		ASSERT_TRUE(processor.drawCross({64, 96, 256, 128}, Yellow));
	}

	{
		PrintScopeRealTime localState{"Processor::drawStraightCross({200, 350, 300, 350}, White)"};

		ASSERT_TRUE(processor.drawStraightCross({200, 350, 300, 350}, White));
	}

	ASSERT_EQ(emptyImage.width(), 800);
	ASSERT_EQ(emptyImage.height(), 600);
	ASSERT_EQ(emptyImage.colorCount(), 3);

	ASSERT_TRUE(FileIO::write(emptyImage, {"./test-assets/tmp_drawing.png"}, true));
}

TEST(PixelFactoryProcessor, move)
{
	PrintScopeRealTime globalStat{"Processor::move(600, -200) [OVERALL]"};

	Pixmap< uint8_t > source;

	ASSERT_TRUE(FileIO::read(ExtraLargeRGB, source));

	ASSERT_EQ(source.width(), 8160);
	ASSERT_EQ(source.height(), 6144);
	ASSERT_EQ(source.colorCount(), 3);

	{
		PrintScopeRealTime localStat{"Processor::move(600, -200)"};

		Processor< uint8_t > processor{source};

		ASSERT_TRUE(processor.move(600, -400));
	}

	ASSERT_EQ(source.width(), 8160);
	ASSERT_EQ(source.height(), 6144);
	ASSERT_EQ(source.colorCount(), 3);

	ASSERT_TRUE(FileIO::write(source, {"./test-assets/tmp_moved.png"}, true));
}

TEST(PixelFactoryProcessor, shift)
{
	PrintScopeRealTime globalStat{"Processor::shift(600, -400) [OVERALL]"};

	Pixmap< uint8_t > source;

	ASSERT_TRUE(FileIO::read(ExtraLargeRGB, source));

	ASSERT_EQ(source.width(), 8160);
	ASSERT_EQ(source.height(), 6144);
	ASSERT_EQ(source.colorCount(), 3);

	{
		PrintScopeRealTime localStat{"Processor::shift(600, -400)"};

		Processor< uint8_t > processor{source};

		ASSERT_TRUE(processor.shift(600, -400));
	}

	ASSERT_EQ(source.width(), 8160);
	ASSERT_EQ(source.height(), 6144);
	ASSERT_EQ(source.colorCount(), 3);

	ASSERT_TRUE(FileIO::write(source, {"./test-assets/tmp_shifted.png"}, true));
}

TEST(PixelFactoryProcessor, shiftTextArea)
{
	PrintScopeRealTime globalStat{"Processor::shiftTextArea(100) [OVERALL]"};

	Pixmap< uint8_t > source;

	ASSERT_TRUE(FileIO::read(ExtraLargeRGB, source));

	ASSERT_EQ(source.width(), 8160);
	ASSERT_EQ(source.height(), 6144);
	ASSERT_EQ(source.colorCount(), 3);

	{
		PrintScopeRealTime localStat{"Processor::shiftTextArea(100)"};

		Processor< uint8_t > processor{source};

		ASSERT_TRUE(processor.shiftTextArea(-100));
	}

	ASSERT_EQ(source.width(), 8160);
	ASSERT_EQ(source.height(), 6144);
	ASSERT_EQ(source.colorCount(), 3);

	ASSERT_TRUE(FileIO::write(source, {"./test-assets/tmp_textShifted.png"}, true));
}

TEST(PixelFactoryProcessor, resizeNearestDown)
{
	PrintScopeRealTime globalStat{"Processor::resize(50%,Nearest) [OVERALL]"};

	Pixmap< uint8_t > source, output;

	ASSERT_TRUE(FileIO::read(ExtraLargeRGB, source));

	ASSERT_EQ(source.width(), 8160);
	ASSERT_EQ(source.height(), 6144);
	ASSERT_EQ(source.colorCount(), 3);

	{
		PrintScopeRealTime localStat{"Processor::resize(50%,Nearest)"};

		output = Processor< uint8_t >::resize(source, 4080, 3072, FilteringMode::Nearest);
	}

	ASSERT_EQ(output.width(), 4080);
	ASSERT_EQ(output.height(), 3072);
	ASSERT_EQ(output.colorCount(), 3);

	ASSERT_TRUE(FileIO::write(output, {"./test-assets/tmp_4080x3072resizeNearest.png"}, true));
}

TEST(PixelFactoryProcessor, resizeLinearDown)
{
	PrintScopeRealTime globalStat{"Processor::resize(50%,Linear) [OVERALL]"};

	Pixmap< uint8_t > source, output;

	ASSERT_TRUE(FileIO::read(ExtraLargeRGB, source));

	ASSERT_EQ(source.width(), 8160);
	ASSERT_EQ(source.height(), 6144);
	ASSERT_EQ(source.colorCount(), 3);

	{
		PrintScopeRealTime localStat{"Processor::resize(50%,Linear)"};

		output = Processor< uint8_t >::resize(source, 4080, 3072, FilteringMode::Linear);
	}

	ASSERT_EQ(output.width(), 4080);
	ASSERT_EQ(output.height(), 3072);
	ASSERT_EQ(output.colorCount(), 3);

	ASSERT_TRUE(FileIO::write(output, {"./test-assets/tmp_4080x3072resizeLinear.png"}, true));
}

TEST(PixelFactoryProcessor, resizeCubicDown)
{
	PrintScopeRealTime globalStat{"Processor::resize(50%,Cubic) [OVERALL]"};

	Pixmap< uint8_t > source, output;

	ASSERT_TRUE(FileIO::read(ExtraLargeRGB, source));

	ASSERT_EQ(source.width(), 8160);
	ASSERT_EQ(source.height(), 6144);
	ASSERT_EQ(source.colorCount(), 3);

	{
		PrintScopeRealTime localStat{"Processor::resize(50%,Cubic)"};

		output = Processor< uint8_t >::resize(source, 4080, 3072, FilteringMode::Cubic);
	}

	ASSERT_EQ(output.width(), 4080);
	ASSERT_EQ(output.height(), 3072);
	ASSERT_EQ(output.colorCount(), 3);

	ASSERT_TRUE(FileIO::write(output, {"./test-assets/tmp_4080x3072resizeCubic.png"}, true));
}

TEST(PixelFactoryProcessor, resizeNearestUp)
{
	PrintScopeRealTime globalStat{"Processor::resize(200%,Nearest) [OVERALL]"};

	Pixmap< uint8_t > source, output;

	ASSERT_TRUE(FileIO::read(LargeRGB, source));

	ASSERT_EQ(source.width(), 1200);
	ASSERT_EQ(source.height(), 800);
	ASSERT_EQ(source.colorCount(), 3);

	{
		PrintScopeRealTime localStat{"Processor::resize(200%,Nearest)"};

		output = Processor< uint8_t >::resize(source, 2400, 1600, FilteringMode::Nearest);
	}

	ASSERT_EQ(output.width(), 2400);
	ASSERT_EQ(output.height(), 1600);
	ASSERT_EQ(output.colorCount(), 3);

	ASSERT_TRUE(FileIO::write(output, {"./test-assets/tmp_2400x1600resizeNearest.png"}, true));
}

TEST(PixelFactoryProcessor, resizeLinearUp)
{
	PrintScopeRealTime globalStat{"Processor::resize(200%,Linear) [OVERALL]"};

	Pixmap< uint8_t > source, output;

	ASSERT_TRUE(FileIO::read(LargeRGB, source));

	ASSERT_EQ(source.width(), 1200);
	ASSERT_EQ(source.height(), 800);
	ASSERT_EQ(source.colorCount(), 3);

	{
		PrintScopeRealTime localStat{"Processor::resize(200%,Linear)"};

		output = Processor< uint8_t >::resize(source, 2400, 1600, FilteringMode::Linear);
	}

	ASSERT_EQ(output.width(), 2400);
	ASSERT_EQ(output.height(), 1600);
	ASSERT_EQ(output.colorCount(), 3);

	ASSERT_TRUE(FileIO::write(output, {"./test-assets/tmp_2400x1600resizeLinear.png"}, true));
}

TEST(PixelFactoryProcessor, resizeCubicUp)
{
	PrintScopeRealTime globalStat{"Processor::resize(200%,Cubic) [OVERALL]"};

	Pixmap< uint8_t > source, output;

	ASSERT_TRUE(FileIO::read(LargeRGB, source));

	ASSERT_EQ(source.width(), 1200);
	ASSERT_EQ(source.height(), 800);
	ASSERT_EQ(source.colorCount(), 3);

	{
		PrintScopeRealTime localStat{"Processor::resize(200%,Cubic)"};

		output = Processor< uint8_t >::resize(source, 2400, 1600, FilteringMode::Cubic);
	}

	ASSERT_EQ(output.width(), 2400);
	ASSERT_EQ(output.height(), 1600);
	ASSERT_EQ(output.colorCount(), 3);

	ASSERT_TRUE(FileIO::write(output, {"./test-assets/tmp_2400x1600resizeCubic.png"}, true));
}

TEST(PixelFactoryProcessor, crop)
{
	PrintScopeRealTime globalStat{"Processor::crop(64, 128, 1600, 1200) [OVERALL]"};

	Pixmap< uint8_t > source, output;

	ASSERT_TRUE(FileIO::read(ExtraLargeRGB, source));

	ASSERT_EQ(source.width(), 8160);
	ASSERT_EQ(source.height(), 6144);
	ASSERT_EQ(source.colorCount(), 3);

	{
		PrintScopeRealTime stat{"Processor::crop(64, 128, 1600, 1200)"};

		output = Processor< uint8_t >::crop(source, {64, 128, 1600, 1200});
	}

	ASSERT_EQ(output.width(), 1600);
	ASSERT_EQ(output.height(), 1200);
	ASSERT_EQ(output.colorCount(), 3);

	ASSERT_TRUE(FileIO::write(output, {"./test-assets/tmp_1600x1200cropped.png"}, true));
}

TEST(PixelFactoryProcessor, extractChannelRed)
{
	PrintScopeRealTime globalStat{"Processor::extractChannel(Red) [OVERALL]"};

	Pixmap< uint8_t > source, output;

	ASSERT_TRUE(FileIO::read(ExtraLargeRGB, source));

	ASSERT_EQ(source.width(), 8160);
	ASSERT_EQ(source.height(), 6144);
	ASSERT_EQ(source.colorCount(), 3);

	{
		PrintScopeRealTime localStat{"Processor::extractChannel(Red)"};

		output = Processor< uint8_t >::extractChannel(source, Channel::Red);
	}

	ASSERT_EQ(output.width(), 8160);
	ASSERT_EQ(output.height(), 6144);
	ASSERT_EQ(output.colorCount(), 1);

	ASSERT_TRUE(FileIO::write(output, {"./test-assets/tmp_extractChannelRed.png"}, true));
}

TEST(PixelFactoryProcessor, extractChannelGreen)
{
	PrintScopeRealTime globalStat{"Processor::extractChannel(Green) [OVERALL]"};

	Pixmap< uint8_t > source, output;

	ASSERT_TRUE(FileIO::read(ExtraLargeRGB, source));

	ASSERT_EQ(source.width(), 8160);
	ASSERT_EQ(source.height(), 6144);
	ASSERT_EQ(source.colorCount(), 3);

	{
		PrintScopeRealTime localStat{"Processor::extractChannel(Green)"};

		output = Processor< uint8_t >::extractChannel(source, Channel::Green);
	}

	ASSERT_EQ(output.width(), 8160);
	ASSERT_EQ(output.height(), 6144);
	ASSERT_EQ(output.colorCount(), 1);

	ASSERT_TRUE(FileIO::write(output, {"./test-assets/tmp_extractChannelGreen.png"}, true));
}

TEST(PixelFactoryProcessor, extractChannelBlue)
{
	PrintScopeRealTime globalStat{"Processor::extractChannel(Blue) [OVERALL]"};

	Pixmap< uint8_t > source, output;

	ASSERT_TRUE(FileIO::read(ExtraLargeRGB, source));

	ASSERT_EQ(source.width(), 8160);
	ASSERT_EQ(source.height(), 6144);
	ASSERT_EQ(source.colorCount(), 3);

	{
		PrintScopeRealTime localStat{"Processor::extractChannel(Blue)"};

		output = Processor< uint8_t >::extractChannel(source, Channel::Blue);
	}

	ASSERT_EQ(output.width(), 8160);
	ASSERT_EQ(output.height(), 6144);
	ASSERT_EQ(output.colorCount(), 1);

	ASSERT_TRUE(FileIO::write(output, {"./test-assets/tmp_extractChannelBlue.png"}, true));
}

TEST(PixelFactoryProcessor, toGrayscale)
{
	PrintScopeRealTime globalStat{"Processor::toGrayscale() [OVERALL]"};

	Pixmap< uint8_t > source, output;

	ASSERT_TRUE(FileIO::read(ExtraLargeRGB, source));

	ASSERT_EQ(source.width(), 8160);
	ASSERT_EQ(source.height(), 6144);
	ASSERT_EQ(source.colorCount(), 3);

	{
		PrintScopeRealTime localStat{"Processor::toGrayscale()"};

		output = Processor< uint8_t >::toGrayscale(source);
	}

	ASSERT_EQ(output.width(), 8160);
	ASSERT_EQ(output.height(), 6144);
	ASSERT_EQ(output.colorCount(), 1);

	ASSERT_TRUE(FileIO::write(output, {"./test-assets/tmp_toGrayscale.png"}, true));
}

TEST(PixelFactoryProcessor, toRGB)
{
	PrintScopeRealTime globalStat{"Processor::toRGB() [OVERALL]"};

	Pixmap< uint8_t > source, output;

	ASSERT_TRUE(FileIO::read(MediumRGBA, source));

	ASSERT_EQ(source.width(), 512);
	ASSERT_EQ(source.height(), 512);
	ASSERT_EQ(source.colorCount(), 4);

	{
		PrintScopeRealTime localStat{"Processor::toRGB()"};

		output = Processor< uint8_t >::toRGB(source);
	}

	ASSERT_EQ(output.width(), 512);
	ASSERT_EQ(output.height(), 512);
	ASSERT_EQ(output.colorCount(), 3);

	ASSERT_TRUE(FileIO::write(output, {"./test-assets/tmp_toRGB.png"}, true));
}

TEST(PixelFactoryProcessor, toRGBA)
{
	PrintScopeRealTime globalStat{"Processor::toRGBA() [OVERALL]"};

	Pixmap< uint8_t > source, output;

	ASSERT_TRUE(FileIO::read(ExtraLargeRGB, source));

	ASSERT_EQ(source.width(), 8160);
	ASSERT_EQ(source.height(), 6144);
	ASSERT_EQ(source.colorCount(), 3);

	{
		PrintScopeRealTime localStat{"Processor::toRGBA()"};

		output = Processor< uint8_t >::toRGBA(source, 0.5F);
	}

	ASSERT_EQ(output.width(), 8160);
	ASSERT_EQ(output.height(), 6144);
	ASSERT_EQ(output.colorCount(), 4);

	ASSERT_TRUE(FileIO::write(output, {"./test-assets/tmp_toRGBA.png"}, true));
}

TEST(PixelFactoryProcessor, mirrorX)
{
	PrintScopeRealTime globalStat{"Processor::mirror(X) [OVERALL]"};

	Pixmap< uint8_t > source, output;

	ASSERT_TRUE(FileIO::read(ExtraLargeRGB, source));

	ASSERT_EQ(source.width(), 8160);
	ASSERT_EQ(source.height(), 6144);
	ASSERT_EQ(source.colorCount(), 3);

	{
		PrintScopeRealTime localStat{"Processor::mirror(X)"};

		output = Processor< uint8_t >::mirror(source, MirrorMode::X);
	}

	ASSERT_EQ(output.width(), 8160);
	ASSERT_EQ(output.height(), 6144);
	ASSERT_EQ(output.colorCount(), 3);

	ASSERT_TRUE(FileIO::write(output, {"./test-assets/tmp_mirrorX.png"}, true));
}

TEST(PixelFactoryProcessor, mirrorY)
{
	PrintScopeRealTime globalStat{"Processor::mirror(Y) [OVERALL]"};

	Pixmap< uint8_t > source, output;

	ASSERT_TRUE(FileIO::read(ExtraLargeRGB, source));

	ASSERT_EQ(source.width(), 8160);
	ASSERT_EQ(source.height(), 6144);
	ASSERT_EQ(source.colorCount(), 3);

	{
		PrintScopeRealTime localStat{"Processor::mirror(Y)"};

		output = Processor< uint8_t >::mirror(source, MirrorMode::Y);
	}

	ASSERT_EQ(output.width(), 8160);
	ASSERT_EQ(output.height(), 6144);
	ASSERT_EQ(output.colorCount(), 3);

	ASSERT_TRUE(FileIO::write(output, {"./test-assets/tmp_mirrorY.png"}, true));
}

TEST(PixelFactoryProcessor, mirrorBoth)
{
	PrintScopeRealTime globalStat{"Processor::mirror(Both) [OVERALL]"};

	Pixmap< uint8_t > source, output;

	ASSERT_TRUE(FileIO::read(ExtraLargeRGB, source));

	ASSERT_EQ(source.width(), 8160);
	ASSERT_EQ(source.height(), 6144);
	ASSERT_EQ(source.colorCount(), 3);

	{
		PrintScopeRealTime localStat{"Processor::mirror(Both)"};

		output = Processor< uint8_t >::mirror(source, MirrorMode::Both);
	}

	ASSERT_EQ(output.width(), 8160);
	ASSERT_EQ(output.height(), 6144);
	ASSERT_EQ(output.colorCount(), 3);

	ASSERT_TRUE(FileIO::write(output, {"./test-assets/tmp_mirrorBoth.png"}, true));
}

TEST(PixelFactoryProcessor, extend)
{
	PrintScopeRealTime globalStat{"Processor::extend() [OVERALL]"};

	Pixmap< uint8_t > source, output;

	ASSERT_TRUE(FileIO::read(ExtraLargeRGB, source));

	ASSERT_EQ(source.width(), 8160);
	ASSERT_EQ(source.height(), 6144);
	ASSERT_EQ(source.colorCount(), 3);

	{
		PrintScopeRealTime localStat{"Processor::extend()"};

		output = Processor< uint8_t >::extend(source, {16, 24, 32, 128}, Red);
	}

	ASSERT_EQ(output.width(), 8160 + 16 + 32);
	ASSERT_EQ(output.height(), 6144 + 24 + 128);
	ASSERT_EQ(output.colorCount(), 3);

	ASSERT_TRUE(FileIO::write(output, {"./test-assets/tmp_extend.png"}, true));
}

TEST(PixelFactoryProcessor, rotateQuarterTurn)
{
	PrintScopeRealTime globalStat{"Processor::rotateQuarterTurn() [OVERALL]"};

	Pixmap< uint8_t > source, output;

	ASSERT_TRUE(FileIO::read(ExtraLargeRGB, source));

	ASSERT_EQ(source.width(), 8160);
	ASSERT_EQ(source.height(), 6144);
	ASSERT_EQ(source.colorCount(), 3);

	{
		PrintScopeRealTime localStat{"Processor::rotateQuarterTurn()"};

		output = Processor< uint8_t >::rotateQuarterTurn(source);
	}

	ASSERT_EQ(output.width(), 6144);
	ASSERT_EQ(output.height(), 8160);
	ASSERT_EQ(output.colorCount(), 3);

	ASSERT_TRUE(FileIO::write(output, {"./test-assets/tmp_rotated+90.png"}, true));
}

TEST(PixelFactoryProcessor, rotateHalfTurn)
{
	PrintScopeRealTime globalStat{"Processor::rotateHalfTurn() [OVERALL]"};

	Pixmap< uint8_t > source, output;

	ASSERT_TRUE(FileIO::read(ExtraLargeRGB, source));

	ASSERT_EQ(source.width(), 8160);
	ASSERT_EQ(source.height(), 6144);
	ASSERT_EQ(source.colorCount(), 3);

	{
		PrintScopeRealTime localStat{"Processor::rotateHalfTurn()"};

		output = Processor< uint8_t >::rotateHalfTurn(source);
	}

	ASSERT_EQ(output.width(), 8160);
	ASSERT_EQ(output.height(), 6144);
	ASSERT_EQ(output.colorCount(), 3);

	ASSERT_TRUE(FileIO::write(output, {"./test-assets/tmp_rotated+180.png"}, true));
}

TEST(PixelFactoryProcessor, rotateThreeQuarterTurn)
{
	PrintScopeRealTime globalStat{"Processor::rotateThreeQuarterTurn() [OVERALL]"};

	Pixmap< uint8_t > source, output;

	ASSERT_TRUE(FileIO::read(ExtraLargeRGB, source));

	ASSERT_EQ(source.width(), 8160);
	ASSERT_EQ(source.height(), 6144);
	ASSERT_EQ(source.colorCount(), 3);

	{
		PrintScopeRealTime localStat{"Processor::rotateThreeQuarterTurn()"};

		output = Processor< uint8_t >::rotateThreeQuarterTurn(source);
	}

	ASSERT_EQ(output.width(), 6144);
	ASSERT_EQ(output.height(), 8160);
	ASSERT_EQ(output.colorCount(), 3);

	ASSERT_TRUE(FileIO::write(output, {"./test-assets/tmp_rotated+270.png"}, true));
}

TEST(PixelFactoryProcessor, inverseColors)
{
	PrintScopeRealTime globalStat{"Processor::inverseColors() [OVERALL]"};

	Pixmap< uint8_t > source, output;

	ASSERT_TRUE(FileIO::read(ExtraLargeRGB, source));

	ASSERT_EQ(source.width(), 8160);
	ASSERT_EQ(source.height(), 6144);
	ASSERT_EQ(source.colorCount(), 3);

	{
		PrintScopeRealTime localStat{"Processor::inverseColors()"};

		output = Processor< uint8_t >::inverseColors(source);
	}

	ASSERT_EQ(output.width(), 8160);
	ASSERT_EQ(output.height(), 6144);
	ASSERT_EQ(output.colorCount(), 3);

	ASSERT_TRUE(FileIO::write(output, {"./test-assets/tmp_inverseColors.png"}, true));
}

TEST(PixelFactoryProcessor, swapChannels)
{
	PrintScopeRealTime globalStat{"Processor::swapChannels() [OVERALL]"};

	Pixmap< uint8_t > source, output;

	ASSERT_TRUE(FileIO::read(ExtraLargeRGB, source));

	ASSERT_EQ(source.width(), 8160);
	ASSERT_EQ(source.height(), 6144);
	ASSERT_EQ(source.colorCount(), 3);

	{
		PrintScopeRealTime localStat{"Processor::swapChannels()"};

		output = Processor< uint8_t >::swapChannels(source);
	}

	ASSERT_EQ(output.width(), 8160);
	ASSERT_EQ(output.height(), 6144);
	ASSERT_EQ(output.colorCount(), 3);

	ASSERT_TRUE(FileIO::write(output, {"./test-assets/tmp_swapChannels.png"}, true));
}

TEST(PixelFactoryProcessor, blit)
{
	Pixmap< uint8_t > source;

	ASSERT_TRUE(FileIO::read(MediumRGBA, source));
	ASSERT_EQ(source.width(), 512);
	ASSERT_EQ(source.height(), 512);
	ASSERT_EQ(source.colorCount(), 4);

	Pixmap< uint8_t > smallImage;

	ASSERT_TRUE(FileIO::read(SmallRGBA, smallImage));
	ASSERT_EQ(smallImage.width(), 64);
	ASSERT_EQ(smallImage.height(), 64);
	ASSERT_EQ(smallImage.colorCount(), 4);

	Processor< uint8_t > proc{source};

	Pixmap< uint8_t > blitImage;

	ASSERT_TRUE(proc.blit(smallImage, {64, 64}, {0, 0, 64, 64}));
	ASSERT_TRUE(proc.blit(smallImage, {64, 64}, {512-64, 512-64, 64, 64}));
	ASSERT_TRUE(proc.blit(smallImage, {64, 64}, {512-64, 0, 64, 64}));

	ASSERT_TRUE(FileIO::write(source, {"./test-assets/tmp_64x64-blit.png"}, true));
}

TEST(PixelFactoryProcessor, copy)
{
	Pixmap< uint8_t > source;

	ASSERT_TRUE(FileIO::read(LargeRGB, source));

	Processor< uint8_t > processor{source};

	{
		PrintScopeRealTime stat{"Processor::copy(pixmapSimple)"};

		Pixmap< uint8_t > chunk;

		ASSERT_TRUE(FileIO::read(MediumRGBA, chunk));

		ASSERT_TRUE(processor.copy(chunk, DrawPixelMode::Normal));
	}

	{
		PrintScopeRealTime stat{"Processor::copy(pixmapSimpleClip)"};

		Pixmap< uint8_t > chunk;

		ASSERT_TRUE(FileIO::read(SmallRGBA, chunk));

		ASSERT_TRUE(processor.copy(chunk, 600, 32, DrawPixelMode::Addition));
	}

	{
		PrintScopeRealTime stat{"Processor::copy(pixmap)"};

		Pixmap< uint8_t > chunk;

		ASSERT_TRUE(FileIO::read(MediumGrayscale, chunk));

		ASSERT_TRUE(processor.copy(chunk, {128, 128, 256, 256}, {400, 260, 256, 256}, DrawPixelMode::Multiply));
	}

	{
		PrintScopeRealTime stat{"Processor::copy(color)"};

		ASSERT_TRUE(processor.copy(Red, {640, 600, 200, 16}, DrawPixelMode::Overlay));
	}

	ASSERT_TRUE(FileIO::write(source, {"./test-assets/tmp_copy.png"}, true));
}

TEST(PixelFactoryProcessor, addAlphaChannel)
{
	PrintScopeRealTime globalStat{"Processor::addAlphaChannel() [OVERALL]"};

	Pixmap< uint8_t > source, output;

	ASSERT_TRUE(FileIO::read(MediumRGB, source));

	ASSERT_EQ(source.width(), 512);
	ASSERT_EQ(source.height(), 512);
	ASSERT_EQ(source.colorCount(), 3);

	{
		PrintScopeRealTime localStat{"Processor::addAlphaChannel()"};

		ASSERT_TRUE(Processor< uint8_t >::addAlphaChannel(source, output));
	}

	ASSERT_EQ(output.width(), 512);
	ASSERT_EQ(output.height(), 512);
	ASSERT_EQ(output.colorCount(), 4);

	ASSERT_TRUE(FileIO::write(output, {"./test-assets/tmp_addAlphaChannel.png"}, true));
}

TEST(PixelFactoryProcessor, removeAlphaChannel)
{
	PrintScopeRealTime globalStat{"Processor::removeAlphaChannel() [OVERALL]"};

	Pixmap< uint8_t > source, output;

	ASSERT_TRUE(FileIO::read(MediumRGBA, source));

	ASSERT_EQ(source.width(), 512);
	ASSERT_EQ(source.height(), 512);
	ASSERT_EQ(source.colorCount(), 4);

	{
		PrintScopeRealTime localStat{"Processor::removeAlphaChannel()"};

		ASSERT_TRUE(Processor< uint8_t >::removeAlphaChannel(source, output));
	}

	ASSERT_EQ(output.width(), 512);
	ASSERT_EQ(output.height(), 512);
	ASSERT_EQ(output.colorCount(), 3);

	ASSERT_TRUE(FileIO::write(output, {"./test-assets/tmp_removeAlphaChannel.png"}, true));
}

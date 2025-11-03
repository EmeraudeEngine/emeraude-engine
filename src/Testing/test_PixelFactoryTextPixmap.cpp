/*
 * src/Testing/test_PixelFactoryTextPixmap.cpp
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
#include "Libs/PixelFactory/TextProcessor.hpp"
#include "Constants.hpp"

using namespace EmEn::Libs;
using namespace EmEn::Libs::PixelFactory;
using namespace EmEn::Libs::Time::Elapsed;

static constexpr auto LoremIpsum =
	"This is dummy text." "\n"
	"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed non risus. Suspendisse lectus tortor, dignissim sit amet, adipiscing nec, ultricies sed, dolor. Cras elementum ultrices diam. Maecenas ligula massa, varius a, semper congue, euismod non, mi. Proin porttitor, orci nec nonummy molestie, enim est eleifend mi, non fermentum diam nisl sit amet erat. Duis semper. Duis arcu massa, scelerisque vitae, consequat in, pretium a, enim. Pellentesque congue. Ut in risus volutpat libero pharetra tempor. Cras vestibulum bibendum augue. Praesent egestas leo in pede. Praesent blandit odio eu enim. Pellentesque sed dui ut augue blandit sodales. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Aliquam nibh. Mauris ac mauris sed pede pellentesque fermentum. Maecenas adipiscing ante non diam sodales hendrerit.\n"
	"Ut velit mauris, egestas sed, gravida nec, ornare ut, mi. Aenean ut orci vel massa suscipit pulvinar. Nulla sollicitudin. Fusce varius, ligula non tempus aliquam, nunc turpis ullamcorper nibh, in tempus sapien eros vitae ligula. Pellentesque rhoncus nunc et augue. Integer id felis. Curabitur aliquet pellentesque diam. Integer quis metus vitae elit lobortis egestas. Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Morbi vel erat non mauris convallis vehicula. Nulla et sapien. Integer tortor tellus, aliquam faucibus, convallis id, congue eu, quam. Mauris ullamcorper felis vitae erat. Proin feugiat, augue non elementum posuere, metus purus iaculis lectus, et tristique ligula justo vitae magna.\n"
	"Aliquam convallis sollicitudin purus. Praesent aliquam, enim at fermentum mollis, ligula massa adipiscing nisl, ac euismod nibh nisl eu lectus. Fusce vulputate sem at sapien. Vivamus leo. Aliquam euismod libero eu enim. Nulla nec felis sed leo placerat imperdiet. Aenean suscipit nulla in justo. Suspendisse cursus rutrum augue. Nulla tincidunt tincidunt mi. Curabitur iaculis, lorem vel rhoncus faucibus, felis magna fermentum augue, et ultricies lacus lorem varius purus. Curabitur eu amet.\n";

TEST(PixelFactoryTextProcessor, write)
{
	Pixmap< uint8_t > image{1024, 1024, ChannelMode::RGB};

	{
		PrintScopeRealTime stat{"Processor::fill(pattern)"};

		Pixmap< uint8_t > pattern;

		ASSERT_TRUE(FileIO::read(SmallPatternRBG_2, pattern));

		ASSERT_TRUE(image.fill(Processor< uint8_t >::toRGB(pattern)));
	}

	/*Font< uint8_t > fixedFont;

	EXPECT_TRUE(fixedFont.readFile(TrueTypeFont, 48, true));

	{
		PrintScopeRealTime stat{"TextProcessor::write(fixedFont)"};

		TextProcessor< uint8_t > textProcessor{image};
		textProcessor.setFont(fixedFont, 48);
		textProcessor.setFontColor(DarkRed);
		textProcessor.setDrawMode(DrawPixelMode::Normal);
		textProcessor.setArea({32, 32, 1024 - 64, 512 - 64});
		textProcessor.setLineSpace(8);

		EXPECT_TRUE(textProcessor.write(LoremIpsum));
	}*/

	/*Font< uint8_t > trueTypeFont;

	EXPECT_TRUE(trueTypeFont.readFile(TrueTypeFont, 32, false));

	{
		PrintScopeRealTime stat{"TextProcessor::write(trueTypeFont)"};

		TextProcessor< uint8_t > textProcessor{image};
		textProcessor.setFont(trueTypeFont, 32);
		textProcessor.setFontColor(Green);
		textProcessor.setDrawMode(DrawPixelMode::Normal);
		textProcessor.setArea({64, 64 + 512, 1024 - 128, 512 - 128});
		textProcessor.setLineSpace(4);

		EXPECT_TRUE(textProcessor.write(LoremIpsum));
	}*/

	ASSERT_TRUE(FileIO::write(image, {"./test-assets/tmp_textPixmap.png"}, true));
}

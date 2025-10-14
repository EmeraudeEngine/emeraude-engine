/*
 * src/Testing/test_MathBasics.cpp
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
#include "Libs/Math/Base.hpp"

using namespace Libs::Math;

TEST(Math, clamp)
{
	EXPECT_EQ(clamp(-0.3, -1.0, 3.0), -0.3);

	EXPECT_EQ(clamp(67, 10, 50), 50);
}

TEST(Math, clampToUnit)
{
	EXPECT_EQ(clampToUnit(-0.3), 0.0);

	EXPECT_EQ(clampToUnit(67.0F), 1.0F);

	EXPECT_EQ(clampToUnit(0.333F), 0.333F);
}

TEST(Math, alignCount)
{
	EXPECT_EQ(alignCount(127, 256), 1);

	EXPECT_EQ(alignCount(256, 256), 1);

	EXPECT_EQ(alignCount(298, 256), 2);

	EXPECT_EQ(alignCount(512, 256), 2);

	EXPECT_EQ(alignCount(640, 256), 3);
}

TEST(Math, timesDivisible)
{
	EXPECT_EQ(timesDivisible(33, 2), 0);

	EXPECT_EQ(timesDivisible(32, 2), 5);

	EXPECT_EQ(timesDivisible(64, 3), 0);

	EXPECT_EQ(timesDivisible(90, 3), 2);

	EXPECT_EQ(timesDivisible(80, 4), 2);
}

/*
 * src/Testing/test_PixelFactoryColor.cpp
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
#include "Libs/PixelFactory/Color.hpp"

using namespace EmEn::Libs;
using namespace EmEn::Libs::PixelFactory;

TEST(PixelFactoryColor, ColorFromInteger)
{
	ASSERT_EQ(ColorFromInteger< uint8_t >(0, 0, 0, 255), Black);
	ASSERT_EQ(ColorFromInteger< uint8_t >(255, 255, 255, 255), White);
	ASSERT_EQ(ColorFromInteger< uint8_t >(255, 0, 0, 255), Red);
	ASSERT_EQ(ColorFromInteger< uint8_t >(0, 255, 0, 255), Green);
	ASSERT_EQ(ColorFromInteger< uint8_t >(0, 0, 255, 255), Blue);

	ASSERT_EQ(ColorFromInteger< uint16_t >(0, 0, 0, 65535), Black);
	ASSERT_EQ(ColorFromInteger< uint16_t >(65535, 65535, 65535, 65535), White);
	ASSERT_EQ(ColorFromInteger< uint16_t >(65535, 0, 0, 65535), Red);
	ASSERT_EQ(ColorFromInteger< uint16_t >(0, 65535, 0, 65535), Green);
	ASSERT_EQ(ColorFromInteger< uint16_t >(0, 0, 65535, 65535), Blue);

	ASSERT_EQ(ColorFromInteger< uint32_t >(0, 0, 0, 4294967295), Black);
	ASSERT_EQ(ColorFromInteger< uint32_t >(4294967295, 4294967295, 4294967295, 4294967295), White);
	ASSERT_EQ(ColorFromInteger< uint32_t >(4294967295, 0, 0, 4294967295), Red);
	ASSERT_EQ(ColorFromInteger< uint32_t >(0, 4294967295, 0, 4294967295), Green);
	ASSERT_EQ(ColorFromInteger< uint32_t >(0, 0, 4294967295, 4294967295), Blue);

	ASSERT_EQ(ColorFromInteger< uint64_t >(0, 0, 0, 18446744073709551615), Black);
	ASSERT_EQ(ColorFromInteger< uint64_t >(18446744073709551615, 18446744073709551615, 18446744073709551615, 18446744073709551615), White);
	ASSERT_EQ(ColorFromInteger< uint64_t >(18446744073709551615, 0, 0, 18446744073709551615), Red);
	ASSERT_EQ(ColorFromInteger< uint64_t >(0, 18446744073709551615, 0, 18446744073709551615), Green);
	ASSERT_EQ(ColorFromInteger< uint64_t >(0, 0, 18446744073709551615, 18446744073709551615), Blue);
}

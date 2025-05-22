/*
 * src/Libs/PixelFactory/Types.hpp
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

#pragma once

/* STL inclusions. */
#include <cstdint>

namespace EmEn::Libs::PixelFactory
{
	/** @brief Enumerate color channel mode. */
	enum class ChannelMode : uint8_t
	{
		Grayscale = 1,
		GrayscaleAlpha = 2,
		RGB = 3,
		RGBA = 4
	};

	/** @brief Enumerate color channel index. */
	enum class Channel : uint8_t
	{
		Red = 0,
		Green = 1,
		Blue = 2,
		Alpha = 3
	};

	/** @brief Enumerate compression types for Pixmap file writing. */
	enum class Compression : uint8_t
	{
		None,
		LossLess,
		Destructive
	};

	/** @brief Enumerate filtering mode for enlarging or reducing an image. */
	enum class FilteringMode : uint8_t
	{
		Nearest,
		Linear,
		Cubic
	};

	/** @brief List drawing mode when copying an image or a color onto another. */
	enum class DrawPixelMode : uint8_t
	{
		Replace,
		Normal, /* (Alpha blending) */
		Addition,
		Subtract,
		Multiply,
		Divide,
		Screen,
		Overlay,
		Difference,
		Darken,
		Lighten
	};

	/** @brief List grayscale conversion mode from RGB color. */
	enum class GrayscaleConversionMode : uint8_t
	{
		/* Gray = (Red + Green + Blue) / 3 */
		Average,
		/* Gray = 0.2989 * Red + 0.5866 * Green + 0.1145 * Blue */
		LumaRec601_1,
		/* Gray = 0.2126 * Red + 0.7152 * Green + 0.0722 * Blue */
		LumaRec709,
		/* Gray = 0.2220 * Red + 0.7067 * Green + 0.0713 * Blue */
		LumaITU,
		/* Gray = ( max(Red, Green, Blue) + min(Red, Green, Blue) ) / 2 */
		Desaturation,
		/* Gray = min(Red, Green, Blue) OR Gray = max(Red, Green, Blue) */
		Decomposition,
		/* Gray = Red OR Gray = Green OR ... */
		SingleChannel,
		ShadesScale
	};

	/** @brief Enumerate mirroring mode. */
	enum class MirrorMode : uint8_t
	{
		X,
		Y,
		Both
	};

	/** @brief List policies for handling the alpha channel. */
	enum class AlphaChannelPolicy : uint8_t
	{
		Add,
		Remove,
		Keep
	};

	/** @brief Enumerate cardinal directions. */
	enum class Direction : uint8_t
	{
		Up,
		Down,
		Right,
		Left
	};

	static constexpr auto LuminanceRec6011Red{0.2989F};
	static constexpr auto LuminanceRec6011Green{0.5866F};
	static constexpr auto LuminanceRec6011Blue{0.1145F};

	static constexpr auto LuminanceRec709Red{0.2126F};
	static constexpr auto LuminanceRec709Green{0.7152F};
	static constexpr auto LuminanceRec709Blue{0.0722F};

	static constexpr auto LuminanceITURed{0.2220F};
	static constexpr auto LuminanceITUGreen{0.7067F};
	static constexpr auto LuminanceITUBlue{0.0713F};

	/**
	 * @brief Structure to wrap raw data for pixmap.
	 * @tparam pixel_data_t The pixel component type for the pixmap depth precision. Default uint8_t.
	 * @tparam dimension_t The type of unsigned integer used for pixmap dimension. Default uint32_t.
	 */
	template< typename pixel_data_t = uint8_t, typename dimension_t = uint32_t >
	requires (std::is_arithmetic_v< pixel_data_t > && std::is_unsigned_v< dimension_t >)
	struct RawPixmapData
	{
		dimension_t width{0};
		dimension_t height{0};
		dimension_t colorCount{0};
		const pixel_data_t * pointer{nullptr};
	};

	/**
	 * @brief Structure for text metrics to use to render text on pixmap.
	 * @tparam dimension_t The type of unsigned integer used for pixmap dimension. Default uint32_t.
	 */
	template< typename dimension_t = uint32_t >
	requires (std::is_unsigned_v< dimension_t >)
	struct TextMetrics
	{
		dimension_t lineHeight{0};
		dimension_t lineSpace{0};
		dimension_t maxColumns{0};
		dimension_t maxRows{0};
	};

	/** @brief Static constant string containing placeholder text. */
	static constexpr auto LoremIpsum{
		"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed non risus. Suspendisse lectus tortor, dignissim sit amet, adipiscing nec, ultricies sed, dolor. Cras elementum ultrices diam. Maecenas ligula massa, varius a, semper congue, euismod non, mi. Proin porttitor, orci nec nonummy molestie, enim est eleifend mi, non fermentum diam nisl sit amet erat. Duis semper. Duis arcu massa, scelerisque vitae, consequat in, pretium a, enim. Pellentesque congue. Ut in risus volutpat libero pharetra tempor. Cras vestibulum bibendum augue. Praesent egestas leo in pede. Praesent blandit odio eu enim. Pellentesque sed dui ut augue blandit sodales. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Aliquam nibh. Mauris ac mauris sed pede pellentesque fermentum. Maecenas adipiscing ante non diam sodales hendrerit.\n"
		"Ut velit mauris, egestas sed, gravida nec, ornare ut, mi. Aenean ut orci vel massa suscipit pulvinar. Nulla sollicitudin. Fusce varius, ligula non tempus aliquam, nunc turpis ullamcorper nibh, in tempus sapien eros vitae ligula. Pellentesque rhoncus nunc et augue. Integer id felis. Curabitur aliquet pellentesque diam. Integer quis metus vitae elit lobortis egestas. Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Morbi vel erat non mauris convallis vehicula. Nulla et sapien. Integer tortor tellus, aliquam faucibus, convallis id, congue eu, quam. Mauris ullamcorper felis vitae erat. Proin feugiat, augue non elementum posuere, metus purus iaculis lectus, et tristique ligula justo vitae magna.\n"
		"Aliquam convallis sollicitudin purus. Praesent aliquam, enim at fermentum mollis, ligula massa adipiscing nisl, ac euismod nibh nisl eu lectus. Fusce vulputate sem at sapien. Vivamus leo. Aliquam euismod libero eu enim. Nulla nec felis sed leo placerat imperdiet. Aenean suscipit nulla in justo. Suspendisse cursus rutrum augue. Nulla tincidunt tincidunt mi. Curabitur iaculis, lorem vel rhoncus faucibus, felis magna fermentum augue, et ultricies lacus lorem varius purus. Curabitur eu amet.\n"
	};
}

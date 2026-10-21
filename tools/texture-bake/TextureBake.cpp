/*
 * tools/texture-bake/TextureBake.cpp
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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/* Local inclusions. */
#include "Graphics/AlphaCoverage.hpp"
#include "Graphics/ImageResource.hpp"
#include "Graphics/KTX2Writer.hpp"
#include "Graphics/TextureCompressor.hpp"
#include "TextureBake.hpp"
#include "IO/IO.hpp"
#include "PixelFactory/FileIO.hpp"
#include "PixelFactory/Pixmap.hpp"
#include "PixelFactory/Processor.hpp"
#include "PixelFactory/Types.hpp"

/*
 * EmeraudeTextureBake — bakes ONE texture of the canonical layout into a KTX2 container ready for a direct upload
 * (graphics § 39, the baked assets, owner decision 2026-10-21). It runs the runtime's own steps, so the baked blocks are
 * the ones the engine would have produced: PackRecipe::fillChannel() for every output channel (the largest source size,
 * a texture resample for the smaller ones), TextureCompressor::selectFormat() for the block format (BC5 for a normal map,
 * BC4 for an opaque grey linear map, BC7 otherwise), TextureCompressor::compress() for the full mip chain (Kaiser-filtered,
 * a colour in linear light, renormalized normals, coverage-preserving alpha masks), KTX2Writer for the container
 * (zstd-supercompressed).
 *
 * Usage:
 *   EmeraudeTextureBake --output <file.ktx2> --kind colour|normal|data [--flip-normal-y] [--zstd <0-22>]
 *                       [--r <source>] [--g <source>] [--b <source>] [--a <source>]
 *   <source> = <image file>:<r|g|b|a>  (one channel of an image)  or  #<0-255>  (a constant)
 *   EmeraudeTextureBake --data-directory <dir> (--material <name> | --materials <directory>) --output <dir> [--zstd <0-22>]
 *   The material mode (graphics § 40, L3; MaterialBake.cpp): the GPUTextures entries of a material definition, or of every
 *   definition under a directory of the Materials store, written to <output>/<TextureSet>.<Purpose>.ktx2.
 * A channel not given is 255 (D4: 1.0). --kind colour is sRGB. --flip-normal-y converts a Direct3D-style normal map
 * (+Y down) to the Khronos convention. The command line is a trust boundary: every value is checked, nothing is guessed.
 * ⚠️ Windows: the arguments are read narrow (the ANSI code page) — keep the paths ASCII there.
 */

namespace
{
	using namespace EmEn;
	using namespace EmEn::Base::PixelFactory;
	using EmEn::TextureBake::ChannelArgument;
	using EmEn::TextureBake::BakeArguments;

	/**
	 * @brief Parses a channel letter.
	 * @param letter The letter (r, g, b, a).
	 * @return std::optional< Channel >
	 */
	[[nodiscard]]
	std::optional< Channel >
	parseChannel (std::string_view letter) noexcept
	{
		if ( letter == "r" )
		{
			return Channel::Red;
		}

		if ( letter == "g" )
		{
			return Channel::Green;
		}

		if ( letter == "b" )
		{
			return Channel::Blue;
		}

		if ( letter == "a" )
		{
			return Channel::Alpha;
		}

		return std::nullopt;
	}

	/**
	 * @brief Parses a channel source: "<image>:<r|g|b|a>" or "#<0-255>".
	 * @param text The argument.
	 * @param argument The source to fill.
	 * @return bool
	 */
	[[nodiscard]]
	bool
	parseSource (std::string_view text, ChannelArgument & argument) noexcept
	{
		if ( text.starts_with('#') )
		{
			const auto digits = text.substr(1);

			if ( digits.empty() || digits.size() > 3 || digits.find_first_not_of("0123456789") != std::string_view::npos )
			{
				return false;
			}

			uint32_t value = 0;

			for ( const auto digit : digits )
			{
				value = (value * 10) + static_cast< uint32_t >(digit - '0');
			}

			if ( value > 255 )
			{
				return false;
			}

			argument.constant = static_cast< uint8_t >(value);
			argument.fromImage = false;

			return true;
		}

		const auto separator = text.rfind(':');

		if ( separator == std::string_view::npos || separator == 0 )
		{
			return false;
		}

		const auto channel = parseChannel(text.substr(separator + 1));

		if ( !channel.has_value() )
		{
			return false;
		}

		argument.image = Base::IO::u8path(std::string{text.substr(0, separator)});
		argument.channel = *channel;
		argument.fromImage = true;

		return true;
	}

	/**
	 * @brief Parses a zstd level (0 to 22).
	 * @param value The argument.
	 * @return std::optional< uint32_t >
	 */
	[[nodiscard]]
	std::optional< uint32_t >
	parseZstdLevel (std::string_view value) noexcept
	{
		if ( value.empty() || value.size() > 2 || value.find_first_not_of("0123456789") != std::string_view::npos )
		{
			return std::nullopt;
		}

		uint32_t level = 0;

		for ( const auto digit : value )
		{
			level = (level * 10) + static_cast< uint32_t >(digit - '0');
		}

		if ( level > 22 )
		{
			return std::nullopt;
		}

		return level;
	}

	/**
	 * @brief Applies one option and its value.
	 * @param option The option.
	 * @param value Its value.
	 * @param parsed The result.
	 * @return bool False, with the reason on the error stream.
	 */
	[[nodiscard]]
	bool
	applyOption (std::string_view option, std::string_view value, BakeArguments & parsed) noexcept
	{
		if ( option == "--output" )
		{
			parsed.output = Base::IO::u8path(std::string{value});

			return true;
		}

		if ( option == "--kind" )
		{
			if ( value != "colour" && value != "normal" && value != "data" )
			{
				std::cerr << "The kind '" << value << "' is not colour, normal or data.\n";

				return false;
			}

			parsed.colour = value == "colour";
			parsed.normal = value == "normal";
			parsed.kindGiven = true;

			return true;
		}

		if ( option == "--zstd" )
		{
			const auto level = parseZstdLevel(value);

			if ( !level.has_value() )
			{
				std::cerr << "The zstd level '" << value << "' is not a number from 0 to 22.\n";

				return false;
			}

			parsed.zstdLevel = *level;

			return true;
		}

		if ( option == "--r" || option == "--g" || option == "--b" || option == "--a" )
		{
			const auto output = parseChannel(option.substr(2));

			/* NOTE: The output channel is one of the four letters parseChannel() accepts: index 0 to 3. */
			if ( !output.has_value() || !parseSource(value, *std::next(parsed.channels.begin(), static_cast< std::ptrdiff_t >(*output))) )
			{
				std::cerr << "The source '" << value << "' of " << option << " is neither <image>:<r|g|b|a> nor #<0-255>.\n";

				return false;
			}

			return true;
		}

		std::cerr << "Unknown option '" << option << "'.\n";

		return false;
	}

	/**
	 * @brief Parses the command line.
	 * @param arguments The command line, without the program name.
	 * @param parsed The result.
	 * @return bool False, with the reason on the error stream.
	 */
	[[nodiscard]]
	bool
	parseArguments (std::span< const std::string_view > arguments, BakeArguments & parsed) noexcept
	{
		for ( size_t index = 0; index < arguments.size(); ++index )
		{
			const auto option = arguments[index];

			if ( option == "--flip-normal-y" )
			{
				parsed.flipNormalY = true;

				continue;
			}

			if ( index + 1 >= arguments.size() )
			{
				std::cerr << "The option '" << option << "' needs a value (or is unknown).\n";

				return false;
			}

			if ( !applyOption(option, arguments[++index], parsed) )
			{
				return false;
			}
		}

		if ( parsed.output.empty() || !parsed.kindGiven )
		{
			std::cerr << "--output and --kind are mandatory.\n";

			return false;
		}

		if ( parsed.flipNormalY && !parsed.normal )
		{
			std::cerr << "--flip-normal-y only applies to --kind normal.\n";

			return false;
		}

		return true;
	}

	/**
	 * @brief Reads an image as RGBA8.
	 * @param filepath The image file.
	 * @param pixmap The pixels.
	 * @return bool
	 */
	[[nodiscard]]
	bool
	readImage (const std::filesystem::path & filepath, Pixmap< uint8_t > & pixmap) noexcept
	{
		Pixmap< uint8_t > decoded;

		if ( !FileIO::read(filepath, decoded) || !decoded.isValid() )
		{
			std::cerr << "Unable to read the image '" << Base::IO::toU8String(filepath) << "'.\n";

			return false;
		}

		pixmap = decoded.colorCount() == 4 ? std::move(decoded) : Processor< uint8_t >::toRGBA(decoded);

		return pixmap.isValid();
	}
}

namespace EmEn::TextureBake
{
	bool
	bakeTexture (const BakeArguments & arguments, const Graphics::TextureCompressor & compressor) noexcept
	{
		/* Every distinct image read once; the packed size is the largest one (D12). */
		std::vector< std::pair< std::filesystem::path, Pixmap< uint8_t > > > images;
		uint32_t width = 0;
		uint32_t height = 0;

		for ( const auto & channel : arguments.channels )
		{
			if ( !channel.fromImage || std::ranges::any_of(images, [&channel] (const auto & entry) { return entry.first == channel.image; }) )
			{
				continue;
			}

			Pixmap< uint8_t > pixmap;

			if ( !readImage(channel.image, pixmap) )
			{
				return false;
			}

			width = std::max(width, static_cast< uint32_t >(pixmap.width()));
			height = std::max(height, static_cast< uint32_t >(pixmap.height()));
			images.emplace_back(channel.image, std::move(pixmap));
		}

		if ( images.empty() )
		{
			std::cerr << "No channel reads an image: nothing to bake.\n";

			return false;
		}

		Pixmap< uint8_t > packed;

		if ( !packed.initialize(width, height, ChannelMode::RGBA) )
		{
			std::cerr << "Unable to allocate the " << width << "x" << height << " packed image.\n";

			return false;
		}

		uint8_t outputIndex = 0;

		for ( const auto & channel : arguments.channels )
		{
			const auto index = outputIndex++;
			const Pixmap< uint8_t > * source = nullptr;

			if ( channel.fromImage )
			{
				/* NOTE: Every image a channel reads was read above: the search always finds it. */
				const auto entry = std::ranges::find_if(images, [&channel] (const auto & candidate) { return candidate.first == channel.image; });
				source = &entry->second;
			}

			if ( !Graphics::PackRecipe::fillChannel(packed, static_cast< Channel >(index), source, channel.channel, channel.constant) )
			{
				std::cerr << "Unable to fill the output channel " << static_cast< unsigned int >(index) << ".\n";

				return false;
			}
		}

		if ( arguments.flipNormalY )
		{
			packed.flipNormalMapY();
		}

		const auto format = Graphics::TextureCompressor::selectFormat(packed, arguments.colour, arguments.normal);
		/* NOTE: Measured on the pixels the source path would have measured: the container cannot be inspected once
		 * written, so it records the answer (graphics § 40, L6). Only BC7 keeps an alpha: a BC5 / BC4 container never
		 * claims one. */
		const auto binaryAlphaMask = format == Graphics::BlockFormat::BC7 && Graphics::AlphaCoverage::isBinaryMask(packed);
		const auto levels = compressor.compress(packed, 0, format, arguments.colour);

		if ( levels.empty() )
		{
			std::cerr << "Unable to compress the packed image.\n";

			return false;
		}

		/* NOTE: The output's directories are created (a texture set may live in sub-directories). */
		if ( !Base::IO::createDirectory(arguments.output, true) )
		{
			std::cerr << "Unable to create the directory of '" << Base::IO::toU8String(arguments.output) << "'.\n";

			return false;
		}

		if ( !Graphics::KTX2Writer::write(arguments.output, Graphics::TextureCompressor::vulkanFormat(format, arguments.colour), levels, arguments.zstdLevel, arguments.fingerprint, binaryAlphaMask) )
		{
			return false;
		}

		std::cout << Base::IO::toU8String(arguments.output) << ' ' << Graphics::to_cstring(format) << ' ' << width << 'x' << height << ' ' << levels.size() << " levels" << (binaryAlphaMask ? ", alpha: binary coverage mask" : "") << '\n';

		return true;
	}
}

int
main (int argc, char * argv[])
{
	/* NOTE: The program name is skipped; argc counts it (and is at least 1 on every platform this runs on). */
	const std::span< char * > argumentValues{argv, static_cast< size_t >(std::max(argc, 1))};
	std::vector< std::string_view > commandLine;
	commandLine.reserve(argumentValues.size() - 1);

	for ( const auto * argument : argumentValues.subspan(1) )
	{
		commandLine.emplace_back(argument);
	}

	Graphics::TextureCompressor compressor;

	if ( !compressor.initialize() )
	{
		std::cerr << "Unable to initialize the block encoders.\n";

		return EXIT_FAILURE;
	}

	/* The material mode (graphics § 40, L3). */
	if ( std::ranges::any_of(commandLine, [] (std::string_view argument) { return argument == "--material" || argument == "--materials"; }) )
	{
		return TextureBake::runMaterialMode(commandLine, compressor);
	}

	BakeArguments arguments;

	if ( !parseArguments(commandLine, arguments) )
	{
		std::cerr << "Usage: EmeraudeTextureBake --output <file.ktx2> --kind colour|normal|data [--flip-normal-y] [--zstd <0-22>] [--r|--g|--b|--a <image>:<r|g|b|a> | #<0-255>]\n"
			"       EmeraudeTextureBake --data-directory <dir> (--material <name> | --materials <directory>) --output <dir> [--zstd <0-22>]\n";

		return EXIT_FAILURE;
	}

	return TextureBake::bakeTexture(arguments, compressor) ? EXIT_SUCCESS : EXIT_FAILURE;
}

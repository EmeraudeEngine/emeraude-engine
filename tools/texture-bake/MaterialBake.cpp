/*
 * tools/texture-bake/MaterialBake.cpp
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

#include "TextureBake.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

/* Local inclusions. */
#include "FastJSON.hpp"
#include "Graphics/Material/Component/Texture.hpp"
#include "Graphics/TexturePacker.hpp"
#include "Graphics/Types.hpp"
#include "IO/IO.hpp"
#include "PixelFactory/TextureName.hpp"
#include "PixelFactory/Types.hpp"

/*
 * The material mode of EmeraudeTextureBake (graphics § 40, L3): reads a material definition as the engine does, groups
 * its maps with the engine's own rule (Graphics::TexturePacker::plan(): the maps of a canonical texture, two at least,
 * read at the same UVs), and bakes one GPUTextures entry per group or lone map, named `<TextureSet>.<Purpose>.ktx2` — the
 * purpose listing ONLY the components the file serves, compacted on consecutive channels from R (a value component is
 * never baked: the engine keeps it a value). Each name is checked with Base::PixelFactory::TextureName before writing.
 * A dev tool: it writes where --output says; the engine itself never writes its data stores (only its cache).
 */

namespace
{
	using namespace EmEn;
	using namespace EmEn::Base::PixelFactory;
	using EmEn::Graphics::ComponentType;
	using EmEn::TextureBake::BakeArguments;

	/** @brief The material mode's command line. */
	struct MaterialArguments final
	{
		std::filesystem::path dataDirectory;
		std::filesystem::path output;
		std::string material;
		std::string materials;
		uint32_t zstdLevel{18};
		bool batch{false};
	};

	/** @brief A map of a definition the bake takes: its JSON key, its role, the purpose that serves it. */
	struct BakeMap final
	{
		const char * key{nullptr};
		ComponentType role{ComponentType::Invalid};
		TexturePurpose purpose{TexturePurpose::Color};
	};

	/* NOTE: The maps GPUTextures serves (graphics § 40, the same as StandardResource::planGPUTextures()). */
	constexpr std::array< BakeMap, 8 > BakeMaps{{
		{.key = Graphics::AlbedoString, .role = ComponentType::Albedo, .purpose = TexturePurpose::Color},
		{.key = Graphics::OpacityString, .role = ComponentType::Opacity, .purpose = TexturePurpose::Opacity},
		{.key = Graphics::NormalString, .role = ComponentType::Normal, .purpose = TexturePurpose::NormalGL},
		{.key = Graphics::AmbientOcclusionString, .role = ComponentType::AmbientOcclusion, .purpose = TexturePurpose::AmbientOcclusion},
		{.key = Graphics::RoughnessString, .role = ComponentType::Roughness, .purpose = TexturePurpose::Roughness},
		{.key = Graphics::MetalnessString, .role = ComponentType::Metalness, .purpose = TexturePurpose::Metalness},
		{.key = Graphics::HeightString, .role = ComponentType::Displacement, .purpose = TexturePurpose::Displacement},
		{.key = Graphics::AutoIlluminationString, .role = ComponentType::AutoIllumination, .purpose = TexturePurpose::Emission}
	}};

	/** @brief The image extensions the Images store holds (Base::PixelFactory::FileIO). */
	constexpr std::array< std::string_view, 7 > ImageExtensions{".png", ".jpg", ".jpeg", ".tga", ".webp", ".tif", ".tiff"};

	/** @brief A map read from a definition: where its pixels come from and where it is read. */
	struct ReadMap final
	{
		const BakeMap * map{nullptr};
		std::filesystem::path image;
		Channel channel{Channel::Red};
		std::string samplingKey;
		bool flipNormalY{false};
		bool enableAlpha{false};
	};

	/**
	 * @brief Finds the file of an Images store resource name.
	 * @param imagesDirectory The Images store directory.
	 * @param name The resource name (no extension).
	 * @return std::optional< std::filesystem::path >
	 */
	[[nodiscard]]
	std::optional< std::filesystem::path >
	findImage (const std::filesystem::path & imagesDirectory, const std::string & name) noexcept
	{
		for ( const auto extension : ImageExtensions )
		{
			auto candidate = imagesDirectory / Base::IO::u8path(name + std::string{extension});

			if ( Base::IO::fileExists(candidate) )
			{
				return candidate;
			}
		}

		return std::nullopt;
	}

	/**
	 * @brief Reads the map of a definition key, when it is declared as a 2D texture.
	 * @param definition The material definition.
	 * @param map The map.
	 * @param imagesDirectory The Images store directory.
	 * @param read The map read.
	 * @return std::optional< bool > Nothing when the key is not a texture; false on an error (written to the error
	 * stream), true when read.
	 */
	[[nodiscard]]
	std::optional< bool >
	readMap (const Json::Value & definition, const BakeMap & map, const std::filesystem::path & imagesDirectory, ReadMap & read) noexcept
	{
		using Graphics::Material::Component::Texture;

		if ( !definition.isMember(map.key) || !definition[map.key].isObject() )
		{
			return std::nullopt;
		}

		const auto & block = definition[map.key];

		if ( Base::FastJSON::getValue< std::string >(block, "Type").value_or(std::string{}) != Graphics::TextureString || !block.isMember("Data") || !block["Data"].isObject() )
		{
			return std::nullopt;
		}

		const auto & data = block["Data"];
		const auto name = Base::FastJSON::getValue< std::string >(data, Texture::JKResourceName);

		if ( !name.has_value() )
		{
			std::cerr << "  " << map.key << ": no image name.\n";

			return false;
		}

		const auto image = findImage(imagesDirectory, *name);

		if ( !image.has_value() )
		{
			std::cerr << "  " << map.key << ": the image '" << *name << "' is not in the Images store.\n";

			return false;
		}

		read.map = &map;
		read.image = *image;

		if ( data.isMember(Texture::JKSourceChannel) )
		{
			const auto channel = Base::FastJSON::asValue< uint32_t >(data[Texture::JKSourceChannel]);

			if ( !channel.has_value() || *channel > 3 )
			{
				std::cerr << "  " << map.key << ": its SourceChannel is not 0 to 3.\n";

				return false;
			}

			read.channel = static_cast< Channel >(*channel);
		}

		/* NOTE: What decides where the map is read (D11), as the engine's planner keys it. */
		Json::Value sampling{Json::objectValue};

		for ( const auto * samplingKey : {Texture::JKChannel, Texture::JKUVWScale, Texture::JKUVWOffset} )
		{
			if ( data.isMember(samplingKey) )
			{
				sampling[samplingKey] = data[samplingKey];
			}
		}

		read.samplingKey = Base::FastJSON::stringify(sampling);
		read.flipNormalY = Base::FastJSON::getValue< bool >(data, Texture::JKFlipNormalMapY).value_or(false);
		read.enableAlpha = Base::FastJSON::getValue< bool >(data, Texture::JKEnableAlpha).value_or(false);

		return true;
	}

	/**
	 * @brief Takes the RGB of an image into the first three output channels.
	 * @param image The image.
	 * @param arguments The texture to fill.
	 */
	void
	setColour (const std::filesystem::path & image, BakeArguments & arguments) noexcept
	{
		for ( const auto channel : {Channel::Red, Channel::Green, Channel::Blue} )
		{
			std::span{arguments.channels}[static_cast< size_t >(channel)] = {.image = image, .channel = channel, .constant = 255, .fromImage = true};
		}
	}

	/**
	 * @brief Takes one channel of an image into an output channel; a grey (one-component) file repeats it in R, G and B
	 * so the block format is BC4 (TextureCompressor::selectFormat() reads the content).
	 * @param read The map.
	 * @param output The output channel.
	 * @param alone Whether the file holds this map only.
	 * @param arguments The texture to fill.
	 */
	void
	setChannel (const ReadMap & read, size_t output, bool alone, BakeArguments & arguments) noexcept
	{
		const auto channels = std::span{arguments.channels};

		if ( alone )
		{
			for ( size_t index = 0; index < 3; ++index )
			{
				channels[index] = {.image = read.image, .channel = read.channel, .constant = 255, .fromImage = true};
			}

			return;
		}

		channels[output] = {.image = read.image, .channel = read.channel, .constant = 255, .fromImage = true};
	}

	/** @brief One GPUTextures entry to bake: its purpose (the tokens) and its texture. */
	struct PlannedEntry final
	{
		std::string purpose;
		BakeArguments bake;
	};

	/**
	 * @brief Plans the entries of a material: the engine's grouping (TexturePacker::plan()) for the packable maps, one
	 * entry per lone map.
	 * @param maps The maps read from the definition.
	 * @param owner The material's name, for the logs.
	 * @param zstdLevel The zstd level of the containers.
	 * @return std::vector< PlannedEntry >
	 */
	[[nodiscard]]
	std::vector< PlannedEntry >
	planEntries (std::span< const ReadMap > maps, std::string_view owner, uint32_t zstdLevel) noexcept
	{
		std::vector< PlannedEntry > entries;

		Base::StaticVector< Graphics::PackCandidate, Graphics::TexturePacker::MaxRoles > candidates;

		for ( const auto & read : maps )
		{
			const auto role = read.map->role;

			/* NOTE: In capacity: six packable roles, each read once. */
			if ( (role == ComponentType::Albedo || role == ComponentType::Opacity || role == ComponentType::AmbientOcclusion || role == ComponentType::Roughness || role == ComponentType::Metalness || role == ComponentType::Displacement) && !candidates.full() )
			{
				candidates.push_back({.role = role, .samplingKey = read.samplingKey});
			}
		}

		const auto plans = Graphics::TexturePacker::plan({candidates.data(), candidates.size()}, owner);

		const auto findMap = [maps] (ComponentType role) -> const ReadMap * {
			const auto found = std::ranges::find_if(maps, [role] (const ReadMap & read) { return read.map->role == role; });

			return found == maps.end() ? nullptr : &*found;
		};

		/* The canonical textures: the planned roles, compacted on consecutive channels from R. */
		for ( const auto & plan : plans )
		{
			PlannedEntry entry;
			entry.bake.zstdLevel = zstdLevel;
			entry.bake.colour = plan.sRGB;
			entry.bake.kindGiven = true;

			size_t channel = 0;

			for ( const auto role : plan.roles )
			{
				const auto * read = findMap(role);

				if ( read == nullptr )
				{
					continue;
				}

				if ( !entry.purpose.empty() )
				{
					entry.purpose += '-';
				}

				entry.purpose += to_cstring(read->map->purpose);

				if ( role == ComponentType::Albedo )
				{
					setColour(read->image, entry.bake);
					channel = 3;

					continue;
				}

				setChannel(*read, channel++, false, entry.bake);
			}

			entries.push_back(std::move(entry));
		}

		/* The lone maps. */
		for ( const auto & read : maps )
		{
			if ( std::ranges::any_of(plans, [&read] (const Graphics::PackPlan & plan) { return std::ranges::find(plan.roles, read.map->role) != plan.roles.end(); }) )
			{
				continue;
			}

			PlannedEntry entry;
			entry.purpose = to_cstring(read.map->purpose);
			entry.bake.zstdLevel = zstdLevel;
			entry.bake.kindGiven = true;

			switch ( read.map->purpose )
			{
				case TexturePurpose::Color :
				case TexturePurpose::Emission :
					entry.bake.colour = true;
					setColour(read.image, entry.bake);
					break;

				case TexturePurpose::NormalGL :
				case TexturePurpose::NormalDX :
					entry.bake.normal = true;
					entry.bake.flipNormalY = read.flipNormalY;
					setColour(read.image, entry.bake);
					break;

				case TexturePurpose::Opacity :
				case TexturePurpose::Roughness :
				case TexturePurpose::Metalness :
				case TexturePurpose::AmbientOcclusion :
				case TexturePurpose::Displacement :
					setChannel(read, 0, true, entry.bake);
					break;
			}

			entries.push_back(std::move(entry));
		}

		return entries;
	}

	/**
	 * @brief Bakes the GPUTextures entries of one material definition.
	 * @param arguments The material mode's command line.
	 * @param materialName The material's resource name (its path in the Materials store, no extension).
	 * @param compressor An initialized compressor.
	 * @return bool False on an error (written to the error stream).
	 */
	[[nodiscard]]
	bool
	bakeMaterial (const MaterialArguments & arguments, const std::string & materialName, const Graphics::TextureCompressor & compressor) noexcept
	{
		const auto stores = arguments.dataDirectory / "data-stores";
		const auto definitionPath = stores / "Materials" / Base::IO::u8path(materialName + ".json");
		const auto definition = Base::FastJSON::getRootFromFile(definitionPath);

		std::cout << materialName << ":\n";

		if ( !definition.has_value() || !definition->isObject() )
		{
			std::cerr << "  unable to read the definition '" << Base::IO::toU8String(definitionPath) << "'.\n";

			return false;
		}

		/* The set: the "TextureSet" key, else the material's name (graphics § 40, T4). */
		auto setName = materialName;

		if ( definition->isMember("TextureSet") )
		{
			const auto declared = Base::FastJSON::getValue< std::string >(*definition, "TextureSet");

			if ( !declared.has_value() || declared->empty() )
			{
				std::cerr << "  its TextureSet is not a non-empty string.\n";

				return false;
			}

			setName = *declared;
		}

		std::vector< ReadMap > maps;
		const auto imagesDirectory = stores / "Images";

		for ( const auto & map : BakeMaps )
		{
			ReadMap read;
			const auto result = readMap(*definition, map, imagesDirectory, read);

			if ( !result.has_value() )
			{
				continue;
			}

			if ( !*result )
			{
				return false;
			}

			maps.push_back(std::move(read));
		}

		/* NOTE: An albedo read with its alpha (EnableAlpha) carries its opacity: the entry must keep that A. */
		const auto albedo = std::ranges::find_if(maps, [] (const ReadMap & read) { return read.map->role == ComponentType::Albedo; });
		const auto hasOpacity = std::ranges::any_of(maps, [] (const ReadMap & read) { return read.map->role == ComponentType::Opacity; });

		if ( albedo != maps.end() && albedo->enableAlpha && !hasOpacity )
		{
			const auto * opacityMap = &*std::ranges::find(BakeMaps, ComponentType::Opacity, &BakeMap::role);
			auto opacity = *albedo;
			opacity.map = opacityMap;
			opacity.channel = Channel::Alpha;
			maps.push_back(std::move(opacity));
		}

		if ( maps.empty() )
		{
			std::cout << "  no map declared as a texture: nothing to bake.\n";

			return true;
		}

		for ( auto & entry : planEntries(maps, materialName, arguments.zstdLevel) )
		{
			const auto entryName = setName + "." + entry.purpose;
			const auto checked = TextureName::parse(entryName, TextureStore::GPU);

			if ( !checked.isValid() )
			{
				std::cerr << "  the entry name '" << entryName << "' is refused: " << to_cstring(checked.error()) << ".\n";

				return false;
			}

			entry.bake.output = arguments.output / Base::IO::u8path(entryName + ".ktx2");

			std::cout << "  ";

			if ( !TextureBake::bakeTexture(entry.bake, compressor) )
			{
				return false;
			}
		}

		return true;
	}

	/**
	 * @brief Parses the material mode's command line.
	 * @param commandLine The command line, without the program name.
	 * @param parsed The result.
	 * @return bool False, with the reason on the error stream.
	 */
	[[nodiscard]]
	bool
	parseMaterialArguments (std::span< const std::string_view > commandLine, MaterialArguments & parsed) noexcept
	{
		for ( size_t index = 0; index + 1 < commandLine.size(); index += 2 )
		{
			const auto option = commandLine[index];
			const auto value = commandLine[index + 1];

			if ( option == "--data-directory" )
			{
				parsed.dataDirectory = Base::IO::u8path(std::string{value});
			}
			else if ( option == "--output" )
			{
				parsed.output = Base::IO::u8path(std::string{value});
			}
			else if ( option == "--material" )
			{
				parsed.material = std::string{value};
			}
			else if ( option == "--materials" )
			{
				parsed.materials = std::string{value};
				parsed.batch = true;
			}
			else if ( option == "--zstd" )
			{
				uint32_t level = 0;
				const auto digits = !value.empty() && value.size() <= 2 && value.find_first_not_of("0123456789") == std::string_view::npos;

				for ( const auto digit : value )
				{
					level = (level * 10) + static_cast< uint32_t >(digit - '0');
				}

				if ( !digits || level > 22 )
				{
					std::cerr << "The zstd level '" << value << "' is not a number from 0 to 22.\n";

					return false;
				}

				parsed.zstdLevel = level;
			}
			else
			{
				std::cerr << "Unknown option '" << option << "' in the material mode.\n";

				return false;
			}
		}

		if ( commandLine.size() % 2 != 0 )
		{
			std::cerr << "Every option of the material mode takes a value.\n";

			return false;
		}

		if ( parsed.dataDirectory.empty() || parsed.output.empty() || (parsed.material.empty() == !parsed.batch) )
		{
			std::cerr << "--data-directory, --output and one of --material / --materials are mandatory.\n";

			return false;
		}

		return true;
	}
}

namespace EmEn::TextureBake
{
	int
	runMaterialMode (std::span< const std::string_view > arguments, const Graphics::TextureCompressor & compressor) noexcept
	{
		MaterialArguments parsed;

		if ( !parseMaterialArguments(arguments, parsed) )
		{
			std::cerr << "Usage: EmeraudeTextureBake --data-directory <dir> (--material <name> | --materials <directory>) --output <dir> [--zstd <0-22>]\n";

			return EXIT_FAILURE;
		}

		if ( !parsed.batch )
		{
			return bakeMaterial(parsed, parsed.material, compressor) ? EXIT_SUCCESS : EXIT_FAILURE;
		}

		/* Every definition under the directory of the Materials store, recursively. */
		const auto materialsStore = parsed.dataDirectory / "data-stores" / "Materials";
		const auto directory = materialsStore / Base::IO::u8path(parsed.materials);
		std::vector< std::string > names;

		const auto walked = Base::IO::forEachDirectoryEntry(directory, true, [&names, &materialsStore] (const std::filesystem::directory_entry & entry) {
			std::error_code error;

			if ( entry.is_regular_file(error) && entry.path().extension() == ".json" )
			{
				auto relative = entry.path().lexically_relative(materialsStore);
				relative.replace_extension();
				names.emplace_back(Base::IO::toGenericU8String(relative));
			}

			return true;
		});

		if ( !walked )
		{
			std::cerr << "Unable to walk the directory '" << Base::IO::toU8String(directory) << "'.\n";

			return EXIT_FAILURE;
		}

		std::ranges::sort(names);

		size_t failed = 0;

		for ( const auto & name : names )
		{
			if ( !bakeMaterial(parsed, name, compressor) )
			{
				++failed;
			}
		}

		std::cout << names.size() << " material(s), " << (names.size() - failed) << " baked, " << failed << " failed.\n";

		return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
	}
}

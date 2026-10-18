/*
 * src/Scenes/Loaders/FBXLoader.hpp
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

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>
#include <vector>

/* Local inclusions for inheritances. */
#include "Interface.hpp"

/* Local inclusions for usages. */
#include "Resources/Manager.hpp"

/* Forward declarations. */
namespace EmEn::Animations
{
	class SkeletonResource;
	class AnimationClipResource;
}

namespace EmEn::Scenes::Loaders
{
	/**
	 * @brief Loads FBX composite assets into engine resource containers.
	 * @note Produces an SceneData with format-agnostic node descriptors.
	 * Uses ufbx (vendored) for parsing. No dependency on Scenes/ types.
	 * The conversion itself is FBX::analyse() (FBXParser.hpp, fuzzed): this class opens the file and
	 * materializes the analysed asset into engine resources.
	 */
	class EMEN_API FBXLoader final : public Interface
	{
		public:

			static constexpr auto ClassId{"FBXLoader"};

			/**
			 * @brief Constructs the loader with access to the resource manager.
			 * @param resources A reference to the engine resource manager.
			 */
			explicit
			FBXLoader (Resources::Manager & resources) noexcept
				: m_resources{resources}
			{

			}

			/** @copydoc EmEn::Scenes::Loaders::Interface::load() */
			[[nodiscard]]
			bool load (const std::filesystem::path & filepath, SceneData & output) noexcept override;

			/** @copydoc EmEn::Scenes::Loaders::Interface::supportsExtension() */
			[[nodiscard]]
			bool
			supportsExtension (std::string_view extension) const noexcept override
			{
				return extension == ".fbx";
			}

			/**
			 * @copydoc EmEn::Scenes::Loaders::Interface::capabilities()
			 * @note FBX carries lights and cameras; this loader does not read them yet. The mask
			 * states what is DELIVERED, so they stay out until the code exists.
			 */
			[[nodiscard]]
			uint32_t
			capabilities () const noexcept override
			{
				return Geometry | Skinning | Animations;
			}

			/** @copydoc EmEn::Scenes::Loaders::Interface::loadAnimationClipsOnly() */
			[[nodiscard]]
			bool loadAnimationClipsOnly (const std::filesystem::path & filepath, const Animations::SkeletonResource & targetSkeleton, std::vector< std::shared_ptr< Animations::AnimationClipResource > > & output) noexcept override;

		private:

			/**
			 * @brief Re-reads the encoded bytes of a texture embedded in an FBX (an image's EncodedSource: ufbx gives no
			 * file offset for embedded content), the FBX parsed without geometry, animation or external files.
			 * @param filepath The FBX file.
			 * @param index The texture's index in the scene.
			 * @param bytes The encoded bytes.
			 * @return bool
			 */
			[[nodiscard]]
			static bool readEmbeddedTexture (const std::filesystem::path & filepath, uint32_t index, std::vector< std::byte > & bytes) noexcept;

			Resources::Manager & m_resources;
	};
}

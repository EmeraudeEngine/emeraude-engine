/*
 * src/SceneLoaders/Interface.hpp
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
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>
#include <vector>

/* Local inclusions for usages. */
#include "LoaderOptions.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Animations
	{
		class SkeletonResource;
		class AnimationClipResource;
	}

	namespace SceneLoaders
	{
		struct SceneData;
	}
}

namespace EmEn::SceneLoaders
{
	/**
	 * @brief Declares what a loader implementation actually delivers in a SceneData.
	 * @note This describes THE LOADER, not the file format. FBX can carry lights and cameras;
	 * our FBX loader does not read them, so it must not advertise them. A caller uses this to
	 * decide how to treat the result — for instance whether to light the scene itself —
	 * WITHOUT hard-coding format names or probing the produced SceneData for emptiness, which
	 * cannot distinguish "the loader ignores lights" from "this asset declares none".
	 */
	enum EMEN_API LoaderCapabilityBits : uint32_t
	{
		None = 0U,
		/** Meshes, their materials and the node hierarchy. */
		Geometry = 1U << 0,
		/** Skeletons and per-vertex skinning data. */
		Skinning = 1U << 1,
		/** Animation clips embedded in the asset. */
		Animations = 1U << 2,
		/** Punctual lights, in the photometric units of LightDescriptor. */
		Lights = 1U << 3,
		/** Authored camera viewpoints. */
		Cameras = 1U << 4
	};

	/**
	 * @brief Common interface for composite asset format loaders.
	 * @note Implementations load resources into engine containers and produce
	 * a format-agnostic SceneData describing the node hierarchy.
	 * No dependency on Scenes/ types.
	 */
	class EMEN_API Interface
	{
		public:

			virtual ~Interface () = default;

			/**
			 * @brief Sets loader options.
			 * @param options The options to apply.
			 */
			void
			setOptions (LoaderOptions options) noexcept
			{
				m_options = std::move(options);
			}

			/**
			 * @brief Returns the loader options.
			 * @return const LoaderOptions &
			 */
			[[nodiscard]]
			const LoaderOptions &
			getOptions () noexcept
			{
				return m_options;
			}

			/**
			 * @brief Loads a composite asset from a file.
			 * @param filepath Path to the asset file.
			 * @param output The SceneData to populate with loaded resources and hierarchy.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool load (const std::filesystem::path & filepath, SceneData & output) noexcept = 0;

			/**
			 * @brief Checks if this loader supports the given file extension.
			 * @param extension The file extension (e.g., ".gltf", ".glb").
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool supportsExtension (std::string_view extension) const noexcept = 0;

			/**
			 * @brief Returns what this loader actually produces, as a mask of LoaderCapabilityBits.
			 * @note Pure virtual on purpose: a loader that gains or loses a capability MUST
			 * restate it here, and a new loader cannot silently inherit a lie.
			 * @return uint32_t
			 */
			[[nodiscard]]
			virtual uint32_t capabilities () const noexcept = 0;

			/**
			 * @brief Loads animation clips from an asset file and binds them to an existing skeleton.
			 * @note Used to attach motion data exported as standalone files (Mixamo split animations,
			 * Maya/Blender per-action exports). Bones are resolved by name against @a targetSkeleton —
			 * channels referencing missing joints are silently dropped.
			 * @note The default implementation returns false. Loaders must override to opt in.
			 * @param filepath Path to the animation-only asset file.
			 * @param targetSkeleton The skeleton whose joint names drive bone resolution.
			 * @param output Vector to which the produced clip resources are appended.
			 * @return bool True if at least one clip was produced.
			 */
			[[nodiscard]]
			virtual
			bool
			loadAnimationClipsOnly (const std::filesystem::path & filepath, const Animations::SkeletonResource & targetSkeleton, std::vector< std::shared_ptr< Animations::AnimationClipResource > > & output) noexcept
			{
				(void)filepath;
				(void)targetSkeleton;
				(void)output;

				return false;
			}

		protected:

			Interface () noexcept = default;

			Interface (const Interface &) = default;

			Interface (Interface &&) = default;

			Interface & operator= (const Interface &) = default;

			Interface & operator= (Interface &&) = default;

		public:

			LoaderOptions m_options;
	};
}

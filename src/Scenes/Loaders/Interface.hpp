/*
 * src/Scenes/Loaders/Interface.hpp
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
#include <future>
#include <memory>
#include <string>
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

	namespace Scenes::Loaders
	{
		struct SceneData;
	}
}

namespace EmEn::Scenes::Loaders
{
	/**
	 * @brief Builds a resource container key that cannot alias another one from the same asset.
	 * @warning ⚠⚠ THE ASSET NAME IS NOT AN IDENTITY. Neither glTF nor FBX imposes any
	 * uniqueness on the names they carry — the identity of a mesh, a material, a texture or an
	 * image is its INDEX in the asset. A resource container keyed on the name alone returns the
	 * FIRST homonym to every later caller, silently: the second mesh named 'Sphere' gets the
	 * first one's geometry AND its material, and nothing is reported.
	 * @note Measured on the Khronos conformance assets (2026-08-28): ClearCoatTest carries
	 * eighteen meshes all named 'ClearCoatSampleMesh' with eighteen different materials, so the
	 * whole grid rendered material 0's red; MetalRoughSpheresNoTextures has ninety-eight named
	 * 'Sphere'; SpecularTest twenty named 'OneSample'; TransmissionTest twelve named 'Sphere' plus
	 * three genuinely different materials all named 'BlueTransWithMask'; TransmissionRoughnessTest
	 * two different images both named 'RoughnessGrid'.
	 * @param prefix The loader's per-asset resource prefix (already ends with a separator).
	 * @param category The resource category segment, ending with '/' (e.g. "Mesh/").
	 * @param assetName The name the asset declares, possibly empty and possibly a duplicate.
	 * @param assetIndex The index of the item in its asset array — the real identity.
	 * @return std::string
	 */
	[[nodiscard]]
	inline
	std::string
	buildResourceKey (const std::string & prefix, std::string_view category, std::string_view assetName, size_t assetIndex) noexcept
	{
		const auto indexString = std::to_string(assetIndex);

		std::string key;
		key.reserve(prefix.size() + category.size() + assetName.size() + indexString.size() + 1);
		key = prefix;
		key += category;

		/* An unnamed item keeps the bare index it always had. */
		if ( !assetName.empty() )
		{
			key += assetName;
			key += '-';
		}

		key += indexString;

		return key;
	}

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
		Geometry = 1U << 0U,
		/** Skeletons and per-vertex skinning data. */
		Skinning = 1U << 1U,
		/** Animation clips embedded in the asset. */
		Animations = 1U << 2U,
		/** Punctual lights, in the photometric units of LightDescriptor. */
		Lights = 1U << 3U,
		/** Authored camera viewpoints. */
		Cameras = 1U << 4U
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
			 * @brief Loads a composite asset from a file, on a worker thread.
			 *
			 * @param filepath Path to the asset file.
			 * @param output The SceneData to populate. It MUST outlive the returned future, and
			 * must not be touched until that future is ready.
			 * @return std::future< bool >
			 *
			 * @note WHY THIS EXISTS. A synchronous load owns the calling thread for as long as the
			 * asset takes — minutes, on a 1.8 GB USD stage. When that thread is the MAIN one, the
			 * window event loop stops being pumped, and a Wayland compositor closes an application
			 * that stops answering its pings. The window dies mid-load; the app only finds out once
			 * the load returns, and tears the stage down immediately. The 60 s swap-chain
			 * acquisition timeout that follows is the last consequence, not the fault — raising it
			 * changes nothing.
			 *
			 * ⚠️ A DEDICATED THREAD, deliberately, and NOT the resource manager's thread pool:
			 * load() creates resources through getOrCreateResource(), whose factories run ON that
			 * pool and are waited upon. Occupying a pool slot for the whole load would invite a
			 * deadlock against the very tasks it is waiting for.
			 *
			 * ⚠️ The caller is responsible for keeping the main loop alive while this runs — that
			 * is the entire point. Pumping window events belongs to the caller; a loader must know
			 * nothing about windowing.
			 */
			[[nodiscard]]
			std::future< bool >
			loadAsync (const std::filesystem::path & filepath, SceneData & output) noexcept
			{
				return std::async(std::launch::async, [this, filepath, &output] () {
					return this->load(filepath, output);
				});
			}

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

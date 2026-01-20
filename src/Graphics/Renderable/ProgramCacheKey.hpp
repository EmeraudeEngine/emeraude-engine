/*
 * src/Graphics/Renderable/ProgramCacheKey.hpp
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <cstdint>
#include <functional>

/* Local inclusions for usages. */
#include "Graphics/Types.hpp"

namespace EmEn::Graphics::Renderable
{
	/**
	 * @brief Defines the type of program to cache.
	 */
	enum class ProgramType : uint8_t
	{
		/** @brief Standard scene rendering program. */
		Rendering = 0,
		/** @brief Shadow casting program. */
		ShadowCasting = 1,
		/** @brief TBN space visualization program (debug). */
		TBNSpace = 2
	};

	/**
	 * @brief Key structure for caching shader programs on a Renderable.
	 *
	 * This key uniquely identifies a shader program configuration based on:
	 * - The type of program (rendering, shadow casting, TBN debug)
	 * - The render pass type (ambient, directional light, etc.)
	 * - The render pass handle (for Vulkan render pass compatibility)
	 * - The material layer index
	 * - Instance-specific flags that affect shader generation
	 *
	 * Two RenderableInstances sharing the same Renderable and having the same
	 * configuration will use the same cached program.
	 */
	struct ProgramCacheKey final
	{
		/** @brief The type of program. */
		ProgramType programType{ProgramType::Rendering};
		/** @brief The render pass type for rendering programs. */
		RenderPassType renderPassType{RenderPassType::SimplePass};
		/** @brief The Vulkan render pass handle for pipeline compatibility. */
		uint64_t renderPassHandle{0};
		/** @brief The material layer index. */
		uint32_t layerIndex{0};
		/** @brief Hash of the material descriptor set layout to ensure pipeline compatibility. */
		size_t materialLayoutHash{0};
		/** @brief Whether the instance uses GPU instancing (Multiple vs Unique). */
		bool isInstancing{false};
		/** @brief Whether lighting code is enabled. */
		bool isLightingEnabled{false};
		/** @brief Whether depth test is disabled. */
		bool isDepthTestDisabled{false};
		/** @brief Whether depth write is disabled. */
		bool isDepthWriteDisabled{false};
		/** @brief Whether bindless textures are enabled (adds a descriptor set). */
		bool isBindlessEnabled{false};

		/**
		 * @brief Computes a hash value for this key.
		 * @return size_t
		 */
		[[nodiscard]]
		size_t
		hash () const noexcept
		{
			size_t h = 0;

			/* Combine all fields into the hash using boost-style hash_combine. */
			const auto hashCombine = [&h] (size_t value) noexcept {
				h ^= value + 0x9e3779b9 + (h << 6) + (h >> 2);
			};

			hashCombine(static_cast< size_t >(programType));
			hashCombine(static_cast< size_t >(renderPassType));
			hashCombine(static_cast< size_t >(renderPassHandle));
			hashCombine(static_cast< size_t >(layerIndex));
			hashCombine(static_cast< size_t >(isInstancing));
			hashCombine(static_cast< size_t >(isLightingEnabled));
			hashCombine(static_cast< size_t >(isDepthTestDisabled));
			hashCombine(static_cast< size_t >(isDepthWriteDisabled));
			hashCombine(materialLayoutHash);
			hashCombine(static_cast< size_t >(isBindlessEnabled));

			return h;
		}

		/**
		 * @brief Equality operator.
		 * @param other The other key to compare.
		 * @return bool
		 */
		[[nodiscard]]
		bool
		operator== (const ProgramCacheKey & other) const noexcept
		{
			return programType == other.programType &&
				renderPassType == other.renderPassType &&
				renderPassHandle == other.renderPassHandle &&
				layerIndex == other.layerIndex &&
				isInstancing == other.isInstancing &&
				isLightingEnabled == other.isLightingEnabled &&
				isDepthTestDisabled == other.isDepthTestDisabled &&
				isDepthWriteDisabled == other.isDepthWriteDisabled &&
				materialLayoutHash == other.materialLayoutHash &&
				isBindlessEnabled == other.isBindlessEnabled;
		}
	};
}

/* Hash specialization for std::unordered_map. */
template<>
struct std::hash< EmEn::Graphics::Renderable::ProgramCacheKey >
{
	[[nodiscard]]
	size_t
	operator() (const EmEn::Graphics::Renderable::ProgramCacheKey & key) const noexcept
	{
		return key.hash();
	}
};

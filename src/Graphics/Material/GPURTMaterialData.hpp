/*
 * src/Graphics/Material/GPURTMaterialData.hpp
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
#include <memory>
#include <vector>

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class TextureInterface;
}

namespace EmEn::Graphics::Material
{
	/** @brief Texture roles relevant for ray tracing reflections. */
	enum class RTTextureRole : uint8_t
	{
		Albedo = 0,
		Normal,
		Roughness,
		Metalness,
		Emission
	};

	/**
	 * @brief Describes a texture to be registered for RT bindless access.
	 * @note The texture pointer is non-owning; the material owns the texture lifetime.
	 */
	struct RTTextureSlot
	{
		RTTextureRole role;
		std::shared_ptr< Vulkan::TextureInterface > texture;
	};
	/**
	 * @brief GPU-side material data for ray tracing shaders (std430 layout).
	 * @note This is NOT a renderable material. It is a flat data struct used exclusively
	 *       as an element in the RT Material SSBO. All material types (BasicResource,
	 *       StandardResource, PBRResource) convert to this normalized PBR representation
	 *       via Material::Interface::exportRTMaterialData().
	 *       Only properties visible/useful in reflections are included.
	 */
	struct GPURTMaterialData
	{
		/* Base PBR properties. */
		float albedo[4]{0.5F, 0.5F, 0.5F, 1.0F};
		float roughness{0.5F};
		float metalness{0.0F};
		float ior{1.5F};
		float specularFactor{1.0F};

		/* Specular color tint (KHR_materials_specular). */
		float specularColor[4]{1.0F, 1.0F, 1.0F, 1.0F};

		/* Emission. */
		float emissionColor[4]{0.0F, 0.0F, 0.0F, 0.0F};
		float emissiveStrength{1.0F};

		/* Clear coat. */
		float clearCoatFactor{0.0F};
		float clearCoatRoughness{0.0F};

		/* Feature flags bitmask. */
		uint32_t flags{0};

		/* Bindless texture indices (-1 = no texture, use scalar value). */
		int32_t albedoTextureIndex{-1};
		int32_t normalTextureIndex{-1};
		int32_t roughnessTextureIndex{-1};
		int32_t metalnessTextureIndex{-1};
		int32_t emissionTextureIndex{-1};

		/* Padding to align to 16 bytes (std430). */
		int32_t _padding[3]{0, 0, 0};

		/* Flag bits for the 'flags' field. */
		static constexpr uint32_t HasAlbedoTexture       = 1U << 0;
		static constexpr uint32_t HasNormalTexture        = 1U << 1;
		static constexpr uint32_t HasRoughnessTexture     = 1U << 2;
		static constexpr uint32_t HasMetalnessTexture     = 1U << 3;
		static constexpr uint32_t HasEmissionTexture      = 1U << 4;
		static constexpr uint32_t HasClearCoat            = 1U << 5;
		static constexpr uint32_t IsEmissive              = 1U << 6;
	};

	/* Verify struct size is a multiple of 16 bytes for std430 alignment. */
	static_assert(sizeof(GPURTMaterialData) % 16 == 0, "GPURTMaterialData must be 16-byte aligned for std430.");
}

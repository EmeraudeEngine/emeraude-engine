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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <cstdint>
#include <memory>
#include <array>

/* Local inclusions for usages. */
#include "PixelFactory/Types.hpp"

namespace EmEn::Vulkan
{
	class TextureInterface;
}

namespace EmEn::Graphics::Material
{
	/** @brief Texture roles relevant for ray tracing reflections. */
	enum class EMEN_API RTTextureRole : uint8_t
	{
		Albedo = 0,
		Normal,
		Roughness,
		Metalness,
		Emission,
		Opacity
	};

	/**
	 * @brief Describes a texture to be registered for RT bindless access.
	 * @note The texture pointer is non-owning; the material owns the texture lifetime.
	 */
	struct EMEN_API RTTextureSlot
	{
		RTTextureRole role;
		/** @brief Texel color channel a scalar role (roughness/metalness) reads — mirrors
		 * the raster component's source channel (glTF packed metallic-roughness: G/B). */
		Base::PixelFactory::Channel channel{Base::PixelFactory::Channel::Red};
		std::shared_ptr< Vulkan::TextureInterface > texture;
	};
	/**
	 * @brief GPU-side material data for ray tracing shaders (std430 layout).
	 * @note This is NOT a renderable material. It is a flat data struct used exclusively
	 *	   as an element in the RT Material SSBO. All material types (BasicResource,
	 *	   StandardResource, StandardResource) convert to this normalized PBR representation
	 *	   via Material::Interface::exportRTMaterialData().
	 *	   Only properties visible/useful in reflections are included.
	 */
	struct EMEN_API GPURTMaterialData
	{
		/* Base PBR properties. */
		std::array< float, 4 > albedo{0.5F, 0.5F, 0.5F, 1.0F};
		float roughness{0.5F};
		float metalness{0.0F};
		float ior{1.5F};
		float specularFactor{1.0F};

		/* Specular color tint (KHR_materials_specular). */
		std::array< float, 4 > specularColor{1.0F, 1.0F, 1.0F, 1.0F};

		/* Emission. */
		std::array< float, 4 > emissionColor{0.0F, 0.0F, 0.0F, 0.0F};
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
		int32_t opacityTextureIndex{-1};

		/* Alpha-test threshold (0..1). When IsAlphaTest is set, hits where the sampled
		 * opacity (or albedo alpha if no opacity texture) is below this threshold are
		 * rejected by the RT trace shader (the ray continues past the hit). */
		float alphaCutoff{0.5F};

		/* Normal map intensity (matches the raster's NormalScale uniform): the decoded
		 * tangent-space normal's XY is scaled by this before renormalization. 1 = as
		 * authored. Occupies the former std430 padding slot (matBase+6.w in shaders). */
		float normalScale{1.0F};

		/* Flag bits for the 'flags' field. */
		static constexpr uint32_t HasAlbedoTexture	   = 1U << 0;
		static constexpr uint32_t HasNormalTexture		= 1U << 1;
		static constexpr uint32_t HasRoughnessTexture	 = 1U << 2;
		static constexpr uint32_t HasMetalnessTexture	 = 1U << 3;
		static constexpr uint32_t HasEmissionTexture	  = 1U << 4;
		static constexpr uint32_t HasClearCoat			= 1U << 5;
		static constexpr uint32_t IsEmissive			  = 1U << 6;
		static constexpr uint32_t HasOpacityTexture	   = 1U << 7;
		static constexpr uint32_t IsAlphaTest			 = 1U << 8;
		/** @brief The roughness texture is a smoothness/gloss map: the sampled texel is
		 * inverted (1 - texel) before the factor applies — raster parity (m_invertRoughness). */
		static constexpr uint32_t RoughnessTexInverted	= 1U << 9;

		/* Source color channel of the roughness/metalness texel, packed in the 'flags'
		 * field as 2-bit indices (0:R, 1:G, 2:B, 3:A) — mirrors the raster components'
		 * source channels so RT reflections shade a hit exactly like the raster does
		 * (glTF packed metallic-roughness: roughness = G, metalness = B).
		 * ⚠️ The GLSL side of this contract lives in the RTR effect shader — keep the
		 * shifts in sync (see Graphics/Effects/Framebuffer/RTR.cpp). */
		static constexpr uint32_t RoughnessChannelShift  = 16U;
		static constexpr uint32_t MetalnessChannelShift  = 18U;
		static constexpr uint32_t ChannelMask			 = 0x3U;
	};

	/* Verify struct size is a multiple of 16 bytes for std430 alignment. */
	static_assert(sizeof(GPURTMaterialData) % 16 == 0, "GPURTMaterialData must be 16-byte aligned for std430.");
}

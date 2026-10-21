/*
 * src/Graphics/MipChain.hpp
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

/* STL inclusions. */
#include <cstdint>
#include <functional>

/* Local inclusions for usages. */
#include "PixelFactory/Pixmap.hpp"

namespace EmEn::Graphics::MipChain
{
	/** @brief What the chain does for a texture's kind (graphics § 39 § The mip filter). */
	struct Treatment final
	{
		/** @brief The colour channels are sRGB-encoded: filtered in linear light (the alpha always linear). */
		bool sRGB{false};
		/** @brief A normal map: every level emitted as unit vectors (the shaders reconstruct Z from X and Y). */
		bool normalMap{false};
		/** @brief The alpha is handled: the colour under the transparent pixels dilated before any filter, and a binary
		 * mask's coverage held at every level. Off for a normal map. */
		bool alpha{false};
	};

	/**
	 * @brief Receives one level of the chain.
	 * @param level The level, 0 = the base.
	 * @param pixmap The level's pixels, valid during the call only.
	 * @return bool False stops the chain.
	 */
	using LevelVisitor = std::function< bool (uint32_t level, const Base::PixelFactory::Pixmap< uint8_t > & pixmap) >;

	/**
	 * @brief Builds a texture's mip chain on the CPU and hands every level to a visitor, base first — the chain of the
	 * block-compressed path (TextureCompressor::compress()) and of the uncompressed one alike, so both show the same
	 * pixels.
	 * @note Every level is the PREVIOUS one reduced by NVIDIA Texture Tools' Kaiser-windowed sinc (Processor::downsample(),
	 * ReductionFilter::Kaiser; width 3, alpha 4, stretch 1, edges mirrored), an sRGB colour in linear light.
	 * @note Alpha (Treatment::alpha): the colour under the transparent pixels is dilated from the visible ones before
	 * anything filters it (Processor::fillTransparentColour()); a BINARY coverage mask has each level's alpha rescaled to
	 * the base coverage (Castaño's correction, AlphaCoverage::rescaleToCoverage()) — on the emitted copy: the chain keeps
	 * filtering the uncorrected level, else the gain compounds and the mask turns opaque a few levels down.
	 * @note Normal map (Treatment::normalMap): every emitted level is renormalized (a null vector becomes (0, 0, 1)), the
	 * chain keeps filtering the unnormalized level.
	 * @note Memory: the previous level and the emitted copy; the visitor stores what it keeps.
	 * @param base The base level: RGBA8, or one channel (grey-scale).
	 * @param levelCount The levels to emit, in [1, full chain]; 0 = the full chain down to 1 × 1.
	 * @param treatment What the texture's kind needs.
	 * @param visitor Receives every level.
	 * @return bool False on an invalid base or a failed reduction (logged), or when the visitor stopped the chain.
	 */
	[[nodiscard]]
	bool build (const Base::PixelFactory::Pixmap< uint8_t > & base, uint32_t levelCount, const Treatment & treatment, const LevelVisitor & visitor) noexcept;

	/**
	 * @brief Returns a copy of an RGBA8 normal map whose every texel is a unit vector.
	 * @note A texel decodes as v = byte / 255 × 2 − 1, the shader's mapping, and encodes back rounded. A null vector (a
	 * flat 128-grey area, an unpainted border) becomes (0, 0, 1). The alpha is left as it was.
	 * @param source A reference to the source pixmap.
	 * @return Base::PixelFactory::Pixmap< uint8_t >
	 */
	[[nodiscard]]
	Base::PixelFactory::Pixmap< uint8_t > renormalizedNormals (const Base::PixelFactory::Pixmap< uint8_t > & source) noexcept;
}

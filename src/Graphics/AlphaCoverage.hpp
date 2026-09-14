/*
 * src/Graphics/AlphaCoverage.hpp
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
#include <cstddef>
#include <cstdint>

/* Local inclusions for usages. */
#include "PixelFactory/Pixmap.hpp"

/**
 * @file
 * @brief Alpha COVERAGE: the fraction of a texture that survives an alpha test.
 *
 * A box-filtered mip chain preserves the alpha MEAN, never the alpha COVERAGE — and an alpha
 * test reads the coverage. The two diverge without bound as a binary mask is minified: measured
 * on Sponza's cypress mask (4096², 93.59 % of texels at alpha ~0 and 6.27 % at ~1, so 99.86 %
 * binary), the fraction passing a 0.5 cutoff falls 6.34 % -> 1.56 % (mip 9) -> 0 % (mip 10),
 * while the fraction passing the engine's former 0.01 G-buffer gate RISES 6.41 % -> 26.56 %
 * (mip 9) -> 100 % (mip 11). That is the whole defect: far foliage either vanishes (a plain
 * cutout on naive mips) or stamps depth and normals over its entire QUAD, and every G-buffer
 * effect then applies on the quad instead of on the visible leaves — the reflections flooded
 * Sponza's canopy with a pale achromatic sky (saturation 9.4 % at the top of the tree against
 * 25.7 % at its base, the top band BRIGHTER than the near leaves though a leaf is green).
 *
 * The fix is Ignacio Castaño's, "Computing Alpha Mipmaps" (NVIDIA, 2010,
 * https://www.nvidia.com/en-us/drivers/np2-mipmapping/): after filtering a level, rescale its
 * alpha so that the fraction passing the cutoff matches the base level's. It costs nothing at
 * runtime — the correction is baked into the stored mip chain — and it fixes the raster pass,
 * the shadow maps and the ray-traced alpha test at once, because all three read the same texels.
 */

namespace EmEn::Graphics::AlphaCoverage
{
	/** @brief The cutoff coverage is measured against, matching glTF's `alphaCutoff` default and
	 * StandardResource::DefaultAlphaThreshold. A mip chain is built once, before any material claims
	 * the texture, so it cannot know a per-material threshold — this is the value the engine also
	 * defaults every cutout to, which keeps the two in agreement for every asset that does not
	 * override it. */
	constexpr float DefaultCutoff{0.5F};

	/** @brief A mask is treated as binary coverage when this fraction of its texels sits within Epsilon of 0 or 1. */
	constexpr float BinaryMaskFraction{0.9F};

	/** @brief Distance to 0 or 1 under which a texel counts as fully transparent or fully opaque. */
	constexpr float BinaryMaskEpsilon{0.02F};

	/** @brief Bisection steps used to solve for the alpha scale. 16 steps resolve the scale to ~1/65536 of its range. */
	constexpr uint32_t SolverIterations{16};

	/**
	 * @brief Returns the index of the alpha channel, or a negative value when the pixmap has none.
	 * @param colorCount The channel count of one texel.
	 * @return int32_t
	 */
	[[nodiscard]]
	constexpr
	int32_t
	alphaChannelIndex (size_t colorCount) noexcept
	{
		/* Grayscale+alpha (2) and RGBA (4) carry alpha last; grayscale (1) and RGB (3) carry none. */
		return ( colorCount == 2 || colorCount == 4 ) ? static_cast< int32_t >(colorCount) - 1 : -1;
	}

	/**
	 * @brief Returns the fraction of texels whose alpha passes a cutoff.
	 * @param pixmap A reference to the pixel data to measure.
	 * @param cutoff The alpha test threshold in [0,1].
	 * @return float The coverage in [0,1]. A pixmap without an alpha channel is fully covered.
	 */
	[[nodiscard]]
	inline
	float
	coverage (const Base::PixelFactory::Pixmap< uint8_t > & pixmap, float cutoff) noexcept
	{
		const auto channel = alphaChannelIndex(pixmap.colorCount());

		if ( channel < 0 || !pixmap.isValid() )
		{
			return 1.0F;
		}

		const auto & data = pixmap.data();
		const auto stride = pixmap.colorCount();
		const auto threshold = static_cast< uint32_t >(cutoff * 255.0F);

		size_t passing = 0;
		size_t total = 0;

		for ( size_t index = static_cast< size_t >(channel); index < data.size(); index += stride )
		{
			if ( static_cast< uint32_t >(data[index]) >= threshold )
			{
				++passing;
			}

			++total;
		}

		return total > 0 ? static_cast< float >(passing) / static_cast< float >(total) : 1.0F;
	}

	/**
	 * @brief Returns whether the alpha channel is a BINARY coverage mask rather than graded translucency.
	 * @note This is what separates a mis-declared cutout from a genuinely translucent surface. Sponza's
	 * cypress leaves are authored `alphaMode = BLEND` yet measure 99.86 % binary — they are a cutout, and
	 * promoting them to one is what stops their quad from stamping the G-buffer. A dirt decal at a uniform
	 * opacity of 0.35 measures 0 % binary and must keep blending: turning IT into a cutout would erase it
	 * from the image entirely.
	 * @param pixmap A reference to the pixel data to measure.
	 * @return bool False when the pixmap carries no alpha channel.
	 */
	[[nodiscard]]
	inline
	bool
	isBinaryMask (const Base::PixelFactory::Pixmap< uint8_t > & pixmap) noexcept
	{
		const auto channel = alphaChannelIndex(pixmap.colorCount());

		if ( channel < 0 || !pixmap.isValid() )
		{
			return false;
		}

		const auto & data = pixmap.data();
		const auto stride = pixmap.colorCount();
		const auto low = static_cast< uint32_t >(BinaryMaskEpsilon * 255.0F);
		const auto high = 255U - low;

		size_t extreme = 0;
		size_t total = 0;
		size_t opaque = 0;

		for ( size_t index = static_cast< size_t >(channel); index < data.size(); index += stride )
		{
			const auto alpha = static_cast< uint32_t >(data[index]);

			if ( alpha <= low || alpha >= high )
			{
				++extreme;
			}

			if ( alpha >= high )
			{
				++opaque;
			}

			++total;
		}

		if ( total == 0 )
		{
			return false;
		}

		/* A fully opaque alpha channel (an RGBA texture whose alpha is simply unused) is binary by this
		 * measure but is NOT a coverage mask: there is nothing to cut out, and promoting such a material
		 * to a cutout would change nothing while making its blending mode a lie. */
		if ( opaque == total )
		{
			return false;
		}

		return static_cast< float >(extreme) / static_cast< float >(total) >= BinaryMaskFraction;
	}

	/**
	 * @brief Rescales the alpha channel in place so that its coverage matches a target.
	 * @note Castaño's method: the scale is solved by bisection because coverage is a monotonic but
	 * discrete step function of the scale — there is no closed form. The scaled alpha is written back
	 * SATURATED, which is what makes a leaf's soft edge widen instead of receding as the level shrinks.
	 * @param pixmap A reference to the pixel data to correct.
	 * @param targetCoverage The coverage to reach, measured on the base level.
	 * @param cutoff The alpha test threshold in [0,1].
	 * @return void
	 */
	inline
	void
	rescaleToCoverage (Base::PixelFactory::Pixmap< uint8_t > & pixmap, float targetCoverage, float cutoff) noexcept
	{
		const auto channel = alphaChannelIndex(pixmap.colorCount());

		if ( channel < 0 || !pixmap.isValid() || targetCoverage <= 0.0F )
		{
			return;
		}

		/* Bracket the scale. The upper bound is generous on purpose: a heavily minified mask can need a
		 * large gain before enough texels reach the cutoff. */
		auto & data = pixmap.data();
		const auto stride = pixmap.colorCount();
		const auto threshold = cutoff * 255.0F;

		const auto coverageAt = [&data, channel, stride, threshold] (float probe) noexcept -> float
		{
			size_t passing = 0;
			size_t total = 0;

			for ( size_t index = static_cast< size_t >(channel); index < data.size(); index += stride )
			{
				if ( static_cast< float >(data[index]) * probe >= threshold )
				{
					++passing;
				}

				++total;
			}

			return total > 0 ? static_cast< float >(passing) / static_cast< float >(total) : 0.0F;
		};

		float lowScale = 0.0F;
		float highScale = 32.0F;

		for ( uint32_t iteration = 0; iteration < SolverIterations; ++iteration )
		{
			const auto probe = (lowScale + highScale) * 0.5F;

			if ( coverageAt(probe) < targetCoverage )
			{
				lowScale = probe;
			}
			else
			{
				highScale = probe;
			}
		}

		/* ⚠️ Take the bound whose coverage lands CLOSEST to the target, never the last probe and never
		 * the upper bound on principle. Coverage is a monotonic STEP function of the scale, and on the
		 * last few levels the steps are enormous: a 4x4 level can only express multiples of 1/16 and a
		 * 1x1 level only 0 or 1, so the upper bound there means "the whole quad is opaque" — the exact
		 * defect this correction exists to remove, reintroduced at the tail of the chain. Measured on
		 * Sponza's cypress (target 6.34 %), the upper bound yields 12.5 % at mip 10 and 100 % at mip
		 * 12; the nearest bound yields 0 %, which fades a quad that is one or two pixels wide instead
		 * of turning it into a solid reflector. Fading a sub-pixel leaf is invisible; stamping it is
		 * what the beige mush was made of. */
		const auto lowError = targetCoverage - coverageAt(lowScale);
		const auto highError = coverageAt(highScale) - targetCoverage;
		const auto scale = lowError <= highError ? lowScale : highScale;

		/* A scale of 1 within the solver's resolution means the filter already preserved the coverage:
		 * writing the channel back would only cost a pass over the data. */
		if ( scale > 0.99F && scale < 1.01F )
		{
			return;
		}

		for ( size_t index = static_cast< size_t >(channel); index < data.size(); index += stride )
		{
			const auto scaled = static_cast< float >(data[index]) * scale;

			data[index] = static_cast< uint8_t >(scaled > 255.0F ? 255.0F : scaled);
		}
	}
}

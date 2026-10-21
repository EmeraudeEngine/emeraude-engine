/*
 * src/Graphics/MipChain.cpp
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

#include "MipChain.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

/* Local inclusions. */
#include "BlobTrait.hpp"
#include "Graphics/AlphaCoverage.hpp"
#include "PixelFactory/Processor.hpp"
#include "PixelFactory/Types.hpp"
#include "Tracer.hpp"
#include "Vulkan/Image.hpp"

namespace EmEn::Graphics::MipChain
{
	using namespace Base::PixelFactory;

	namespace
	{
		constexpr auto ClassId{"MipChain"};

		/**
		 * @brief Generates the next mip level: NVTT's Kaiser-windowed sinc (Processor::downsample(), ReductionFilter::Kaiser),
		 * the colour of an sRGB texture filtered in linear light (graphics § 39 § The mip filter).
		 * @note ⚠️ It was Processor::resize(Linear) — a bilinear resample from the top-left corner, which reads the
		 * corner pixel alone for a 1 × 1 target and lost the MEAN of every texture along its chain. Measured on the
		 * 23 % needle mask of the conifer (1022 × 2048): 0.35 at 3 × 8, 0.001 at 1 × 4. Every card small enough to
		 * sample those levels vanished — the pines of the forest benches were bare trunks beyond ~300 m, and their
		 * imposter atlas baked bare. Then an exact area-weighted box (2026-09 → 2026-10-21): the mean kept, but the
		 * softest filter, in the bytes' space.
		 * @param source A reference to the source pixmap.
		 * @param sRGB Whether the colour channels are sRGB-encoded.
		 * @return Pixmap< uint8_t >
		 */
		[[nodiscard]]
		Pixmap< uint8_t >
		generateMip (const Pixmap< uint8_t > & source, bool sRGB) noexcept
		{
			const auto newWidth = std::max(source.width() / 2, static_cast< decltype(source.width()) >(1));
			const auto newHeight = std::max(source.height() / 2, static_cast< decltype(source.height()) >(1));

			Pixmap< uint8_t > mip;

			if ( !Processor< uint8_t >::downsample(source, newWidth, newHeight, mip, ReductionFilter::Kaiser, sRGB) )
			{
				return {};
			}

			return mip;
		}

		/**
		 * @brief Fills the colour under the transparent pixels of an RGBA image (Processor::fillTransparentColour()).
		 * @note A pixel is a hole below the alpha test's cutoff for a binary mask (it is never drawn, its colour only
		 * reaches the screen through a filter), at alpha 0 only for graded alpha (a semi-transparent pixel's colour is
		 * seen). The alpha, the coverage it carries, is untouched.
		 * @param pixmap The image (RGBA or one channel: a one-channel image has no hole).
		 * @param binaryMask Whether its alpha is a binary coverage mask.
		 * @param dilated The dilated copy, written when this returns true.
		 * @return bool False when there is nothing to fill (or the fill failed: logged): use the image itself.
		 */
		[[nodiscard]]
		bool
		dilateHoles (const Pixmap< uint8_t > & pixmap, bool binaryMask, Pixmap< uint8_t > & dilated) noexcept
		{
			if ( pixmap.colorCount() != 4 )
			{
				return false;
			}

			const auto holeThreshold = binaryMask ? static_cast< uint8_t >(std::lround(AlphaCoverage::DefaultCutoff * 255.0F)) : uint8_t{1};
			const auto & bytes = pixmap.data();
			bool hasHole = false;

			for ( size_t offset = 3; offset < bytes.size(); offset += 4 )
			{
				if ( bytes[offset] < holeThreshold )
				{
					hasHole = true;

					break;
				}
			}

			if ( !hasHole )
			{
				return false;
			}

			dilated = pixmap;

			if ( !Processor< uint8_t >::fillTransparentColour(dilated, holeThreshold) )
			{
				Tracer::warning(ClassId, "Unable to dilate the colour under the transparent pixels: the mip chain may darken the edges.");

				return false;
			}

			return true;
		}
	}

	bool
	build (const Pixmap< uint8_t > & base, uint32_t levelCount, const Treatment & treatment, const LevelVisitor & visitor) noexcept
	{
		if ( !base.isValid() || (base.colorCount() != 4 && base.colorCount() != 1) )
		{
			Tracer::error(ClassId, "Invalid pixmap: must be valid RGBA8 or one-channel (grey-scale) data.");

			return false;
		}

		/* NOTE: A one-channel normal map (nonsense, yet possible) is expanded: it is renormalized on three components. */
		std::optional< Pixmap< uint8_t > > expanded;

		if ( base.colorCount() == 1 && treatment.normalMap )
		{
			expanded.emplace(Processor< uint8_t >::toRGBA(base));
		}

		const auto & pixmap = expanded.has_value() ? *expanded : base;
		const auto fullChain = Vulkan::Image::getMIPLevels(pixmap.width(), pixmap.height());
		const auto count = levelCount == 0 ? fullChain : std::min(levelCount, fullChain);

		/* A BINARY coverage mask (foliage, a grate, a sprite) needs its COVERAGE preserved down the chain, not its alpha
		 * mean: a filter keeps the mean and lets the fraction passing the alpha test collapse — measured on Sponza's
		 * cypress mask, 6.34 % at the base against 1.56 % at mip 9 and 0 % at mip 10 (box filter), i.e. a canopy that
		 * thins out and then disappears with distance. Castaño's correction rescales each level's alpha to match the base
		 * coverage. Graded alpha (a translucent decal, a glass pane) is left strictly alone — there is no coverage to hold. */
		const auto preserveCoverage = treatment.alpha && AlphaCoverage::isBinaryMask(pixmap);
		const auto baseCoverage = preserveCoverage ? AlphaCoverage::coverage(pixmap, AlphaCoverage::DefaultCutoff) : 0.0F;

		if ( preserveCoverage )
		{
			TraceInfo{ClassId} <<
				"Binary alpha mask detected (base coverage " << (baseCoverage * 100.0F) <<
				" %): preserving coverage across the mip chain.";
		}

		/* The colour under the transparent pixels is dilated from the visible ones before anything filters it
		 * (Processor::fillTransparentColour(), pull-push): a texture stores black (or anything) there, and both the
		 * chain's filter and the sampler's bilinear tap mixed it into the visible edge — Sponza's cypress leaves kept 56 %
		 * of their green at mip 8 (2026-10-21, engine item alpha-mask-textures-lose-coverage-and-colour-in-the-mip-chain).
		 * A pixel is a hole below the alpha test's cutoff for a binary mask (it is never drawn, its colour only reaches the
		 * screen through a filter), at alpha 0 only for graded alpha (a semi-transparent pixel's colour is seen). The alpha
		 * itself, the coverage it carries, is untouched. */
		Pixmap< uint8_t > dilated;
		const auto & first = treatment.alpha && dilateHoles(pixmap, preserveCoverage, dilated) ? dilated : pixmap;

		/* The emitted level: renormalized for a normal map, its alpha rescaled for a binary mask — on a copy, the chain
		 * keeping the filtered level. */
		const auto emit = [&] (uint32_t level, const Pixmap< uint8_t > & filtered) {
			if ( treatment.normalMap )
			{
				return visitor(level, renormalizedNormals(filtered));
			}

			if ( preserveCoverage && level > 0 )
			{
				auto corrected = filtered;

				AlphaCoverage::rescaleToCoverage(corrected, baseCoverage, AlphaCoverage::DefaultCutoff);

				return visitor(level, corrected);
			}

			return visitor(level, filtered);
		};

		if ( !emit(0, first) )
		{
			return false;
		}

		/* NOTE: Only a colour texture is sRGB; a normal map is never filtered in linear light. */
		const auto sRGB = treatment.sRGB && !treatment.normalMap;
		Pixmap< uint8_t > current = first;

		for ( uint32_t level = 1; level < count; ++level )
		{
			current = generateMip(current, sRGB);

			if ( !current.isValid() )
			{
				TraceError{ClassId} << "Failed to generate mip level " << level << " !";

				return false;
			}

			if ( !emit(level, current) )
			{
				return false;
			}
		}

		return true;
	}

	Pixmap< uint8_t >
	renormalizedNormals (const Pixmap< uint8_t > & source) noexcept
	{
		constexpr auto decode = [] (uint8_t value) noexcept {
			return (static_cast< float >(value) / 255.0F * 2.0F) - 1.0F;
		};

		constexpr auto encode = [] (float value) noexcept {
			return static_cast< uint8_t >(std::lround(std::clamp((value + 1.0F) * 0.5F * 255.0F, 0.0F, 255.0F)));
		};

		auto result = source;
		auto & bytes = result.data();

		for ( size_t offset = 0; offset + 3 < bytes.size(); offset += 4 )
		{
			const auto x = decode(bytes[offset]);
			const auto y = decode(bytes[offset + 1]);
			const auto z = decode(bytes[offset + 2]);
			const auto length = std::sqrt((x * x) + (y * y) + (z * z));

			if ( length < 1.0e-6F )
			{
				bytes[offset] = encode(0.0F);
				bytes[offset + 1] = encode(0.0F);
				bytes[offset + 2] = encode(1.0F);

				continue;
			}

			bytes[offset] = encode(x / length);
			bytes[offset + 1] = encode(y / length);
			bytes[offset + 2] = encode(z / length);
		}

		return result;
	}
}

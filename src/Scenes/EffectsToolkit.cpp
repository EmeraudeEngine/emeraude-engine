/*
 * src/Scenes/EffectsToolkit.cpp
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

#include "EffectsToolkit.hpp"

/* STL inclusions. */
#include <cstdint>
#include <memory>

/* Local inclusions. */
#include "Animations/Sequence.hpp"
#include "Libs/Math/Base.hpp"
#include "Libs/PixelFactory/Color.hpp"
#include "Scenes/Component/PointLight.hpp"
#include "Scenes/Component/SphericalPushModifier.hpp"
#include "Scenes/LensEffects/BlackAndWhite.hpp"
#include "Scenes/LensEffects/ChromaBleeding.hpp"
#include "Scenes/LensEffects/ChromaticAberration.hpp"
#include "Scenes/LensEffects/DustAndHair.hpp"
#include "Scenes/LensEffects/ColorGrading.hpp"
#include "Scenes/LensEffects/CRTPhosphor.hpp"
#include "Scenes/LensEffects/Dithering.hpp"
#include "Scenes/LensEffects/FilmGrain.hpp"
#include "Scenes/LensEffects/Flicker.hpp"
#include "Scenes/LensEffects/FrameMasking.hpp"
#include "Scenes/LensEffects/PhosphorBloom.hpp"
#include "Scenes/LensEffects/Pixelation.hpp"
#include "Scenes/LensEffects/Retro.hpp"
#include "Scenes/LensEffects/Scanlines.hpp"
#include "Scenes/LensEffects/SignalGhosting.hpp"
#include "Scenes/LensEffects/VHSAberration.hpp"
#include "Scenes/LensEffects/VerticalJitter.hpp"
#include "Scenes/LensEffects/Vignetting.hpp"
#include "Scenes/Node.hpp"

namespace EmEn::Scenes
{
	using namespace Animations;
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Libs::PixelFactory;
	using namespace LensEffects;

	std::shared_ptr< Component::PointLight >
	EffectsToolkit::createFlashEffect (Node & node, const Color< float > & color, float radius, float intensity, uint32_t duration) noexcept
	{
		const auto effect = node.componentBuilder< Component::PointLight >("Flash")
			.setup([&] (auto & component) {
				/* NOTE: repeat = 0 means no repetition (play once then stop). */
				const auto interpolation = std::make_shared< Sequence >(duration, 0);
				interpolation->addKeyFrame(0.0F, Variant{intensity}, InterpolationType::Cosine);
				interpolation->addKeyFrame(0.5F, Variant{intensity * 0.85F}, InterpolationType::Cosine);
				interpolation->addKeyFrame(1.0F, Variant{0.0F}, InterpolationType::Cosine);
				interpolation->play();

				component.setColor(color);
				component.setRadius(radius);
				component.setIntensity(intensity);
				component.addAnimation(Component::PointLight::Intensity, interpolation);
			}).build();

		return effect;
	}

	std::shared_ptr< Component::SphericalPushModifier >
	EffectsToolkit::createBlowEffect (Node & node, float radius, float maxMagnitude, uint32_t duration) noexcept
	{
		const auto effect = node.componentBuilder< Component::SphericalPushModifier >("Blow")
			.setup([&] (auto & component) {
				/* NOTE: repeat = 0 means no repetition (play once then stop). */
				const auto interpolation = std::make_shared< Sequence >(duration, 0);
				interpolation->addKeyFrame(0.0F, Variant{maxMagnitude}, InterpolationType::Cosine);
				interpolation->addKeyFrame(0.5F, Variant{maxMagnitude * 0.035F}, InterpolationType::Cosine);
				interpolation->addKeyFrame(1.0F, Variant{0.0F}, InterpolationType::Cosine);
				interpolation->play();

				component.createSphericalInfluenceArea(radius, radius * 0.25F);
				component.addAnimation(Component::SphericalPushModifier::Magnitude, interpolation);
			}).build();

		return effect;
	}

	Saphir::FramebufferEffectsList
	EffectsToolkit::hitchcock60s () noexcept
	{
		/* 1. Chromatic aberration: very subtle barrel distortion from vintage 60s lenses.
		 * Must be first because it overrides fragment fetching.
		 * No radial fringing — classic cinema lenses were well-corrected for that. */
		auto chromaticEffect = std::make_shared< ChromaticAberration >(0.002F);
		chromaticEffect->enableBarrelDistortion(true);
		chromaticEffect->setBarrelStrength(0.03F);

		/* 2. Vertical jitter: film pull-down claw mechanical instability.
		 * Overrides fragment fetching (re-samples with displaced UV).
		 * Must be before non-fetching effects. */
		auto jitterEffect = std::make_shared< VerticalJitter >(0.002F);

		/* 3. Flicker: arc-lamp projector luminosity instability.
		 * Multiplicative global brightness variation, before grain and B&W. */
		auto flickerEffect = std::make_shared< Flicker >(0.08F);

		/* 4. Film grain: applied on color image, then converted to B&W (natural film behavior).
		 * 35mm Kodak Plus-X / Tri-X stock had visible but organic grain. */
		auto grainEffect = std::make_shared< FilmGrain >(0.18F);
		grainEffect->setSize(1.5F);

		/* 5. Black and white: high contrast, slightly dark midtones, pure film noir. */
		auto bwEffect = std::make_shared< BlackAndWhite >();
		bwEffect->setContrast(1.4F);
		bwEffect->setGamma(0.85F);
		bwEffect->setBrightness(-0.03F);

		/* 6. Dust and hair: projector gate debris shadows.
		 * After B&W because particles are projected on the already-exposed film. */
		auto dustEffect = std::make_shared< DustAndHair >(0.3F);

		/* 7. Phosphor bloom: soft halation around bright areas, simulating
		 * the light scattering within the film emulsion (silver halide diffusion). */
		auto bloomEffect = std::make_shared< PhosphorBloom >(0.15F);
		bloomEffect->setThreshold(0.55F);
		bloomEffect->setSpread(3.0F);

		/* 8. Vignetting: wide soft darkening, old lens optics feel. */
		auto vignetteEffect = std::make_shared< Vignetting >(0.7F);
		vignetteEffect->setRadius(0.35F);
		vignetteEffect->setSoftness(0.55F);

		/* 9. Frame masking: subtle projection screen edges (35mm gate mask).
		 * Larger radius and softer edges than CRT — this is a film projector, not a tube. */
		auto frameMasking = std::make_shared< FrameMasking >(0.06F);
		frameMasking->setEdgeSoftness(0.02F);

		return {
			chromaticEffect, jitterEffect, flickerEffect,
			grainEffect, bwEffect, dustEffect,
			bloomEffect, vignetteEffect, frameMasking
		};
	}

	Saphir::FramebufferEffectsList
	EffectsToolkit::goldenHour () noexcept
	{
		/* Color grading: warm shift, punchy contrast, vibrant colors, lifted shadows. */
		auto colorGrading = std::make_shared< ColorGrading >();
		colorGrading->setSaturation(1.2F);
		colorGrading->setHue(0.12F);
		colorGrading->setContrast(1.25F);
		colorGrading->setBrightness(0.04F);
		colorGrading->setGamma(1.1F);

		/* Vignetting: subtle cinematic framing. */
		auto vignetteEffect = std::make_shared< Vignetting >(0.5F);
		vignetteEffect->setRadius(0.4F);
		vignetteEffect->setSoftness(0.5F);

		return {colorGrading, vignetteEffect};
	}

	Saphir::FramebufferEffectsList
	EffectsToolkit::VHSToPureSignal () noexcept
	{
		/* 1. Color grading: washed-out warm tones, lifted blacks, slight desaturation.
		 * VHS tape degrades the signal — colors lose punch. */
		auto colorGrading = std::make_shared< ColorGrading >();
		colorGrading->setSaturation(0.85F);
		colorGrading->setHue(0.08F);
		colorGrading->setContrast(0.9F);
		colorGrading->setBrightness(0.03F);
		colorGrading->setGamma(1.15F);

		/* 2. Chroma bleeding: strong NTSC/PAL horizontal color smearing.
		 * VHS tape has poor chroma bandwidth — worse than broadcast. */
		auto chromaBleeding = std::make_shared< ChromaBleeding >(0.5F);
		chromaBleeding->setSpread(4.0F);

		/* 3. VHS tracking: scrolling horizontal displacement band + head switching bar.
		 * Core K7 tape artifact — tracking errors and head drum switching. */
		auto vhsEffect = std::make_shared< VHSAberration >(0.6F);
		vhsEffect->setBandHeight(0.08F);
		vhsEffect->setDisplacement(0.025F);
		vhsEffect->setScrollSpeed(0.12F);
		vhsEffect->enableHeadSwitching(true);
		vhsEffect->setHeadSwitchHeight(0.02F);
		vhsEffect->setHeadSwitchBrightness(1.0F);

		/* 4. Film grain: analog noise texture (always last). */
		auto grainEffect = std::make_shared< FilmGrain >(0.12F);
		grainEffect->setSize(1.8F);

		return {colorGrading, chromaBleeding, vhsEffect, grainEffect};
	}

	Saphir::FramebufferEffectsList
	EffectsToolkit::VHSToAnalog80s () noexcept
	{
		/* 1. Chromatic aberration: radial RGB fringing + barrel distortion (CRT glass curvature).
		 * Must be first because it overrides fragment fetching. */
		auto chromaticEffect = std::make_shared< ChromaticAberration >(0.02F);
		chromaticEffect->enableRadial(true);
		chromaticEffect->enableBarrelDistortion(true);
		chromaticEffect->setBarrelStrength(0.12F);

		/* 2. Color grading: washed-out warm tones, lifted blacks, slight desaturation. */
		auto colorGrading = std::make_shared< ColorGrading >();
		colorGrading->setSaturation(0.85F);
		colorGrading->setHue(0.08F);
		colorGrading->setContrast(0.9F);
		colorGrading->setBrightness(0.03F);
		colorGrading->setGamma(1.15F);

		/* 3. Chroma bleeding: strong NTSC/PAL horizontal color smearing.
		 * VHS tape has poor chroma bandwidth — worse than broadcast. */
		auto chromaBleeding = std::make_shared< ChromaBleeding >(0.5F);
		chromaBleeding->setSpread(4.0F);

		/* 4. VHS tracking: scrolling horizontal displacement band + head switching bar. */
		auto vhsEffect = std::make_shared< VHSAberration >(0.6F);
		vhsEffect->setBandHeight(0.08F);
		vhsEffect->setDisplacement(0.025F);
		vhsEffect->setScrollSpeed(0.12F);
		vhsEffect->enableHeadSwitching(true);
		vhsEffect->setHeadSwitchHeight(0.02F);
		vhsEffect->setHeadSwitchBrightness(1.0F);

		/* 5. Scanlines: CRT horizontal raster scan lines. */
		auto scanlines = std::make_shared< Scanlines >(0.25F);
		scanlines->setLineWidth(2.0F);
		scanlines->setGapWidth(2.0F);

		/* 6. Phosphor bloom: CRT phosphor luminous halo around bright areas. */
		auto phosphorBloom = std::make_shared< PhosphorBloom >(0.25F);
		phosphorBloom->setThreshold(0.65F);
		phosphorBloom->setSpread(2.0F);

		/* 7. CRT phosphor grid: visible RGB aperture grille pattern. */
		auto crtPhosphor = std::make_shared< CRTPhosphor >(0.15F);
		crtPhosphor->setScale(1.0F);

		/* 8. Vignetting: CRT screen darkening at edges. */
		auto vignetteEffect = std::make_shared< Vignetting >(0.3F);
		vignetteEffect->setRadius(0.5F);
		vignetteEffect->setSoftness(0.6F);

		/* 9. Frame masking: CRT tube rounded corners. */
		auto frameMasking = std::make_shared< FrameMasking >(0.04F);
		frameMasking->setEdgeSoftness(0.01F);

		/* 10. Film grain: analog noise texture (always last). */
		auto grainEffect = std::make_shared< FilmGrain >(0.12F);
		grainEffect->setSize(1.8F);

		return {
			chromaticEffect, colorGrading, chromaBleeding,
			vhsEffect, scanlines, phosphorBloom, crtPhosphor,
			vignetteEffect, frameMasking, grainEffect
		};
	}

	Saphir::FramebufferEffectsList
	EffectsToolkit::analog80s () noexcept
	{
		/* 1. Chromatic aberration: radial RGB fringing + barrel distortion (CRT glass curvature).
		 * Must be first because it overrides fragment fetching. */
		auto chromaticEffect = std::make_shared< ChromaticAberration >(0.02F);
		chromaticEffect->enableRadial(true);


		chromaticEffect->enableBarrelDistortion(true);
		chromaticEffect->setBarrelStrength(0.12F);

		/* 2. Color grading: slight warmth from CRT phosphors, otherwise clean signal. */
		auto colorGrading = std::make_shared< ColorGrading >();
		colorGrading->setSaturation(0.9F);
		colorGrading->setHue(0.04F);
		colorGrading->setContrast(0.95F);
		colorGrading->setBrightness(0.02F);
		colorGrading->setGamma(1.1F);

		/* 3. Scanlines: CRT horizontal raster scan lines. */
		auto scanlines = std::make_shared< Scanlines >(0.25F);
		scanlines->setLineWidth(2.0F);
		scanlines->setGapWidth(2.0F);

		/* 4. Phosphor bloom: CRT phosphor luminous halo around bright areas. */
		auto phosphorBloom = std::make_shared< PhosphorBloom >(0.25F);
		phosphorBloom->setThreshold(0.65F);
		phosphorBloom->setSpread(2.0F);

		/* 5. CRT phosphor grid: visible RGB aperture grille pattern. */
		auto crtPhosphor = std::make_shared< CRTPhosphor >(0.15F);
		crtPhosphor->setScale(1.0F);

		/* 6. Vignetting: CRT screen darkening at edges. */
		auto vignetteEffect = std::make_shared< Vignetting >(0.3F);
		vignetteEffect->setRadius(0.5F);
		vignetteEffect->setSoftness(0.6F);

		/* 7. Frame masking: CRT tube rounded corners. */
		auto frameMasking = std::make_shared< FrameMasking >(0.04F);
		frameMasking->setEdgeSoftness(0.01F);

		/* 8. Film grain: minimal analog noise — clean signal. */
		auto grainEffect = std::make_shared< FilmGrain >(0.06F);
		grainEffect->setSize(1.2F);

		return {
			chromaticEffect, colorGrading,
			scanlines, phosphorBloom, crtPhosphor,
			vignetteEffect, frameMasking, grainEffect
		};
	}

	Saphir::FramebufferEffectsList
	EffectsToolkit::SatelliteToPureSignal () noexcept
	{
		/* 1. Color grading: washed-out warm tones from satellite signal degradation. */
		auto colorGrading = std::make_shared< ColorGrading >();
		colorGrading->setSaturation(0.85F);
		colorGrading->setHue(0.08F);
		colorGrading->setContrast(0.9F);
		colorGrading->setBrightness(0.03F);
		colorGrading->setGamma(1.15F);

		/* 2. Chroma bleeding: light NTSC/PAL horizontal color smearing from composite decoding. */
		auto chromaBleeding = std::make_shared< ChromaBleeding >(0.25F);
		chromaBleeding->setSpread(2.0F);

		/* 3. Signal ghosting: multipath satellite reception (faint duplicate shifted right). */
		auto signalGhosting = std::make_shared< SignalGhosting >(0.1F);
		signalGhosting->setOffset(0.012F);

		/* 4. Analog tracking band: scrolling horizontal displacement (no head switching). */
		auto vhsEffect = std::make_shared< VHSAberration >(0.4F);
		vhsEffect->setBandHeight(0.06F);
		vhsEffect->setDisplacement(0.015F);
		vhsEffect->setScrollSpeed(0.08F);

		/* 5. Film grain: analog noise texture (always last). */
		auto grainEffect = std::make_shared< FilmGrain >(0.10F);
		grainEffect->setSize(1.5F);

		return {colorGrading, chromaBleeding, signalGhosting, vhsEffect, grainEffect};
	}

	Saphir::FramebufferEffectsList
	EffectsToolkit::SatelliteToAnalog80s () noexcept
	{
		/* 1. Chromatic aberration: radial RGB fringing + barrel distortion (CRT glass curvature).
		 * Must be first because it overrides fragment fetching. */
		auto chromaticEffect = std::make_shared< ChromaticAberration >(0.02F);
		chromaticEffect->enableRadial(true);
		chromaticEffect->enableBarrelDistortion(true);
		chromaticEffect->setBarrelStrength(0.12F);

		/* 2. Color grading: washed-out warm tones, lifted blacks, slight desaturation. */
		auto colorGrading = std::make_shared< ColorGrading >();
		colorGrading->setSaturation(0.85F);
		colorGrading->setHue(0.08F);
		colorGrading->setContrast(0.9F);
		colorGrading->setBrightness(0.03F);
		colorGrading->setGamma(1.15F);

		/* 3. Chroma bleeding: light NTSC/PAL horizontal color smearing.
		 * Satellite signal goes through composite decoding — less degradation than VHS tape. */
		auto chromaBleeding = std::make_shared< ChromaBleeding >(0.25F);
		chromaBleeding->setSpread(2.0F);

		/* 4. Signal ghosting: multipath satellite reception (faint duplicate shifted right). */
		auto signalGhosting = std::make_shared< SignalGhosting >(0.1F);
		signalGhosting->setOffset(0.012F);

		/* 5. Analog tracking band: scrolling horizontal displacement (no head switching).
		 * Satellite signal can exhibit similar tracking drift artifacts. */
		auto vhsEffect = std::make_shared< VHSAberration >(0.4F);
		vhsEffect->setBandHeight(0.06F);
		vhsEffect->setDisplacement(0.015F);
		vhsEffect->setScrollSpeed(0.08F);

		/* 6. Scanlines: CRT horizontal raster scan lines. */
		auto scanlines = std::make_shared< Scanlines >(0.25F);
		scanlines->setLineWidth(2.0F);
		scanlines->setGapWidth(2.0F);

		/* 6. Phosphor bloom: CRT phosphor luminous halo around bright areas. */
		auto phosphorBloom = std::make_shared< PhosphorBloom >(0.25F);
		phosphorBloom->setThreshold(0.65F);
		phosphorBloom->setSpread(2.0F);

		/* 7. CRT phosphor grid: visible RGB aperture grille pattern. */
		auto crtPhosphor = std::make_shared< CRTPhosphor >(0.15F);
		crtPhosphor->setScale(1.0F);

		/* 8. Vignetting: CRT screen darkening at edges. */
		auto vignetteEffect = std::make_shared< Vignetting >(0.3F);
		vignetteEffect->setRadius(0.5F);
		vignetteEffect->setSoftness(0.6F);

		/* 9. Frame masking: CRT tube rounded corners. */
		auto frameMasking = std::make_shared< FrameMasking >(0.04F);
		frameMasking->setEdgeSoftness(0.01F);

		/* 10. Film grain: analog noise texture (always last). */
		auto grainEffect = std::make_shared< FilmGrain >(0.10F);
		grainEffect->setSize(1.5F);

		return {
			chromaticEffect, colorGrading, chromaBleeding, signalGhosting,
			vhsEffect, scanlines, phosphorBloom, crtPhosphor,
			vignetteEffect, frameMasking, grainEffect
		};
	}

	Saphir::FramebufferEffectsList
	EffectsToolkit::retro8Bits () noexcept
	{
		/* 1. Pixelation: blocky low-resolution pixel grid.
		 * Must be first because it overrides fragment fetching (snaps UV before sampling). */
		auto pixelation = std::make_shared< Pixelation >(4.0F);

		/* 2. Retro color quantization: reduce to 8 levels per channel (512 colors). */
		auto retro = std::make_shared< Retro >(8);

		/* 3. Ordered Bayer dithering: classic 4x4 pattern, subtle. */
		auto dithering = std::make_shared< Dithering >(0.15F);
		dithering->setMatrixSize(4);

		/* 4. Scanlines: light CRT raster lines for a micro-computer feel. */
		auto scanlines = std::make_shared< Scanlines >(0.15F);
		scanlines->setLineWidth(2.0F);
		scanlines->setGapWidth(2.0F);

		/* 5. Phosphor bloom: discreet phosphor glow around bright areas. */
		auto phosphorBloom = std::make_shared< PhosphorBloom >(0.15F);
		phosphorBloom->setThreshold(0.7F);
		phosphorBloom->setSpread(1.5F);

		/* 6. Vignetting: light edge darkening. */
		auto vignetteEffect = std::make_shared< Vignetting >(0.2F);
		vignetteEffect->setRadius(0.5F);
		vignetteEffect->setSoftness(0.6F);

		/* 7. Frame masking: subtle rounded corners. */
		auto frameMasking = std::make_shared< FrameMasking >(0.03F);
		frameMasking->setEdgeSoftness(0.01F);

		return {
			pixelation, retro, dithering,
			scanlines, phosphorBloom,
			vignetteEffect, frameMasking
		};
	}
}
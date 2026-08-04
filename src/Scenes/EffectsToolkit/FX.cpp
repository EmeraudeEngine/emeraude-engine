/*
 * src/Scenes/EffectsToolkit/FX.cpp
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

#include "FX.hpp"

/* Local inclusions. */
#include "Animations/Sequence.hpp"
#include "Graphics/Photometry.hpp"

namespace EmEn::Scenes::EffectsToolkit::FX
{
	using namespace Animations;
	using namespace Base;
	using namespace Base::Math;
	using namespace Base::PixelFactory;

	std::shared_ptr< Component::PointLight >
	createFlashEffect (Node & node, const Color< float > & settlingTint, float cullingRadius, float peakLumens, uint32_t duration) noexcept
	{
		/* The keyframes drive setIntensity(), which takes CANDELA, so the authored
		 * luminous power is converted once here rather than on every interpolated frame. */
		const auto peakCandela = Graphics::Photometry::candelaFromPointLumens(peakLumens);

		const auto effect = node.componentBuilder< Component::PointLight >("Flash")
			.setup([&] (auto & component) {
				/* INTENSITY — the detonation envelope, and the ONLY thing that shapes the
				 * flash in time.
				 *
				 * ⚠️ Animating the RADIUS used to be the way to do this, and it no longer
				 * works: under the photometric windowed inverse square the radius is
				 * `saturate(1 - (d/r)^4)^2`, a culling WINDOW that sits at 1.0 over almost the
				 * whole range and only bites near d == r. The falloff is carried by
				 * `1 / (d^2 + 1)`, which depends on the distance alone. Growing the radius
				 * therefore does not brighten anything — it just moves the hard cut outwards.
				 *
				 * Shape: a near-instant peak followed by a fast decay, which is what a
				 * detonation does — the shock front flashes, then the fuel burns off as an
				 * expanding, cooling fireball. Holding near-peak for half the duration (the
				 * pre-photometric curve) does not read as an explosion, it reads as a lamp
				 * being switched on, and it floods the whole scene.
				 * NOTE: repeat = 0 means no repetition (play once then stop). */
				const auto intensityRamp = std::make_shared< Sequence >(duration, 0);
				intensityRamp->addKeyFrame(0.00F, Variant{peakCandela}, InterpolationType::Cosine);
				intensityRamp->addKeyFrame(0.06F, Variant{peakCandela * 0.55F}, InterpolationType::Cosine);
				intensityRamp->addKeyFrame(0.15F, Variant{peakCandela * 0.22F}, InterpolationType::Cosine);
				intensityRamp->addKeyFrame(0.35F, Variant{peakCandela * 0.07F}, InterpolationType::Cosine);
				intensityRamp->addKeyFrame(0.70F, Variant{peakCandela * 0.015F}, InterpolationType::Cosine);
				intensityRamp->addKeyFrame(1.00F, Variant{0.0F}, InterpolationType::Cosine);
				intensityRamp->play();

				/* COLOUR — the fireball cools as it burns: the shock front is white hot, the
				 * soot-laden fuel that follows radiates around 1500-2000 K, so the flash
				 * settles through the yellows into the caller's tint. */
				const auto colorRamp = std::make_shared< Sequence >(duration, 0);
				colorRamp->addKeyFrame(0.00F, Variant{Color< float >{1.0F, 1.0F, 1.0F, 1.0F}}, InterpolationType::Cosine);
				colorRamp->addKeyFrame(0.05F, Variant{Color< float >{1.0F, 0.95F, 0.75F, 1.0F}}, InterpolationType::Cosine);
				colorRamp->addKeyFrame(0.18F, Variant{Color< float >{1.0F, 0.80F, 0.35F, 1.0F}}, InterpolationType::Cosine);
				colorRamp->addKeyFrame(1.00F, Variant{settlingTint}, InterpolationType::Cosine);
				colorRamp->play();

				component.setColor(Color< float >{1.0F, 1.0F, 1.0F, 1.0F});
				/* The radius is a CULLING BOUND, not a dimmer — set it where the contribution
				 * becomes negligible and leave it alone. */
				component.setRadius(cullingRadius);
				component.setLuminousPower(peakLumens);
				component.addAnimation(Component::PointLight::Intensity, intensityRamp);
				component.addAnimation(Component::PointLight::Color, colorRamp);
			}).build();

		return effect;
	}

	std::shared_ptr< Component::SphericalPushModifier >
	createBlowEffect (Node & node, float radius, float maxMagnitude, uint32_t duration) noexcept
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
}

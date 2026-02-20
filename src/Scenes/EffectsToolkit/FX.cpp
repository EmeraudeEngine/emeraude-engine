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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "FX.hpp"

/* Local inclusions. */
#include "Animations/Sequence.hpp"

namespace EmEn::Scenes::EffectsToolkit::FX
{
	using namespace Animations;
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Libs::PixelFactory;

	std::shared_ptr< Component::PointLight >
	createFlashEffect (Node & node, const Color< float > & color, float radius, float intensity, uint32_t duration) noexcept
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

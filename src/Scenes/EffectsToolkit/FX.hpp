/*
 * src/Scenes/EffectsToolkit/FX.hpp
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

/* Local inclusions for usages. */
#include "PixelFactory/Color.hpp"
#include "Scenes/Component/PointLight.hpp"
#include "Scenes/Component/SphericalPushModifier.hpp"
#include "Scenes/Node.hpp"

namespace EmEn::Scenes::EffectsToolkit::FX
{
	/**
	 * @brief Creates a detonation flash: a point light that peaks instantly, decays fast, and
	 * cools from white through the yellows to a settling tint.
	 * @note PHOTOMETRIC UNITS. The power is authored in LUMENS, as a light is sold, and
	 * converted to candela internally — same contract as
	 * `Scenes::Toolkit::generate{Point,Spot,Directional}Light()`.
	 * @warning Do NOT try to shape the flash with the radius. Under the windowed inverse square
	 * the radius is only a culling bound: `saturate(1 - (d/r)^4)^2` sits at 1.0 over almost the
	 * whole range, while `1 / (d^2 + 1)` carries the falloff. Growing it brightens nothing, it
	 * moves the hard cut. The envelope lives in the intensity keyframes.
	 * @param node A reference to a scene node.
	 * @param settlingTint The colour the flash cools DOWN to; it always starts white hot.
	 * @param cullingRadius The distance at which the contribution becomes negligible, in metres.
	 * @param peakLumens The luminous power at the detonation peak, in lumens.
	 * @param duration The animation duration in milliseconds.
	 * @return std::shared_ptr< Component::PointLight >
	 */
	EMEN_API std::shared_ptr< Component::PointLight > createFlashEffect (Node & node, const Base::PixelFactory::Color< float > & settlingTint, float cullingRadius, float peakLumens, uint32_t duration) noexcept;

	/**
	 * @brief Creates a temporary spherical push force.
	 * @param node A reference to a scene node.
	 * @param radius The push limit radius.
	 * @param maxMagnitude The force of the push.
	 * @param duration The animation duration in milliseconds.
	 * @return std::shared_ptr< Component::SphericalPushModifier >
	 */
	EMEN_API std::shared_ptr< Component::SphericalPushModifier > createBlowEffect (Node & node, float radius, float maxMagnitude, uint32_t duration) noexcept;
}

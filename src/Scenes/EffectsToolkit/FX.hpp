/*
 * src/Scenes/EffectsToolkit.FX.hpp
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

/* Local inclusions for usages. */
#include "Libs/PixelFactory/Color.hpp"
#include "Scenes/Component/PointLight.hpp"
#include "Scenes/Component/SphericalPushModifier.hpp"
#include "Scenes/Node.hpp"

namespace EmEn::Scenes::EffectsToolkit::FX
{
	/**
	 * @brief Creates a flashing point light.
	 * @param node A reference to a scene node.
	 * @param color A reference to a color.
	 * @param radius The point light radius.
	 * @param intensity The point light initial intensity.
	 * @param duration The animation duration in milliseconds.
	 * @return std::shared_ptr< Component::PointLight >
	 */
	std::shared_ptr< Component::PointLight > createFlashEffect (Node & node, const Libs::PixelFactory::Color< float > & color, float radius, float intensity, uint32_t duration) noexcept;

	/**
	 * @brief Creates a temporary spherical push force.
	 * @param node A reference to a scene node.
	 * @param radius The push limit radius.
	 * @param maxMagnitude The force of the push.
	 * @param duration The animation duration in milliseconds.
	 * @return std::shared_ptr< Component::SphericalPushModifier >
	 */
	std::shared_ptr< Component::SphericalPushModifier > createBlowEffect (Node & node, float radius, float maxMagnitude, uint32_t duration) noexcept;
}

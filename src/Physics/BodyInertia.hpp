/*
 * src/Physics/BodyInertia.hpp
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
#include <array>
#include <optional>

/* Local inclusions for usages. */
#include "Math/Matrix.hpp"
#include "Math/Vector.hpp"
#include "BodyPhysicalProperties.hpp"
#include "CollisionModelInterface.hpp"

namespace EmEn::Physics
{
	/**
	 * @brief Returns the inverse inertia tensor of a body about its centre of mass, in the body's axes.
	 * @note The explicit tensor of the properties wins; without one, the collision shape gives it as a uniform solid
	 * of the body's mass (physics overhaul P3, decision 8c). A zero matrix means the body does not rotate: no tensor
	 * at all (a point, no shape yet, a null mass) or a singular one.
	 * @param properties A reference to the body's physical properties.
	 * @param model A pointer to the collision model. May be nullptr.
	 * @param scaling A reference to the scaling factor of the body's world frame.
	 * @return Base::Math::Matrix< 3, float >
	 */
	[[nodiscard]]
	inline
	Base::Math::Matrix< 3, float >
	localInverseInertia (const BodyPhysicalProperties & properties, const CollisionModelInterface * model, const Base::Math::Vector< 3, float > & scaling) noexcept
	{
		const Base::Math::Matrix< 3, float > noRotation{std::array< float, 9 >{}};

		auto inertia = properties.inertiaTensor();

		if ( !inertia.has_value() && model != nullptr )
		{
			inertia = model->solidInertia(properties.mass(), scaling);
		}

		if ( !inertia.has_value() )
		{
			return noRotation;
		}

		return inertia.value().tryInverse().value_or(noRotation);
	}

	/**
	 * @brief Returns a body's centre of mass in its frame, scaled with it: the explicit one of the properties
	 * (BodyPhysicalProperties::setCenterOfMass()), else the collision shape's centroid, else the origin.
	 * @note The one source of the centre of mass of the physics: the solver's body, the contact impulses and
	 * Scenes::Node::getWorldCenterOfMass() all read it here.
	 * @param properties A reference to the body's physical properties.
	 * @param model A pointer to the collision model. May be nullptr.
	 * @param scaling A reference to the scaling factor of the body's world frame.
	 * @return Base::Math::Vector< 3, float >
	 */
	[[nodiscard]]
	inline
	Base::Math::Vector< 3, float >
	localCenterOfMass (const BodyPhysicalProperties & properties, const CollisionModelInterface * model, const Base::Math::Vector< 3, float > & scaling) noexcept
	{
		if ( const auto & explicitCentre = properties.centerOfMass(); explicitCentre.has_value() )
		{
			const auto & centre = explicitCentre.value();

			return {centre[0] * scaling[0], centre[1] * scaling[1], centre[2] * scaling[2]};
		}

		return model != nullptr ? model->centerOfMassOffset(scaling) : Base::Math::Vector< 3, float >{};
	}

	/**
	 * @brief Turns a body-axes inverse inertia tensor into world space: R · I⁻¹ · Rᵀ.
	 * @param localInverse A reference to the inverse inertia in the body's axes.
	 * @param rotation A reference to the body's world rotation (body axes → world).
	 * @return Base::Math::Matrix< 3, float >
	 */
	[[nodiscard]]
	inline
	Base::Math::Matrix< 3, float >
	worldInverseInertia (const Base::Math::Matrix< 3, float > & localInverse, const Base::Math::Matrix< 3, float > & rotation) noexcept
	{
		auto transposed = rotation;
		transposed.transpose();

		return rotation * localInverse * transposed;
	}
}

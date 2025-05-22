/*
 * src/Animations/AnimatableInterface.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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
#include <cstddef>
#include <map>
#include <memory>

/* Local inclusions for usages. */
#include "Libs/Variant.hpp"
#include "AnimationInterface.hpp"

namespace EmEn::Animations
{
	/**
	 * @brief The AnimatableInterface class
	 */
	class AnimatableInterface
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			AnimatableInterface (const AnimatableInterface & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			AnimatableInterface (AnimatableInterface && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return AnimatableInterface &
			 */
			AnimatableInterface & operator= (const AnimatableInterface & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return AnimatableInterface &
			 */
			AnimatableInterface & operator= (AnimatableInterface && copy) noexcept = default;

			/**
			 * @brief Destructs the animatable interface.
			 */
			virtual ~AnimatableInterface () = default;

			/**
			 * @brief Adds an animatable interfaced object where the animation is modified by an interpolation.
			 * @param animationID The animation identifier.
			 * @param animation A reference to an animation smart pointer.
			 * @return bool
			 */
			bool
			addAnimation (uint8_t animationID, const std::shared_ptr< AnimationInterface > & animation) noexcept
			{
				return m_animations.emplace(animationID, animation).second;
			}

			/**
			 * @brief Removes an animatable interfaced object.
			 * @param animationID The animation identifier.
			 * @return bool
			 */
			bool
			removeAnimation (uint8_t animationID) noexcept
			{
				return m_animations.erase(animationID) > 0;
			}

			/**
			 * @brief This method is called within the logic loop to update every registered animation.
			 * @param cycle The current engine cycle.
			 * @return void
			 */
			void updateAnimations (size_t cycle) noexcept;

			/**
			 * @brief Clears every animation.
			 * @return void
			 */
			void
			clearAnimations () noexcept
			{
				m_animations.clear();
			}

		protected:

			/**
			 * @brief Constructs a default animatable interface.
			 */
			AnimatableInterface () noexcept = default;

		private:

			/**
			 * @brief Plays the identified animations.
			 * @param animationID The animation ID.
			 * @param value A reference to a std::any.
			 * @param cycle The current engine cycle.
			 * @return bool
			 */
			virtual bool playAnimation (uint8_t animationID, const Libs::Variant & value, size_t cycle) noexcept = 0;

			std::map< uint8_t, std::shared_ptr< AnimationInterface > > m_animations;
	};
}

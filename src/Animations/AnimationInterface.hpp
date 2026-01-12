/*
 * src/Animations/AnimationInterface.hpp
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

/* Local inclusions for usages. */
#include "Libs/Variant.hpp"

namespace EmEn::Animations
{
	/**
	 * @brief The AnimationInterface class
	 */
	class AnimationInterface
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			AnimationInterface (const AnimationInterface & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			AnimationInterface (AnimationInterface && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return AnimationInterface &
			 */
			AnimationInterface & operator= (const AnimationInterface & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return AnimationInterface &
			 */
			AnimationInterface & operator= (AnimationInterface && copy) noexcept = default;

			/**
			 * @brief Destructs the animation interface.
			 */
			virtual ~AnimationInterface () = default;

			/**
			 * @brief Gets the next value during the animation.
			 * @note Cannot be const because the method can change the internal value of the object.
			 * @return Variant
			 */
			virtual Libs::Variant getNextValue () noexcept = 0;

			/**
			 * @brief Returns whether the animation is running.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool isPlaying () const noexcept = 0;

			/**
			 * @brief Returns whether the animation is stopped.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool isPaused () const noexcept = 0;

			/**
			 * @brief Returns whether the animation is finished for a non-loop animation.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool isFinished () const noexcept = 0;

			/**
			 * @brief Plays the animation.
			 * @return bool
			 */
			virtual bool play () noexcept = 0;

			/**
			 * @brief Pauses the animation.
			 * @return bool
			 */
			virtual bool pause () noexcept = 0;

		protected:

			/**
			 * @brief Constructs an animation interface.
			 */
			AnimationInterface () noexcept = default;
	};
}

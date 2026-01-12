/*
 * src/Animations/ConstantValue.hpp
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
#include "AnimationInterface.hpp"

namespace EmEn::Animations
{
	/**
	 * @brief Animation using a simple constant value.
	 * @note ie, This can be used to add 1 degree each logic cycle to a rotation.
	 * @extends EmEn::Animations::AnimationInterface This is an animation.
	 */
	class ConstantValue final : public AnimationInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ConstantValue"};

			/**
			 * @brief Constructs a default constant value.
			 */
			ConstantValue () noexcept = default;

			/**
			 * @brief Constructs a constant value with a next value [std::move].
			 * @param value The next value.
			 */
			explicit
			ConstantValue (Libs::Variant value) noexcept
				: m_value{std::move(value)}
			{

			}

			/** @copydoc EmEn::Animations::AnimationInterface::getNextValue() */
			Libs::Variant
			getNextValue () noexcept override
			{
				return m_value;
			}

			/** @copydoc EmEn::Animations::AnimationInterface::isPlaying() */
			[[nodiscard]]
			bool
			isPlaying () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Animations::AnimationInterface::isPaused() */
			[[nodiscard]]
			bool
			isPaused () const noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::Animations::AnimationInterface::isFinished() */
			[[nodiscard]]
			bool
			isFinished () const noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::Animations::AnimationInterface::play() */
			bool
			play () noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Animations::AnimationInterface::pause() */
			bool
			pause () noexcept override
			{
				return false;
			}

			/**
			 * @brief Set the next value.
			 * @param value A reference to a variant for the next value.
			 * @return void
			 */
			void
			setValue (const Libs::Variant & value) noexcept
			{
				m_value = value;
			}

		private:

			Libs::Variant m_value;
	};
}

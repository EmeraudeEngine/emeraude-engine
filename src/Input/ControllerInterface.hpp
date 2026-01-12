/*
 * src/Input/ControllerInterface.hpp
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
#include <string>

namespace EmEn::Input
{
	/**
	 * @brief The controller interface class.
	 * @note This is intended to be used as an object that read the current state of an input device.
	 */
	class ControllerInterface
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			ControllerInterface (const ControllerInterface & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			ControllerInterface (ControllerInterface && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return ControllerInterface &
			 */
			ControllerInterface & operator= (const ControllerInterface & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return ControllerInterface &
			 */
			ControllerInterface & operator= (ControllerInterface && copy) noexcept = default;

			/**
			 * @brief Destructs the controller interface.
			 */
			virtual ~ControllerInterface () = default;

			/**
			 * @brief Disables the device.
			 * @param state The state.
			 * @return void
			 */
			virtual void disable (bool state) noexcept = 0;

			/**
			 * @brief Returns whether the device is disabled.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool disabled () const noexcept = 0;

			/**
			 * @brief Returns whether the controller is connected.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool isConnected () const noexcept = 0;

			/**
			 * @brief Returns the raw state of the controller as a string for debugging.
			 * @return std::string
			 */
			[[nodiscard]]
			virtual std::string getRawState () const noexcept = 0;

		protected:

			/**
			 * @brief Constructs a default controller interface.
			 */
			ControllerInterface () noexcept = default;
	};
}

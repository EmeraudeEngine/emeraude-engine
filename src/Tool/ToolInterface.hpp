/*
 * src/Tool/ToolInterface.hpp
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

/* Forward declarations. */
namespace EmEn
{
	class Arguments;
}

namespace EmEn::Tool
{
	/**
	 * @brief The tool interface.
	 */
	class ToolInterface
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			ToolInterface (const ToolInterface & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			ToolInterface (ToolInterface && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return ToolInterface &
			 */
			ToolInterface & operator= (const ToolInterface & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return ToolInterface &
			 */
			ToolInterface & operator= (ToolInterface && copy) noexcept = delete;
			
			/**
			 * @brief Destructs the tool interface.
			 */
			virtual ~ToolInterface () = default;
			
			/**
			 * @brief Executes the tool.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool execute () noexcept = 0;

		protected:

			/**
			 * @brief Constructs a tool interface.
			 */
			ToolInterface () noexcept = default;
	};
}

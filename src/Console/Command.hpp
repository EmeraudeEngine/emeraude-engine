/*
 * src/Console/Command.hpp
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
#include <functional>
#include <string>
#include <vector>

/* Local inclusions. */
#include "Argument.hpp"
#include "Output.hpp"

namespace EmEn::Console
{
	/** @brief Typedef of a function used by the console. */
	using Binding = std::function< bool (const Arguments &, Outputs &) >;

	/**
	 * @brief Container for a specific command.
	 */
	class Command final
	{
		public:

			/**
			 * @brief Constructs a console command.
			 * @param binding The command to execute in the container [std::move].
			 * @param help A way to explain that command [std::move].
			 */
			Command (Binding binding, std::string help) noexcept
				: m_binding{std::move(binding)},
				m_help{std::move(help)}
			{

			}

			/**
			 * @brief Returns the binding.
			 * @return const Binding &
			 */
			[[nodiscard]]
			const Binding &
			binding () const noexcept
			{
				return m_binding;
			}

			/**
			 * @brief Returns the command usage instructions.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			help () const noexcept
			{
				return m_help;
			}

		private:

			Binding m_binding;
			std::string m_help;
	};
}

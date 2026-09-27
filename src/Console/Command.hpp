/*
 * src/Console/Command.hpp
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
#include <functional>
#include <string>

/* Local inclusions for usages. */
#include "Argument.hpp"
#include "CommandSignature.hpp"
#include "Output.hpp"

namespace EmEn::Console
{
	/** @brief Typedef of a function used by the console. */
	using Binding = std::function< bool (const Arguments &, Outputs &) >;

	/**
	 * @brief Container for a specific command.
	 * @note Every command is TYPED (2026-09-27, owner decision: the untyped form was deleted once the
	 * last of the 137 commands had migrated): it carries its signature — declared parameters and hints —
	 * and its binding validates and converts the arguments before calling the author's callable.
	 */
	class EMEN_API Command final
	{
		public:

			/**
			 * @brief Constructs a console command.
			 * @param binding The validating binding built by bindCommand() [std::move].
			 * @param help The description followed by the generated usage [std::move].
			 * @param description The description alone, as the author wrote it [std::move].
			 * @param primaryName The first name of the alias list this command was bound with [std::move].
			 * @param signature The declared contract [std::move].
			 */
			Command (Binding binding, std::string help, std::string description, std::string primaryName, CommandSignature signature) noexcept
				: m_binding{std::move(binding)},
				m_help{std::move(help)},
				m_description{std::move(description)},
				m_primaryName{std::move(primaryName)},
				m_signature{std::move(signature)}
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

			/**
			 * @brief Returns the description alone, without the generated usage line.
			 * @note What a machine client shows next to the declared parameters (an MCP tool description).
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			description () const noexcept
			{
				return m_description;
			}

			/**
			 * @brief Returns the first name of the alias list the command was bound with ("exit" for
			 * "exit,quit,shutdown").
			 * @note Lets a machine client expose one tool per command instead of one per alias.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			primaryName () const noexcept
			{
				return m_primaryName;
			}

			/**
			 * @brief Returns the declared signature.
			 * @return const CommandSignature &
			 */
			[[nodiscard]]
			const CommandSignature &
			signature () const noexcept
			{
				return m_signature;
			}

		private:

			Binding m_binding;
			std::string m_help;
			std::string m_description;
			std::string m_primaryName;
			CommandSignature m_signature;
	};
}

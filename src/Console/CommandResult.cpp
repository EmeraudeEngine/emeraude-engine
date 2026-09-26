/*
 * src/Console/CommandResult.cpp
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

#include "CommandResult.hpp"

/* STL inclusions. */
#include <iterator>

namespace EmEn::Console
{
	CommandResult::CommandResult (Output output, bool succeeded) noexcept
		: m_succeeded{succeeded}
	{
		m_outputs.emplace_back(std::move(output));
	}

	CommandResult::CommandResult (Outputs outputs, bool succeeded) noexcept
		: m_outputs{std::move(outputs)},
		m_succeeded{succeeded}
	{

	}

	CommandResult
	CommandResult::fromOutputs (Outputs outputs, bool succeeded) noexcept
	{
		return {std::move(outputs), succeeded};
	}

	CommandResult
	CommandResult::success (std::string message) noexcept
	{
		return {Output{Severity::Success, std::move(message)}, true};
	}

	CommandResult
	CommandResult::info (std::string message) noexcept
	{
		return {Output{Severity::Info, std::move(message)}, true};
	}

	CommandResult
	CommandResult::warning (std::string message) noexcept
	{
		return {Output{Severity::Warning, std::move(message)}, true};
	}

	CommandResult
	CommandResult::error (std::string message) noexcept
	{
		return {Output{Severity::Error, std::move(message)}, false};
	}

	CommandResult
	CommandResult::json (std::string document) noexcept
	{
		return {Output::json(std::move(document)), true};
	}

	CommandResult
	CommandResult::binary (std::vector< uint8_t > bytes, std::string mimeType, std::string description) noexcept
	{
		return {Output::binary(std::move(bytes), std::move(mimeType), std::move(description)), true};
	}

	CommandResult &
	CommandResult::add (Output output) & noexcept
	{
		m_outputs.emplace_back(std::move(output));

		return *this;
	}

	CommandResult &&
	CommandResult::add (Output output) && noexcept
	{
		m_outputs.emplace_back(std::move(output));

		return std::move(*this);
	}

	bool
	CommandResult::moveTo (Outputs & outputs) noexcept
	{
		outputs.insert(outputs.end(), std::make_move_iterator(m_outputs.begin()), std::make_move_iterator(m_outputs.end()));
		m_outputs.clear();

		return m_succeeded;
	}
}

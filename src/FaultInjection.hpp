/*
 * src/FaultInjection.hpp
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
#include <string_view>

/* Forward declarations. */
namespace EmEn
{
	class Arguments;
}

/*
 * Startup fault injection (Ave Robustus II P3, owner decisions 2026-10-16): a TEST build (CMake option
 * EMERAUDE_FAULT_INJECTION, off by default, never a release) makes the startup of one named service fail, so every
 * failure path of the startup can be run in turn — each must exit cleanly, without a leak, a hang or a VUID.
 * docs/subsystems/source-tree/08-startup-fault-injection.md.
 */
namespace EmEn::FaultInjection
{
	/** @brief The command-line argument naming the service whose startup fails: `--inject-startup-failure=NAME`. */
	constexpr auto StartupFailureArgument{"--inject-startup-failure"};

	/**
	 * @brief Reads the argument once. Does nothing in a build without EMERAUDE_FAULT_INJECTION.
	 * @pre Main process, main thread, right after the arguments service and before any other service starts.
	 * @param arguments A reference to the arguments service.
	 */
	EMEN_API void configure (const Arguments & arguments) noexcept;

	/**
	 * @brief Returns whether the startup of this service is made to fail (the failure is traced). Always false in a
	 * build without EMERAUDE_FAULT_INJECTION.
	 * @note Read-only after configure(): any thread.
	 * @param serviceName The service's name (ServiceInterface::name()).
	 * @return bool
	 */
	[[nodiscard]]
	EMEN_API bool startupFailureInjected (std::string_view serviceName) noexcept;
}

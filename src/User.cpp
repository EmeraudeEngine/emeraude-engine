/*
 * src/User.cpp
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

#include "User.hpp"

/* Local inclusions. */
#include "Settings.hpp"
#include "Tracer.hpp"

namespace EmEn
{
	const size_t User::ClassUID{getClassUID(ClassId)};

	User::User (PrimaryServices & primaryServices) noexcept
		: ServiceInterface(ClassId),
		m_primaryServices(primaryServices)
	{

	}

	size_t
	User::classUID () const noexcept
	{
		return ClassUID;
	}

	bool
	User::is (size_t classUID) const noexcept
	{
		return classUID == ClassUID;
	}

	bool
	User::usable () const noexcept
	{
		return m_flags[ServiceInitialized];
	}

	void
	User::setUserID (uint64_t identifier) noexcept
	{
		m_userID = identifier;
	}

	void
	User::setUsername (const std::string & username) noexcept
	{
		m_username = username;
	}

	uint64_t
	User::userID () const noexcept
	{
		return m_userID;
	}

	const std::string &
	User::username () const noexcept
	{
		return m_username;
	}

	bool
	User::onInitialize () noexcept
	{
		m_flags[ServiceInitialized] = true;

		return true;
	}

	bool
	User::onTerminate () noexcept
	{
		m_flags[ServiceInitialized] = false;

		return true;
	}
}

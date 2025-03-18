/*
 * src/Graphics/ExternalInput.cpp
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

#include "ExternalInput.hpp"

/* Local inclusions. */
#include "Arguments.hpp"
#include "Graphics/Renderer.hpp"
#include "Settings.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics
{
	using namespace EmEn::Libs;

	const size_t ExternalInput::ClassUID{getClassUID(ClassId)};

	ExternalInput::ExternalInput () noexcept
		: ServiceInterface(ClassId)
	{

	}

	bool
	ExternalInput::usable () const noexcept
	{
		return m_flags[ServiceInitialized];
	}

	size_t
	ExternalInput::classUID () const noexcept
	{
		return ClassUID;
	}

	bool
	ExternalInput::is (size_t classUID) const noexcept
	{
		return classUID == ClassUID;
	}

	bool
	ExternalInput::onInitialize () noexcept
	{
		/* FIXME: Complete the method ... */

		m_flags[ServiceInitialized] = true;

		return true;
	}

	bool
	ExternalInput::onTerminate () noexcept
	{
		m_flags[ServiceInitialized] = false;

		/* FIXME: Complete the method ... */

		return true;
	}
}
